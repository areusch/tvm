import abc
import contextlib
import logging
import serial
import serial.tools.list_ports
import string
import subprocess
import typing


_LOG = logging.getLogger(__name__)


class SerialPortNotFoundError(Exception):
  """Raised when SerialSession cannot find the serial port specified."""


class Transport(metaclass=abc.ABCMeta):

    def __enter__(self):
      self.open()
      return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
      self.close()

    @abc.abstractmethod
    def open(self):
      raise NotImplementedError()

    @abc.abstractmethod
    def close(self):
      raise NotImplementedError()

    @abc.abstractmethod
    def read(self, n):
      raise NotImplementedError()

    @abc.abstractmethod
    def write(self, data):
      raise NotImplementedError()


class TransportLogger(Transport):

  def __init__(self, name, child, logger=None, level=logging.DEBUG):
    self.name = name
    self.child = child
    self.logger = logger or _LOG
    self.level = level

  @classmethod
  def _to_hex(cls, data):
    lines = []
    if not data:
      lines.append('')
      return lines

    for i in range(0, (len(data) + 15) // 16):
      chunk = data[i * 16:(i + 1) * 16]
      hex_chunk = ' '.join(f'{c:02x}' for c in chunk)
      ascii_chunk = ''.join(chr(c) if chr(c) in string.printable else '.' for c in chunk)
      lines.append(f'{i:4x}  {hex_chunk:47}  {ascii_chunk}')

    return lines

  def open(self):
    self.logger.log(self.level, 'opening transport')
    self.child.open()

  def close(self):
    self.logger.log(self.level, 'closing transport')
    return self.child.close()

  def read(self, n):
    data = self.child.read(n)
    hex_lines = self._to_hex(data)
    if len(hex_lines) > 1:
      self.logger.log(self.level, '%s read %4d B -> [%d B]:\n%s',
                      self.name, n, len(data), '\n'.join(hex_lines))
    else:
      self.logger.log(self.level, '%s read %4d B -> [%d B]: %s', self.name, n, len(data), hex_lines[0])

    return data

  def write(self, data):
    hex_lines = self._to_hex(data)
    if len(hex_lines) > 1:
      self.logger.log(self.level, '%s write      <- [%d B]:\n%s', self.name, len(data), '\n'.join(hex_lines))
    else:
      self.logger.log(self.level, '%s write      <- [%d B]: %s', self.name, hex_lines[0])

    self.child.write(data)
    return len(data)


class SerialTransport(Transport):

    def __init__(self, grep=None, port_path=None, **kw):
        self._port_path = port_path
        self._grep = grep
        self._kw = kw
        if self._port_path is None and self._grep is None:
            raise SerialPortNotFoundError('Must specify one of grep= or port_path=')

    def open(self):
        if self._port_path is not None:
            port_path = self._port_path
        else:
            ports = list(serial.tools.list_ports.grep(self._grep, include_links=True))
            if len(ports) != 1:
                raise SerialPortNotFoundError(
                    f'grep expression should find 1 serial port; found {ports!r}')

            port_path = ports[0].device

        self._port = serial.Serial(port_path, **kw)

    def close(self):
        self._port.close()
        self._port = None

    def read(self, n):
        return self._port.read(n)

    def write(self, data):
        return self._port.write(data)


class SubprocessTransport(Transport):

  def __init__(self, args, **kw):
    self.args = args
    self.kw = kw
    self.popen = None

  def open(self):
    self.kw['stdout'] = subprocess.PIPE
    self.kw['stdin'] = subprocess.PIPE
    self.popen = subprocess.Popen(self.args, **self.kw)

  def write(self, data):
    self.popen.stdin.write(data)
    self.popen.stdin.flush()

  def read(self, n):
    data = bytearray()
    while len(data) < n:
      data += self.popen.stdout.read(n)

    return data

  def close(self):
    self.popen.kill()


TransportContextManager = typing.ContextManager[Transport]

import contextlib
import serial
import serial.tools.list_ports
import typing


class SerialPortNotFoundError(Exception):
  """Raised when SerialSession cannot find the serial port specified."""


TransportContextManager = typing.ContextManager[Transport]


def transport_context_manager(fcreate, *args, **kw):
  chan = fcreate(*args, **kw)
  yield chan
  chan.Close()


class Transport(metaclass=abc.ABCMeta):

    @abc.abstractmethod
    def close(self):
      raise NotImplementedError()

    @abc.abstractmethod
    def read(self, n):
      raise NotImplementedError()

    @abc.abstractmethod
    def write(self, data):
      raise NotImplementedError()


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
        return self

    def close(self):
        self._port.close()
        self._port = None

    def read(self, n):
        return self._port.read(n)

    def write(self, data):
        return self._port.write(data)

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""Defines a Transport implementation using pyserial."""

import atexit
import logging
import serial
import serial.tools.list_ports
from .base import Transport


class SerialPortNotFoundError(Exception):
  """Raised when SerialTransport cannot find the serial port specified."""


class SerialTransport(Transport):

    _OPEN_PORTS = []

    @classmethod
    def close_atexit(cls):
        for port in cls._OPEN_PORTS:
            try:
                port.close()
            except Exception:
                _LOG.warn('exception closing serial port', exc_info=True)
                pass

        cls._OPEN_PORTS = []

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

            logging.debug('Opening serial port: %s', ports[0].device)
            port_path = ports[0].device

        self._port = serial.Serial(port_path, timeout=0.1, exclusive=True, **self._kw)
        self._port.cancel_read()
        self._port.reset_input_buffer()
        self._port.reset_output_buffer()
        self._OPEN_PORTS.append(self._port)

    def close(self):
        self._port.close()
        self._OPEN_PORTS.remove(self._port)
        self._port = None

    def read(self, n):
        to_return = bytearray()
        while not to_return:
            to_return.extend(self._port.read(n))

        while True:
            this_round = self._port.read(n - len(to_return))
            if not this_round:
                break
            to_return.extend(this_round)

        return to_return

    def write(self, data):
        # NOTE(areusch): Due to suspected flaky ST-Link VCP OS X driver, write 1 byte at a time.
        total_written = 0
        while len(data) > 0:
          num = self._port.write(data[:1])
          data = data[num:]
          assert num > 0
          total_written += num

        self._port.flush()
        return total_written

atexit.register(SerialTransport.close_atexit)

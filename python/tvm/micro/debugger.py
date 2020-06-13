import abc
import os
import signal
import subprocess
import threading
from . import transport


class Debugger(metaclass=abc.ABCMeta):

  @abc.abstractmethod
  def Start(self):
    """Start the debugger, but do not block on it.

    The runtime will continue to be driven in the background.
    """
    raise NotImplementedError()

  @abc.abstractmethod
  def Stop(self):
    """Terminate the debugger."""
    raise NotImplementedError()


class GdbDebugger(Debugger):
  """Handles launching, suspending signals, and potentially dealing with terminal issues."""

  @abc.abstractmethod
  def PopenKwargs(self):
    raise NotImplementedError()

  def Start(self):
    kw = self.PopenKwargs()
    self.old_signal = signal.signal(signal.SIGINT, signal.SIG_IGN)
    self.popen = subprocess.Popen(**kw)

  def Stop(self):
    self.popen.terminate()
    signal.signal(signal.SIGINT, self.old_signal)


class GdbTransportDebugger(GdbDebugger):
  """A debugger that uses a single GDB subprocess as both the transport and the debugger.

  Opens pipes for the target's stdin and stdout, launches GDB and configures GDB's target arguments
  to read and write from the pipes using /dev/fd.
  """

  def __init__(self, args, **popen_kw):
    self.args = args
    self.popen_kw = popen_kw

  def PopenKwargs(self):
    stdin_read, stdin_write = os.pipe()
    stdout_read, stdout_write = os.pipe()

    os.set_inheritable(stdin_read, True)
    os.set_inheritable(stdout_write, True)

    sysname = os.uname()[0]
    if sysname == 'Darwin':
      args = ['lldb',
              '-O', f'target create {self.args[0]}',
              '-O', f'settings set target.input-path /dev/fd/{stdin_read}',
              '-O', f'settings set target.output-path /dev/fd/{stdout_write}']
      if len(self.args) > 1:
        args.extend(['-O', 'settings set target.run-args {}'.format(' '.join(self.args[1:]))])
    elif sysname == 'Linux':
      args = ['gdb', '--args'] + self.args + ['</dev/fd/{stdin_read}', '>/dev/fd/{stdout_write}']
    else:
      raise NotImplementedError(f'System {sysname} is not yet supported')

    self.stdin = os.fdopen(stdin_write, 'wb', buffering=0)
    self.stdout = os.fdopen(stdout_read, 'rb', buffering=0)

    return {
      'args': args,
      'pass_fds': [stdin_read, stdout_write],
    }

  def _WaitForProcessDeath(self):
    self.popen.wait()
    self.stdin.close()
    self.stdout.close()

  def Start(self):
    to_return = super(GdbTransportDebugger, self).Start()
    threading.Thread(target=self._WaitForProcessDeath, daemon=True).start()
    return to_return

  def Stop(self):
    self.stdin.close()
    self.stdout.close()
    super(GdbTransportDebugger, self).Stop()

  class _Transport(transport.Transport):
    def __init__(self, gdb_transport_debugger):
      self.gdb_transport_debugger = gdb_transport_debugger

    def open(self):
      pass  # Pipes opened by parent class.

    def write(self, data):
      return self.gdb_transport_debugger.stdin.write(data)

    def read(self, n):
      return self.gdb_transport_debugger.stdout.read(n)

    def close(self):
      pass  # Pipes closed by parent class.

  def Transport(self):
    return self._Transport(self)


class GdbRemoteDebugger(GdbDebugger):

  def __init__(self, gdb_binary, remote_hostport, debug_binary, wrapping_context_manager=None, **popen_kw):
    self.gdb_binary = gdb_binary
    self.remote_hostport = remote_hostport
    self.debug_binary = debug_binary
    self.wrapping_context_manager = wrapping_context_manager
    self.popen_kw = popen_kw

  def PopenKwargs(self):
    kw = {
      'args': [self.gdb_binary,
               '-iex', f'file {self.debug_binary}',
               '-iex', f'target remote {self.remote_hostport}'],
    }
    kw.update(self.popen_kw)

    return kw

  def Start(self):
    self.wrapping_context_manager.__enter__()
    super(GdbRemoteDebugger, self).Start()

  def Stop(self):
    try:
      super(GdbRemoteDebugger, self).Stop()
    finally:
      self.wrapping_context_manager.__exit__(None, None, None)

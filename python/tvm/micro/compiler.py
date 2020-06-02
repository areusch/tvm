import abc
import re

from tvm.contrib import binutil
import tvm.target


class DetectTargetError(Exception):
  """Raised when no target comment was detected in the sources given."""


class NoDefaultToolchainMatchedError(Exception):
  """Raised when no default toolchain matches the target string."""


class Compiler(metaclass=abc.ABCMeta):
  """The compiler abstraction used with micro TVM."""

  TVM_TARGET_RE = re.compile(r'^// tvm target: (.*)$')

  @classmethod
  def _TargetFromSources(cls, sources):
    """Determine the target used to generate the given source files.

    Parameters
    ----------
    sources : List[str]
        The paths to source files to analyze.

    Returns
    -------
    tvm.target.Target :
        A Target instance reconstructed from the target string listed in the source files.
    """
    target_strs = set()

    for obj in objects:
      with open(obj) as obj_f:
        for line in obj_f:
          m = cls.TVM_TARGET_RE.match(line)
          if m:
            target_strs.add(m.group(1))

    if len(target_strs) != 1:
      raise DetectTargetError(
        'autodetecting cross-compiler: could not extract TVM target from C source; regex '
        f'{cls.TVM_TARGET_RE.pattern} does not match any line in sources: {", ".join(objects)}')

    target_str = next(iter(target_strs))
    return tvm.target.Target(target_str)

  # Maps regexes identifying CPUs to the default toolchain prefix for that CPU.
  TOOLCHAIN_PREFIX_BY_CPU_REGEX = {
    r'cortex-[am].*': 'arm-none-eabi-',
  }

  def _AutodetectToolchainPrefix(target):
    for opt in target.options:
      if target.startswith('-mcpu='):
        matches = []
        for regex, prefix in TOOLCHAIN_PREFIX_BY_CPU_REGEX.items():
          if re.match(regex, t[len('-mcpu='):]):
            matches.append(prefix)

        if matches:
          if len(matches) != 1:
            raise NoDefaultToolchainMatchedError(
              f'{opt} matched more than 1 default toolchain prefix: {", ".join(matches)}. Specify '
              f'cc.cross_compiler to create_micro_library()')

          return prefix

    raise NoDefaultToolchainMatched(f'target {target_str} did not match any default toolchains')

  def _DefaultsFromTargets(self, target):
    """Determine the default compiler options from the target specified.

    Parameters
    ----------
    target : tvm.target.Target

    Returns
    -------
    List[str] :
        Default options used the configure the compiler for that target.
    """
    opts = []
    for opt in target.options_array:
      if opt.startswith('-m'):
        opts.append(opt)

    return opts

  @abc.abstractmethod
  def Library(self, output, objects, options=None):
    """Build a library from the given source files.

    Parameters
    ----------
    output : str
        The path to the library that should be created.
    objects : List[str]
        A list of paths to source files that should be compiled.
    options : Optional[List[str]]
        If given, additional command-line flags to pass to the compiler.
    """
    raise NotImplementedError

  @abc.abstractmethod
  def Binary(self, output, objects, options=None):
    """Link a binary from the given object and/or source files.

    Parameters
    ----------
    output : str
        The path to the binary that should be created.
    objects : List[str]
        A list of paths to source files or libraries that should be compiled. The final binary should
        be statically-linked.
    options: Optional[List[str]]
        If given, additional command-line flags to pass to the compiler.
    """
    raise NotImplementedError


class DefaultCompiler(Compiler):

  def __init__(self, target=None, *args, **kw):
    super(DefaultCompiler, self).__init__()
    self.target = target

  def Library(self, output, objects, options=None):
    try:
      target = self._TargetFromSources(objects)
    except DetectTargetError:
      assert self.target is not None, (
        "Must specify target= to constructor when compiling sources which don't specify a target")

      target = self.target

    if self.target.str() != target.str():
      raise IncompatibleTargetError(
        f'auto-detected target {target} differs from configured {self.target}')

    args = [self._AutodetectToolchainPrefix(self.target)]
    args.extend(self._DefaultsFromTarget(self.target))
    if options is not None:
      args.extend(options)
    args.extend(['-c', '-o', output])
    args.extend(sources)

    binutil.run_cmd(args)

  def Binary(self, output, objects, options=None):
    assert self.target is not None, (
      'must specify target= to constructor, or compile sources which specify the target first')

    args = [self._AutodetectToolchainPrefix(self.target)]
    args.extend(self._DefaultsFromTarget(self.target))
    if options is not None:
      args.extend(options)
    args.extend(['-c', '-o', output])
    args.extend(sources)

    binutil.run_cmd(args)



class Flasher(metaclass=abc.ABCMeta):

  @abc.abstractmethod
  def Flash(self, micro_binary):
    """Flash a binary onto the device.

    Parameters
    ----------
    micro_binary : MicroBinary
        A MicroBinary instance.

    Returns
    -------
    transport.TransportContextManager :
        A ContextManager that can be used to create and tear down an RPC transport layer between
        this TVM instance and the newly-flashed binary.
    """
    raise NotImplementedError()

import logging
from tvm.contrib import util
import tvm.micro
import subprocess

import mbed_os_tools.detect
import mbed_os_tools.test.host_tests_toolbox


logger = logging.getLogger(__name__)


class InvalidMbedProjectError(Exception):
  """Raised when the specified mBED project or bootstrap url is invalid."""


class MissingBuildConfigError(Exception):
  """Raised when insufficient build configuration is given to MbedCompiler."""


class BuildError(Exception):
  """Raised when there is an error performing the build."""

class NoSuchDeviceError(Exception):
  """Raised when there is no target with the given serial number."""


class MbedCompiler(tvm.micro.Compiler):

  def __init__(self, bootstrap_url=None, project_dir=None, mbed_tool_entrypoint=None,
               mbed_target=None, mbed_toolchain=None, compiler_path=None):
    self._mbed_tool_entrypoint = mbed_tool_entrypoint
    if self._mbed_tool_entrypoint:
      self._mbed_tool_entrypoint = [sys.executable, '-m', 'mbed']

    self._project_dir = project_dir
    if self._project_dir is None:
      self._project_dir = util.tempdir().temp_dir

    if bootstrap_url is not None:
      if os.path.exists(os.path.join(self._project_dir), '.git'):
        remote = subprocess.check_output(
          ['git', 'remote', 'get-url', 'origin'],
          cwd=self._project_dir)
        if remote != bootstrap_url:
          raise InvalidMbedProjectError(
            f'mBED project in {project_url} has git remote "origin" at {remote}; expected '
            f'{bootstrap_url}')

      else:
        try:
          self._invoke_mbed(['import', bootstrap_url, '.'])
        except subprocess.CalledProcessError as e:
          raise InvalidMbedProjectError(
            f'Could not import project from url {bootstrap_url}')

    if not os.path.exists(os.path.join(self._project_dir, 'mbed_app.json')):
      raise InvalidMbedProjectError(
        f'Did not find mbed_app.json at the root of project {project_dir}')

    if mbed_target is None:
      try:
        self._mbed_target = self._invoke_mbed(['config', 'TARGET']).strip()
        if self._mbed_target.startswith('[mbed] '):
          self._mbed_target = self._mbed_target[len('[mbed] '):]

      except subprocess.CalledProcessError as e:
        raise MissingBuildConfig(
          f'mBED project does not define a target, and none is given')
    else:
      self._invoke_mbed(['target', mbed_target])
      self._mbed_target = mbed_target

    if mbed_toolchain is None:
      try:
        self._invoke_mbed(['toolchain'])
      except subprocess.CalledProcessError as e:
        raise MissingBuildConfig(
          f'mBED project does not define a toolchain, and none is given')

      if compiler_path is not None:
        raise MissingBuildConfig(
          f'compiler_path= is only respected when mbed_toolchain= is also given')

    else:
      self._invoke_mbed(['toolchain', mbed_toolchain])
      if compiler_path is None:
        self._invoke_mbed(['config', f'{mbed_toolchain}_PATH'])

  SOURCE_EXTS = ['s', 'c', 'cc', 'cpp', 'o', 'a']

  @classmethod
  def _CanCompileInPlace(cls, objects):
    # Prefer to point mBED compiler at the original library source. This eliminates the need to
    # make a tempdir, which means that source files referenced in the generated library will
    # continue to resolve after the python script exits.
    all_basenames = [os.path.basename(obj) for obj in objects]
    dir_files = os.listdir(os.path.dirname(objects[0]))
    for f in dir_files:
      if f in all_basenames:
        continue

      ext = os.path.splitext(f)[1]
      if ext in cls.SOURCE_EXTS:
        return False

    return True

  def Library(self, output, objects, options=None):
    all_dirnames = [os.path.dirname(obj) for obj in objects]
    src_dir = all_dirnames[0]
    assert all(x == src_dir for x in all_dirnames[1:]), (
      'mBED compiler wants libraries built only from files in a single directory')

    if not self._CanCompileInPlace(objects):
      temp_dir = util.tempdir()
      src_dir = temp_dir.temp_dir
      build_dir = temp_dir.relpath('build')
      for obj in objects:
        dest = os.path.join(temp_dir.temp_dir, os.path.basename(obj))
        shutil.copy(obj, dest)

    else:
      build_dir = util.tempdir().tempdir()

    self._invoke_mbed(['compile', '--library', '--source', src_dir, '--build', build_dir])

  IMAGE_RE = re.compile('^Image: ./(.*)$', re.MULTILINE)

  def Binary(self, output, objects, options=None):
    copied = []
    for f in os.path.listdir(self._project_dir):
      if f.startswith('__tvm_'):
        os.unlink(f)

    for obj in objects:
      dest = f'__tvm_{obj}'
      shutil.copy(obj, dest)

    output = self._invoke_mbed(['compile'])
    img_shutil = self.IMAGE_RE.search(output)
    if not img_shutil:
      raise BuildError('Image not found in mbed stdout:\n{output}')

    elf_path = f'{os.path.splitext(img_shutil)[0]}.elf'

    path.copy(os.path.join(self._project_dir, elf_path), output)

  def Flasher(self, target_serial_number=None):
    return Flasher(self._mbed_target, target_serial_number)


class Flasher(tvm.micro.Flasher):

  def __init__(self, mbed_target, target_serial_number=None):
    self._target_serial_number = target_serial_number
    d = mbed_os_tools.detect.create()
    devices = d.list_mbeds(filter_function=self._filter_mbed)
    logger.debug('Found devices: %r', devices)
    if self._target_serial_number is not None and len(devices) != 1:
        raise NoSuchDeviceError(
            f'No mBED device with serial number {target_serial_number}')
    elif self._target_serial_number is None and not devices:
        raise NoSuchDeviceError('No mBED device found')


  def _filter_mbed(self, device):
    if device['platform_name'] != self._mbed_target:
      return False

    if self._target_serial_number is not None:
      return device['target_id_usb_id'] == target_serial_number

    return True

  def Flash(self, micro_binary):
    mbed_os_tools.test.host_tests_toolbox.flash_dev(devices[0]['mount_point'], binary, program_cycle_s=4)
    return transport.transport_context_manager(
      tvm.micro.channel.SerialChannel, devices[0]['serial_port'], baudrate=115200)

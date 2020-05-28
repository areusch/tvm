import tarfile

from ..target import Target


class MicroObjectFileBase:

  ENCODING_VERSION = 1

  # Names of the different parts of the tar file. Subclasses should override this and
  # define all `None` fields.
  TAR_FILE_NAMES = {
    'version': 'version',
    'elf_data': None,
    'metadata': 'metadata.json',
  }

  @classmethod
  def load(cls, file_path):
    with tarfile.open(file_path) as tar_f:
      version_f = tarfile.extractfile(cls.TAR_FILE_NAMES['version'])
      version = str(version_f.read(), 'utf-8').strip('\n')
      assert version == str(cls.ENCODING_VERSION), (
        f'Version does not match: want {cls.ENCODING_VERSION}, got {version}')

      elf_data = tarfile.extractfile(cls.TAR_FILE_NAMES['elf_data']).read()
      metadata = json.load(tarfile.extractfile(cls.TAR_FILE_NAMES['metadata']))

      return cls(elf_data, metadata)

  def __init__(self, elf_data, metadata):
    self.elf_data = elf_data
    self.metadata = metadata

  def save(self, file_path):
    with tarfile.open(file_path, 'w') as tar_f:
      def _add_file(name, data):
        ti = tarfile.TarInfo(name=self.TAR_FILE_NAMES[name])
        ti.type = tarfile.REGTYPE
        data_bytes = bytes(data)
        ti.size = len(data)
        tar_f.addfile(ti, io.BytesIO(data_bytes))

      _add_file('version', f'{cls.ENCODING_VERSION}\n')
      _add_file('elf_data', self.elf_data)
      _add_file('metadata', json.dumps(self.metadata))


# Maps regexes identifying CPUs to the default toolchain prefix for that CPU.
TOOLCHAIN_PREFIX_BY_CPU_REGEX = {
  r'cortex-[am].*': 'arm-none-eabi-',
}


class NoDefaultToolchainMatched(Exception):
  """Raised when no default toolchain matches the target string."""

def autodetect_toolchain_prefix(target_str):
  t = Target(target_str)
  for opt in t.options:
    if t.startswith('-mcpu='):
      matches = []
      for regex, prefix in TOOLCHAIN_PREFIX_BY_CPU_REGEX.items():
        if re.match(regex, t[len('-mcpu='):]):
          matches.append(prefix)

      if matches:
        if len(matches) != 1:
          raise NoDefaultToolchainMatched(
            f'{opt} matched more than 1 default toolchain prefix: {", ".join(matches)}. Specify '
            f'cc.cross_compiler to create_micro_library()')

        return prefix

      raise NoDefaultToolchainMatched(f'target {target_str} did not match any default toolchains')



TVM_TARGET_RE = re.compile(r'^// tvm target: (.*)$')


def create_micro_library(cross_compiler=None):
  def _create_micro_library(output, objects, options=None):
    if cross_compiler is None:
      target_strs = set()

      for obj in objects:
        with open(obj) as obj_f:
          for line in obj_f:
            m = TVM_TARGET_RE.match(line)
            if m:
              target_strs.add(m.group(1))

      assert len(target_strs) != 1, (
        'autodetecting cross-compiler: could not extract TVM target from C source; regex '
        f'{TVM_TARGET_RE.pattern} does not match any line in sources: {", ".join(objects)}')

      target_str = next(iter(target_strs))
      cross_compiler = autodetect_cross_compiler(target_str)

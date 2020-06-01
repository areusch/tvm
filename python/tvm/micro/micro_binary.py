from tvm.contrib import util
from . import base


class MicroBinary(base.MicroObjectFileBase):

  TAR_FILE_NAMES = dict(MicroObjectFileBase.TAR_FILE_NAMES.items())
  TAR_FILE_NAMES['elf_data'] = '{tar_file_root}/{tar_file_root}.elf'


def build_micro_binary(compiler, name, libraries, options=None):
  temp = util.tempdir()
  libs = []
  for lib in libraries:
    dest = util.find_available_filename(temp_dir.relpath(os.path.basename(lib.name)))
    with open(dest, 'wb') as dest_f:
      dest_f.write(lib.elf_data)

  bin_path = temp.relpath(name)
  compiler.Binary(bin_path, libs, options)
  with open(bin_path, 'rb') as bin_f:
    elf_data = bin_f.read()

  return MicroBinary(name, elf_data, {})

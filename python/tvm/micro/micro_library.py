import tarfile

from tvm.contrib import util
from . import base
from . import compiler


class MicroLibrary(base.MicroObjectFileBase):

  TAR_FILE_NAMES = dict(base.MicroObjectFileBase.TAR_FILE_NAMES.items())
  TAR_FILE_NAMES['elf_data'] = '{tar_file_root}/{tar_file_root}.o'


def create_micro_library(output, objects, options=None):
  """Create a MicroLibrary using the default compiler options.

  Parameters
  ----------
  output : str
      Path to the output file, expected to end in .tar.
  objects : List[str]
      Paths to the source files to include in the library.
  options : Optional[List[str]]
      If given, additional command-line flags for the compiler.
  """
  temp_dir = util.tempdir()
  cc = compiler.DefaultCompiler()
  output = temp_dir.relpath('micro-library.o')
  cc.Library(output, objects, options=options)

  with open(output, 'rb') as output_f:
    elf_data = output_f.read()

  # TODO(areusch): Define a mechanism to determine compiler and linker flags for each lib
  # enabled by the target str, and embed here.
  micro_lib = MicroLibrary('', elf_data, {'target': cc.target.str()})
  micro_lib.save(output)

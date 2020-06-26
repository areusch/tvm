import glob
import os.path
import shutil
import typing
from ..micro_library import MicroLibrary


GLOB_PATTERNS = ['__tvm_*', 'libtvm__*']


def populate_tvm_objs(project_dir : str, objects : typing.List[MicroLibrary]):
    """Replace TVM-populated objects in a project with new ones.

    To produce micro binaries, a template project is often re-used. Some sources are provided by the
    template project and others by TVM. TVM populates files with a TVM-specific prefix (i.e. the
    patterns given in GLOB_PATTERNS). This function deletes any TVM-populated files and replaces
    them with those provided in `objects`.

    Parameters
    ----------
    project_dir : str
        Path to the project source tree.

    objects : List[MicroLibrary]
        List of MicroLibraries. Members of their `libraries` lists are copied to project_dir.
    """
    for p in GLOB_PATTERNS:
      for f in glob.glob(os.path.join(project_dir, p)):
        if os.path.isfile(f) or os.path.islink(f):
          os.unlink(f)
        elif os.path.isdir(f):
          shutil.rmtree(f)
        else:
          assert False, f"Don't know how to remove directory entry {f}"

    copied = []
    for obj in objects:
      for lib_file in obj.library_files:
        obj_base = os.path.basename(lib_file)
        if obj_base.endswith('.a'):
          dest_base = f'libtvm__{obj_base}'
        else:
          dest_base = f'__tvm_{obj_base}'

        dest = os.path.join(project_dir, dest_base)
        shutil.copy(obj.abspath(lib_file), dest)
        copied.append(dest_base)

    return copied

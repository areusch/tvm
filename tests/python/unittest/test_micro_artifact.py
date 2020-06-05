import json
import os
import shutil

from tvm.contrib import util
from tvm.micro import artifact


FILE_LIST = ['label1', 'label2', 'label12', 'unlabelled']


TEST_METADATA = {'foo': 'bar'}


TEST_LABELS = {'label1': ['label1', 'label12'],
               'label2': ['label2', 'label12']}


def build_artifact(artifact_path):
  os.mkdir(artifact_path)

  for f in FILE_LIST:
    with open(os.path.join(artifact_path, f), 'w') as lib_f:
      lib_f.write(f'{f}\n')

  sub_dir = os.path.join(artifact_path, 'sub_dir')
  os.mkdir(sub_dir)
  os.symlink('label1', os.path.join(artifact_path, 'rel_symlink'))
  os.symlink('label2', os.path.join(artifact_path, 'abs_symlink'), 'label2')
  os.symlink(os.path.join(artifact_path, 'sub_dir'),
             os.path.join(artifact_path, 'abs_dir_symlink'))


  art = artifact.Artifact(artifact_path, TEST_LABELS, TEST_METADATA)

  return art


def test_basic_functionality():
  temp_dir = util.tempdir()
  artifact_path = temp_dir.relpath('foo')
  art = build_artifact(artifact_path)

  assert art.abspath('bar') == os.path.join(artifact_path, 'bar')

  for label, paths in TEST_LABELS.items():
    assert art.label(label) == paths
    assert art.label_abspath(label) == [os.path.join(artifact_path, p) for p in paths]


def test_archive():
  temp_dir = util.tempdir()
  art = build_artifact(temp_dir.relpath('foo'))

  # Create archive
  archive_path = art.archive(temp_dir.temp_dir)
  assert archive_path == temp_dir.relpath('foo.tar')

  # Inspect created archive
  unpack_dir = temp_dir.relpath('unpack')
  os.mkdir(unpack_dir)
  shutil.unpack_archive(archive_path, unpack_dir)

  for path in FILE_LIST:
    with open(os.path.join(unpack_dir, 'foo', path)) as f:
      assert f.read() == f'{path}\n'

  with open(os.path.join(unpack_dir, 'foo', 'metadata.json')) as metadata_f:
    metadata = json.load(metadata_f)

  assert metadata['version'] == 1
  assert metadata['labelled_files'] == TEST_LABELS
  assert metadata['metadata'] == TEST_METADATA

  # Unarchive and verify basic functionality
  unarchive_base_dir = temp_dir.relpath('unarchive')
  unarch = artifact.Artifact.unarchive(archive_path, unarchive_base_dir)

  assert unarch.metadata == TEST_METADATA
  assert unarch.labelled_files == TEST_LABELS
  for f in FILE_LIST:
    assert os.path.exists(os.path.join(unarchive_base_dir, f))


if __name__ == '__main__':
  test_basic_functionality()
  test_archive()
  # TODO: tests for dir symlinks, symlinks out of bounds, loading malformed artifact tars.

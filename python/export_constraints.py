import argparse
import collections
import os
import os.path
import re
import subprocess
import typing

import gen_requirements


# A regular expression where group 1 matches the package name part of a requirements.txt line.
PACKAGE_NAME_RE = re.compile(r"^([^=<>^~]+).*$")


def _parse_requirements_txt(path : str) -> collections.OrderedDict:
    """Parse a requirements.txt-style file.

    Parameters
    ----------
    path : str
        The path to the requirements.txt-style file.

    Returns
    -------
    OrderedDict[str, str] :
        An OrderedDict whose keys are the package name and values are the full requirements.txt
        line(s) specifying those packages. Keys are ordered in the same order as they appear in the
        file.
    """
    parsed = collections.OrderedDict()
    with open(path) as f:
        package_lines = []
        package_name = None
        is_continuation = False
        for line in f:
            if not is_continuation:
                if package_lines:
                    parsed[package_name] = "".join(package_lines)
                package_name = PACKAGE_NAME_RE.match(line.rstrip("\n")).group(1)
                package_lines = []
            package_lines.append(line)
            is_continuation = line.rstrip("\n").endswith("\\")

        if package_lines:
            parsed[package_name] = "".join(package_lines)

    return parsed


def _make_export_file_name(output_base : str, piece_name : str):
    return f"{output_base}-{piece_name}.txt"


def _export_piece(piece_name : str, output_base : str, remove_piece_name : typing.Optional[str] = None):
    """Export the packages for a given piece, potentially subtrating those from another.

    Parameters
    ----------
    piece_name : str
        The name of a piece of the dependencies, as mentioned in
        gen_requirements.REQUIREMENTS_BY_PIECE.
    output_base : str
        The value of the --output-base command-line parameter. A file
        (output_base + piece_name + ".txt") will be created.
    remove_piece_name : str
        If given, remove packages from the created output file that exist in the output file
        previously created by _export_piece(remove_piece_name, output_base). Produces a delta
        constraints file.
    """
    poetry_args = ["poetry", "export", "-f", "requirements.txt"]
    if piece_name == "dev":
        poetry_args += ["--dev"]
    elif piece_name != "core":
        poetry_args += ["--extras", piece_name]

    export_file_name = _make_export_file_name(output_base, piece_name)
    export_file_parent = os.path.dirname(os.path.realpath(export_file_name))
    if not os.path.exists(export_file_parent):
      os.makedirs(export_file_parent)
    poetry_args += ["--output", export_file_name]
    print(" ".join(poetry_args))
    subprocess.check_call(poetry_args)

    if remove_piece_name:
        piece_deps = _parse_requirements_txt(export_file_name)
        remove_piece_deps = _parse_requirements_txt(_make_export_file_name(output_base, remove_piece_name))
        piece_deps = collections.OrderedDict(
            (piece, deps) for piece, deps in piece_deps.items() if piece not in remove_piece_deps)

        with open(export_file_name, "w") as export_f:
            for _, deps in piece_deps.items():
                export_f.write(deps)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
      "--output-base",
      required=True,
      help=("Output base where the various constraint files will get written. May include a "
            "directory portion plus a filename prefix e.g. docker/deps/ci_gpu. Multiple files "
            "will be written with this prefix (one per piece defined under REQUIREMENTS_BY_PIECE."))
    return parser.parse_args()


def main():
    args = parse_args()
    _export_piece("core", args.output_base)
    _export_piece("dev", args.output_base, remove_piece_name="core")
    for piece, _ in gen_requirements.REQUIREMENTS_BY_PIECE:
        if piece in ("core", "dev"):
            continue
        _export_piece(piece, args.output_base, remove_piece_name="core")


if __name__ == "__main__":
    main()

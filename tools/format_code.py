"""Format or check owned C sources without touching vendored/generated code."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check without changing files')
    parser.add_argument('--clang-format', default='clang-format', help='Formatter executable')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    files = sorted(
        path for folder in ('lib', 'app', 'tests')
        for path in (root / folder).rglob('*')
        if path.suffix in ('.c', '.h') and path.name != 'font_atlas.h'
    )
    options = ['--dry-run', '--Werror'] if args.check else ['-i']
    # One file per invocation also avoids command-line length limits on Windows.
    for path in files:
        subprocess.run([args.clang_format, *options, str(path)], cwd=root, check=True)


if __name__ == '__main__':
    main()

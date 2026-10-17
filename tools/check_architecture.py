"""Check pengUI ownership and transitive SDL-free core dependencies."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
lib = root / 'lib/pengui'
includes = re.compile(r'^\s*#include\s*[<"]([^">]+)', re.M)
errors = []


def source(path):
    text = path.read_text()
    if path.name == 'pengui.h':
        # The public header exposes SDL only when its target enables that section.
        text = text.split('#if defined(PUI_WITH_SDL) && PUI_WITH_SDL', 1)[0]
    return text


def core(path):
    relative = path.relative_to(lib).as_posix()
    return (relative.startswith('src/core/') or
            relative.startswith('src/widgets/') and not path.name.endswith('_sdl.c') or
            relative in ('src/layout/layout.c', 'src/layout/yaml.c', 'include/pengui/pengui.h'))


files = [p for p in lib.rglob('*') if p.suffix in ('.c', '.h')]
for path in files:
    for name in includes.findall(source(path)):
        if name.startswith(('app/', 'engine/', 'editor/', 'editors/', 'penguin/')):
            errors.append(f'{path.relative_to(root)}: forbidden dependency {name}')

pending = [p for p in files if core(p)]
visited = set()
while pending:
    path = pending.pop()
    if path in visited:
        continue
    visited.add(path)
    for name in includes.findall(source(path)):
        if name.startswith(('SDL', 'view/', 'rendering/')):
            errors.append(f'{path.relative_to(root)}: core reaches SDL dependency {name}')
        candidates = [path.parent / name, lib / 'src' / name, lib / 'include' / name]
        target = next((p.resolve() for p in candidates if p.is_file()), None)
        if target and target.is_relative_to(lib):
            pending.append(target)

for path in (root / 'app').rglob('*'):
    if path.suffix not in ('.c', '.h'):
        continue
    for name in includes.findall(path.read_text()):
        if 'lib/pengui/src' in name or name.startswith(('core/', 'widgets/', 'view/', 'rendering/', 'layout/')):
            errors.append(f'{path.relative_to(root)}: app must use public headers')
for error in errors:
    print(error, file=sys.stderr)
if errors:
    sys.exit(1)
print('pengUI widget ownership and transitive core / SDL boundaries: OK')

"""Keep pengUI independent of the playground and keep its core SDL-free."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
errors = []
for path in (root / 'lib/pengui').rglob('*'):
    if path.suffix not in ('.c', '.h'):
        continue
    source = path.read_text()
    if path.name == 'pengui.h':
        source = source.split('#if defined(PENGUI_WITH_SDL) && PENGUI_WITH_SDL', 1)[0]
    for include in re.findall(r'^\s*#include\s*[<"]([^">]+)', source, re.M):
        if include.startswith(('app/', 'engine/', 'editor/', 'editors/', 'penguin/')):
            errors.append(f'{path.relative_to(root)}: forbidden dependency {include}')
        if path.name in ('core.c', 'text_input.c', 'layout.c', 'pengui.h'):
            if include.startswith('SDL') or include in ('pengui/view.h', 'pengui/ui_sdl.h'):
                errors.append(f'{path.relative_to(root)}: core must remain SDL-free')
for path in (root / 'app').rglob('*'):
    if path.suffix in ('.c', '.h') and 'lib/pengui/src' in path.read_text():
        errors.append(f'{path.relative_to(root)}: app must use public headers')
for error in errors:
    print(error, file=sys.stderr)
if errors:
    sys.exit(1)
print('pengUI core / SDL / application boundaries: OK')

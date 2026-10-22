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
            relative in ('src/layout/layout.c', 'src/layout/popup.c', 'src/layout/compose.c', 'src/layout/yaml.c', 'include/pengui/pengui.h'))


files = [p for p in lib.rglob('*') if p.suffix in ('.c', '.h')]
for path in files:
    for name in includes.findall(source(path)):
        if name.startswith(('app/', 'apps/', 'engine/', 'editor/', 'editors/', 'penguin/')):
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

for path in (root / 'apps').rglob('*'):
    if path.suffix not in ('.c', '.h'):
        continue
    for name in includes.findall(path.read_text()):
        if 'lib/pengui/src' in name or name.startswith(('core/', 'widgets/', 'view/', 'rendering/', 'layout/')):
            errors.append(f'{path.relative_to(root)}: app must use public headers')
# Concrete editors meet only in the registry. Editor internals remain module-local.
for path in (root / 'editors').rglob('*'):
    if path.suffix not in ('.c', '.h'):
        continue
    domain = path.relative_to(root / 'editors').parts[0]
    for name in includes.findall(path.read_text()):
        if (name.startswith('editors/') and not name.startswith(f'editors/{domain}/') or
                name.startswith(('shell/', 'editor/', 'app/', 'apps/')) or
                name.startswith('editor_host/') and name != 'editor_host/editor.h'):
            errors.append(f'{path.relative_to(root)}: editor must depend only on its own module and common contract')
for path in (root / 'apps').rglob('*.c'):
    if path == root / 'apps/demo/editor_host/registry.c':
        continue
    for name in includes.findall(path.read_text()):
        if name.startswith('editors/'):
            errors.append(f'{path.relative_to(root)}: concrete editor includes belong in the registry')
# The editor coordinates lifecycle through the inspector facade, never individual forms.
editor_entry = root / 'editors/vector/editor.c'
entry_text = editor_entry.read_text()
if re.search(r'editor->inspector\.', entry_text):
    errors.append('editors/vector/editor.c: inspector internals belong behind its lifecycle contract')
for name in includes.findall(entry_text):
    if 'ui/inspector/' in name and not name.endswith('/inspector/inspector.h'):
        errors.append(f'editors/vector/editor.c: use the inspector facade instead of {name}')
# Vector model/history may use plain state and geometry headers, but not UI/event wiring.
vector = root / 'editors/vector'
# UI bindings are organized by panel; the root contains only the layout entry and guidance.
for path in (vector / 'ui').iterdir():
    if path.is_file() and path.suffix in ('.c', '.h'):
        errors.append(f'{path.relative_to(root)}: place UI bindings in their panel directory')
for area in ('document', 'history', 'object', 'path', 'mask', 'preview', 'image', 'repeat', 'effects', 'gradient', 'export'):
    pending = [p for p in (vector / area).rglob('*') if p.suffix in ('.c', '.h')
               and not (area in ('path', 'mask', 'preview', 'repeat', 'effects', 'gradient', 'export') and p.name in ('render.c', 'render.h'))
               and not (area in ('preview', 'gradient') and p.name in ('tool.c', 'tool.h'))
               and not (area == 'image' and p.stem in ('assets', 'import', 'render'))]
    visited = set()
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        visited.add(path)
        for name in includes.findall(path.read_text()):
            if name.startswith(('SDL', 'editor_host/')):
                errors.append(f'{path.relative_to(root)}: model/history must not reach native UI or host APIs')
            candidates = [path.parent / name, root / name]
            target = next((p.resolve() for p in candidates if p.is_file()), None)
            if not target or not target.is_relative_to(vector):
                continue
            relative = target.relative_to(vector).as_posix()
            if (relative == 'internal.h' or relative.startswith(('ui/', 'persistence/')) or
                    relative.startswith('workspace/') and relative not in ('workspace/viewport.h', 'workspace/selection_state.h', 'workspace/box_tool_state.h') or
                    relative in ('artboard/tool.h', 'rectangle/tool.h', 'rectangle/render.h', 'path/render.h', 'text/tool.h', 'text/render.h')):
                errors.append(f'{path.relative_to(root)}: model/history reaches UI wiring {relative}')
            else:
                pending.append(target)

# Rectangle geometry stays reusable across object types: no object, tool or host imports.
for path in (vector / 'geometry').rglob('*'):
    if path.suffix not in ('.c', '.h'):
        continue
    for name in includes.findall(path.read_text()):
        target = next((p.resolve() for p in (path.parent / name, root / name) if p.is_file()), None)
        if name.startswith(('SDL', 'editor_host/')) or (target and target.is_relative_to(vector) and not target.is_relative_to(vector / 'geometry')):
            errors.append(f'{path.relative_to(root)}: rectangle geometry must stay independent of objects and UI')

for error in errors:
    print(error, file=sys.stderr)
if errors:
    sys.exit(1)
print('pengUI widget ownership and transitive core / SDL boundaries: OK')

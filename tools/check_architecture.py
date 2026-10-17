"""Check owned module boundaries, including transitive includes of headless models."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'src'
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.MULTILINE)
PUBLIC = [SRC, SRC / 'engine/include', SRC / 'ui/include', SRC / 'platform/sdl/include']
MODEL = SRC / 'editors/graphics/vector'
MODEL_FILES = ['document/document.c', 'document/boolean.c', 'document/svg.c', 'document/metadata.c', 'document/history.c',
               'viewport/grid_view.c', 'viewport/preview_view.c', 'document/nanosvg.c']


def includes(path):
    for name in INCLUDE.findall(path.read_text(encoding='utf-8')):
        candidates = [path.parent / name, *(base / name for base in PUBLIC)]
        target = next((p.resolve() for p in candidates if p.is_file()), None)
        yield name, target


def check(root=SRC):
    errors = []
    for path in sorted(root.rglob('*')):
        if path.suffix not in ('.c', '.h'):
            continue
        relative = path.relative_to(SRC).as_posix()
        shared = relative.startswith(('application/', 'shell/', 'editor/'))
        if shared:
            content = path.read_text(encoding='utf-8')
            if re.search(r'\b(MapPayload|VectorEditor|PeScriptEditor)\b|workspace\.(map|script|vector)\b|pe_(map|script|vector)_active_instance', content):
                errors.append(f'{relative}: shared integration must not interpret concrete editor state')
        if relative.startswith('editors/'):
            own_key = 'vector' if relative.startswith('editors/graphics/vector/') else 'map' if relative.startswith('editors/game/map/') else 'script'
            for key in re.findall(r'pe_(map|script|vector)_active_instance', path.read_text(encoding='utf-8')):
                if key != own_key:
                    errors.append(f'{relative}: foreign editor instance access {key}')
        for name, target in includes(path):
            location = target.relative_to(SRC).as_posix() if target and target.is_relative_to(SRC) else ''
            if shared and location.startswith('editors/'):
                if relative != 'application/editors.c' or not name.endswith('/adapter.h'):
                    errors.append(f'{relative}: shared integration may only register public editor entry headers')
            if relative.startswith(('engine/', 'ui/', 'services/')):
                if location.startswith(('application/', 'shell/', 'editor/', 'editors/')):
                    errors.append(f'{relative}: forbidden application dependency {name}')
            if relative.startswith('editors/'):
                own = 'editors/graphics/vector/' if relative.startswith('editors/graphics/vector/') else 'editors/game/map/' if relative.startswith('editors/game/map/') else 'editors/script/'
                if location.startswith('editors/') and not location.startswith(own):
                    errors.append(f'{relative}: cross-editor include {name}')
                if location == 'application/internal.h':
                    errors.append(f'{relative}: use the frontend host, not the composition root')
    # Pure models/contracts must stay headless, even through an indirect include.
    starts = [*SRC.joinpath('engine').rglob('*.h'), *SRC.joinpath('engine').rglob('*.c'),
              *SRC.joinpath('editor/document').rglob('*.h'), *SRC.joinpath('editor/document').rglob('*.c'),
              *(MODEL / name for name in MODEL_FILES),
              *SRC.joinpath('ui/core').glob('*.c'), SRC / 'ui/include/penguin/ui.h',
              *SRC.joinpath('services/json').glob('*.c'), *SRC.joinpath('services/terminal').glob('*.c')]
    visited = set()
    pending = list(starts)
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        visited.add(path)
        for name, target in includes(path):
            if name.startswith('SDL') or target and target.is_relative_to(SRC) and target.relative_to(SRC).as_posix().startswith(('application/', 'shell/', 'platform/window/')):
                errors.append(f'{path.relative_to(SRC)}: headless boundary reaches {name}')
            elif target and target.is_relative_to(SRC):
                pending.append(target)
    return errors


if __name__ == '__main__':
    failures = check()
    for failure in failures:
        print(failure, file=sys.stderr)
    if failures:
        sys.exit(1)
    print('Module boundaries and headless model dependencies: OK')

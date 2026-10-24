# Tabler Icons — complete pinned catalog

Upstream: https://github.com/tabler/tabler-icons
Version: v3.49.0
Pinned revision: bbed884d15354b5cebf2493371f20dc2d5e83eaf
License: MIT, see [LICENSE](LICENSE).

All **6,238 icons** from this revision are included: **5,184 outline** and
**1,054 filled** icons. The original, unmodified SVG files are bundled in
`sources.tar.gz`, with `outline/` and `filled/` directories. The archive contains
only the icon sources; it is not unpacked or loaded by the application.

Every available YAML name is listed in [catalog.txt](catalog.txt).

```yaml
icon: tabler:artboard
icon: tabler:outline/heart
icon: tabler:filled/heart
```

`tabler:<name>` is the short form for an outline icon. `outline/` is optional;
`filled/` selects a filled variant. Names are case-sensitive and unknown names
resolve to `ICON_NONE`. Existing built-in names such as `save` remain valid.
In C use `pui_icon_from_name("tabler:filled/heart")` and `pui_icon(...)`.
Numeric icon IDs are implementation details; persist names instead.

## Offline rendering and memory

There is **no automatic icon download**. The entire catalog is embedded in the
SDL library. No network access, resource path or SVG parser is needed at runtime.
Only a texture page needed for drawing is decoded and cached by its renderer;
the application does not create thousands of textures at startup.

`lib/pengui/src/rendering/tabler_atlas.bin` contains compressed PNG pages of
16 tintable 64x64 masks each. `tabler_atlas.h` maps names to pages and slots.
CMake embeds the binary asset into a generated C source during the normal build,
without Python or additional packages. SDL releases cached pages with the renderer.
Colors follow widget state. These masks target small UI icons, not arbitrary-size
SVG artwork or a general-purpose SVG renderer.

## Rebuilding or updating the catalog

Regeneration is a development task; it is not performed during a normal build:

1. Install `CairoSVG==2.8.2` and Pillow, plus Cairo's native library if needed.
2. Run `python tools/generate_tabler_icons.py` from the repository root.
3. Run `python tools/format_code.py`, build, and test.
4. Commit `tabler_atlas.bin`, `tabler_atlas.h`, and `catalog.txt` together.

For a version update, replace `sources.tar.gz` with the complete outline and
filled sources from the chosen upstream revision, update provenance and license,
then regenerate all artifacts. The generated pages use lossless PNG compression;
individual icon pixels are rendered from the original SVGs using CairoSVG.
New upstream releases are not downloaded automatically.

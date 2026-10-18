# Tabler Icons (outline selection)

Upstream: https://github.com/tabler/tabler-icons
Version: v3.49.0
Pinned revision: bbed884d15354b5cebf2493371f20dc2d5e83eaf
License: MIT, see [LICENSE](LICENSE).

This directory contains 52 unmodified outline SVGs from `icons/outline/` in that
revision. This is a selection, not the complete upstream catalog. The SVGs are
the source assets; the normal application does not load or download them.

`tools/generate_tabler_icons.py` converts the selected SVGs into embedded 64x64
alpha masks in `lib/pengui/src/rendering/tabler_atlas.h`. Runtime textures are
created lazily and tinted by pengUI, so active/disabled/hover styles remain
consistent. Small UI icons up to 64px are the intended use; these masks are not
a general-purpose SVG renderer.

To add another outline icon:

1. Copy the original SVG from the pinned upstream revision into `icons/` here.
2. Install the development dependencies: `python -m pip install Pillow svgpathtools`.
3. Run `python tools/generate_tabler_icons.py` from the repository root.
4. Run `python tools/format_code.py`, then build and test.
5. Commit the SVG and generated header together, keeping the upstream license.

Use `icon: tabler:<filename-without-svg>` in YAML, for example
`icon: tabler:artboard` or `icon: tabler:file-plus`. C consumers use
`pui_icon_from_name("tabler:artboard")`, then `pui_icon(...)`.
Names are case-sensitive; unknown names resolve to `ICON_NONE`. All files in
`icons/` are supported. Existing built-in names such as `save` remain valid.

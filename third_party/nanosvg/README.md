# NanoSVG

Upstream: https://github.com/memononen/nanosvg

Vendored revision: `239e102ec2c691f2902e20ace2ed36ee4a35cfe6`.
The unmodified `src/nanosvg.h` and `src/nanosvgrast.h` headers retain the upstream zlib license notices. They are compiled once as C by `src/editor/editors/vector/nanosvg.c`. No network download is needed to build this dependency.

SHA-256:

- `nanosvg.h`: `e34fd5d084be106cea972d19ce5d27fd96d17ba89f8d06bdceee058420c8b2b0`
- `nanosvgrast.h`: `79a9c5f4db19debf9f3a648a1589e96d92854f245a5cb4f3d823f263785234d8`

Penguin restricts SVG import to supported solid-color geometry and stores its own artboard metadata separately. NanoSVG is not a general XML/SVG document editor.

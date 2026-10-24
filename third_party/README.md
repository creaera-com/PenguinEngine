# Retained dependencies

- libyaml 0.2.5: YAML parsing; license in libyaml/LICENSE.
- stb_image: bounded PNG/JPEG decoding; license in stb/LICENSE and stb_image.h.
- Unity: C tests; license in unity/LICENSE.txt.
- JetBrains Mono 2.304: source font and SIL OFL in resources/ui/fonts.
- stb_truetype 1.26: text rasterization; fetched and SHA-256 verified by cmake/TextFonts.cmake; MIT/public-domain license in the header.
- SDL3 >= 3.4 (fetch pin 3.4.18): found or fetched by CMake; not vendored here.

The previous engine-only dependencies are excluded from this branch.

Tabler Icons: vollständiger eingebetteter Katalog von v3.49.0 (6.238 Outline-/Filled-Icons), MIT. Quellen, Lizenz und
Regenerationsanleitung unter [tabler-icons/README.md](tabler-icons/README.md).

- stb_image_write 1.16: PNG/JPEG export, vendored from [nothings/stb](https://github.com/nothings/stb/blob/master/stb_image_write.h); MIT/public domain, license in stb/LICENSE and the header.
  Retained header SHA-256: `92269d6955c03809dd9193e2ead06503b94c24756aa1f30c7949f3dcca037b7e`.

- libtess2: der gemeinsame Geometriekern von pengDraw verwendet die mitgelieferten C-Quellen;
  SGI Free Software License B 2.0, Lizenz unter libtess2/LICENSE.txt.

# Abhängigkeiten

| Bibliothek | Version/Revision | Verwendung | Lizenz |
| --- | --- | --- | --- |
| SDL3 | 3.2.10, oder installiertes SDL3 >= 3.2 | Plattform und Rendering | zlib, im SDL-Quellarchiv |
| NanoSVG | 239e102ec2c691f2902e20ace2ed36ee4a35cfe6 | SVG-Geometrie und Rastervorschau | zlib, Hinweise in `nanosvg/*.h` |
| Unity | 2.6.1 | C-Unit-Tests | MIT, `unity/LICENSE.txt` |
| cJSON | 1.7.19 | Projekt-/Szenen-JSON | MIT, `cjson/LICENSE` |
| QuickJS-ng | 0.10.1 | JavaScript-Syntaxprüfung, ohne Ausführen von Scripts | MIT, `quickjs-ng/LICENSE` |
| stb_image | f0569113c93ad095470c54bf34a17b36646bbbb5 | PNG-Laden | MIT/Public Domain, `stb/LICENSE` und Header |

Unity, cJSON, stb_image und die benötigten QuickJS-ng-Quellen sind vendort. QuickJS-ng stammt aus dem unveränderten Release-Archiv https://github.com/quickjs-ng/quickjs/archive/refs/tags/v0.10.1.tar.gz; gebaut werden nur die Bibliotheksquellen, keine CLI und keine OS-Module. Nur SDL wird bei Bedarf durch CMake heruntergeladen. Der SHA-256 des gepinnten SDL-Archivs wird von CMake geprüft. Drittanbieterquellen werden nicht mit Projektformatierung verändert.

Das Beispiel-Tileset wurde eigens prozedural erzeugt und verwendet keine Grafiken anderer Spiele. Für den eigenen Engine-Code ist noch keine Repository-Lizenz festgelegt.

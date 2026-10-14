# Abhängigkeiten

| Bibliothek | Version/Revision | Verwendung | Lizenz |
| --- | --- | --- | --- |
| SDL3 | 3.2.10, oder installiertes SDL3 >= 3.2 | Plattform und Rendering | zlib, im SDL-Quellarchiv |
| Unity | 2.6.1 | C-Unit-Tests | MIT, `unity/LICENSE.txt` |
| cJSON | 1.7.19 | Projekt-/Szenen-JSON | MIT, `cjson/LICENSE` |
| stb_image | f0569113c93ad095470c54bf34a17b36646bbbb5 | PNG-Laden | MIT/Public Domain, `stb/LICENSE` und Header |

Unity, cJSON und stb_image sind vendort. Nur SDL wird bei Bedarf durch CMake heruntergeladen. Der SHA-256 des gepinnten SDL-Archivs wird von CMake geprüft. Drittanbieterquellen werden nicht mit Projektformatierung verändert.

Das Beispiel-Tileset wurde eigens prozedural erzeugt und verwendet keine Grafiken anderer Spiele. Für den eigenen Engine-Code ist noch keine Repository-Lizenz festgelegt.

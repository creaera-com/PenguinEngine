# Codestil und API-Dokumentation

Unsere Header enthalten Doxygen-Kommentare für Module, Datentypen, Felder und Funktionen. Die öffentliche Engine-/UI-API beschreibt zusätzlich Parameter, Einheiten und wichtige Besitz- und Fehlerregeln. Editorinterne Verträge sind ausdrücklich als intern gekennzeichnet. Generierte Fontdaten und Fremdbibliotheken werden nicht als eigene API dokumentiert.

## Dokumentation erzeugen

Doxygen installieren und CMake danach erneut konfigurieren. Graphviz ist nicht erforderlich. Unter Debian/Ubuntu beispielsweise `sudo apt install doxygen`; unter macOS `brew install doxygen`. Unter Windows Doxygen installieren und dessen Programmverzeichnis in PATH aufnehmen.

```sh
cmake -S . -B build
cmake --build build --target api_docs
```

Die Startseite liegt anschließend unter `build/docs/html/index.html`. Die Dokumentation wird nur mit dem expliziten Target `api_docs` erzeugt. Fehlt Doxygen, bleiben normale Builds unverändert möglich; das optionale Target steht dann nicht zur Verfügung. Falls CMake den Installationspfad nicht findet, `-DDOXYGEN_EXECUTABLE=/pfad/zu/doxygen` beim Konfigurieren angeben.

Die Konfiguration liegt in `docs/doxygen/Doxyfile.in`, der Einstieg mit Modulgruppen in `docs/doxygen/mainpage.dox`. CMake erzeugt daraus `build/Doxyfile` mit absoluten Pfaden für den jeweiligen Buildordner. Dokumentationswarnungen lassen den Dokumentationsbuild fehlschlagen.

## Theme und Navigation

Die HTML-Doku verwendet **Doxygen Awesome 2.3.4** mit einer dauerhaft sichtbaren, aufklappbaren Navigation links, integrierter Suche und orangefarbenen Akzenten. Das Theme folgt der Hell-/Dunkeleinstellung des Systems; auf schmalen Bildschirmen passt sich die Navigation an.

Die unveränderten MIT-lizenzierten Styles liegen unter `third_party/doxygen-awesome-css/`. Eigene Farben und Abstände werden in `docs/doxygen/penguin-theme.css` überschrieben. Sidebarbreite und `TREEVIEW_WIDTH` müssen übereinstimmen (aktuell 335 Pixel). Alle Dateien werden lokal mitgeliefert; beim Erzeugen oder Lesen wird kein CDN benötigt.

Nach einer Änderung der Doxygen-Konfiguration CMake erneut konfigurieren und `api_docs` bauen. Die Zieladresse bleibt `build/docs/html/index.html`.

## Kommentare pflegen

Die Beschreibung soll den Vertrag erklären: Was tut eine Funktion, wem gehören ihre Ressourcen, welche Einheiten haben ihre Werte und was geschieht bei Fehlern? Parametertexte gehören zur Deklaration im Header. Implementierungsdetails und Begründungen für einzelne Algorithmen bleiben bei der Implementierung.

```c
/**
 * @brief Wandelt eine Position aus dem Viewport in Weltpixel um.
 * @param camera Kamera mit positivem Zoomfaktor.
 * @param sx Horizontale Position relativ zum Viewport in Pixeln.
 * @param sy Vertikale Position relativ zum Viewport in Pixeln.
 * @param wx Ausgabe der horizontalen Weltposition in Pixeln.
 * @param wy Ausgabe der vertikalen Weltposition in Pixeln.
 */
void pe_screen_to_world(const PeCamera *camera, float sx, float sy, float *wx, float *wy);
```

`@file` beschreibt die Datei, `@brief` den Zweck und `///<` ein Datenfeld. Bei geliehenen Zeigern werden Eigentümer und Gültigkeitsdauer genannt. Ein erfolgreicher Doxygenlauf prüft Syntax und fehlende Kommentare; die fachliche Richtigkeit der Texte bleibt Teil des Reviews.

## Formatierung

Für eigenen C-Code verwenden wir clang-format **23.1.3**; die Regeln stehen in `.clang-format`. Auch Beispiele und Tests folgen diesen Regeln. Bibliotheken unter `third_party/` und der generierte `font_atlas.h` bleiben ausgeschlossen.

- Vier Leerzeichen Einrückung, bis zu 100 Zeichen je Zeile.
- Immer geschweifte Klammern für `if`, `else`, `for`, `while` und `do`, auch bei einer Anweisung. `else if` darf als zusammenhängende Kette stehen.
- Leerzeilen zwischen Funktionsdefinitionen und zwischen logischen Abschnitten, beispielsweise nach einer Eingabeprüfung vor der eigentlichen Verarbeitung.
- Höchstens eine aufeinanderfolgende Leerzeile. Die Abschnittstrennung innerhalb einer Funktion erfolgt bewusst; nicht nach jeder Anweisung.

`InsertBraces` und `SeparateDefinitionBlocks` setzen die grundlegenden Regeln beim Formatieren um. Bei Präprozessor-/Makrokonstruktionen kann clang-format keine vollständige semantische Prüfung ersetzen; solche Blöcke weiterhin sorgfältig reviewen.

Alle eigenen Quellen formatieren bzw. prüfen:

```sh
python tools/format_code.py
python tools/format_code.py --check
```

Das Skript benötigt clang-format in PATH. Für eine konkrete Installation lässt sich `--clang-format /pfad/zu/clang-format` angeben. Die CI nutzt dieselbe Dateiauswahl und Version und erzeugt zusätzlich die API-Dokumentation.

## Handbuch in der Doxygen-Navigation

Die Seitennavigation enthält unter **Handbuch** auch die Markdown-Anleitungen aus `docs/` und die README. Projektstruktur und Architektur stehen dabei am Anfang; weitere Gruppen führen zu Oberfläche/Werkzeugen und Entwicklung/Tests. Die Seiten werden direkt aus den Markdown-Dateien erzeugt, sodass GitHub und Doxygen dieselben Inhalte verwenden. Neue Anleitungen unter `docs/` werden automatisch eingelesen; ihre Einordnung wird in `docs/doxygen/mainpage.dox` mit `@subpage` gepflegt.

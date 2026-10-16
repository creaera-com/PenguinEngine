# Projektstruktur und Einstieg

PenguinEngine besteht aus einer wiederverwendbaren Engine, einer UI-Bibliothek und einer Editor-Anwendung. Der gesamte eigene C-Code liegt unter `src/`. Fremdbibliotheken bleiben unter `third_party/`, Layouts und Fonts unter `resources/`.

```text
src/
├── main.c                     Dünner Programmeinstieg
├── application/               Start, Hauptschleife und Zusammenbau der Anwendung
│   ├── application.h          pe_application_run als Einstieg
│   ├── editors.c              Registrierung der konkreten Editoren
│   └── internal.h             Composition Root mit konkreten Editor-Headern
├── platform/
│   ├── window/                SDL-Fenster, Hit-Tests und Fensterzustand
│   ├── sdl/                   Szenenrenderer und Textur-Lebenszyklus
│   └── terminal/              PTY-Prozessvertrag und Plattformimplementierungen
├── ui/                        In Spielen und Editor nutzbare Widgets
│   ├── core/                  Fokus, Eingaben und Widget-Geometrie
│   ├── rendering/             SDL-Darstellung, Icons und Font-Atlas
│   └── include/penguin/       Öffentliche UI-Header
├── shell/                     Gemeinsamer sichtbarer Rahmen der Anwendung
│   ├── shell.c                Spalten, Navigation, Toolbar und Statuszeile
│   ├── input.c                Globale Ereignisverteilung
│   ├── explorer/              Projekt-Dateibaum
│   ├── dialogs/               Gemeinsame Textfelder und Eingabeabläufe
│   └── console/               Geteilte Terminalansicht und Shellkommunikation
├── editor/                    Infrastruktur für austauschbare Editoren
│   ├── module.h               Editor-Lebenszyklus und Aktionsvertrag
│   ├── host.h                 Privater Frontend-Host; Dokumenttypen bleiben opaque
│   ├── document/              Gemeinsamer SDL-freier Dokumentvertrag
│   ├── layout/                YAML-Schema, Rendering und Bindings
│   └── workspace/             Wechsel, Entladen, Rückfall und Sitzungsdateien
├── editors/                   Konkrete fachliche Werkzeuge
│   ├── game/map/              Spielszene, Tilemap, ECS-Inspector und Play
│   ├── graphics/vector/
│   │   ├── document/          Formen, Pfade, SVG, Boolesche Operationen und Undo
│   │   ├── tools/             Aktionen, Eingaben und Feldvalidierung
│   │   ├── panels/            Eigenschaften und Objektbaum
│   │   └── viewport/          Zeichnung, Kamera, Grid und 1:1-Vorschau
│   └── script/                Textansicht und Script-Werkzeuge
├── services/
│   ├── json/                  Modellunabhängiges Einlesen und Validieren
│   ├── project/               Projektkontext der geöffneten Editor-Anwendung
│   └── terminal/              SDL-freies VT-Modell und Tastencodierung
└── engine/                    Laufzeitcode für die erstellten Spiele
    ├── include/penguin/       Öffentliche Engine- und Script-APIs
    ├── ecs/                   Entities, Komponenten und Hierarchie
    ├── scene/                 Szenenmodell und JSON-Szenendateien
    ├── tilemap/               Tile-Zugriff und Bearbeitungsoperationen
    ├── camera/                Kamera- und Koordinatenrechnung
    ├── collision/             Tile-Kollisionen
    ├── history/               Szenen-Undo/Redo
    ├── project/               Projektsuche und Manifestformat
    ├── scripting/             Textmodell und JavaScript-Sprachunterstützung
    └── runtime/               Spielaktualisierung
```

`editor/` beschreibt den gemeinsamen Vertrag, `editors/` die einzelnen Implementierungen. Mit „Shell“ ist die Programmoberfläche gemeint; die Betriebssystem-Shell läuft im gemeinsamen Konsolendienst. Neue Oberflächen-Widgets gehören nach `ui/`, nicht in einen bestimmten Editor.

## Wo fange ich an?

| Aufgabe | Einstieg |
| --- | --- |
| Anwendung starten oder Argument ergänzen | `src/main.c`, `src/application/application.c` |
| Gemeinsame Toolbar oder Panels anordnen | `resources/editor/ui/shell.yaml`, `resources/editor/ui/editors/*.yaml` |
| Neuen Editor anbinden | `src/editor/module.h`, `src/application/editors.c` |
| Speichern/Undo in einem Editor ändern | dessen `adapter.c`, dann Dokumentmodell |
| Tilemap-/Scene-Funktion ergänzen | `src/editors/game/map/`, fachliche Laufzeitoperation in `src/engine/` |
| Vector-Werkzeug ergänzen | `src/editors/graphics/vector/tools/` und `document/` |
| UI-Element in Editor und Spiel nutzen | `src/ui/include/penguin/`, `core/`, `rendering/` |
| Fehler beim Editorwechsel untersuchen | `src/editor/workspace/lifecycle.c`, danach Editor-Checkpoint |
| Terminal-Plattformport ergänzen | `src/platform/terminal/process.h` |

Die [Architektur-Anleitung](ARCHITEKTUR.md) erklärt Abhängigkeiten, Besitzregeln und konkrete Erweiterungsabläufe. Die [YAML-Doku](UI_YAML.md) beschreibt die Oberfläche.

## CMake und CLion

Jede Domäne pflegt ihre Quellen in einer eigenen `CMakeLists.txt`. Die Datei im Repository-Wurzelverzeichnis verbindet die Module und definiert Build-Optionen. `cmake/Dependencies.cmake` enthält die Fremdbibliotheken und SDL-Suche. Neue Quelldateien werden ausdrücklich im zuständigen Modul eingetragen; wir verwenden kein automatisches Dateiglob für C-Quellen.

Nach diesem Refactoring in CLion das CMake-Projekt neu laden. Programme heißen weiterhin `penguin_editor`, `penguin_game` und `penguin_ui_demo`; YAML-, Projekt-, Szenen- und SVG-Dateien behalten ihre Pfade und Formate. Die ausführbaren Dateien liegen weiterhin im Build-Verzeichnis (bei Visual Studio gegebenenfalls unter `Debug/`).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Für Tests wird Python 3.9 oder neuer für die Architekturprüfung benötigt. Ein Produktionsbuild mit `-DBUILD_TESTING=OFF` benötigt kein Python. Doxygen und der Font-Bake bleiben optionale Werkzeuge.

```sh
python3 tools/check_architecture.py
cmake -S . -B build-core -DPENGUIN_BUILD_EDITOR=OFF
cmake --build build-core --parallel 4
ctest --test-dir build-core --output-on-failure
```

Mit ausgeschaltetem Editor laufen Dokumentvertrag und Modelltests ohne SDL. Ein Testbuild enthält zusätzlich die echten Editor-Ereignistests; ein Produktionsbuild verwendet dieselben Module ohne Testfunktionen. `--smoke` beendet sich in beiden Fällen nach wenigen Frames.

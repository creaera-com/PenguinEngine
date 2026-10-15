# Projektstruktur

Unser produktiver C-Code liegt unter `src/`, geordnet nach fachlicher Verantwortung. Tests, Beispielprogramme, Ressourcen und Fremdbibliotheken besitzen eigene Verzeichnisse.

```text
PenguinEngine/
├── src/
│   ├── engine/
│   │   ├── include/penguin/     Öffentliche Engine- und Script-Schnittstellen
│   │   ├── project/             Projektsuche und JSON-Projektmanifest
│   │   ├── scene/               Szeneninitialisierung und JSON-Szenendateien
│   │   ├── ecs/                 Entity-Kennungen und Komponentenmasken
│   │   ├── tilemap/             Tile-Zugriff, Füllen und Rechteckwerkzeug
│   │   ├── history/             Szenenänderungen mit Undo/Redo
│   │   ├── camera/              Welt-/Bildschirmkoordinaten und Zoom
│   │   ├── collision/           Tile-Kollision und kollisionsgeprüfte Bewegung
│   │   ├── runtime/             Laufzeitaktualisierung und Spielersteuerung
│   │   └── scripting/           Textdokumente und JavaScript-Sprachunterstützung
│   ├── editor/
│   │   ├── app/                 Start, Hauptschleife, Shell und Eingabeverteilung
│   │   ├── browser/             Projekt-Dateibaum und Dateityperkennung
│   │   ├── dialogs/             Modale Rückfragen und Texteingabefelder
│   │   ├── editors/
│   │   │   ├── map/             Map-Dokument, Werkzeuge, Eingaben, Panels und Play
│   │   │   └── scripts/         Textansicht, Script-Aktionen, Infopanel und Checkpoint
│   │   ├── layout/              YAML-Loader, Darstellung und Binding-/Aktionsadapter
│   │   └── workspace/           Dokumentaktionen, Editor-Lebenszyklus und Checkpoints
│   ├── ui/
│   │   ├── include/penguin/     Öffentliche UI-Schnittstellen
│   │   ├── core/                Eingabe, Fokus, Layout und Widget-Zustände
│   │   └── rendering/           SDL-Darstellung, Icons und eingebetteter Font-Atlas
│   ├── platform/
│   │   └── sdl/                SDL-Szenenrenderer und öffentliche Render-Schnittstelle
│   └── shared/
│       └── json/               Gemeinsames JSON-Einlesen und grundlegende Validierung
├── resources/
│   ├── editor/ui/              Ausgelieferte Shell- und Editor-YAML-Dateien
│   └── ui/fonts/               Originalschrift und SIL Open Font License
├── examples/                   Eigenständige Beispielprogramme und Spielprojekte
├── tests/                      Unity-Tests und CMake-/SDL-Integrationstests
├── third_party/                Mitgelieferte Bibliotheken und deren Lizenzen
├── docs/                       Anleitungen und Screenshots
├── tools/                      Entwicklungswerkzeuge, etwa der Font-Atlas-Generator
└── cmake/                      Hilfsskripte für den Build
```

## Zuordnung von Code

Ein Modul liegt bei der Domäne, deren Aufgabe es erfüllt. Der Script-Editor gehört beispielsweise nach `src/editor/editors/scripts/`, die JavaScript-Syntaxprüfung dagegen nach `src/engine/scripting/`: Sie benötigt keine Editoroberfläche. Weitere Editoren erhalten später eigene Ordner unter `src/editor/editors/`, etwa `dialogs/` oder `fx/`.

Interne Header liegen neben der zugehörigen Implementierung. Öffentlich nutzbare Engine-, UI- und SDL-Schnittstellen liegen in den jeweiligen `include/penguin/`-Verzeichnissen. CMake exportiert deren Include-Pfade. Aufrufer verwenden weiterhin beispielsweise `#include "penguin/engine.h"` und `#include "penguin/ui.h"`.

Die Engine hängt nicht vom Editor ab. Die UI-Bibliothek ist ebenfalls unabhängig vom Editor und kann in Spielen verwendet werden. Editorinterne Header wie `layout/ui_layout.h` und `workspace/session.h` werden ausschließlich vom Editor und seinen Tests verwendet.

`src/shared/json/` enthält die von Szenen- und Projektdateien gemeinsam verwendeten JSON-Helfer: begrenztes Einlesen, Zahlenprüfung, Fehlertexte und sichere relative Pfade. Diese Helfer kennen weder Szenen noch Editoren. Gemeinsam verwendete Widgets bleiben dagegen bei ihrer fachlichen Zuständigkeit unter `src/ui/`.

## Zusammenspiel der Module

`src/editor/app/main.c` liest Programmargumente, initialisiert die Anwendung und führt die Hauptschleife aus. Die Anwendungshülle zeichnet `app/shell.c`; `app/input.c` verteilt Eingaben an globale Bedienung, Dialoge, Script-Editor und Map-Eingabe. Map-spezifische Tasten, Mausaktionen, Zoom und Play-Aktualisierung liegen unter `editors/map/`.

Der Map-Editor ist in `document.c` (Dateioperationen, Tileset und Bearbeitungstransaktionen), `tools.c` (Pinsel, Füllen und Auswahl), `input.c`, `actions.c`, `fields.c` (Inspector-Werte), `panels.c`, `canvas.c`, `runtime.c` und `checkpoint.c` aufgeteilt. Der Script-Editor besitzt seine Textansicht sowie eigene Aktions-, Panel- und Checkpoint-Module.

`workspace/documents.c` koordiniert globale Befehle wie Speichern, Undo/Redo und Öffnen. `workspace/lifecycle.c` sichert den aktuellen UI-Zustand, entlädt den bisherigen Editor und aktiviert den nächsten. Die tatsächlichen Dokumentdaten serialisieren die jeweiligen `checkpoint.c`-Module. Diese erhalten Dokument und Kontext als Argumente und benötigen weder den globalen Editorzustand noch ein geöffnetes Fenster. Ihre eigenständigen Unity-Tests verifizieren auch nach dem Wiederherstellen unabhängiges Undo/Redo.

Die interne Anwendungsschnittstelle `app/editor_internal.h` führt die privaten Modulverträge zusammen und beschreibt den gemeinsamen Editorzustand. Die UI-Adapter greifen weiterhin auf diese eine Anwendungsinstanz zu. Das ist eine Aufteilung in fachliche C-Module, noch keine vollständige Entkopplung aller UI-Adapter oder Unterstützung mehrerer Editorfenster. Engine und wiederverwendbare UI-Bibliothek kennen diesen Zustand nicht. Änderungen daran bleiben auf den Editor beschränkt.

Die öffentlichen Engine-Funktionen aus `penguin/engine.h` sind unverändert. Ihre Implementierungen liegen nun in der jeweiligen Domäne statt in einem gemeinsamen `scene/core.c`. Projektmanifeste werden unter `engine/project/` verarbeitet; Szenenserialisierung bleibt unter `engine/scene/`.

## Tests und Produktionsbuild

Die bisherigen GUI-Smoke-Prüfungen liegen unter `tests/editor_smoke.c`. Sie werden nur bei `BUILD_TESTING=ON` in den Editor eingebunden. `--smoke` führt dann dieselben Eingabe- und Editorwechselprüfungen wie bisher aus und beendet sich nach wenigen Frames.

Mit `-DBUILD_TESTING=OFF` enthält der Editor diese Testfunktionen nicht. `--smoke` bleibt als kurzer Start-/Rendercheck verfügbar. Der Produktionsbuild und die normalen Programme verwenden ansonsten dieselben Module.

## Build und Ressourcen

Die CMake-Targets und Namen der Programme bleiben gleich. Nach dem Aktualisieren des Repositories in CLion das CMake-Projekt neu laden. Beim Bauen kopiert CMake `resources/editor/ui/` weiterhin nach `build/ui-layouts/`. Der normale Programmstart und die Projektsuche bleiben gleich.

Eigene Layouts lassen sich wie bisher laden:

```sh
build/penguin_editor --ui resources/editor/ui/shell.yaml
```

Wenn bisher ein eigener `--ui`-Pfad auf `editor/ui/shell.yaml` zeigte, muss er auf den neuen Ressourcenpfad angepasst werden. **F6** lädt die aktive Beschreibung erneut. Die Details stehen in [UI_YAML.md](UI_YAML.md).

`tools/bake_font.py` liest die Schrift aus `resources/ui/fonts/` und schreibt den generierten Header nach `src/ui/rendering/font_atlas.h`. Der normale Build benötigt weiterhin kein Python und kein Pillow.

Die Befehle zum Bauen und Testen stehen in der [README](../README.md); die Testfälle in [TESTPLAN.md](TESTPLAN.md).

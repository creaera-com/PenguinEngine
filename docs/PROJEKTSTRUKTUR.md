# Projektstruktur

Unser produktiver C-Code liegt unter `src/`, geordnet nach fachlicher Verantwortung. Tests, Beispielprogramme, Ressourcen und Fremdbibliotheken besitzen eigene Verzeichnisse.

```text
PenguinEngine/
├── src/
│   ├── engine/
│   │   ├── include/penguin/     Öffentliche Engine- und Script-Schnittstellen
│   │   ├── project/             Projektsuche und Pfadauflösung
│   │   ├── scene/               Szenenmodell, Bearbeitung und JSON-Serialisierung
│   │   └── scripting/           Textdokumente und JavaScript-Sprachunterstützung
│   ├── editor/
│   │   ├── app/                 Programmstart und Anwendungshülle
│   │   ├── editors/
│   │   │   ├── map/             Interne Map-Editor-Datentypen
│   │   │   └── scripts/         Script-Editor mit Textansicht und Eingaben
│   │   ├── layout/              YAML-Laden und Schema-Validierung
│   │   └── workspace/           Temporäre Sitzungs-Checkpoints
│   ├── ui/
│   │   ├── include/penguin/     Öffentliche UI-Schnittstellen
│   │   ├── core/                Eingabe, Fokus, Layout und Widget-Zustände
│   │   └── rendering/           SDL-Darstellung, Icons und eingebetteter Font-Atlas
│   └── platform/
│       └── sdl/                SDL-Szenenrenderer und öffentliche Render-Schnittstelle
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

Ein Verzeichnis `src/shared/` ergänzen wir erst, wenn tatsächlich fachlich unabhängige Hilfsfunktionen für mehrere Domänen entstehen. Ein Baustein wird nicht allein deshalb nach Shared verschoben, weil ihn mehrere Aufrufer nutzen: Gemeinsam verwendete Widgets gehören beispielsweise weiterhin zu `src/ui/`.

## Noch zusammenhängende Implementierungen

Dieses Refactoring verschiebt vorhandene Module, ohne Funktionen oder Datenformate zu verändern. `src/engine/scene/core.c` enthält weiterhin gemeinsam Szeneninitialisierung, Entities, Tile-Werkzeuge, Undo/Redo, Kamera und einfache Kollisionen. Das spätere Aufteilen dieser Datei in kleinere Module kann separat erfolgen.

Auch `src/editor/app/main.c` enthält noch einen großen Teil der Map-Bedienung, der Anwendungshülle und des Editor-Lebenszyklus. Die Map-Datentypen liegen bereits in `src/editor/editors/map/map_editor.h`; der Script-Editor besitzt schon ein eigenes Implementierungsmodul. Das schrittweise Herauslösen der Map-Funktionen ist ein weiterer Refactoring-Schritt. Die Ordnerstruktur allein führt noch keine vollständige Modultrennung ein.

## Build und Ressourcen

Die CMake-Targets und Namen der Programme bleiben gleich. Nach dem Aktualisieren des Repositories in CLion das CMake-Projekt neu laden. Beim Bauen kopiert CMake `resources/editor/ui/` weiterhin nach `build/ui-layouts/`. Der normale Programmstart und die Projektsuche bleiben gleich.

Eigene Layouts lassen sich wie bisher laden:

```sh
build/penguin_editor --ui resources/editor/ui/shell.yaml
```

Wenn bisher ein eigener `--ui`-Pfad auf `editor/ui/shell.yaml` zeigte, muss er auf den neuen Ressourcenpfad angepasst werden. **F6** lädt die aktive Beschreibung erneut. Die Details stehen in [UI_YAML.md](UI_YAML.md).

`tools/bake_font.py` liest die Schrift aus `resources/ui/fonts/` und schreibt den generierten Header nach `src/ui/rendering/font_atlas.h`. Der normale Build benötigt weiterhin kein Python und kein Pillow.

Die Befehle zum Bauen und Testen stehen in der [README](../README.md); die Testfälle in [TESTPLAN.md](TESTPLAN.md).

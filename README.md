# pengUI

Eigenständige UI-Bibliothek in C17 mit SDL3-Darstellung und einer kleinen Widget-Spielwiese.
Dieser Entwicklungsbranch enthält bewusst keine Engine, kein ECS und keine Spiele- oder Grafikeditoren. Der bisherige Engine-Stand bleibt in den anderen Branches und der Git-Historie erhalten.

## Starten

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
./build/pengui_playground
```

Bei Mehrkonfigurations-Generatoren liegt die App unter `build/Debug/`. In CLion das CMake-Projekt öffnen und `pengui_playground` starten. SDL3 wird, falls nicht installiert, in gepinnter Version über CMake geladen. Der Programmstart benötigt weder Projekt noch Assets einer Engine. JetBrains Mono ist als Atlas eingebettet; Python und Pillow werden nur zum Neuerzeugen dieses Atlas benötigt.

## Kleine, getrennte Bausteine

```text
lib/pengui/
  include/pengui/    Öffentliche Schnittstelle
    ui.h            Interaktionen und Layout-Geometrie ohne SDL
    layout.h        Geprüfte YAML-Beschreibung ohne SDL
    ui_sdl.h        Zeichnen, Theme, Symbole und Schrift
    view.h          YAML-Darstellung und Widgetzustände
  src/
    core.c          Buttons, Tabs, Splitter, Scrollen, Fokus, Clipping
    text_input.c    UTF-8-Textbearbeitung
    layout.c        YAML laden und prüfen
    sdl.c           Einzelne Widgets zeichnen
    image.c         PNG-Dateien laden
    view/           Zustand, Eingaben und Zeichnen der YAML-Oberfläche
app/                Beispiel-Anwendung und ihre Aktionen
resources/          playground.yaml und lizenzierte Schriftquelle
tests/              Unity-Tests und App-Smoke-Test
```

Die App kennt ausschließlich öffentliche pengUI-Header. Die Bibliothek kennt die App nicht. Es gibt keine globale Anwendungsinstanz. Jede `UiView` besitzt ihre Eingabe-, Tab-, Tree- und Textzustände. Das aus der bisherigen UI übernommene Theme ist vorerst global; mehrere gleichzeitige unterschiedliche View-Themes sind noch kein unterstützter Vertrag.

CMake-Targets:

- `pengUI::core`: UI-Interaktion und YAML, ohne SDL.
- `pengUI::sdl`: Darstellung und YAML-View; baut auf dem Core auf.
- `pengui_playground`: eigenständige Demo, kein Teil der Bibliothek.

Nur den Core bauen:

```sh
cmake -S . -B build-core -DPENGUI_BUILD_SDL=OFF -DPENGUI_BUILD_APP=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

Für eigene CMake-Anwendungen kann das Projekt mit `add_subdirectory(...)` eingebunden und `pengUI::sdl` verlinkt werden. Dafür `PENGUI_BUILD_APP=OFF` setzen und bei Bedarf `BUILD_TESTING=OFF`. Es gibt noch kein installiertes SDK-Paket.

## YAML und Aktionen

Die App lädt `playground.yaml` neben ihrer Programmdatei, unabhängig vom Arbeitsverzeichnis. Die Quelldatei ist `resources/playground.yaml`; nach Änderungen erneut bauen oder mit `--layout resources/playground.yaml` direkt die Quelldatei öffnen. F6 oder der Reload-Button laden die Datei neu. Ungültiges YAML bleibt mit einer Fehlermeldung auf stderr ohne Austausch der laufenden Oberfläche; erfolgreicher Reload setzt Widgetzustände zurück.

```yaml
schema: pengui/v1
window: {title: Meine App, width: 1000, height: 700}
root:
  type: column
  children:
    - {type: label, text: Hallo pengUI}
    - {type: input, id: name, text: Mein Name}
    - {type: button, id: greet, text: Begrüßen, action: app.greet}
```

`ui_layout_load()` liefert eine geprüfte, besitzende Beschreibung; `ui_view_new()` erzeugt daraus Widgetzustände. Die Anwendung hält Layout und Renderer am Leben, bis sie die View mit `ui_view_free()` freigibt. Aktionen werden über einen Callback mit Anwendungs-Kontext, Aktionsname und Widget-ID ausgelöst. `ui_view_text()` liest Eingaben, `ui_view_set_text()` ändert Eingabe- oder Labeltexte. Callbacks dürfen die aktive View nicht freigeben; die Beispiel-App verschiebt Reload deshalb ans Frame-Ende.

Unterstützte Knoten: `row`, `column`, `panel`, `toolbar`, `button`, `label`, `separator`, `spacer`, `tabs` mit `tab`, `section`, `input`, `tree` mit `item` und `split` mit genau zwei Kindern. Gemeinsame Eigenschaften sind unter anderem `id`, `text`, `enabled`, `width`, `height`, `padding`, `gap`; `open` steuert anfangs geöffnete Bereiche. Breite/Höhe 0 verteilt freien Platz unter flexiblen Kindern. Splitter verwenden `axis`, `initial` (Größe des ersten Bereichs), `minimum_first` und `minimum_second`. Tabs passen sich ihren Beschriftungen an, lassen sich verschieben und per Mausrad horizontal scrollen. Trees scrollen, klappen auf und haben abwechselnde Zeilenfarben sowie Verbindungslinien.

Buttons akzeptieren `icon`, `image` (PNG relativ zur YAML-Datei) und `image_padding` (Standard 4 px). Die View besitzt ihre Textur-Caches und gibt sie bei Reload/Freigabe frei. Fehlende Bilder zeigen ein Ersatzsymbol. PNGs sind auf 1024 × 1024 begrenzt. Verfügbare Icons stehen in `src/view/draw.c` unterhalb von `lib/pengui/`.

YAML v1 ist absichtlich klein: höchstens 128 Knoten, 12 Ebenen und 256 KiB Eingabe. Unbekannte Eigenschaften/Widgets, doppelte IDs/Schlüssel, fehlerhafte Zahlen und ungültige Kindstrukturen werden abgelehnt. Es gibt noch keine Bindingsprache, kein Docking und keine Undo-Verwaltung. Fachliche Aktionen und Dokumentmodelle gehören später in die jeweilige Anwendung.

Einzelne Widgets lassen sich auch direkt in C über `ui.h` und `ui_sdl.h` nutzen, ohne YAML oder `UiView`.

## Prüfungen und Abhängigkeiten

Unity prüft den übernommenen UI-Kern, YAML-Validierung und native SDL-Eingaben. Zusätzlich startet die App mit `--smoke` aus einem anderen Arbeitsverzeichnis. `--screenshot datei.bmp` nimmt die Oberfläche auf. `PENGUI_SANITIZERS=ON` aktiviert ASan/UBSan. Die Architekturprüfung verhindert App-/Engine-Abhängigkeiten in der Bibliothek. Tests, Formatierung und Desktop-Builds für Linux, Windows und macOS sind in GitHub Actions eingerichtet.

Enthaltene Fremdsoftware: libyaml (MIT), stb_image (MIT/Public Domain), Unity (MIT) und JetBrains Mono (SIL OFL). Die jeweiligen Lizenztexte bleiben bei den Quellen erhalten. SDL3 wird separat bezogen (zlib-Lizenz). Keine JavaScript-, Engine-, Terminal- oder SVG-Abhängigkeiten werden gebaut.

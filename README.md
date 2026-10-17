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
    pengui.h        Gesamte öffentliche API; SDL-Teil optional
  src/
    core/           Gemeinsamer Kontext: Fokus, Pointer-Capture, Clipping
    widgets/        Je Widget Logik und ggf. ein *_sdl.c-Gegenstück
    layout/         YAML-Prüfung, Geometrie und Anordnung der Container
    rendering/      Text, Icons, Bilder, Theme und Zeichenprimitive
    view/           Zustand, Ereignisweiterleitung und Zeichen-Dispatch
    window.c        Rahmenlose Fensterbedienung
app/                Beispiel-Anwendung und ihre Aktionen
resources/          playground.yaml und lizenzierte Schriftquelle
tests/              Unity-Tests und App-Smoke-Test
```

Die App kennt ausschließlich öffentliche pengUI-Header. Die Bibliothek kennt die App nicht. Es gibt keine globale Anwendungsinstanz. Jede `PUIView` besitzt ihre Eingabe-, Tab-, Tree- und Textzustände. Das aus der bisherigen UI übernommene Theme ist vorerst global; mehrere gleichzeitige unterschiedliche View-Themes sind noch kein unterstützter Vertrag.

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

`pui_layout_load()` liefert eine geprüfte, besitzende Beschreibung; `pui_view_new()` erzeugt daraus Widgetzustände. Die Anwendung hält Layout und Renderer am Leben, bis sie die View mit `pui_view_free()` freigibt. Aktionen werden über einen Callback mit Anwendungs-Kontext, Aktionsname und Widget-ID ausgelöst. `pui_view_text()` liest Eingaben, `pui_view_set_text()` ändert Eingabe- oder Labeltexte. Callbacks dürfen die aktive View nicht freigeben; die Beispiel-App verschiebt Reload deshalb ans Frame-Ende.

Unterstützte Knoten: `row`, `column`, `panel`, `toolbar`, `button`, `label`, `separator`, `spacer`, `tabs` mit `tab`, `section`, `input`, `tree` mit `item` und `split` mit genau zwei Kindern. Gemeinsame Eigenschaften sind unter anderem `id`, `text`, `enabled`, `width`, `height`, `padding`, `gap`; `open` steuert anfangs geöffnete Bereiche. Breite/Höhe 0 verteilt freien Platz unter flexiblen Kindern. Splitter verwenden `axis`, `initial` (Größe des ersten Bereichs), `minimum_first` und `minimum_second`. Tabs passen sich ihren Beschriftungen an, lassen sich verschieben und per Mausrad horizontal scrollen. Trees scrollen, klappen auf und haben abwechselnde Zeilenfarben sowie Verbindungslinien.

Buttons akzeptieren `icon`, `image` (PNG relativ zur YAML-Datei) und `image_padding` (Standard 4 px). Die View besitzt ihre Textur-Caches und gibt sie bei Reload/Freigabe frei. Fehlende Bilder zeigen ein Ersatzsymbol. PNGs sind auf 1024 × 1024 begrenzt. Verfügbare Icons stehen in `src/rendering/icons.c` unterhalb von `lib/pengui/`.

YAML v1 ist absichtlich klein: höchstens 128 Knoten, 12 Ebenen und 256 KiB Eingabe. Unbekannte Eigenschaften/Widgets, doppelte IDs/Schlüssel, fehlerhafte Zahlen und ungültige Kindstrukturen werden abgelehnt. Es gibt noch keine Bindingsprache, kein Docking und keine Undo-Verwaltung. Fachliche Aktionen und Dokumentmodelle gehören später in die jeweilige Anwendung.

Einzelne Widgets lassen sich auch direkt in C über `pengui/pengui.h` nutzen, ohne YAML oder `PUIView`.

## Prüfungen und Abhängigkeiten

Unity prüft den übernommenen UI-Kern, YAML-Validierung und native SDL-Eingaben. Zusätzlich startet die App mit `--smoke` aus einem anderen Arbeitsverzeichnis. `--screenshot datei.bmp` nimmt die Oberfläche auf. `PENGUI_SANITIZERS=ON` aktiviert ASan/UBSan. Die Architekturprüfung verhindert App-/Engine-Abhängigkeiten in der Bibliothek. Tests, Formatierung und Desktop-Builds für Linux, Windows und macOS sind in GitHub Actions eingerichtet.

Enthaltene Fremdsoftware: libyaml (MIT), stb_image (MIT/Public Domain), Unity (MIT) und JetBrains Mono (SIL OFL). Die jeweiligen Lizenztexte bleiben bei den Quellen erhalten. SDL3 wird separat bezogen (zlib-Lizenz). Keine JavaScript-, Engine-, Terminal- oder SVG-Abhängigkeiten werden gebaut.

## Eigene Fensterbuttons

Die Playground-Toolbar enthält rechts `window.minimize`, `window.maximize` und `window.close`. Maximieren wechselt bei maximiertem Fenster zu Wiederherstellen, einschließlich Symbolwechsel. Die Anwendung reicht diese Aktionen an `pui_window_action()` weiter. Schließen erzeugt eine SDL-Schließanfrage; die Anwendung entscheidet, ob sie tatsächlich beendet wird.

`draggable: true` markiert in YAML einen Bereich, dessen freie Fläche das Fenster verschieben darf. Buttons und Eingabefelder bleiben bedienbar; die Fensterränder erlauben Größenänderungen. `pui_window_attach()` installiert den Hit-Test und entfernt den nativen Rahmen. Falls das SDL-Backend Hit-Tests nicht unterstützt, bleibt der native Rahmen als bedienbare Rückfalllösung erhalten. Der `PUIWindow`-Kontext muss stabil bleiben; nach YAML-Reload wird sein View-Zeiger aktualisiert, vor Freigabe wird `pui_window_detach()` aufgerufen.

## Ein öffentlicher Header

```c
#include <pengui/pengui.h>
```

`pengUI::core` stellt über diesen Header Widgets, Geometrie und YAML ohne SDL bereit. Beim Verlinken von `pengUI::sdl` setzt CMake für die Bibliothek und ihre Verbraucher zusätzlich `PUI_WITH_SDL=1`; dann enthält derselbe Header auch Darstellung, View und Fensterbedienung. Ohne CMake muss das Makro für SDL-Verbraucher beim Kompilieren gesetzt und gegen die SDL-Komponente sowie SDL3 verlinkt werden. Die Implementierung bleibt intern auf mehrere Dateien verteilt.

## Wo gehört Widget-Code hin?

| Bereich | Ohne SDL | Darstellung und native Eingaben |
| --- | --- | --- |
| Buttons | `widgets/button.c` | `widgets/button_sdl.c` einschließlich Bildbuttons und YAML-Aktionen |
| Textfelder | `widgets/text_input.c` für UTF-8, Cursor und Auswahl | `widgets/text_input_sdl.c` für Zeichnen, Fokus, Clipboard und Tastatur |
| Tabs | `widgets/tabs.c` | `widgets/tabs_sdl.c` einschließlich YAML-Initialisierung und Scrollen |
| Tree | `widgets/tree.c` | `widgets/tree_sdl.c` einschließlich Aufklappen, Zeilen und Scrollen |
| Scrollbar | `widgets/scrollbar.c` | `widgets/scrollbar_sdl.c` |
| Splitter | `widgets/splitter.c` | `widgets/splitter_sdl.c` |
| Kleine Elemente | `widgets/basic.c` | `widgets/basic_sdl.c` für Labels, Separatoren und Sections |

Alle Pfade dieser Tabelle sind relativ zu `lib/pengui/src/`. Die zwei Dateien je größerem Widget halten den Core unabhängig von SDL; sie bilden gemeinsam den Einstieg für das jeweilige Element. `view/events.c` und `view/draw.c` leiten lediglich weiter. `layout/layout_sdl.c` verteilt den verfügbaren Platz; `layout/yaml.c` liest und prüft Beschreibungen. `rendering/image.c` lädt PNGs, während die Darstellung eines Bildbuttons beim Button bleibt. Die öffentliche API bleibt ausschließlich in `pengui/pengui.h`.

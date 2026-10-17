# pengUI

Eigenständige UI-Bibliothek in C17 mit SDL3-Darstellung und einer kleinen Widget-Spielwiese.
Dieser Entwicklungsbranch enthält keine Engine und kein ECS. Die zusätzliche Editor-Shell bietet leere Gerüste für Vector, Scripts und Game. Der bisherige Engine-Stand bleibt in den anderen Branches und der Git-Historie erhalten.

## Dokumentation

[Architektur und Funktionsweise der UI](docs/ARCHITEKTUR.md) erklärt den Programmstart,
Layout und Rendering, Widgetzustände, Mausinteraktion und Fokus, Tab-Navigation, Popups,
C-Aktionen sowie Lebensdauer und Reload. Mit Codeverweisen und einer Anleitung zum
Ergänzen eigener Widgets.

[Editor-Shell und modulare Editoren](docs/EDITOREN.md) beschreibt die zusätzliche
`pengui_editor`-App, den konfigurierbaren Starteditor, die Modulstruktur und austauschbare
YAML-Bereiche.

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
apps/demo/                Beispiel-Anwendung und ihre Aktionen
resources/          playground.yaml und lizenzierte Schriftquelle
tests/              Unity-Tests und App-Smoke-Test
```

Die App kennt ausschließlich öffentliche pengUI-Header. Die Bibliothek kennt die App nicht. Es gibt keine globale Anwendungsinstanz. Jede `PUIView` besitzt ihre Eingabe-, Tab-, Tree- und Textzustände. Das aus der bisherigen UI übernommene Theme ist vorerst global; mehrere gleichzeitige unterschiedliche View-Themes sind noch kein unterstützter Vertrag.

CMake-Targets:

- `pengUI::core`: UI-Interaktion und YAML, ohne SDL.
- `pengUI::sdl`: Darstellung und YAML-View; baut auf dem Core auf.
- `pengui_playground`: eigenständige Widget-Demo, kein Teil der Bibliothek.
- `pengui_editor`: Shell mit wechselbaren Editor-Gerüsten; Starteditor in `resources/application.yaml`.

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

Unterstützte Knoten: `row`, `column`, `panel`, `toolbar`, `button`, `toggle`, `checkbox`, `button_group`, `dropdown` mit `option`, `popup_button`, `slot`, `label`, `separator`, `spacer`, `tabs` mit `tab`, `section`, `input`, `tree` mit `item` und `split` mit zwei oder drei Kindern. Gemeinsame Eigenschaften sind unter anderem `id`, `text`, `enabled`, `width`, `height`, `padding`, `gap`; `open` steuert anfangs geöffnete Bereiche. Breite/Höhe 0 verteilt freien Platz unter flexiblen Kindern. Splitter verwenden `axis`, `initial` (Größe des ersten Bereichs), `minimum_first` und `minimum_second`. Tabs passen sich ihren Beschriftungen an, lassen sich verschieben und per Mausrad horizontal scrollen. Trees scrollen, klappen auf und haben abwechselnde Zeilenfarben sowie Verbindungslinien.

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
| Toggle / Checkbox / Auswahlgruppen | `widgets/toggle.c` für Zustandswechsel und exklusive Auswahl | `widgets/toggle_sdl.c`, `widgets/checkbox_sdl.c`, `widgets/button_group_sdl.c` |
| Dropdown / Popup-Button | `layout/popup.c` für Positionierung | `widgets/dropdown_sdl.c`, `widgets/popup_button_sdl.c`; gemeinsame Überlagerungen in `view/popup.c` |
| Textfelder | `widgets/text_input.c` für UTF-8, Cursor und Auswahl | `widgets/text_input_sdl.c` für Zeichnen, Fokus, Clipboard und Tastatur |
| Tabs | `widgets/tabs.c` | `widgets/tabs_sdl.c` einschließlich YAML-Initialisierung und Scrollen |
| Tree | `widgets/tree.c` | `widgets/tree_sdl.c` einschließlich Aufklappen, Zeilen und Scrollen |
| Scrollbar | `widgets/scrollbar.c` | `widgets/scrollbar_sdl.c` |
| Splitter | `widgets/splitter.c` | `widgets/splitter_sdl.c` |
| Kleine Elemente | `widgets/basic.c` | `widgets/basic_sdl.c` für Labels, Separatoren und Sections |

Alle Pfade dieser Tabelle sind relativ zu `lib/pengui/src/`. Die zwei Dateien je größerem Widget halten den Core unabhängig von SDL; sie bilden gemeinsam den Einstieg für das jeweilige Element. `view/events.c` und `view/draw.c` leiten lediglich weiter. `layout/layout_sdl.c` verteilt den verfügbaren Platz; `layout/yaml.c` liest und prüft Beschreibungen. `rendering/image.c` lädt PNGs, während die Darstellung eines Bildbuttons beim Button bleibt. Die öffentliche API bleibt ausschließlich in `pengui/pengui.h`.

## Split mit zwei oder drei Bereichen

`axis: horizontal` ordnet die Bereiche nebeneinander an, `vertical` untereinander. `flex` bestimmt, welcher Bereich Größenänderungen des Fensters aufnimmt: `first`, `second` oder `last`, bei drei Kindern auch `middle`. Standard ist bei zwei Kindern `second`, bei drei Kindern `middle`. Die übrigen Bereiche behalten ihre eingestellte Breite bzw. Höhe. Greifer verändern diese Größen; reine Fensteränderungen überschreiben die Wunschgrößen nicht. Wenn das Fenster zu klein wird, werden die Bereiche begrenzt bzw. die Mindestgrößen proportional verkleinert. Bei mehr Platz stehen die eingestellten Größen wieder zur Verfügung.

Drei Spalten mit flexibler Mitte benötigen keine Verschachtelung:

```yaml
type: split
axis: horizontal
flex: middle
initial: 240
initial_last: 280
minimum_first: 160
minimum_second: 240
minimum_last: 160
gap: 9
children:
  - {type: panel, text: Links}
  - {type: panel, text: Mitte}
  - {type: panel, text: Rechts}
```

`initial` ist die Wunschgröße des ersten Bereichs, `initial_second` die des zweiten (bei drei Kindern), `initial_last` die des letzten. Die Größe des flexiblen Bereichs ergibt sich aus dem verbleibenden Platz. Bei zwei Kindern fällt `initial_last` ohne Angabe auf `initial` zurück; die bestehenden Zweiteilungen bleiben mit ihrer bisherigen Voreinstellung kompatibel.

Für einen Editor oben und eine Konsole mit fester Ausgangshöhe unten:

```yaml
type: split
axis: vertical
flex: first
initial_last: 220
minimum_first: 200
minimum_second: 100
gap: 9
children:
  - {type: panel, text: Editor}
  - {type: panel, text: Konsole}
```

Bei drei Kindern gelten die Mindestgrößen `minimum_first`, `minimum_second` und `minimum_last`. Bei zwei Kindern beschreibt `minimum_second` weiterhin den unteren/rechten Bereich; ein explizites `minimum_last` kann ihn überschreiben.

## Toggle, Checkbox und exklusive Button-Gruppen

`toggle` ist ein Button, der zwischen an/aus wechselt. `checkbox` zeigt denselben booleschen
Zustand als anklickbares Kästchen mit Beschriftung. `checked` setzt den Anfangszustand
(Standard: `false`). Beide unterstützen `enabled` und `action`; Toggles auch `icon` und `image`.

```yaml
type: column
padding: 0
children:
  - {type: toggle, id: grid, text: Raster, icon: grid, checked: true, action: app.grid}
  - {type: checkbox, id: snap, text: Am Raster ausrichten, action: app.snap}
  - type: button_group
    id: tools
    gap: 2
    children:
      - {type: button, id: select, text: Auswahl, checked: true, action: app.select}
      - {type: button, id: draw, text: Zeichnen, action: app.draw}
      - {type: button, id: erase, text: Radieren, action: app.erase}
```

Eine `button_group` enthält ausschließlich `button`-Kinder und hält genau eines davon aktiv.
Ein erneuter Klick auf das aktive Element schaltet es nicht aus. Ohne `checked: true` wird
der erste bedienbare Button ausgewählt. Mehrere anfängliche Auswahlen, eine deaktivierte
Anfangsauswahl oder Gruppen ohne bedienbares Kind werden beim Laden abgelehnt. Jede Gruppe
ist unabhängig. Die Buttons teilen den verfügbaren Platz gleichmäßig; einzelne
Kind-Breiten/Höhen werden dabei nicht ausgewertet. `axis: vertical` ordnet sie untereinander
an; standardmäßig stehen sie nebeneinander. `padding` ist bei Gruppen standardmäßig 0.

Bei einer Zustandsänderung wird die `action` des Toggles, der Checkbox bzw. des neu gewählten
Gruppenbuttons aufgerufen. Im Callback ist der neue Zustand bereits abfragbar:

```c
bool snap = pui_view_checked(view, "snap");
pui_view_set_checked(view, "snap", true);
pui_view_set_checked(view, "draw", true); /* deaktiviert die anderen Buttons dieser Gruppe */
```

Programmatische Änderungen lösen keine Aktion aus. Die aktive Gruppenauswahl lässt sich
nicht mit `false` leeren. Eine neu geladene YAML setzt die Zustände auf ihre Anfangswerte
zurück; dauerhafte Anwendungszustände verwaltet die App. Ohne YAML stehen `pui_toggle()`
und `pui_radio()` im SDL-freien Core bereit; `pui_draw_button()` und `pui_draw_checkbox()`
übernehmen die Darstellung. Nach einem Mausklick kann das fokussierte Element mit Leertaste
oder Enter aktiviert werden. Tab und Umschalt+Tab wechseln zwischen sichtbaren, bedienbaren
Textfeldern in YAML-Reihenfolge und springen am Ende wieder zum Anfang. Eingaben in inaktiven
Tabs, eingeklappten oder deaktivierten Bereichen werden übersprungen. Das neu fokussierte
Feld markiert seinen Text zum direkten Ersetzen. Text lässt sich außerdem mit gedrückter
linker Maustaste markieren; beim Ziehen über den Feldrand scrollt langer Text horizontal.
Doppelklick markiert ein Wort, Dreifachklick das ganze Feld. Nach einem Doppelklick erweitert
Ziehen die Auswahl in ganzen Wörtern. Strg+A bzw. Cmd+A markiert ebenfalls alles. Buttons gehören bisher nicht zur Tab-Reihenfolge.

## Eigene Button-Aktionen in C

In `resources/playground.yaml` bekommt der Button einen frei gewählten Aktionsnamen:

```yaml
- {type: button, id: greet_button, text: Begrüßen, action: app.greet}
```

Deine Funktion gehört in die Anwendung, zum Beispiel in `apps/demo/application.c` vor die bestehende
Funktion `action()`. Größere Funktionen können später in eigene C-Dateien der App wandern.

```c
static void greet(Playground *app) {
    pui_view_set_text(app->view, "status", "Hallo aus meiner C-Funktion!");
}
```

Ergänze in der vorhandenen `action()`-Funktion einen Zweig vor dem abschließenden `else`:

```c
} else if (!strcmp(name, "app.greet")) {
    greet(app);
} else {
    /* bisherige Standardbehandlung */
}
```

Eine einzelne Funktion muss nicht zusätzlich bei pengUI registriert werden. Die zentrale
Weiterleitung erfolgt bereits in der Render-Schleife der App:

```c
pui_view_draw(app.view, &app.pui, area, action, &app);
```

Dabei ist `action` der Callback und `&app` dein Anwendungskontext. Der Callback erhält
`name` aus YAML (`app.greet`) und `widget` aus der Widget-ID (`greet_button`). Der String ist
kein C-Funktionsname, den pengUI automatisch aufruft: Die Zuordnung übernimmt deine
`action()`-Funktion. Auch Toggle- und Checkbox-Änderungen kommen dort an; deren aktuellen
Wert kannst du mit `pui_view_checked(app->view, widget)` abfragen.

## Dropdown mit Werten

```yaml
- type: dropdown
  id: blend_mode
  action: app.blend_changed
  value: normal
  children:
    - {type: option, text: Normal, value: normal}
    - {type: option, text: Multiplizieren, value: multiply}
    - {type: option, text: Negativ multiplizieren, value: screen}
```

`text` ist die sichtbare Beschriftung, `value` der davon unabhängige String für deine App.
Werte müssen innerhalb des Dropdowns eindeutig und nicht leer sein. Ohne Anfangswert wird
die erste aktivierte Option gewählt. Deaktivierte Optionen (`enabled: false`) lassen sich
nicht auswählen. Eine geänderte Auswahl ruft die Aktion des Dropdowns mit dessen Widget-ID
auf. Die bisherige Callback-Signatur bleibt gleich; der neue Wert ist im Callback verfügbar:

```c
/* Im bestehenden action()-Callback in apps/demo/application.c: */
} else if (!strcmp(name, "app.blend_changed")) {
    const char *value = pui_view_value(app->view, widget);
    printf("Gewählt: %s\n", value);
}
```

`pui_view_set_value(view, "blend_mode", "multiply")` wählt programmatisch, ohne ein Event
auszulösen. Ein ungültiger oder deaktivierter Wert wird abgelehnt. Eine erneute Wahl des
bereits aktiven Werts erzeugt ebenfalls kein Event. Die Demo zeigt den gewählten Wert in
ihrer Statusmeldung. Pfeiltasten, Home/End und Enter/Leertaste bedienen die geöffnete Liste;
längere Listen unterstützen Mausrad und Scrollbar.

## Button mit aufklappendem Panel

```yaml
- type: popup_button
  id: brush_settings
  text: Pinsel
  icon: settings
  placement: right
  popup_width: 250
  popup_height: 180
  children:
    - type: panel
      padding: 12
      gap: 8
      children:
        - {type: label, text: Pinselgröße}
        - {type: input, id: brush_size, text: "16"}
        - {type: checkbox, id: smoothing, text: Glätten, checked: true}
        - {type: button, text: Anwenden, action: app.apply_brush}
```

Der Popup-Button enthält genau ein normales `panel`. Darin kannst du die bestehenden
Layouts und Widgets verwenden, auch weitere Dropdowns oder Popup-Buttons. Die Aktionen
der enthaltenen Buttons funktionieren wie gewohnt. Das Öffnen selbst löst keine Aktion aus.
Der Inhalt liegt über dem übrigen Layout und verschiebt es nicht.

`placement: below` (Standard) öffnet darunter, `right` daneben. Wenn die Fläche am
Fensterrand nicht reicht, wechselt das Popup auf die andere Seite und wird bei Bedarf
auf den verfügbaren Bereich begrenzt. Standardgrößen: Panel 260 × 200 Pixel, Dropdown
so breit wie der Button und maximal 200 Pixel hoch. Beide unterstützen `popup_width` und
`popup_height`. Große allgemeine Panels werden begrenzt, aber nicht automatisch scrollbar;
verwende passende Abmessungen bzw. Widgets mit eigener Scrollfunktion.

Escape, ein Klick außerhalb oder ein erneuter Klick auf den Auslöser schließen das Popup.
Der schließende Klick wird verbraucht und aktiviert kein darunterliegendes Element. Bei
verschachtelten Popups wird zuerst das oberste geschlossen. Texteingaben und Tab-Navigation
bleiben auf die Textfelder im obersten Panel beschränkt. Fenster-Fokusverlust schließt alle
Popups. Anfangszustände und ausgewählte Werte werden bei YAML-Neuladen zurückgesetzt.

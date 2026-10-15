# Penguin UI: gemeinsame C-Bibliothek

Penguin UI ist eine kleine eigene UI-Bibliothek für Editor und SDL3-Spiele. Sie kennt weder ECS noch Szenen, Assets oder JavaScript. Editor-spezifische Ansichten bleiben außerhalb der Bibliothek.

## Aufbau

| CMake-Target | Dateien | Aufgabe / Abhängigkeiten |
| --- | --- | --- |
| `penguin_ui` | `ui/include/penguin/ui.h`, `ui/src/core.c` | Eingabe, Fokus, Capture, Layout und Widget-Zustände; C17, keine SDL-Abhängigkeit. |
| `penguin_ui_sdl` | `ui/include/penguin/ui_sdl.h`, `ui/src/sdl.c` | Flat-Rendering, Icons und Themes; nutzt `penguin_ui` und SDL3. |
| `penguin_ui_demo` | `examples/ui/main.c` | Eigenständiges Beispiel ohne Engine-/Editor-/Script-Bibliotheken. |
| `penguin_ui_tests` | `tests/test_ui.c` | 28 Unity-Tests der UI-Regeln ohne Fenster. |

Die beiden Bibliotheken werden statisch gebaut. Sie sind keine Kopie der UI in jedem Spiel: Editor und Demo verlinken dieselben Targets. Der Kern lässt sich auch mit einem anderen Renderer verwenden; momentan ist nur der SDL3-Backend implementiert.

## Bereits enthalten

- Buttons und Icon-Buttons: Aktivierung beim Loslassen, Disabled-Zustand und Hover.
- Flat-Tabs: Auswahl, Umsortieren per Ziehen und Scroll-Modell bei Platzmangel.
- Klappbare Abschnitte: Überschrift und gespeicherter Offen-/Geschlossen-Zustand.
- Scrollbars: vertikal/horizontal, Thumb-Ziehen und seitenweises Scrollen beim Klick auf den Track.
- Splitter: Ziehen mit Mindest-/Maximalwerten, auch außerhalb des ursprünglichen Griffbereichs.
- Spaltenlayout: Mindestbreiten und Platz für die mittlere Arbeitsfläche.
- Clip-Bereiche: Zeichen-/Hit-Test-Grenzen und verschachtelte Rechtecke.
- Fokus-ID und Tastaturaktivierung von fokussierten Buttons über Enter/Leertaste.
- Gemeinsame gezeichnete Icons und ein konfigurierbares `UiTheme`.

Map und Scripts sind eigenständige Arbeitsbereiche. Ihre Auswahl sitzt als Buttons in der oberen globalen Toolbar und kann umsortiert, aber nicht geschlossen werden. Mehrere Script-Dokumente, schließbare Dateitabs und freies Docking sind noch nicht enthalten.

## Das Flat-Design

Buttons und Tabs haben gerade Kanten, eine einzelne flächige Hintergrundfarbe und keine Umrandung, Rundung oder Schatten. Aktive Buttons sind orange mit dunklem Text. Aktive Tabs verwenden das Panelgrau, helle Schrift und eine 2 px hohe orange Markierung oben; sie schließen unten bündig an den Panelinhalt an. Neben den Reitern bleibt die dunkle Grundfläche sichtbar. Inaktive Elemente sind dunkelgrau. Hover und Disabled verwenden eigene einfarbige Zustände.

Das Standardtheme verwendet unter anderem:

| Rolle | Farbe |
| --- | --- |
| Canvas | `#101010` |
| Panel | `#181818` |
| Button | `#242424` |
| Hover | `#333333` |
| Akzent | `#C28E5C` |

`ui_dark_theme()` liefert ein Theme als Wert. Spiele können eine Kopie ändern und an `ui_draw_button_themed()` übergeben. Es wird kein globales Theme verändert. Die bestehenden Kurzfunktionen wie `ui_draw_button()` verwenden das Standardtheme.

Die Icons bleiben SDL-Linien und Flächen. Text nutzt `ui_text(renderer, x, y, utf8, rgba)` mit eingebetteter **JetBrains Mono Regular 2.304** (13 px, Zeichenraster 8 × 16). Die Textur wird pro Renderer einmal angelegt und von SDL mit dem Renderer freigegeben. Es gibt keine installierte Schrift, Dateipfad- oder SDL_ttf-Abhängigkeit. UTF-8 wird decodiert; druckbare ASCII- und Latin-1-Zeichen einschließlich äöüÄÖÜß werden dargestellt. Weitere Zeichen erscheinen als `?`; der Dokumentinhalt bleibt erhalten. Ligaturen, komplexe Schriftformung und DPI-abhängige Neurasterung sind noch nicht implementiert.

Originalschrift und SIL Open Font License liegen in `ui/assets/`. Bei Weitergabe die Lizenz beilegen. `tools/bake_font.py` erzeugt mit Pillow den eingecheckten Alpha-Atlas `ui/src/font_atlas.h` neu. Reguläre CMake-Builds benötigen dafür weder Python noch Pillow.

## Zustände und Frame-Ablauf

Die Anwendung besitzt einen `UiContext` und die Modelle ihrer Widgets, beispielsweise `UiTabs` oder `UiScroll`. Die Bibliothek allokiert dafür keinen dynamischen Speicher.

1. `ui_init()` einmal beim Start aufrufen.
2. Mausereignisse in Reihenfolge an `ui_pointer()` weitergeben; `down` beschreibt die linke Maustaste. Bei Mausbewegung den bisherigen `context.down`-Zustand beibehalten.
3. Bei Enter/Leertaste für UI-Bedienung `ui_activate()` setzen. Texteingaben dürfen gleichzeitig keine Buttons aktivieren.
4. Layout und Widgets auswerten, dann rendern.
5. Am Ende `ui_end_frame()` aufrufen. Es entfernt Frame-Ereignisse, erhält aber laufendes Capture und Fokus.
6. Bei Fokusverlust, Abbruch oder notwendigem Moduswechsel `ui_cancel()` aufrufen.

Ein Press und Release zwischen zwei Renderframes wird berücksichtigt. Die Aktivierung findet nur statt, wenn der Press im Button begann und der Release dort endet. Ein Loslassen außerhalb führt keine Aktion aus. Während eines Drags gehört die Maus dem aufgenommenen Widget; andere Widgets übernehmen sie nicht.

Jedes Widget benötigt eine **stabile, eindeutige, von null verschiedene ID**. Beispielsweise `ui_id("main_menu_play")`. IDs dürfen nicht aus wechselnden Bildschirmkoordinaten oder Arraypositionen beim Umsortieren abgeleitet werden. Tab-Auswahl bezieht sich auf IDs; Umsortieren verändert die Inhaltszuordnung nicht. Wiederholte Widgets benötigen zusätzlich einen stabilen Objekt-/Bereichsschlüssel. Die Editor-Buttons verwenden Aufrufstelle und Widget-Scope, sodass Resize und Scrollen ihre ID nicht ändern.

## Ein Button in einem Spiel

```c
#include "penguin/ui_sdl.h"

UiContext ui;
ui_init(&ui); // einmal, nicht in jedem Frame

// In jedem Frame, nachdem SDL-Ereignisse in ui_pointer() übernommen wurden:
UiId play_id = ui_id("main_menu_play");
UiRect bounds = {40, 80, 160, 32};
if (ui_button(&ui, play_id, bounds, true)) {
    // Spiel starten
}
ui_draw_button(renderer,
    (SDL_FRect){bounds.x, bounds.y, bounds.w, bounds.h},
    ICON_PLAY, "Play", false, true, ui_hovered(&ui, bounds));
ui_end_frame(&ui);
```

`examples/ui/main.c` zeigt den vollständigen SDL-Ereignis- und Renderablauf mit Tabs, Scrollbar, Buttons, klappbarem Bereich und Spaltentrennern.

## In ein anderes CMake-Projekt einbinden

```cmake
find_package(SDL3 3.2 CONFIG REQUIRED)
add_subdirectory(path/to/PenguinEngine/ui penguin-ui)
target_link_libraries(my_game PRIVATE penguin_ui_sdl)
```

Damit werden keine Engine-, Editor- oder JavaScript-Targets eingebunden. Wenn SDL erst später eingerichtet wird, `penguin_ui_add_sdl()` danach aufrufen. Ohne SDL lässt sich direkt `penguin_ui` für die Zustands-/Layoutlogik verwenden.

Im Engine-Repo:

```sh
cmake --build build --target penguin_ui_demo --config Debug
./build/penguin_ui_demo
```

In CLion die Run-Konfiguration `penguin_ui_demo` wählen. Unter Visual Studio liegt das Programm üblicherweise in `build/Debug/penguin_ui_demo.exe`.

## Verwendung im Editor

Die beiden vertikalen Trennlinien verändern die Breite von Dateibaum und Inspector. Der Mauszeiger wechselt auf Resize; Mindestbreiten erhalten bedienbare Seitenbereiche und mindestens 320 Pixel mittlere Arbeitsfläche. Inspector-Controls und Tile-Palette passen ihre Breiten beziehungsweise Spaltenzahl an.

Dateibaum, Inspector und Scripteditor verwenden das gemeinsame Scroll-Modell. Scrollbars werden nur angezeigt, wenn der Inhalt nicht in den sichtbaren Bereich passt. Die Inspector-Abschnitte Tilemap/Layers, Entity und Tile palette können eingeklappt werden.

Layout- und UI-Zustände verändern keine Szenendaten und gehören nicht zum Undo-Verlauf. Sie bleiben zunächst für die Sitzung erhalten; Speichern eines Benutzerlayouts über Programmstarts hinweg ist noch nicht implementiert.

Dateibrowser und Scripteditor behalten ihre spezialisierten Funktionen. Die Bibliothek stellt deren Scroll-/Fokus-/Clip-Grundlagen bereit; sie enthält keinen fertigen Codeeditor oder Dateibrowser.

## Prüfung

28 Unity-Tests prüfen unter anderem Press-/Release-Reihenfolge, schnelles Klicken, Disabled/Blocked, exklusives Capture, Fokus, Clip-Grenzen, Drag-Clamping, Spaltenbreiten, Scrollbar-Geometrie, Thumb-Ziehen, Track-Klicks und Tab-Auswahl/-Umsortierung.

Editor-Smoke-Tests ziehen beide Spaltentrenner, klappen einen Inspector-Bereich zu, sortieren Tabs um und ziehen eine Script-Scrollbar. Sie prüfen, dass dadurch weder Szene noch Scripttext verändert werden. Das UI-Demo hat einen eigenen SDL-Dummy-Smoke-Test.

## Nächste Komponenten

Nach Bedarf ergänzen wir Checkboxen/Radio-Buttons, Slider, Dropdowns, allgemeine Text-/Zahlenfelder, Menüleisten, Dialogbausteine und Listen-/Baumansichten. Dazu gehören später Tab-Navigation, eine bessere Textdarstellung und gespeicherte Benutzerlayouts. Jede Komponente soll einen kleinen Daten-/Eingabeteil und eine getrennte Darstellung erhalten. Freies Docking und ein vollständiges IDE-Framework bleiben größere, separate Aufgaben.

Der Editor verwendet `UI_CANVAS` (#101010) als dunkle Grundfläche und `UI_PANEL` (#202020) für abgesetzte Panels. Toolbar, Dateibaum und Inspector haben 3 px Außenabstand. Buttons verwenden #292929, Hover #393939; die orange Akzentfarbe bleibt erhalten. Map- und Script-Arbeitsflächen berücksichtigen dieselben Abstände bei Darstellung und Eingabe.


Die Seitenpanels verwenden ebenfalls `UiTabs`: links **Explorer / Scene**, rechts **Inspector / Info**. Die beiden Panel-Tabgruppen und die globale Arbeitsbereichsauswahl haben unabhängige Auswahl und Reihenfolge; Tabs können innerhalb ihrer Gruppe umsortiert werden. Scene listet die lebenden Entities der bearbeiteten Szene. Ein Klick wählt die Entity aus, aktiviert Map und Inspector und ändert keine Szenendaten. Während Play oder einer laufenden Bearbeitung ist diese Auswahl gesperrt. Info zeigt Kartengröße, Tilegröße, Ebenen-/Entity-Anzahl und Tastenkürzel. Explorer, Scene und Info behalten eigene Scrollpositionen. Zusätzliche Ansichten können später mit einer stabilen Tab-ID und einem eigenen Inhaltsrenderer ergänzt werden; gruppenübergreifendes Docking ist noch nicht vorhanden.

Zwischen den Spalten werden keine Trennlinien gezeichnet, auch nicht bei Hover. Drei vertikal angeordnete, mittig platzierte Punkte markieren jeden Resize-Griff. Sie sind im Ruhezustand grau und bei Hover oder aktivem Ziehen orange. Der gesamte Zwischenraum bleibt bedienbar und zeigt den horizontalen Resize-Cursor.


Tabbreiten folgen dem Inhalt: `ui_text_width()` misst UTF-8 nach denselben festen Zeichenbreiten wie `ui_text()`, nicht nach UTF-8-Bytes. `ui_tab_content_width(icon, label)` addiert Icon und Innenabstände. Mit `ui_tabs_set_width(tabs, id, content_width + 4)` bekommt jeder Tab seine eigene Slotbreite inklusive 4 px Zwischenraum. `UiTab.width = 0` erhält die bisherige Fallbackbreite aus `ui_tabs_update`/`ui_tab_rect`. Die Messung und Darstellung bleiben im SDL-Backend, das Layoutmodell bleibt SDL-frei. Beim Umsortieren wandert die Breite mit der ID; Ziele werden über die Mittelpunkte der Nachbartabs bestimmt, damit unterschiedlich breite Tabs bei gehaltenem Mauszeiger nicht hin- und herspringen. Überlaufende Tabreihen lassen sich mit dem Mausrad über der Kopfzeile scrollen. Hinter dem letzten Tab wird kein zusätzlicher Zwischenraum gescrollt.

Die obere Toolbar enthält Hamburger-Menü (zunächst ohne Funktion), Speichern, Undo/Redo und die Icon-Auswahl der Arbeitsbereiche, getrennt durch schmale Linien mit Abstand. Speichern und Undo/Redo gehören zum aktiven Dokument. Spezifische Werkzeuge stehen direkt über dem jeweiligen Editorinhalt. Tooltips und Tastenkürzel bleiben verfügbar.


## Arbeitsbereiche und kontextbezogene Werkzeugleisten

| Bereich | Inhalt |
| --- | --- |
| Globale Toolbar | Menü-Platzhalter, Speichern, Undo/Redo, Map-/Scripts-Icons |
| Map-Werkzeugleiste | Play/Stop, PNG-Import, Pinsel/Radierer/Rechteck/Füllen/Auswahl, Kollisionen, Raster |
| Scripts-Werkzeugleiste | JavaScript-Syntaxprüfung |
| Seitenpanels | Explorer/Scene und Inspector/Info, unabhängig von der globalen Auswahl |

`editor_views` in `editor/src/main.c` ist die zentrale Liste der Arbeitsbereiche. Ein `EditorView` enthält stabile ID, Namen, Icon, Aktionsanzahl und zwei Callbacks: `draw_tools(area)` und `draw_content()`. Aus der Liste entstehen die Navigationsbuttons. `select_workspace(id)` verwaltet Auswahl und Textfokus; `map_active()`/`scripts_active()` leiten den Zustand aus der ausgewählten ID ab, ohne zweite Zustandsvariable. Beim Wechsel bleiben die vorhandenen Editor-Daten einschließlich Kamera, Auswahl, Text und Undo erhalten.

Für einen weiteren Bereich eine neue stabile ID, eigenen Datenzustand, Toolbar-/Inhaltsrenderer und einen Eintrag in der Liste ergänzen. Passende Eingaben, Kurzbefehle, Speichern und Inspector-Inhalte müssen für den neuen Editor ebenfalls ergänzt werden. Dies ist eine interne C-Erweiterungsstelle, noch keine dynamische Plugin-API. Es gibt keine leeren Platzhalter für künftige Tools.

Werkzeugbuttons bleiben Icons mit Tooltips. `action_count` bestimmt die benötigten Zeilen; bei schmaler mittlerer Spalte werden die Aktionen auf weitere Zeilen verteilt. Der Canvas beginnt unter der vollständigen Toolbar, und alle Toolbar-Zeilen erhalten Eingabevorrang vor Karte/Text. F5 startet ausschließlich im Map-Bereich; während Play ist der Wechsel zu anderen Editoren gesperrt. Die Syntaxprüfung steht nur einmal in der Scripts-Werkzeugleiste; die Dokumentkopfzeile zeigt den Dateinamen.

Die Statusleiste ist 28 px hoch und zeigt EDIT/PLAY, Saved/Modified, Dokumentdetails und die letzte Meldung in einer Zeile. Die Meldung folgt der gemessenen Breite der Details und wird bei Platzmangel rechts abgeschnitten, ohne andere Angaben zu überdecken. Die zentralen Panel-Abstände betragen 3 px.

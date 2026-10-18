# Vektoreditor: vom Button-Klick zur Dokumentänderung

Diese Anleitung beschreibt den aktuellen Stand der App `pengui_editor` auf dem
pengUI-Branch. Als Beispiele dienen „Add Document“, das Aufziehen eines Artboards
und Undo/Redo. Alle genannten Funktionen existieren im aktuellen Code.

Die Oberfläche und das Dokument haben unterschiedliche Aufgaben: YAML beschreibt
Buttons, Tabs und Panels. Die fachlichen Daten liegen im aktiven Editor in C-Strukturen; beim Editorwechsel
werden sie vorübergehend in Sitzungsdateien ausgelagert. Ein Artboard ist ein Datensatz im Dokument, kein eigenes UI-Widget.

## 1. Welche Datei übernimmt welche Aufgabe?

| Datei | Aufgabe |
| --- | --- |
| [editor.yaml](../editors/vector/ui/editor.yaml) | Beschreibt die Vector-Oberfläche und die Aktionsnamen der Buttons. |
| [application.c](../apps/demo/editor/application.c) | Verarbeitet SDL-Ereignisse, zeichnet die Shell und verteilt Aktionen. |
| [host.c](../apps/demo/editor_host/host.c) | Leitet an den aktiven Editor weiter und verwaltet dessen Lebensdauer. |
| [editor.h](../apps/demo/editor_host/editor.h) | Definiert die gemeinsame Schnittstelle `EditorDefinition`. |
| [adapter.c](../editors/vector/adapter.c) | Hinterlegt die Vector-Funktionen für diese Schnittstelle. |
| [editor.c](../editors/vector/editor.c) | Ordnet Aktionen dem Dokument, Werkzeug oder der History zu. |
| [document/document.c](../editors/vector/document/document.c) | Initialisiert Dokumentdaten, sucht IDs und verwaltet ihren Besitz; ohne UI-Aufrufe. |
| [workspace/documents.c](../editors/vector/workspace/documents.c) | Verbindet Dokumente mit Tabs, merkt Erstellungen vor und übernimmt sie am Frame-Ende. |
| [artboard/tool.c](../editors/vector/artboard/tool.c) | Verarbeitet den Maus-Drag und hält die vorläufige Geometrie. |
| [artboard.c](../editors/vector/artboard/artboard.c) | Erstellt den Änderungsbefehl und implementiert dessen `apply` und `revert`. |
| [history.c](../editors/vector/history/history.c) | Führt Befehle aus und verwaltet Undo/Redo. |
| [canvas.c](../editors/vector/workspace/canvas.c) | Zeichnet Dokumentdaten und Vorschau; verarbeitet Zoom und Pan. |
| [ruler.c](../editors/vector/workspace/ruler.c) | Zeichnet die Pixel-Lineale aus Artboard-Geometrie und Zoom. |

Der Host kennt die gemeinsame Schnittstelle, aber keine Artboard-Struktur.
pengUI kennt Widgets und Canvas-Callbacks, aber keine Vector-Dokumente.

## 2. Wie wird aus YAML ein Funktionsaufruf?

Der Button steht beispielsweise so in der Vector-YAML:

```yaml
- {type: button, id: vector_add_artboard, icon: plus, action: vector.add_artboard, width: 30}
```

`id` identifiziert das Widget. `action` ist zunächst nur eine Zeichenkette.
Der YAML-Parser sucht keine gleichnamige C-Funktion und ruft auch keine auf.

Beim Laden speichert pengUI beides in einem `PUINode`. Die App übergibt beim
Zeichnen ihren Callback `action` und ihren Anwendungskontext `&app`:

```c
pui_view_draw(app.shell.view, &app.shell.input, area, action, &app);
```

Wenn ein Button aktiviert wird, ruft pengUI diesen Callback sinngemäß so auf:

```c
action(&app, "vector.add_artboard", "vector_add_artboard");
```

Der zweite Parameter ist der Aktionsname, der dritte die Widget-ID. Unterschiedliche
Buttons können dadurch dieselbe Aktion verwenden. Die Zuordnung zur fachlichen
Funktion erfolgt in C, in diesem Fall durch die Vergleiche in `vector_editor_action()`.

### Der Klick wird teilweise erst beim Zeichnen ausgewertet

In [events.c](../lib/pengui/src/view/events.c) übernimmt `pui_view_event()` die
SDL-Mausereignisse in den `PUIContext`. Der Kontext merkt unter anderem Position,
Drücken und Loslassen. Viele Widgets werten diese Informationen erst im nächsten
`pui_view_draw()` aus, wenn auch ihre aktuelle Bildschirmgeometrie bekannt ist.

Für normale Buttons prüft `pui_button()` in
[button.c](../lib/pengui/src/widgets/button.c), ob die Interaktion gültig ist.
Typischerweise muss die Maustaste auf dem Button gedrückt und dort wieder
losgelassen werden. [button_sdl.c](../lib/pengui/src/widgets/button_sdl.c)
ruft anschließend den App-Callback mit Aktionsname und Widget-ID auf.

`pui_view_event()` kann deshalb `false` zurückgeben, obwohl es Mauszustand
aktualisiert hat. `false` bedeutet hier nicht „das Ereignis wurde überhaupt
nicht betrachtet“, sondern erlaubt die weitere Verarbeitung durch den Editor.

## 3. Der gemeinsame Weg durch App, Host und Adapter

Für `vector.add_artboard` und `vector.add_document` sieht die Aufrufkette so aus:

```text
SDL-Mausereignisse
  -> events() in application.c
  -> pui_view_event() merkt den Pointer-Zustand

Beim Zeichnen des Frames:
  pui_view_draw()
    -> pui_widget_button_draw()
    -> pui_button() erkennt die Aktivierung
    -> action(&app, Aktionsname, Widget-ID)
    -> editor_host_action()
    -> host->active->action(host->instance, ...)
    -> vector_editor_action()
```

Die Felder `action`, `event`, `canvas` und `commit_ui` in `EditorDefinition` sind
**Funktionszeiger**. `adapter.c` trägt beispielsweise
`.action = vector_editor_action` ein. Der Host kann die Funktion dadurch aufrufen,
ohne selbst den konkreten Vector-Header einzubinden.

`host->instance` zeigt auf die vom aktiven Editor angelegte Instanz. Die gemeinsame
Schnittstelle verwendet dafür `void *`; der Vektoreditor wandelt diesen Zeiger
wieder in `VectorEditor *` um.

Die App behandelt Fensteraktionen und Editorwechsel selbst. Aktionen wie
`editor.undo` werden weitergereicht, weil `undo` keine registrierte Editor-ID ist.
Sie erreichen damit ebenfalls `vector_editor_action()` des aktiven Editors.

## 4. Beispiel: „Add Document“ erzeugt einen neuen Dokument-Tab

Der Aktionsname lautet `vector.add_document`. Hier wird während des
Button-Callbacks noch kein Tab in den gerade durchlaufenen UI-Baum eingefügt.

1. `vector_editor_action()` ruft `vector_document_tabs_request()` auf.
2. Die Workspace-Anbindung prüft das Tab-Limit und erhöht `editor->document_tabs.pending`.
   Das ist ein Zähler vorgemerkter Dokumente, noch kein neues Dokument.
3. Nach dem Zeichnen und `SDL_RenderPresent()` ruft die App am Frame-Ende
   `editor_host_commit()` auf.
4. Ohne anstehenden Editorwechsel ruft der Host den Adapter-Eintrag `commit_ui`
   auf, also `vector_editor_commit_ui()`.
5. `vector_document_tabs_commit()` lässt `vector_document_init()` eine lokale
   `VectorDocument`-Struktur vorbereiten: ID, Titel, nächste Artboard-ID sowie einen Viewport mit Zoom `1`.
6. `vector_canvas_layout()` erzeugt in C ein Layout mit einem Canvas-Knoten.
   Eine eigene Dokument-YAML gibt es nicht.
7. `pui_view_add_tab()` kopiert diesen Inhalt in die View, ergänzt den Tab und
   wählt ihn aus. Beispielsweise heißen Tab und Canvas danach
   `vector_document_1` und `vector_document_1.canvas`.
8. Erst nach erfolgreichem Einfügen übernimmt `vector_documents_append()` das
   vorbereitete Dokument in `documents.items[documents.count]` und erhöht `count`.
9. Das temporäre Canvas-Layout wird mit `pui_layout_free()` freigegeben.
   Die View verwendet ihre eigene Kopie. Im nächsten Frame erscheint der Tab.

Schlägt das Einfügen fehl, bleibt der bisherige UI-Baum erhalten. Es wird kein
Dokumenteintrag angehängt und keine Dokumentnummer verbraucht. Der verbleibende
Anforderungszähler wird zurückgesetzt.

Der Aufschub schützt die laufende UI-Traversierung. Eine einfache Änderung an
Dokumentdaten muss dagegen keinen UI-Baum austauschen. Die beiden Fälle haben
deshalb unterschiedliche Zeitpunkte für ihre Änderung.

Bei einer leeren Dokumentverwaltung merkt `vector_document_tabs_commit()` automatisch
eine Erstellung vor, falls nicht bereits eine explizite Anforderung vorliegt. Das gilt
auch nach dem Einlesen eines leeren Checkpoints. Die übrige Erstellung läuft über
denselben oben beschriebenen Weg; es gibt keinen zweiten Erzeugungsmechanismus.

Das Anlegen eines **Dokuments** ist aktuell nicht Teil der Undo-History.
Die History wird bisher für das Anlegen von **Artboards innerhalb eines Dokuments** verwendet.

## 5. Beispiel: Ein Artboard mit der Maus aufziehen

### Der Button aktiviert zunächst nur das Werkzeug

Bei `vector.add_artboard` ermittelt `vector_document_tabs_active()` anhand des
gewählten Tabs das zugehörige Dokument. Beim Start ist die Dokument-Tab-Leiste
zunächst leer; der erste UI-Commit ergänzt automatisch ein Anfangsdokument, sofern
noch keine explizite Erstellung vorgemerkt ist. Ohne aktives Dokument erscheint bei
dieser Aktion lediglich eine Statusmeldung.

Mit aktivem Dokument ruft `vector_editor_action()`
`vector_artboard_tool_arm(document)` auf. Dabei wird das Werkzeug aktiviert und
eine eventuell vorherige Vorschau verworfen. Es entsteht noch kein Artboard und
noch kein History-Eintrag. `vector_toolbar_sync()` aus
[ui/toolbar.c](../editors/vector/ui/toolbar.c) hält die Aktiv-Markierung des
`vector_add_artboard`-Toggles synchron zum Werkzeugzustand. Beim Erstellen oder
Abbrechen wird der Toggle wieder inaktiv.

### Der Maus-Drag läuft über die Ereignis-Schnittstelle

Nach dem Klick erhält der Editor die folgenden Mausereignisse über einen
anderen Adapter-Eintrag als die Button-Aktion:

```text
SDL-Mausereignis
  -> events() in application.c
  -> pui_view_event()
  -> falls nicht konsumiert: editor_host_event()
  -> host->active->event(host->instance, ...)
  -> vector_editor_event()
  -> vector_canvas_event()
  -> vector_artboard_tool_event()
```

Beim Drücken der linken Maustaste prüft das Werkzeug, ob der Canvas getroffen
wurde. Es rechnet die Mausposition in Dokumentkoordinaten um und merkt den
Startpunkt. Bei Bewegungen wird nur der Endpunkt der Vorschau aktualisiert.
Die Anzahl der Artboards und die History bleiben währenddessen unverändert.

Beim Loslassen normalisiert `vector_artboard_tool_rect()` das Rechteck.
Es hat auch beim Ziehen nach links oder oben eine positive Breite und Höhe.
Koordinaten werden auf ganze Dokument-Pixel gerundet. Breite und Höhe müssen
mindestens einen Pixel betragen; ein einfacher Klick erzeugt kein Artboard.

Escape oder Fokusverlust verwirft die Vorschau. Beim Wechsel des Dokument-Tabs
wird das Werkzeug des bisherigen Dokuments ebenfalls zurückgesetzt.

### Hier wird das Dokument tatsächlich verändert

Für ein gültiges Rechteck führt das Loslassen zu dieser Kette:

```text
vector_artboard_tool_event()
  -> vector_artboard_add(document, rect)
  -> vector_history_execute(&document->history, document, command)
  -> command.apply(document, command.data)
  -> apply() in artboard.c
  -> document->artboards[document->artboard_count++] = command->artboard
```

`vector_artboard_add()` prüft Geometrie und Kapazität. Danach reserviert es auf
dem Heap einen `AddArtboard`-Datensatz mit der neuen Geometrie, Artboard-ID,
Einfügeposition und vorherigen aktiven Artboard-ID.

Der History wird dieser Datensatz zusammen mit zwei Funktionszeigern übergeben:

```c
(VectorCommand){apply, revert, command}
```

`apply` und `revert` sind `static`-Funktionen in `artboard.c`. Andere Dateien können
sie nicht über ihren Namen aufrufen; über die übergebenen Funktionszeiger kann
die History sie dennoch ausführen.

Die **eigentliche Änderung am Artboard-Array** erfolgt in `apply()`. Es kopiert
das Artboard an das Ende des gültigen Bereichs und setzt `active_artboard` auf
dessen ID. Nach erfolgreichem Ausführen wird der Befehl in der History behalten;
`next_artboard_id` wird erhöht. Das Werkzeug endet nach dem erfolgreichen Anlegen.

## 6. Wie wird die Änderung sichtbar?

Die View zeichnet ihren Canvas über einen registrierten Callback:

```text
pui_view_draw()
  -> pui_view_draw_node(): PUI_NODE_CANVAS
  -> draw_canvas() im Host
  -> host->active->canvas(host->instance, ...)
  -> vector_editor_canvas()
  -> vector_canvas_draw()
```

Der Vektoreditor ordnet die Canvas-ID dem Dokument zu. `vector_canvas_draw()`
liest dessen Artboard-Array und zeichnet die Rechtecke. `vector_ruler_draw()`
berechnet die Lineale daraus. Während eines Drags kommt die Vorschau aus dem
Werkzeugzustand hinzu.

Es wird kein separates Widget pro Artboard erstellt. Nach einer Datenänderung
reicht der nächste Frame, um das neue Ergebnis zu zeichnen. Das Dokument enthält
auch kein fertig gerendertes Bild seiner Zeichenfläche.

## 7. Das interne Speichermodell

„Speichern im Arbeitsspeicher“ und „Speichern als Datei“ sind hier getrennte Dinge.
Der aktuelle Vektoreditor hält aktive Daten im RAM und lagert sie beim Wechsel in temporäre
Sitzungsdateien aus; SVG-Laden und -Speichern sind noch nicht implementiert. Der Save-Button hat noch keinen entsprechenden Vector-Handler.

### Besitzstruktur

```text
EditorApp                         lokale Struktur in editor_application_run()
  EditorHost
    checkpoints ----------------> Liste aus Editordefinition + temporaerer FILE-Datei
    instance -------------------> VectorEditor             Heap: calloc()
                                    layout --------------> PUILayout
                                    document_tabs          vorgemerkte UI-Anforderungen
                                      pending
                                    documents              eingebettete Struktur
                                      items[15]             eingebettetes Array
                                        VectorDocument
                                          id, title
                                          artboards[64]     eingebettetes Array
                                          artboard_count
                                          next_artboard_id
                                          active_artboard
                                          history
                                            commands[128]   eingebettetes Array
                                              apply         Funktionszeiger
                                              revert        Funktionszeiger
                                              data -------> AddArtboard auf dem Heap
                                            count, cursor
                                          viewport
                                          artboard_tool
```

Die tatsächlichen Definitionen stehen in
[internal.h](../editors/vector/internal.h),
[document.h](../editors/vector/document/document.h),
[artboard.h](../editors/vector/artboard/artboard.h),
[history.h](../editors/vector/history/history.h),
[viewport.h](../editors/vector/workspace/viewport.h) und
[tool_state.h](../editors/vector/artboard/tool_state.h).
Die letzten beiden Header beschreiben nur Zustandswerte. Renderer und Ereignisverarbeitung
liegen getrennt in `workspace/canvas.c` bzw. `artboard/tool.c`.

**Eingebettet** bedeutet: Diese Daten liegen direkt innerhalb der übergeordneten
Struktur. Für ein einzelnes Artboard wird kein zusätzlicher Heap-Speicher
reserviert. Das `calloc()` der Editorinstanz reserviert bereits Platz für alle
15 Dokumentplätze einschließlich ihrer Arrays und History-Einträge. `count`
bestimmt, welche Plätze tatsächlich verwendet werden. Das ist einfach und
begrenzt, reserviert aber auch ungenutzte Kapazität.

Zusätzlich werden die Nutzdaten erfolgreicher Änderungsbefehle einzeln mit
`malloc()` angelegt. Die Größe der Strukturen in Bytes hängt von Plattform,
Zeigergröße und Compiler-Ausrichtung ab; `sizeof(VectorEditor)` bzw.
`sizeof(VectorDocument)` liefern die tatsächlichen Größen im jeweiligen Build.

### Was enthält ein Dokument?

| Daten | Bedeutung |
| --- | --- |
| `id`, `title[64]` | Laufende Dokument-ID und direkt gespeicherter Titeltext. |
| `artboards[64]` | Artboard-Datensätze; jeder enthält `unsigned id` und `PUIRect rect`. |
| `artboard_count` | Nur die Plätze von `0` bis `artboard_count - 1` gehören zum aktuellen Dokumentinhalt. |
| `next_artboard_id` | Nächste ID; wird durch Undo nicht zurückgesetzt. Nach verworfenem Redo-Zweig können Nummernlücken entstehen. |
| `active_artboard` | ID des zuletzt aktivierten Artboards, aktuell beim Anlegen bzw. Wiederherstellen gesetzt; `0` bedeutet keines. |
| `history` | Befehle und Position innerhalb von Undo/Redo, getrennt pro Dokument. |
| `viewport` | Bildschirmbereich, Verschiebung, Zoom und Zustand einer Pan-Geste. |
| `artboard_tool` | Aktivierung und Start-/Endpunkt der vorläufigen Zeichenaktion. |

`PUIRect` enthält `x`, `y`, `w`, `h` als `float`. Artboards verwenden Dokument-Pixel.
Die Mausaktion rundet zwar auf ganze Pixel, die Speicherstruktur bleibt aber eine
Gleitkomma-Struktur. Artboards sind derzeit weder ECS-Entities noch SVG-Knoten.

Dokumente liegen nicht in der View. Die View hält beispielsweise Tab-Auswahl,
Splitgrößen und Widgettexte; der Vektoreditor hält die fachlichen Daten.
`vector_document_tabs_active()` verbindet beides über die ausgewählte Tab-ID.
Eine geänderte Tab-Reihenfolge sortiert deshalb nicht das Dokument-Array um.

Da die Arrays nicht wachsen oder umgezogen werden, bleibt ein Zeiger auf einen
Dokumenteintrag beim Ergänzen weiterer Dokumente gültig. Er darf aber nicht nach
dem Entladen des Editors weiterverwendet werden. Eine vollständige Kopie eines
bereits benutzten `VectorDocument` wäre keine unabhängige Kopie: Die
History-`data`-Zeiger würden auf dieselben Heap-Daten zeigen. Solche Dokumente
dürfen daher nicht einfach als besitzende Kopien weitergeführt und doppelt freigegeben werden.

### Dokumentkoordinaten und Bildschirmkoordinaten

Für die horizontale Achse gilt:

```text
Bildschirm-X = Canvas-X + Viewport-Verschiebung-X + Dokument-X * Zoom
Dokument-X   = (Bildschirm-X - Canvas-X - Viewport-Verschiebung-X) / Zoom
```

Für Y gilt dieselbe Rechnung. Die Werkzeugfunktion `update_endpoint()` verwendet
die zweite Formel und rundet das Ergebnis. `vector_canvas_draw()` verwendet die
erste Formel zum Zeichnen.

Zoom und Pan ändern also nicht die gespeicherte Artboard-Geometrie. Der Viewport
wird pro Dokument aufbewahrt und ist nicht Teil der Undo-History. Undo/Redo kann
die Ansicht allerdings auf das aktive Artboard zentrieren, weil
`vector_editor_action()` nach erfolgreicher Änderung `vector_canvas_center()` aufruft.
Die Lineale beginnen relativ zum jeweiligen Artboard bei `0`; ihre Unterteilung
berücksichtigt den Zoom, ihre Dicke bleibt in Bildschirm-Pixeln konstant.

## 8. Wie speichert die History Änderungen?

Die History speichert **Befehle mit Änderungsdaten**, keine vollständigen
Dokument-Snapshots und keine kompletten Bilder. Beim Anlegen eines Artboards
enthält `AddArtboard` eine Kopie genau dieses Artboards sowie Einfügeindex und
vorherige aktive ID. Die History benötigt keinen dauerhaften Zeiger auf das
Dokument: Beim Ausführen bekommt sie den aktuellen Dokumentzeiger als Argument.

`count` zählt alle gespeicherten Befehle einschließlich rückgängig gemachter.
`cursor` zählt die momentan angewendeten Befehle:

```text
Nach A, B, C:        [ A B C ]    count = 3, cursor = 3
Nach Undo:          [ A B | C ]  count = 3, cursor = 2
Nach erneutem Undo: [ A | B C ]  count = 3, cursor = 1
Nach Redo:          [ A B | C ]  count = 3, cursor = 2
```

Links vom Strich liegt der angewendete Teil; rechts stehen mögliche Redo-Schritte.

- `vector_history_undo()` ruft `revert()` des Befehls bei `cursor - 1` auf und
  verringert den Cursor nur bei Erfolg. Für das Anlegen entfernt `revert()` den
  letzten gültigen Artboard-Eintrag und stellt die vorherige aktive ID wieder her.
- `vector_history_redo()` ruft `apply()` bei `cursor` auf und erhöht den Cursor
  bei Erfolg. Dasselbe Artboard mit derselben ID und Geometrie erscheint wieder.
- Eine neue Änderung nach Undo verwirft den bisherigen Redo-Zweig. Erst wenn
  der neue Befehl erfolgreich angewendet wurde, werden dessen alte Nutzdaten
  freigegeben und die neue History fortgeschrieben.

Beim Undo wird der Artboard-Arrayplatz nicht mit `free()` gelöscht: Er ist Teil
der Editorstruktur. Stattdessen wird `artboard_count` verringert. Alte Bytes
können im nun unbenutzten Platz liegen bleiben, gehören aber nicht mehr zum
Dokument. Die Kopie im History-Befehl ermöglicht das spätere Redo.

Die Nutzdaten eines Befehls bleiben auch nach Undo erhalten. Sie werden erst
beim Verwerfen des Redo-Zweigs oder beim Zerstören der History freigegeben.

### Wer gibt welchen Speicher frei?

| Speicher | Besitzer und Freigabe |
| --- | --- |
| `VectorEditor` und seine eingebetteten Arrays | Vom Editor mit `calloc()` angelegt; `vector_editor_destroy()` gibt die Instanz mit `free()` frei. |
| Layout der Editorinstanz | Vom Vektoreditor gehalten; Freigabe mit `pui_layout_free()` beim Zerstören. |
| Temporäres Canvas-Layout eines neuen Tabs | In `vector_document_tabs_commit()` angelegt und nach dem Kopieren in die View sofort freigegeben. |
| Zusammengesetztes View-Layout und Widgetzustände | Von der View verwaltet; beim Austausch bzw. Zerstören freigegeben. |
| `AddArtboard` vor erfolgreichem `vector_history_execute()` | Gehört dem Aufrufer; `vector_artboard_add()` gibt es bei einem Fehler frei. |
| `AddArtboard` nach erfolgreicher Übernahme | Gehört der History; Freigabe bei verworfenem Redo-Zweig oder `vector_history_destroy()`. |
| Temporäre Checkpoint-Dateien und Listeneinträge | Gehören dem Host; alte Fassungen werden nach erfolgreichem Wechsel ersetzt, alle Dateien beim Beenden geschlossen und gelöscht. |
| SDL-Renderer und Fenster | Gehören der App; werden beim Beenden nach der UI freigegeben. |

Die Callback-Funktionen selbst sind Programmcode und werden nicht pro Befehl
reserviert oder freigegeben. Nur die Daten hinter `VectorCommand.data` sind
separate Heap-Allokationen.

## 9. Grenzen und Lebensdauer im aktuellen Stand

Es gibt höchstens 15 Dokumente, 64 Artboards pro
Dokument und 128 Befehlsplätze pro History. Das gemeinsame UI-Layout hat außerdem
ein Limit von 128 Knoten. Grenzen führen zur Ablehnung weiterer Änderungen;
die History entfernt nicht automatisch ihre ältesten Befehle.

Alle beschriebenen Abläufe laufen synchron auf dem App-/UI-Thread. Es gibt
hier keinen Hintergrundthread und keine parallele Dokumentbearbeitung.

Beim Wechsel zwischen Dokument-Tabs bleiben die Dokumente im aktiven Vektoreditor
im Speicher. Ein Wechsel zu einem anderen **Editor** oder ein Reload sichert nun
zunächst einen Checkpoint und entlädt anschließend die Vectorinstanz.
`vector_editor_destroy()` lässt zunächst über die Workspace-Anbindung alle
nativen Gesten abbrechen. `vector_documents_destroy()` gibt anschließend die
History-Nutzdaten aller Dokumente frei. Anschließend wird die Instanz freigegeben. Der Checkpoint hält
keine Zeiger auf diese freigegebenen Daten.

### Checkpoint-Ablauf im Host

```text
editor_host_commit()
  -> bisheriges commit_ui(): ausstehende Dokument-Tabs übernehmen
  -> editor_checkpoint_capture()
       -> vector_editor_checkpoint(): Werte in neue temporäre Datei schreiben
       -> Datei flushen; bisherigen Checkpoint noch behalten
  -> Ziel-Editor create()
  -> editor_checkpoint_restore(), falls ein Checkpoint vorliegt
       -> vector_editor_restore(): Daten und History laden, Layout vorbereiten
  -> pui_view_replace_slot(): vollständig vorbereitete UI übernehmen
  -> editor_checkpoint_publish(): neue Datei ersetzt die bisherige Fassung
  -> bisheriges destroy(): alte Laufzeitdaten freigeben
```

Bei einem Reload ist der Ziel-Editor derselbe. Er erhält direkt den gerade frisch
geschriebenen Checkpoint. Bei einem Wechsel zu Vector wird dessen zuletzt abgelegte
Sitzungsdatei gelesen. Der Adapter-Vertrag in `EditorDefinition` besitzt dafür die
optionalen Funktionen `checkpoint()` und `restore()`.

Fehlschläge beim Sichern, Erzeugen, Laden oder Mounten brechen den Wechsel ab.
Der bisher aktive Editor und der zuvor erfolgreich abgelegte Checkpoint bleiben
erhalten. Das Vorbereiten erfolgt vor dem Austausch der laufenden Oberfläche.

### Was steht in der temporären Datei?

[checkpoint.c](../editors/vector/persistence/checkpoint.c) schreibt eine Versionssignatur,
Dokumentanzahl, Tab-Reihenfolge und Tab-Auswahl. Zusätzlich wird die Auswahl des
linken Navigationstabs gespeichert. Danach folgen pro Dokument
Identität, Titel, Artboard-Array, nächste und aktive Artboard-ID, Zoom/Verschiebung
sowie History-Anzahl, Cursor und die Änderungsdaten aller Befehle.

Die Einträge umfassen auch rückgängig gemachte Befehle: Redo funktioniert nach dem
Zurückwechseln weiter. [artboard.c](../editors/vector/artboard/artboard.c) schreibt für diese
Befehle einen Typkennzeichner und die Artboard-Daten mit Einfügeindex und vorheriger
aktiver ID. Funktionszeiger und Heap-Adressen werden niemals gespeichert.
Beim Lesen werden neue Nutzdaten reserviert und die passenden lokalen `apply`-/
`revert`-Funktionen zugeordnet. Ein Probe-Durchlauf prüft die Konsistenz der History
mit dem gespeicherten Dokumentzustand.

Die Wiederherstellung baut neue Canvas-Tabs in einem neuen Layout auf. Die
Reihenfolge wird rekonstruiert und der gespeicherte Tab über dessen `checked`-
Anfangswert ausgewählt. `pui_view_tab_at()` liefert beim Sichern die aktuelle
Anzeigereihenfolge. Laufende Mausgesten und Capture, die Werkzeug-Vorschau und
sonstige Widgetzustände werden nicht serialisiert. Der Bildschirmbereich des
Viewports wird beim Zeichnen neu berechnet; Pan und Zoom bleiben erhalten.

Das Format verwendet native C-Zahlenrepräsentationen und gilt nur innerhalb der
laufenden Sitzung. Es ist kein portables Dokumentformat. Verweise auf Dokumente
vor dem Wechsel bleiben ungültig: Nach der Rückkehr sind neue Strukturen und
History-Nutzdaten entstanden, auch wenn ihre fachlichen IDs gleich geblieben sind.

Die temporäre Datei gehört dem Host, nicht der Editorinstanz.
[checkpoint.c im Host](../apps/demo/editor_host/checkpoint.c) hält nur kleine
Listeneinträge und offene Dateien; inaktive Modelle, Canvas-Views und History-
Nutzdaten bleiben nicht im RAM. Die Module sind statisch verlinkt; „Entladen“
bezeichnet weiterhin die Laufzeitdaten, nicht den Maschinencode. Während eines
Wechsels können kurzzeitig bisherige und neue Instanz gleichzeitig existieren.

[platform/temp_file.c](../apps/demo/platform/temp_file.c) kapselt die Anlage privater,
beim Schließen gelöschter Dateien im temporären Bereich des Betriebssystems.
Beim App-Ende schließt `editor_host_destroy()` alle Dateien. Ein Neustart beginnt
mit leeren Dokumenten; eine Wiederherstellung nach einem Neustart oder Absturz
ist nicht Bestandteil dieses Mechanismus. YAML wird durch Dokumentänderungen
nicht beschrieben. Eine Rückfrage beim Beenden und reguläres SVG-Speichern
sind weiterhin offen.

## 10. Wo setzt eine weitere Änderungsfunktion an?

Für eine neue fachliche Bearbeitung sind die vorhandenen Stellen ein Muster:

1. Einen Aktionsnamen in YAML eintragen, falls ein Button benötigt wird.
2. Die Aktion in `vector_editor_action()` auflösen und das aktive Dokument bestimmen.
3. Für eine Mausgeste zunächst nur vorläufigen Werkzeugzustand ändern.
4. Die abgeschlossene Änderung als Befehl mit `apply`, `revert` und eigenen
   Änderungsdaten an die History übergeben.
5. Die Darstellung aus den aktuellen Dokumentdaten zeichnen.

Die History fordert, dass `apply()` und `revert()` bei einem Fehler keine
Teiländerung hinterlassen. Neue Befehle müssen ihre erforderlichen Prüfungen
entsprechend vor der eigentlichen Mutation durchführen. Wenn die Nutzdaten
später ihrerseits Heap-Zeiger enthalten, muss auch der Freigabevertrag erweitert
werden: Aktuell ruft die History lediglich `free(command.data)` auf. Neue Befehlstypen
benötigen außerdem eine Erweiterung des Checkpoint-Codecs.

Damit bleiben UI-Beschreibung, Interaktionszustand, fachliche Mutation und
Undo/Redo nachvollziehbar getrennt. Die konkrete Struktur für zukünftige
Vektorformen oder einen SVG-Import ist damit noch nicht festgelegt.


## Artboard-Auswahl und Eigenschaften

Ein erneuter Klick auf das aktive Artboard-Werkzeug bricht es ab. Der Auswahlbutton
beendet das Werkzeug ebenfalls. Artboards lassen sich anschließend auf der Zeichenfläche
oder im Dokument-Tree auswählen.

Der Inspector enthält den Slot `selection_properties`. Die UI-Anbindung
`editors/vector/ui/inspector.c` setzt dort bei vorhandener Auswahl die Beschreibung
`artboard_inspector.yaml` ein und entfernt sie wieder ohne Auswahl. Die Felder werden
bei Enter oder beim Feldwechsel mit Tab beziehungsweise Shift+Tab übernommen. Name, Breite und Höhe werden über
`vector_artboard_update()` als ein Vorher-/Nachher-Befehl in die dokumenteigene History
aufgenommen. Ungültige Werte verändern das Dokument nicht; das Feld zeigt wieder den
bisherigen Wert. Größen sind Dokumentpixel und liegen zwischen 1 und 1.000.000; Namen
haben maximal 63 UTF-8-Bytes.

Die Artboard-ID bleibt beim Umbenennen unverändert. Beschriftung über der Zeichenfläche
und Tree-Zeile lesen den Namen aus demselben Artboard. Der Dokument-Tab behält
seinen Dokumentnamen. Tab und Shift+Tab übernehmen den Wert des verlassenen Feldes
und fokussieren das nächste beziehungsweise vorherige Eingabefeld. Auswahl und UI-Beschriftungen
sind keine eigenen Undo-Schritte. Die temporären Checkpoints sichern Namen, Geometrie,
Auswahl sowie Erstellungs- und Änderungsbefehle einschließlich noch möglicher Redo-Schritte.


## Auswahlwerkzeug und Verschieben

Der Auswahlbutton `vector_selection` ist aktiv, solange das Artboard-Werkzeug nicht
aktiv ist. Nur im Auswahlmodus können bestehende Artboards auf der Zeichenfläche
oder im Tree ausgewählt werden. Ein Klick in die Artboard-Fläche wählt aus; ein
Verschieben beginnt ausschließlich am Namen über dem Artboard.

`workspace/selection.c` berechnet dieselben Label-Grenzen für Rendering und Hit-Test.
Beim Ziehen enthält `VectorSelectionTool` nur den vorläufigen Versatz. Die Zeichenfläche
stellt diesen dar, während die gespeicherte Artboard-Geometrie unverändert bleibt.
Mausbewegungen werden durch den Zoom in Dokumentpixel umgerechnet. Beim Loslassen
führt `vector_artboard_move()` genau einen Änderungsbefehl aus, sofern die Position
verändert wurde. Undo/Redo und temporäre Checkpoints verwenden den vorhandenen
Vorher-/Nachher-Befehl. Escape, Fokusverlust, Dokument- oder Werkzeugwechsel verwerfen
die Vorschau und geben den Maus-Capture frei.


## Größe mit Auswahlgreifern ändern

Das ausgewählte Artboard zeigt im Auswahlwerkzeug acht 7-Pixel-Greifer an Ecken und
Kantenmitten. Ihre Bildschirmgröße bleibt beim Zoomen konstant; eine etwas größere
Trefferfläche erleichtert die Bedienung. Ecken ändern beide Achsen, Kantenmitten nur
ihre jeweilige Achse. Die gegenüberliegende Kante bleibt fest, Breite und Höhe werden
auf mindestens einen Dokumentpixel begrenzt; ein Überkreuzen spiegelt das Artboard nicht.

`geometry/resize.h` und `resize.c` enthalten die vom Objekttyp unabhängige Berechnung
von Greiferpositionen, Trefferflächen und Rechteckänderungen. Sie kennen weder Artboards
noch SDL-Ereignisse oder History. Weitere Objekte können diese Geometrie später verwenden.
`workspace/selection.c` übernimmt die Mausinteraktion und Vorschau; beim Loslassen schreibt
`vector_artboard_set_rect()` Position und Größe gemeinsam als einen History-Befehl.
Inspector und Lineale folgen der übernommenen Größe. Abbrechen verwirft die Vorschau
wie beim Verschieben.


## Sichtbarkeit und Sperren

`object/state.h` definiert gemeinsame Flags `VECTOR_OBJECT_HIDDEN` und
`VECTOR_OBJECT_LOCKED` für Artboards und zukünftige Ebenen, Gruppen und Formen.
Null bedeutet sichtbar und ungesperrt. Aktuell gibt es weiterhin Artboards als
Dokumentobjekte; die Flags führen keine zusätzlichen Ebenen oder Objekttypen ein.

Im Dokument-Tree stehen rechts Auge und Schloss als separate Tree-Table-Zellen.
Sie verändern den Zustand der betreffenden Zeile, ohne die aktuelle Auswahl zu wechseln.
Der ausgewählte Eintrag zeigt dieselben Zustände über die YAML-Checkboxen
`artboard_visible` und `artboard_locked` im Inspector. Beide UI-Anbindungen rufen
`vector_artboard_set_state()` auf. Der Befehl nutzt dieselbe Vorher-/Nachher-History wie
Geometrie und Name. Ein laufender Maus-Preview wird vorher abgebrochen. Undo/Redo und
temporäre Checkpoints sichern auch die Flags.

Ausgeblendete Artboards werden auf der Zeichenfläche vollständig übersprungen.
Gesperrte Artboards bleiben sichtbar, sind dort aber weder auswählbar noch verschiebbar
oder skalierbar; sie zeigen keine Greifer. Auch Namens- und Größenänderungen durch den
Inspector werden bei gesperrten Artboards abgewiesen und der bisherige Feldwert wird
wiederhergestellt. Im Tree können versteckte und gesperrte Einträge weiterhin ausgewählt
werden, damit sie über Tree oder Inspector wieder eingeblendet oder entsperrt werden können.
Die Zustandsbefehle selbst bleiben deshalb auch bei gesperrten Objekten verfügbar.


## Rechtecke als Artboard-Kinder

Der YAML-Button `vector_add_rect` mit Aktion `vector.add_rect` aktiviert das
Rechteck-Werkzeug. Erneuter Klick, Escape, Fokusverlust oder Werkzeugwechsel brechen
es ab. Eine linke Maustaste innerhalb eines sichtbaren, entsperrten Artboards beginnt
die Vorschau. Der Endpunkt wird auf dessen Fläche begrenzt. Beim Loslassen erzeugt
`vector_rectangle_add()` einen History-Befehl; vor dem Loslassen werden keine
Rechteckdaten und keine History-Einträge angelegt. Nach Erfolg ist wieder das
Auswahlwerkzeug aktiv.

`rectangle/rectangle.h` beschreibt `VectorRectangle` mit stabiler ID, `parent_id`, Name,
eigenen Objektflags und einem lokalen Rechteck. Positionen beziehen sich auf die linke
obere Artboard-Ecke. `rectangle/tool.c` verarbeitet die Geste, `rectangle/render.c`
wendet den Artboard-Transform an und beschneidet die Ausgabe auf das Artboard.
Beim Verschieben des Artboards folgen die Kinder einschließlich der Live-Vorschau.
Eine Artboard-Größenänderung verändert den Ausschnitt, skaliert jedoch keine Kinder.

Der Dokument-Tree führt Kinder direkt nach ihrem Eltern-Artboard mit Tiefe 1 auf.
Artboards und Rechtecke haben eigene ID-Räume; ihre Tree-IDs unterscheiden die Typen.
Ein ausgewähltes Rechteck öffnet die zugehörigen Tree-Vorfahren. Auswahl ist im
Auswahlwerkzeug per Zeichenfläche oder Tree möglich. Der Inspector lädt dann
`rectangle_inspector.yaml` in den gemeinsamen Slot `selection_properties`.
Name, Breite, Höhe, Sichtbarkeit und Sperre sind über eigene bindings in
`ui/rectangle_inspector.c` bearbeitbar. Im Tree beziehen sich Auge und Schloss auf
die eigenen Flags; effektive Sichtbarkeit und Sperre berücksichtigen zusätzlich
die Flags des Eltern-Artboards. Eine eigene Sperre oder Elternsperre verhindert
Geometrie- und Namensänderungen; eigene Zustandsflags bleiben bearbeitbar.

Die History gehört weiterhin dem Dokument. `persistence/commands.c` verteilt native
Befehlstypen an die jeweiligen Artboard- und Rechteck-Codecs. Checkpoints speichern
Rechtecke, Eltern-IDs, Auswahl, ID-Zähler und die gesamte History einschließlich Redo.
Es sind aktuell höchstens 64 Artboards und 64 Rechtecke pro Dokument vorgesehen; alle
128 Tree-Zeilen passen in die bestehende Runtime-Tree-Grenze. Rechtecke erhalten
standardmäßig eine blaue, im Inspector bearbeitbare Füllung. Das Auswahlwerkzeug
unterstützt Verschieben und Skalieren der Rechtecke per Maus wie unten beschrieben.


## Rechtecke per Maus verschieben und skalieren

`workspace/selection.c` bündelt nun die Auswahlinteraktion für Artboards und Rechtecke.
Die reine Gestenstruktur steht in `workspace/selection_state.h`; Dokumentdaten hängen
weiterhin nicht von SDL-Ereignisverarbeitung ab. `VectorSelectionTool.rectangle_id`
unterscheidet Kind-Rechtecke von einer Artboard-Geste. Die vorhandene Rechteckgeometrie
in `geometry/resize.c` berechnet beide Arten von Größenänderungen.

Ein ausgewähltes, sichtbares und ungesperrtes Rechteck zeigt dieselben acht weißen
7-Pixel-Greifer wie ein Artboard. Ecken ändern beide Dimensionen, Kantenmitten eine.
Die Mindestgröße ist ein Dokumentpixel, die gegenüberliegende Kante bleibt fest.
Ein Ziehen an der Rechteckfläche verschiebt das Rechteck relativ zu seinem Artboard.
Artboards behalten das Verschieben über ihren Namen. Zoom wird bei Mausbewegungen
berücksichtigt; die Bildschirmgröße der Greifer bleibt konstant.

`vector_rectangle_display_rect()` liefert die lokale Vorschau. Rendering und
Auswahlrahmen verwenden diese Werte bereits während des Ziehens; gespeicherte
Geometrie und History bleiben unverändert. Erst Loslassen ruft
`vector_rectangle_update()` mit einem gemeinsamen Vorher-/Nachher-Befehl auf.
Ein Klick ohne Bewegung erzeugt keinen History-Eintrag. Escape, Fokusverlust und
Werkzeug- oder Editorwechsel verwerfen die Vorschau. Eigene und geerbte Sperren bzw.
Ausblendungen verhindern Mausänderungen.

Kinder dürfen beim Transformieren über Artboard-Grenzen reichen; ihre Füllung bleibt
am Artboard beschnitten. Der ausgewählte Begrenzungsrahmen samt Greifern bleibt auch
außerhalb sichtbar. Ein bereits ausgewähltes Rechteck kann dort an seiner Fläche
wieder zurückgezogen werden. Die Eltern-ID ändert sich durch Verschieben nicht.


## Rechteck-Füllfarbe und Farb-Vorschau

`VectorRectangle.fill` speichert `0xRRGGBBAA`; der Standard ist `0x7393AFFF`.
`rectangle_inspector.yaml` enthält das allgemeine pengUI-Widget `rectangle_fill`.
Die Aktionen `vector.rectangle.fill_preview` und `vector.rectangle.fill` werden
in `ui/rectangle_inspector.c` in Vorschau bzw. Modelländerung übersetzt.

Während eines Reglerzugs hält `VectorDocument.fill_preview` nur Rechteck-ID, vorläufige
Farbe und ein Aktiv-Flag. Der Renderer liest diese Farbe anstelle der gespeicherten
Füllung. Modellfarbe und History bleiben unverändert. Beim Loslassen führt
`vector_rectangle_set_fill()` genau einen vorhandenen Vorher-/Nachher-Befehl aus.
Eine Hex- oder numerische Eingabe übernimmt der Picker mit Enter oder beim Feldwechsel.
Escape, Fokusverlust oder Werkzeug-/Dokumentwechsel verwerfen die vorläufige Farbe.
Eigene und geerbte Sperren verhindern die Übernahme.

Beim Zeichnen wird der Alphakanal mit SDL-Blending verrechnet; Auswahlrahmen und
Greifer bleiben als Editorhilfen sichtbar. Die vorherige Renderer-Blendeinstellung
wird wiederhergestellt. Checkpoints sichern die gespeicherte Füllfarbe einschließlich
Undo- und Redo-Befehlen, jedoch keine aktive Farbvorschau. Nach Undo/Redo oder
Editorwechsel synchronisiert der Inspector den Modellwert zurück in den Picker.

## Raster und Tools-Panel

Die rechte Spalte in `editors/vector/ui/editor.yaml` ist ein vertikaler Split:
oben Inspector, unten Tools. `initial_last` legt die Anfangshoehe des unteren
Bereichs fest; der Greifer veraendert die Aufteilung. Die Rasterfarbe oeffnet
wie die Rechteck-Fuellfarbe einen Popup-Farbwaehler, damit beide Panels auch
bei kleiner Hoehe bedienbar bleiben.

`vector.grid.toggle` schaltet die Darstellung ueber den Toolbar-Button um.
`ui/grid.c` bindet Rastertyp, Breite, Hoehe und Farbe aus den YAML-Widgets an
`VectorDocument.grid`. Das Raster startet ausgeblendet mit 16 x 16 px und
halbtransparentem Grau. Breite und Hoehe akzeptieren 1 bis 10000 Dokumentpixel;
ungueltige Eingaben werden auf den vorherigen Wert zurueckgesetzt.

`grid/state.h` enthaelt ausschliesslich Werte. Die Einstellungen sind
Ansichtshilfen und erzeugen keine Schritte in der Artwork-History. Sie gelten
je Dokument und werden beim Editorwechsel im temporaeren Checkpoint gesichert
(Schema `PVCKPT10`). Es gibt damit noch keine dauerhafte Dokumentdatei.

`grid/render.c` zeichnet die Linien artboardlokal ueber dem Artwork und erhaelt
den SDL-Clip- und Blendzustand. Standardzellen sind rechteckig; Hexagonalzellen
haben die Spitze oben und versetzte Zeilen. Breite/Hoehe beschreiben deren
umschliessendes Rechteck; bei gleichen Werten sind die Sechsecke entsprechend
gestreckt. Fuer regelmaessige Sechsecke ist Breite/Hoehe etwa 0,866.
Zoom und Verschieben folgen der Zeichenflaeche. Rendering wird auf den sichtbaren
Ausschnitt begrenzt; bei weniger als 4 Bildschirm-Pixeln Zellbreite oder -hoehe
wird das Raster voruebergehend ausgelassen. Das verhindert ueberdichte Linien
und unnoetige Zeichenarbeit. Der Magnet-Button (`vector.grid.snap`) aktiviert das Einrasten fuer Rechtecke,
unabhaengig von der Rasteranzeige. Die Toleranz im Tools-Tab ist ein Fangabstand
in **Bildschirm-Pixeln** (Standard 5, gueltig 0 bis 100). Dadurch bleibt das
Mausgefuehl beim Zoomen gleich. 0 erlaubt nur exakt getroffene Rasterpunkte.

`grid/snap.c` enthaelt wiederverwendbare Geometrie ohne Dokument-, SDL-Eingabe-
oder UI-Abhaengigkeiten. `vector_grid_snap_point` dient dem Setzen neuer Punkte;
`vector_grid_snap_rect` richtet verschobene oder skalierte Rechtecke aus.
Das orthogonale Raster faengt beide Achsen unabhaengig; beim Verschieben wird
die jeweils naechste Rechteckkante verwendet. Isometrische und hexagonale Raster
fangen Rasterknoten. Verschieben behaelt die Groesse, Skalieren die gegenueberliegende
Kante und die Mindestgroesse von einem Pixel bei. Seitengreifer veraendern weiterhin
nur ihre eigene Achse. An Artboardgrenzen hat die Begrenzung der Erstellung Vorrang.

`rectangle/tool.c` nutzt die Punktfunktion fuer Anfang und Ende neuer Rechtecke.
`workspace/selection.c` nutzt die Rechteckfunktion fuer die Mausvorschau.
Nur beim Loslassen wird das Ergebnis ueber die bestehende Dokument-History
uebernommen; Esc verwirft die Vorschau. Formulareingaben und Artboardgesten
bleiben direkte Werteingaben ohne Einrasten.

Der zusaetzliche Typ `VECTOR_GRID_ISOMETRIC` zeichnet Rauten. Breite und Hoehe
beschreiben die Diagonalen einer Raute und bestimmen die Schraegstellung
(z.B. 32 x 16 fuer ein 2:1-Raster). Die UI-Werte `standard`, `hexagonal` und
`isometric` sind stabil; Standard wird in der UI jetzt als Orthogonal bezeichnet.
Magnetstatus und Toleranz werden mit den anderen Rastereinstellungen pro
Dokument beim Editorwechsel wiederhergestellt.


## Mehrfachauswahl von Rechtecken

Im Auswahlwerkzeug ersetzt ein Klick auf ein unmarkiertes Rechteck die Auswahl.
Shift-Klick fuegt ein Rechteck hinzu oder entfernt es; das funktioniert auch im
Dokument-Tree. Ein Klick auf ein bereits markiertes Rechteck behaelt die Gruppe,
damit man sie durch Ziehen gemeinsam verschieben kann. Ein Klick auf freie
Flaeche hebt die Rechteckauswahl auf. Artboards bleiben einzeln auswaehlbar.

Ziehen auf freier Zeichen- oder Artboardflaeche spannt einen Auswahlrahmen auf.
Ab drei Bildschirm-Pixeln Bewegung werden die sichtbaren Rechteckflaechen auf
Ueberschneidung getestet, auch bei rueckwaerts gezogenem Rahmen. Mit Shift
bleibt die bisherige Auswahl erhalten. Ausgeblendete oder gesperrte Objekte und
deren Kinder werden uebersprungen. Esc oder Fokusverlust verwirft den Rahmen
und stellt die Auswahl vor der Geste wieder her. Die Auswahl kann mehrere
Artboards umfassen.

`document/selection.c` verwaltet stabile IDs in `VectorDocument.selection`.
`active_rectangle` bezeichnet weiterhin das primaere Objekt, etwa fuer die
Navigation im Tree. `workspace/marquee.c` verarbeitet Rahmen und Overlay;
`workspace/selection.c` verarbeitet Klicks und Bewegungen. Die transiente
Geste merkt sich ihre Teilnehmer. Beim Rasterfangen bestimmt das angeklickte
Rechteck einen gemeinsamen Versatz; Abstaende und Groessen bleiben erhalten.

`rectangle/transform.c` fuehrt eine gemeinsame Verschiebung als **einen**
History-Befehl aus. Er prueft alle IDs, Sperren und Zielkoordinaten vor der
Mutation. Fehlt ein Teilnehmer oder ist ein Ziel ungueltig, bleibt die ganze
Gruppe unveraendert. Undo/Redo wendet die gespeicherten Vorher-/Nachher-Rechtecke
atomar an. Checkpoint-Typ 5 speichert diese Werte; Schema `PVCKPT10` speichert
zusaetzlich die Auswahl-IDs. Ein Editorwechsel verwirft Gestenvorschauen,
behaelt aber Auswahl und abgeschlossene Gruppenbewegungen.

Der Inspector laedt bei mehreren Rechtecken `ui/multi_selection.yaml` mit
Auswahlanzahl und Bedienhinweisen. Gemeinsames Skalieren und gemeinsame
Eigenschaftseingaben sind noch nicht enthalten; Groessen und Eigenschaften
werden weiterhin an einzeln ausgewaehlten Rechtecken bearbeitet. Markierungen
erscheinen sowohl auf der Zeichenflaeche als auch im Dokument-Tree.

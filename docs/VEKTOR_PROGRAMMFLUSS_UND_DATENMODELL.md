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
denselben oben beschriebenen Weg. Das automatisch erzeugte Anfangsdokument bekommt
über `vector_document_init_default()` ein ausgewähltes Artboard bei (0, 0) mit
1920 × 1080 Pixeln. Dessen Erstellung läuft über `vector_artboard_add()` und ist damit
rückgängig machbar und im Checkpoint enthalten. Explizit angelegte weitere Dokumente
bleiben zunächst leer. Ein bereits vorhandenes Dokument wird nicht erneut befüllt.

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
[ui/toolbar/toolbar.c](../editors/vector/ui/toolbar/toolbar.c) hält die Aktiv-Markierung des
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
`editors/vector/ui/inspector/inspector.c` setzt dort bei vorhandener Auswahl die Beschreibung
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
Rechteckdaten und keine History-Einträge angelegt. Nach Erfolg bleibt das
Rechteck-Werkzeug aktiv, sodass weitere Rechtecke direkt nacheinander angelegt
werden koennen. Jedes Rechteck bekommt einen eigenen Undo-Schritt. Erst erneuter
Button-Klick, Escape, Fokusverlust oder Werkzeugwechsel beendet das Werkzeug.

`rectangle/rectangle.h` beschreibt `VectorRectangle` mit stabiler ID, `parent_id`, Name,
eigenen Objektflags und einem lokalen Rechteck. Positionen beziehen sich auf die linke
obere Artboard-Ecke. `workspace/box_tool.c` verarbeitet die Geste, `rendering/objects.c`
wendet den Artboard-Transform an und beschneidet die Ausgabe auf das Artboard.
Beim Verschieben des Artboards folgen die Kinder einschließlich der Live-Vorschau.
Eine Artboard-Größenänderung verändert den Ausschnitt, skaliert jedoch keine Kinder.

Der Dokument-Tree führt Kinder direkt nach ihrem Eltern-Artboard mit Tiefe 1 auf.
Artboards und Rechtecke haben eigene ID-Räume; ihre Tree-IDs unterscheiden die Typen.
Ein ausgewähltes Rechteck öffnet die zugehörigen Tree-Vorfahren. Auswahl ist im
Auswahlwerkzeug per Zeichenfläche oder Tree möglich. Der Inspector lädt dann
`rectangle_inspector.yaml` in den gemeinsamen Slot `selection_properties`.
Name, Breite, Höhe, Sichtbarkeit und Sperre sind über eigene bindings in
`ui/inspector/object/object.c` bearbeitbar. Im Tree beziehen sich Auge und Schloss auf
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
in `ui/inspector/object/object.c` in Vorschau bzw. Modelländerung übersetzt.

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
`ui/tools/grid.c` bindet Rastertyp, Breite, Hoehe und Farbe aus den YAML-Widgets an
`VectorDocument.grid`. Das Raster startet ausgeblendet mit 16 x 16 px und
halbtransparentem Grau. Breite und Hoehe akzeptieren 1 bis 10000 Dokumentpixel;
ungueltige Eingaben werden auf den vorherigen Wert zurueckgesetzt.

`grid/state.h` enthaelt ausschliesslich Werte. Die Einstellungen sind
Ansichtshilfen und erzeugen keine Schritte in der Artwork-History. Sie gelten
je Dokument und werden beim Editorwechsel im temporaeren Checkpoint gesichert
(Schema `PVCKPT29`). Es gibt damit noch keine dauerhafte Dokumentdatei.

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
Beim Verschieben von Rechtecken bleiben die Mausversaetze bis zur Rasterpruefung
ungerundet. Andernfalls koennen gebrochene Objektpositionen bei hohem Zoom einen
Fangpunkt innerhalb der Bildschirm-Toleranz verfehlen. Das gilt auch fuer gedrehte
Rechtecke und den gemeinsamen Versatz einer Mehrfachauswahl.

`grid/snap.c` enthaelt wiederverwendbare Geometrie ohne Dokument-, SDL-Eingabe-
oder UI-Abhaengigkeiten. `vector_grid_snap_point` dient dem Setzen neuer Punkte;
`vector_grid_snap_rect` richtet verschobene oder skalierte Rechtecke aus.
Das orthogonale Raster faengt beide Achsen unabhaengig; beim Verschieben wird
die jeweils naechste Rechteckkante verwendet. Isometrische und hexagonale Raster
fangen Rasterknoten. Verschieben behaelt die Groesse, Skalieren die gegenueberliegende
Kante und die Mindestgroesse von einem Pixel bei. Seitengreifer veraendern weiterhin
nur ihre eigene Achse. An Artboardgrenzen hat die Begrenzung der Erstellung Vorrang.

`workspace/box_tool.c` nutzt die Punktfunktion fuer Anfang und Ende neuer Rechtecke.
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
atomar an. Checkpoint-Typ 5 speichert diese Werte; Schema `PVCKPT29` speichert
zusaetzlich die Auswahl-IDs. Ein Editorwechsel verwirft Gestenvorschauen,
behaelt aber Auswahl und abgeschlossene Gruppenbewegungen.

Der Inspector laedt bei mehreren Rechtecken `ui/inspector/selection/selection.yaml` mit
Auswahlanzahl und Bedienhinweisen. Gemeinsame Eigenschaftseingaben sind noch
nicht enthalten; Formulare bearbeiten weiterhin einzeln ausgewaehlte Rechtecke. Markierungen
erscheinen sowohl auf der Zeichenflaeche als auch im Dokument-Tree.


### Greifer und gemeinsames Skalieren

Auch die Mehrfachauswahl besitzt acht weisse Greifer am gemeinsamen Rahmen.
Ecken skalieren beide Achsen, Kanten nur eine. Die gegenueberliegende Kante
bleibt fest; ein Ueberziehen spiegelt die Gruppe nicht. Die Abstaende und
Groessen innerhalb der Gruppe werden relativ zum gemeinsamen Rahmen skaliert.
Das gilt auch fuer Rechtecke in unterschiedlichen Artboards: Die Rechnung erfolgt
in Weltkoordinaten, das Ergebnis wird wieder relativ zum jeweiligen Eltern-Artboard
gespeichert. Elternzuordnungen bleiben erhalten.

`workspace/group_transform.c` erfasst beim Start Originalrechtecke und Elternpositionen
in der transienten Geste. Jede Vorschau entsteht neu aus diesem Snapshot, ohne
kumulative Rundungsfehler. `geometry/scale.c` beschreibt die unabhaengige Abbildung
eines Rechtecks zwischen Ausgangs- und Zielrahmen. Der Rasterfang wirkt auf den
Gruppenrahmen relativ zum Artboard des primaeren Rechtecks. Mindest- und
Maximalgroessen begrenzen die gemeinsame Skalierung so, dass kein Mitglied
kleiner als ein Pixel wird. Ungueltige Zielkoordinaten werden fuer die ganze
Vorschau verworfen; Groessenbeschraenkungen haben Vorrang vor dem Rasterfang.

Beim Loslassen uebergibt `vector_group_commit` alle Zielrechtecke an
`vector_rectangles_set_rects`. `rectangle/transform.c` verwendet fuer Verschieben
und Skalieren denselben atomaren Vorher-/Nachher-Befehl (Checkpoint-Typ 5).
Damit entstehen fuer eine gemeinsame Skalierung genau ein Undo-Schritt und
vollstaendig wiederherstellbare Redo-Daten. Esc, Fokusverlust und Editorwechsel
verwerfen die laufende Groessenvorschau. Abgeschlossene Skalierungen bleiben
beim Editorwechsel inklusive History erhalten (Schema `PVCKPT29`).

### Shift und Alt waehrend Zeichnen und Skalieren

- **Shift** erzeugt beim Aufziehen von Rechtecken oder Artboards ein Quadrat.
  Beim Skalieren erhaelt es das urspruengliche Seitenverhaeltnis des Objekts
  beziehungsweise des gemeinsamen Auswahlrahmens.
- **Alt** verwendet den Startpunkt beim Zeichnen als Mittelpunkt. Beim Skalieren
  bleibt der bisherige Mittelpunkt fest und die gegenueberliegende Kante bewegt
  sich entsprechend mit.
- **Shift + Alt** kombiniert beide Vorgaben. Die Tasten koennen waehrend einer
  Geste gedrueckt und losgelassen werden; die Vorschau reagiert auch ohne weitere
  Mausbewegung. Shift am bereits sichtbaren Greifer startet eine Skalierung,
  waehrend Shift auf der Objektflaeche weiterhin die Auswahl erweitert.

Bei Eckgreifern bestimmt fuer feste Proportionen die Achse mit der groesseren
relativen Groessenaenderung den Skalierungsfaktor. Bei Kantengreifern bestimmt die
gezogene Achse die Groesse; die andere Achse waechst symmetrisch um ihre Mitte.
Eine Geste spiegelt Objekte weiterhin nicht durch Ueberziehen der Gegenseite.
Rechtecke werden beim Anlegen innerhalb ihres Artboards gehalten, auch bei
mittigem Zeichnen. Bestehende Objekte duerfen beim Skalieren wie bisher ueber
den Artboardrand hinausreichen und werden dort nur in der Darstellung beschnitten.

`geometry/constraints.c` berechnet die Form ohne SDL-Eingaben oder Objektwissen.
`workspace/modifiers.c` uebersetzt native Maus-/Shift-/Alt-Ereignisse in fluechtige
Flags der Geste. Die Werkzeuge halten rohe Endpunkte beziehungsweise Versatze,
sodass ein Tastenwechsel immer aus dem urspruenglichen Zustand neu berechnet
wird. Artboards, einzelne Rechtecke und Gruppen nutzen diese gemeinsame Geometrie.
`vector_grid_resize_constrained` schlaegt eine Rasterposition vor und wendet dann
die Form- und Groessenbedingungen an; diese haben Vorrang vor dem Rasterfang.

Die Flags werden weder in die Dokument-History noch in Checkpoints aufgenommen.
Erst beim Loslassen wird die resultierende Geometrie als ein bestehender
Einzel- oder Gruppenbefehl gespeichert. Abbrechen und Undo/Redo funktionieren
unabhaengig davon, welche Tasten beim Zeichnen oder Skalieren gehalten wurden.


### Rotationsring in der Mitte der Auswahl

Im Auswahlwerkzeug erscheint fuer einzelne Rechtecke und Mehrfachauswahlen ein
runder Greifer am Mittelpunkt. Den **Ringrand** mit der linken Maustaste greifen
und um die Mitte ziehen; das innere Loch bleibt zum Verschieben verfuegbar.
Der Ring hat bei jeder Zoomstufe dieselbe Bildschirmgroesse. Artboards werden
weiterhin nicht gedreht. Shift rastet die Drehung relativ zum Gestenstart in
15-Grad-Schritten ein, auch beim Druecken/Loslassen ohne weitere Mausbewegung.
Der Mittelpunkt bleibt waehrend der Rotation fest, Alt aendert ihn nicht.

`VectorRectangle.angle` speichert einen im Uhrzeigersinn laufenden Winkel in
Grad, mit vorzeichenbehafteter Darstellung von -360 bis 360. `rect` beschreibt weiterhin Groesse und
Lage vor der Drehung; gedreht wird um dessen Mittelpunkt. Die Winkel sind
Teil von History und temporaeren Checkpoints (Schema `PVCKPT29`).

`lib/pengdraw/src/geometry/rotation.c` berechnet Punkte, Ecken, umschliessende Rahmen und exakte
Trefferflaechen ohne SDL oder Objektwissen. `workspace/rotation.c` besitzt die
transiente Geste: urspruengliche Positionen/Winkel/Elternpositionen, gemeinsamen
Pivot und momentanen Drehwinkel. Bei einer Gruppe drehen sich die Rechteckmitten
um den gemeinsamen Pivot und die individuellen Winkel werden entsprechend
weitergedreht. Groessen und Eltern-IDs bleiben erhalten. `rectangle/transform.c`
uebernimmt Geometrien und Winkel beim Loslassen als einen atomaren Befehl.
Esc, Fokusverlust, Werkzeug- und Editorwechsel verwerfen die Vorschau.

`rendering/objects.c` zeichnet gedrehte Rechtecke als zwei Dreiecke mit
transparenter Fuellfarbe und behaelt das Clipping am Artboard bei. Auswahltests
und Auswahlrahmen pruefen die gedrehte Form, nicht deren alte Flaeche.
Einzelne Groessengreifer drehen mit dem Rechteck mit; `workspace/oriented_resize.c`
uebersetzt Mausbewegungen in dessen lokale Achsen und erhaelt die gegenueberliegende
Kante beziehungsweise den Mittelpunkt bei Alt. Rasterfang nutzt die tatsaechlichen
gedrehten Ecken oder den bewegten Greifer.

Gruppen werden weiterhin ueber ihren achsenparallelen gemeinsamen Rahmen
skaliert. Sobald eine Gruppe gedrehte Mitglieder enthaelt, wird dabei immer
proportional skaliert: Eine freie nichtproportionale Skalierung wuerde sonst
Scherungen erzeugen, die das Rechteckmodell mit Winkel und zwei Groessen nicht
abbilden kann. Individuell gedrehte Rechtecke lassen sich an ihren eigenen
Achsen weiterhin frei skalieren.


### Winkelanzeige in Grad und Inspector

Die Anzeige am Rotationsring verwendet `Grad`. Waehrend einer Geste werden die
kleinen Winkelaenderungen fortlaufend aufsummiert: Eine Drehung im Uhrzeigersinn
kann damit 180, 270 und 360 Grad erreichen, ohne bei 180 auf negative Werte
umzuspringen. Gegen den Uhrzeigersinn werden negative Werte angezeigt.
Nach weiteren vollen Umdrehungen wird wieder innerhalb einer Umdrehung angezeigt.
Vollstaendige positive/negative Umdrehungen koennen als 360 beziehungsweise -360
angezeigt und gespeichert werden. Die geometrische Berechnung behandelt sie wie 0.

Im Rechteck-Inspector beschreibt `rectangle_angle` den Winkel mit dem Zusatz
`Grad`. Er zeigt waehrend der Rotation den momentanen Objektwinkel, waehrend die
Anzeige neben dem Greifer die Aenderung seit Gestenbeginn zeigt. Ohne laufende
Geste kann der Winkel direkt eingegeben werden. Enter, Tab oder Fokuswechsel
uebernimmt den Wert als einen Undo-Schritt; ungueltige Eingaben werden auf den
bisherigen Wert zurueckgesetzt. Objekte und gesperrte Eltern behalten ihre Sperre.

`vector_rectangle_set_angle` validiert endliche Werte und erhaelt das Vorzeichen
beim Begrenzen auf eine Umdrehung, etwa 450 auf 90 oder -450 auf -90.
`lib/pengdraw/src/geometry/rotation.c` trennt diese Darstellung (`vector_angle_turn`) von der
kurzen Winkeldifferenz (`vector_angle_normalize`) fuer die Gestenberechnung.
Die Checkpoint-Validierung fuer Einzel- und Gruppenbefehle akzeptiert die
vorzeichenbehafteten Winkel einschliesslich voller Umdrehungen (Schema `PVCKPT29`).


## Einfarbige Rechteckrahmen

`VectorRectangle.stroke` speichert `VectorStroke` mit Farbe (`0xRRGGBBAA`),
Staerke in Dokument-Pixeln und Ausrichtung. Neue Rechtecke haben einen deaktivierten,
dunkelgrauen Rahmen mit Staerke `0` und zentrierter Ausrichtung.

Der Inspector beschreibt die Eingabe `rectangle_stroke_width`, die exklusive Button-Gruppe
mit `rectangle_stroke_inside`, `rectangle_stroke_center`, `rectangle_stroke_outside`
und den Farbdialog `rectangle_stroke_color` in YAML. Die UI-Anbindung uebersetzt deren
Aktionen in `vector_rectangle_set_stroke()`. Ungueltige Zahlen, negative Staerken und
geerbte Sperren verhindern die Aenderung; das Formular zeigt danach den Modellwert.
Die Farbauswahl wird beim Abschluss der Eingabe uebernommen.

- **Innen:** Der Rahmen liegt vollstaendig innerhalb der geometrischen Kante.
- **Mittig:** Je eine halbe Rahmenstaerke liegt innen und aussen.
- **Aussen:** Der Rahmen liegt vollstaendig ausserhalb der geometrischen Kante.

Breite, Hoehe, Auswahlgreifer und die geometrischen Auswahltests bleiben auf das
urspruengliche Rechteck bezogen. Skalieren aendert die Rahmenstaerke nicht.
`rectangle/stroke_render.c` zeichnet einen Dreiecksring mit Transparenz, Zoom und
Drehung. Die Dreiecke ueberlappen sich nicht, damit Alpha nicht mehrfach angewendet
wird. Dicke Innenrahmen duerfen die gesamte Fuellung bedecken. Das Clipping am
Artboard gilt auch fuer aussen liegende Rahmen. Ein deaktivierter Rahmen erzeugt
keine zusaetzliche Kontur; Auswahlmarkierungen bleiben davon unabhaengig sichtbar.

Rahmenaenderungen verwenden die vorhandenen vollstaendigen Vorher-/Nachher-Snapshots
von Rechteckbefehlen (Codec 4). Undo/Redo und temporaere Editor-Checkpoints erhalten
alle Rahmenwerte; die private Checkpoint-Signatur lautet jetzt `PVCKPT29`.


## Objektbefehle und dokumentuebergreifende Zwischenablage

Ein Rechtsklick auf der Zeichenflaeche oder im Dokument-Tree oeffnet das radiale
Kontextmenue. Ein Linksklick auf einen Sektor fuehrt dessen Aktion aus oder
oeffnet ein Untermenue. `Esc` kehrt eine Ebene zurueck (erste Ebene: schliessen);
ein Klick ausserhalb oder der Verlust des Fensterfokus schliessen es ganz.
Nicht verfuegbare Aktionen sind deaktiviert. Ein Rechtsklick auf ein bereits
markiertes Rechteck behaelt eine Mehrfachauswahl bei. Im Tree wird die Zeile
unter dem Zeiger ausgewaehlt, ohne das Artboard einzuklappen.

| Aktion | Tastatur | Verhalten |
| --- | --- | --- |
| Kopieren | Strg+C | Auswahl als unabhaengige Werte kopieren; kein History-Eintrag. |
| Ausschneiden | Strg+X | Erst erfolgreich loeschen, dann die Zwischenablage ersetzen. |
| Einfuegen | Strg+V | Neue Objekt-IDs vergeben und die neuen Objekte auswaehlen. |
| Duplizieren | Strg+D | Kopien in den bisherigen Eltern anlegen; Zwischenablage behalten. |
| Loeschen | Entf oder Ruecktaste | Auswahl samt eventuellen Nachkommen entfernen. |

Auf macOS kann statt Strg die Command-Taste verwendet werden. pengUI verarbeitet
Textfeld-Eingaben vor den Objektkuerzeln: Kopieren, Ausschneiden, Einfuegen und
Loeschen in einem fokussierten Textfeld betreffen weiterhin dessen Text.

`object/clipboard.h` beschreibt `VectorClipboard`: eigene Arrays von Artboard-
und Rechteckwerten, ohne Zeiger in das Quelldokument. Bei einer Rechteckauswahl
sind die Eltern als Koordinatenreferenz enthalten; beim Kopieren eines Artboards
werden alle Kinder eingeschlossen. Farbe, Rahmen, Drehung, Name und Objektflags
werden mitkopiert. Es handelt sich um eine interne Objektzwischenablage;
Textfelder verwenden davon unabhaengig die Betriebssystem-Zwischenablage.

`ui/context_menu/object_actions.c` uebersetzt Menueaktionen und native Tastaturereignisse in
`vector_objects_*()`-Operationen. `editor.c` leitet nur weiter. Die Zwischenablage
gehoert zur Editorinstanz, nicht zu einem Dokument-Tab. Sie bleibt beim Wechsel
zwischen Dokumenten erhalten und wird auch im temporaeren Editor-Checkpoint
mitgesichert. Das Menue selbst ist fluechtiger UI-Zustand und wird nicht gespeichert.

Beim Einfuegen von Rechtecken wird das aktive sichtbare, entsperrte Artboard als
Ziel genutzt. Ohne Zielartboard werden die kopierten Quell-Artboards als Eltern
angelegt. Ein kopiertes komplettes Artboard erzeugt immer ein neues Artboard.
Relative Abstaende einer Auswahl bleiben erhalten. Kopien bekommen einen Versatz
von 16 Dokument-Pixeln. Beim Duplizieren bleiben Rechtecke in ihren jeweiligen
Eltern; beim Kopieren zwischen unterschiedlichen Eltern wird deren raeumlicher
Versatz beim Einfuegen in ein gemeinsames Ziel beruecksichtigt.

Loeschen, Ausschneiden, Duplizieren und Einfuegen sind jeweils **ein** atomarer
History-Befehl fuer die gesamte Auswahl. `ObjectEdit` besitzt Vorher-/Nachher-Werte
von Objekten, Reihenfolge, Eltern-IDs und Auswahl. Er speichert keine History,
Viewport- oder Editorzeiger. Undo stellt auch die Auswahl wieder her. Neue IDs
werden nach Undo nicht wiederverwendet. Gesperrte Kinder schuetzen auch ihr
Artboard vor dem Loeschen. Fehler durch Kapazitaet, Sperren oder ungueltige Werte
lassen Dokument und Zwischenablage unveraendert.

`persistence/commands.c` registriert den strukturellen Befehl als Codec 6.
Checkpoint-Schema `PVCKPT29` speichert ihn und die Objektzwischenablage. Beim Laden
werden Objektwerte, Eltern, Auswahl und der gesamte History-Verlauf validiert.


## Ebenenreihenfolge innerhalb eines Elternknotens

Im radialen Kontextmenue fuehrt **Ebenen** zu vier Aktionen:

- **Ganz nach oben:** Auswahl vor alle uebrigen Geschwister legen.
- **Eins hoch:** Auswahl um ein nicht ausgewaehltes Geschwister nach vorne bewegen.
- **Ganz nach unten:** Auswahl hinter alle uebrigen Geschwister legen.
- **Eins runter:** Auswahl um ein nicht ausgewaehltes Geschwister nach hinten bewegen.

Oben im Dokument-Tree entspricht vorne in der Zeichenflaeche. Das Modell behaelt
seine bisherige Zeichenreihenfolge von hinten nach vorne; `ui/document_tree/document_tree.c`
zeigt Artboards und Kinder deshalb jeweils in umgekehrter Reihenfolge an.
Die Hit-Tests waehlen weiterhin das vorderste sichtbare Objekt.

`object/order.c` berechnet stabile Permutationen der Objekt-IDs. Bei einer Auswahl
mehrerer Rechtecke erfolgt die Rechnung fuer jeden Elternknoten getrennt. Kinder
wechseln weder ihr Artboard noch dessen lokale Koordinaten. Ihre relative Reihenfolge
bleibt erhalten, auch bei nicht zusammenhaengender Auswahl. Nicht ausgewaehlte
Geschwister behalten ebenfalls ihre relative Reihenfolge. Ein Schritt bewegt jeden
markierten Block ueber genau ein unmarkiertes Geschwister. Ein ausgewaehltes
Artboard wird zwischen den anderen Artboards auf der Wurzelebene verschoben;
seine Kinder bleiben unveraendert.

Gesperrte Auswahlobjekte oder Eltern verhindern die Aenderung fuer die ganze Auswahl.
Bereits erreichte Grenzen erzeugen keinen History-Eintrag und sind im Untermenue
deaktiviert. Jede wirksame Aktion ist ein atomarer Undo/Redo-Befehl. Er speichert
nur die ID-Reihenfolge vor und nach der Aktion, keine Kopien von Farben, Geometrie
oder UI-Zustand. Beim Anwenden werden Reihenfolge, IDs und Elternzuordnung geprueft,
bevor die Objektarrays umgeordnet werden. Auswahl, Namen, Flags und Geometrie bleiben
erhalten. Die Objektzwischenablage wird nicht veraendert.

Der private Befehl hat Codec 7 und wird im Checkpoint `PVCKPT29` gespeichert.
Beim Editorwechsel bleiben Reihenfolge und vollstaendige Undo/Redo-History erhalten.
Das Untermenue selbst wird ueber verschachtelte `radial_menu`-Eintraege in
`editors/vector/ui/editor.yaml` beschrieben; die UI-Anbindung bleibt in
`ui/context_menu/object_actions.c`.


## Ausrichten und gleichmaessig verteilen

Unter **Transformieren → Ausrichten** stehen acht Aktionen fuer Rechteckauswahlen:
links, horizontal zentriert, rechts, horizontal verteilen (**H. Abstand**), oben,
vertikal mittig, unten und vertikal verteilen (**V. Abstand**).
Ausrichten benoetigt mindestens zwei, Verteilen mindestens drei sichtbare,
entsperrte Rechtecke. Erreichte Zielpositionen sind deaktiviert und erzeugen
keinen weiteren History-Eintrag. Einzelne Rechtecke werden nicht automatisch am
Artboard ausgerichtet.

`object/alignment.c` berechnet zuerst einen unveraenderlichen Plan. Fuer jedes
Rechteck werden seine gedrehten geometrischen Aussenkanten in Dokumentkoordinaten
bestimmt; die Rahmenstaerke zaehlt nicht zu diesen Grenzen. Links/rechts/oben/unten
beziehen sich auf die entsprechenden aeussersten Grenzen der gesamten Auswahl.
Zentrieren und Mittig gleichen die Mittelpunkte auf der jeweiligen Achse an.

Beim Verteilen werden die Objekte raeumlich entlang der Achse sortiert. Gleiche
Startpositionen behalten ihre bisherige Reihenfolge. Der gemeinsame Abstand ist
`(Gesamtspanne - Summe der Objektgroessen) / (Anzahl - 1)`: unterschiedliche Breiten
oder Hoehen werden beruecksichtigt. Die aeusseren Auswahlgrenzen bleiben bestehen.
Bei zu wenig Platz kann der gemeinsame Abstand negativ sein, was eine gleichmaessige
Ueberlappung beschreibt. Es wird kein frei waehlbarer Abstandswert vorgegeben.

Alle Operationen verschieben nur die lokale Position der Rechtecke. Breite, Hoehe,
Drehung, Farben, Rahmen, Ebenenreihenfolge, Eltern und Auswahl bleiben erhalten.
Auch eine Auswahl aus unterschiedlichen Artboards nutzt gemeinsame Dokumentkoordinaten;
die Ergebnisse werden wieder als Positionen im jeweiligen Eltern-Artboard gespeichert.
Dabei duerfen Objekte wie bei bisherigen numerischen Positionsaenderungen ausserhalb
ihres Artboards liegen; das Artboard-Clipping bleibt bestehen. Rasterfang greift hier
nicht ein, damit die berechneten Kanten und Abstaende exakt zusammenpassen.

Die vorhandene Schnittstelle `vector_rectangles_set_rects()` uebernimmt den fertigen
Plan atomar als einen Transform-Befehl (Codec 5). Schlaegt eine Validierung oder eine
Sperrpruefung fehl, wird kein Objekt bewegt. Undo/Redo und Editor-Checkpoints brauchen
keinen neuen Befehlstyp oder ein neues Schema. Die acht Menueeintraege stehen in
`ui/editor.yaml`; `ui/context_menu/object_actions.c` ordnet deren Aktionen und Verfuegbarkeit
ueber eine Tabelle den fachlichen Ausrichtungsoperationen zu.


## Pfadgrundlage und boolesche Flaechenoperationen

Unter **Flaechen** im radialen Menue stehen **In Pfad**, **Kontur zu Pfad**,
**Vereinigen**, **Subtrahieren**, **Schnittmenge**, **XOR** und **Aufteilen** bereit.
Konvertieren benoetigt mindestens eine Form; die fuenf booleschen Operationen
benoetigen mindestens zwei sichtbare, entsperrte Objekte im **selben Artboard**.
Bereits konvertierte Pfade koennen gemeinsam mit Rechtecken weiterverarbeitet werden.

### Fachliche Daten und Abhaengigkeiten

`object/shape.h` beschreibt `VectorShape`, den gemeinsamen Wertetyp fuer Geometrie,
Transformation, Eltern-ID, Name, Fuellung und Rahmen. Die vorhandene Sammlung
`document.rectangles` und ihre Properties-/Transform-APIs behalten vorerst ihre Namen;
`VectorRectangle` ist dazu ein dokumentierter Kompatibilitaetsalias auf `VectorShape`.
Pfadalgorithmen sind deshalb keine Erweiterungen des Rechteck-Zeichenwerkzeugs:

- `lib/pengdraw/include/pengdraw/pengdraw.h`, `lib/pengdraw/src/geometry/path.c`: geschlossene Konturen, Bounds, Winding, Komponenten und Hit-Tests.
- `lib/pengdraw/src/geometry/tessellation.c`: reine C-Anbindung von libtess2 fuer Flaechen, Dreiecke und Konturbaender.
- `object/shape.c`: normierte Pfadpunkte in Elternkoordinaten abbilden und drehen.
- `path/operations.c`: Konvertierung, boolesche Plaene und atomare Ersetzung der Auswahl.
- `path/render.c`: SDL-Darstellung der gefuellten Flaechen, Loecher und Rahmen.
- `ui/context_menu/path_actions.c`: YAML-Aktionen und Verfuegbarkeit an die Fachfunktionen binden.

libtess2 ist als C-Quellcode mit fester Revision und SGI Free Software License B unter
`third_party/libtess2` enthalten. Der Build benoetigt dafuer keinen Download und keinen
C++-Compiler. Nur `pengui_editor_support` verwendet diese Bibliothek; pengUI und der
SDL-freie Core bekommen keine Pfad- oder Tessellierungsabhaengigkeit.

### Pfadgeometrie und Konvertierung

`VectorPath` speichert bis zu **128 Punkte** und **32 geschlossene Konturen** als Werte.
Konturenden sind exklusive Indizes. Aeussere Konturen haben positive Umlaufrichtung,
Loecher negative; die Fuellung folgt der Nonzero-Winding-Regel. Mehrere getrennte
Flaechen duerfen zu einem Pfadobjekt gehoeren.

Ein parametrisches Rechteck hat ein leeres `shape.path`. **In Pfad** ersetzt dies durch
eine geschlossene Kontur mit vier Punkten; Objekt-ID, Name, Ebenenposition, Auswahl,
primaeres Objekt, Winkel, Fuellung und Rahmen bleiben erhalten. Die gespeicherten
Punkte sind auf das Transformationsrechteck normiert. Breite/Hoehe aendern, Skalieren,
Verschieben und Drehen funktionieren deshalb weiterhin mit denselben Werkzeugen.
Sehr duenne boolesche Ergebnisse verwenden wegen der bestehenden Transform-Grenze
ein mindestens 1 px grosses Bezugsrechteck; die eigentliche Flaeche wird nicht vergroessert.

Diese erste Grundlage beschreibt **Polygonpfade mit geraden Segmenten**. Ein
Knotenwerkzeug, Bezier-Kurven und Ellipsen sind noch nicht implementiert. Ein spaeteres
Kurvenmodell kann hier ergaenzt und fuer Flaechenberechnungen kontrolliert abgeflacht werden.
Es gibt durch diese Aenderung noch keinen SVG-Dateiimport oder -export.

### Fuellung, Kontur und Loecher

Fuellfarbe und `VectorStroke` bleiben getrennt von der Pfadgeometrie. Der Renderer
trianguliert nur die gefuellten Bereiche; Loecher bleiben frei, auch bei Transparenz.
Rahmen werden als zusammenhaengende Flaechenbaender mit Miter-Ecken (an sehr spitzen
Ecken begrenzt/bevel) berechnet. Zentrierte Rahmen liegen beidseitig der Kontur;
innen- und aussenliegende Rahmen werden an der gefuellten Flaeche geclippt. Das gilt
auch an den Raendern von Loechern. Dreiecke ueberlappen sich nicht, sodass Alpha nicht
mehrfach angewendet wird. Breite ist weiterhin in Dokument-Pixeln; Zoom veraendert
nur die Darstellung. Artboard-Clipping gilt fuer Fuellung und Kontur.

**Kontur zu Pfad** macht den Rahmen selbst zu einer gefuellten Flaeche mit der bisherigen
Rahmenfarbe. Die alte Fuellung entfaellt dabei, die Rahmenstaerke des neuen Pfads ist 0.
Diese Operation ist getrennt von **In Pfad**, das das gesamte Aussehen erhaelt.
Hit-Tests und Auswahlrahmen pruefen bei Pfaden die tatsaechlich gefuellte Geometrie;
ein Klick in ein Loch kann ein dahinterliegendes Objekt auswaehlen.

### Ergebnisse, Reihenfolge und History

Boolesche Berechnungen verwenden die gedrehte Geometrie in Elternkoordinaten, nicht
Bildschirmkoordinaten oder das Raster. Die Ausrichtung des Ergebnisses wird in die
Punkte eingerechnet, sein gespeicherter Drehwinkel ist danach 0.

- **Vereinigen:** alle gefuellten Bereiche zusammenfassen.
- **Subtrahieren:** alle weiter vorne liegenden Auswahlobjekte von der hintersten
  ausgewaehlten Flaeche abziehen; die Ebenenreihenfolge bestimmt die Operanden.
- **Schnittmenge:** nur die von allen ausgewaehlten Objekten belegte Flaeche behalten.
- **XOR:** Bereiche mit ungerader Ueberdeckungszahl behalten.
- **Aufteilen:** an allen Ueberschneidungen zerlegen und jede zusammenhaengende
  Teilflaeche als eigenes Objekt erzeugen, einschliesslich eventueller Loecher.

Die Ergebnisse erben Fuellung und Rahmen des hintersten ausgewaehlten Objekts.
Originalrahmen werden nicht automatisch als Flaeche mitverrechnet; dafuer vorher
**Kontur zu Pfad** benutzen. Alle Operanden werden durch frisch benannte Pfadobjekte
mit neuen IDs an der obersten bisherigen Auswahlposition ersetzt. Unbeteiligte
Objekte behalten ihre relative Reihenfolge. Die Ergebnisse werden ausgewaehlt.
Eine leere Schnittmenge oder vollstaendige Subtraktion entfernt die Operanden.

`vector_objects_replace_selection()` verwendet den strukturellen Snapshot-Befehl
(Codec 6): Berechnung, Geometriepruefung und Kapazitaetspruefung passieren vor der
Modellaenderung. Jede Operation ist ein Undo-Schritt; Undo stellt die Originalformen
samt Eigenschaften und Auswahl wieder her. Pfadwerte werden auch beim Kopieren,
Ausschneiden, Duplizieren, tabuebergreifenden Einfuegen und Editorwechsel erhalten.
Die Checkpoint-Signatur lautet wegen des erweiterten Formwerts nun **PVCKPT29**.

Die bisherigen 64 Objekte pro Dokument bleiben als Grenze bestehen. Ueberschreitet
ein Ergebnis das Punkt-, Kontur- oder Objektlimit oder scheitert die Berechnung,
bleiben Dokument, Zwischenablage und History unveraendert. Das Menue meldet den Fehler.


## Spiegeln und Vierteldrehungen

Unter **Rechtsklick → Transformieren** stehen neben dem Ausrichten-Untermenue vier
weitere Aktionen fuer Rechtecke und Pfade zur Verfuegung:

- **H. spiegeln:** links und rechts an einer vertikalen Achse vertauschen.
- **V. spiegeln:** oben und unten an einer horizontalen Achse vertauschen.
- **Links 90 Grad:** gegen den Uhrzeigersinn drehen.
- **Rechts 90 Grad:** im Uhrzeigersinn drehen.

Als Bezug dient die Mitte des gemeinsamen Auswahlrahmens in Dokumentkoordinaten.
Bei einem Einzelobjekt ist dies dessen Transformationsmitte. Bei Mehrfachauswahl
werden auch die Positionen der Objekte um diesen gemeinsamen Bezug gespiegelt bzw.
gedreht. Ein Objekt behaelt seinen Elternknoten; die Rechnung funktioniert daher
auch mit Auswahlen aus unterschiedlichen Artboards. Ergebnisse duerfen wie bei
bisherigen Transformationen ausserhalb ihres Artboards liegen und bleiben geclippt.
Artboards selbst erhalten mit diesen Aktionen keine Drehung.

`object/orientation.c` erstellt einen vollstaendigen Plan mit bestehenden Objekt-IDs.
Vierteldrehungen verwenden exakte Achsentausche statt einer trigonometrischen
Naeherung. Beim Spiegeln wird der gespeicherte Winkel negiert; Pfadpunkte werden
zusaetzlich auf ihrer normierten lokalen Achse gespiegelt. Jede Kontur wird danach
in ihrer Reihenfolge umgekehrt: Die aeussere Fuellung bleibt positiv und Loecher
bleiben negativ orientiert. Rahmen folgen dadurch auch gespiegelten Lochkonturen
weiterhin innen, mittig oder aussen.

Breite/Hoehe der lokalen Form, Name, Fuellung, Rahmen, ID, Eltern, Ebenenreihenfolge,
Auswahl und primaeres Objekt bleiben erhalten. Die Winkelangabe im Inspector zeigt
nach Links/Rechts entsprechend -90 bzw. +90 Grad mehr; volle Umdrehungen werden nach
der bestehenden vorzeichenbehafteten Winkelkonvention behandelt. Rasterfang greift
bei diesen exakten Operationen nicht ein.

Sperren, unsichtbare Auswahlobjekte, ungueltige Zielgeometrie und Kapazitaetsgrenzen
verhindern die gesamte Aenderung. Ein Spiegeln ohne Modellwirkung ist deaktiviert,
zum Beispiel bei einem einzelnen ungedrehten parametrischen Rechteck. Jede wirksame
Aktion verwendet einen strukturellen Snapshot-Befehl (Codec 6). Undo/Redo stellt auch
die originale Pfadpunktreihenfolge wieder her; die temporaeren Editor-Checkpoints
und die unabhaengige Objektzwischenablage funktionieren unveraendert.

Die YAML-Konfiguration liegt in `ui/editor.yaml`, die Weiterleitung und Menue-
Verfuegbarkeit in `ui/context_menu/orientation_actions.c`. Der pengUI-Menue-Stack wird weiter
verwendet; fuer den neuen Werkzeugsatz ist keine Erweiterung der UI-Bibliothek noetig.


## Objektausrichtung und intelligente Hilfslinien

Der Toggle `vector_element_snap` mit Aktion `vector.element.snap` aktiviert pro Dokument
magnetische Objektausrichtung. Er ist unabhaengig vom Raster-Magneten. Die vorhandene
Einrasttoleranz gilt in Bildschirm-Pixeln (Standard 5 px) und wird durch den Zoom geteilt.

Beim Verschieben werden die linken/rechten Kanten, Ober-/Unterkanten und Mittelpunkte
von sichtbaren Objekt-Boundingboxen verglichen. Rotierte Rechtecke und Pfade verwenden
ihre rotierten achsenparallelen Grenzen. Ausgeblendete Objekte und die bewegte Auswahl
werden ausgeschlossen; gesperrte sichtbare Objekte koennen als Referenz dienen.
Eine Mehrfachauswahl rastet als gemeinsame Boundingbox ein. Artboards werden ebenfalls
unterstuetzt; ihre eigenen Kinder sind beim Verschieben keine Referenz.

Zwischen unmittelbaren Nachbarn werden zusaetzlich gleiche horizontale oder vertikale
Abstaende erkannt. Die Nachbarn muessen die senkrechte Projektion der Auswahl ueberlappen.
Gestrichelte Linien markieren Ausrichtungen, zwei gestrichelte Strecken mit Endmarken
zeigen gleich grosse Luecken an. Bei konkurrierenden Treffern gewinnt pro Achse die
kleinste Korrektur; bei Gleichstand hat Kantenausrichtung Vorrang. Objekt-Treffer haben
Vorrang vor dem Raster, waehrend ungetroffene Achsen weiter am Raster einrasten koennen.

`geometry/element_snap.c` berechnet Korrektur und Hilfslinien ohne Editorabhaengigkeiten.
`workspace/element_snap.c` liefert die Referenzgeometrie und zeichnet die Vorschau.
Die gemeinsame Bewegungsvorschau nutzt dieselbe Berechnung beim Zeichnen und Loslassen.
Erst das Loslassen erzeugt einen History-Schritt; Escape verwirft die Vorschau.
Der Toggle wird im temporaeren Checkpoint `PVCKPT29` gespeichert, Hilfslinien und laufende
Gesten nicht. Die Funktion wirkt zunaechst beim Verschieben, nicht beim Skalieren/Zeichnen.


## Persistente Objektgruppen

Im radialen Untermenue **Ebenen** liegen `vector.objects.group` (Gruppieren) und
`vector.objects.ungroup` (Gruppe aufloesen). Der erste Befehl braucht mindestens zwei
unabhaengige Objekte oder Gruppen im selben Artboard. Gesperrte Auswahlen und
artboarduebergreifendes Gruppieren werden abgelehnt. Nicht moegliche Aktionen sind grau.

`object/group.c` verwaltet die fachlichen Befehle. Ein Formwert hat jetzt `group_id`:
0 bedeutet unabhaengig, gleiche IDs verbinden die Mitglieder eines Artboards. Die
Gruppenkennung wird aus einer bestehenden eindeutigen Form-ID abgeleitet; beim Kopieren
oder Duplizieren wird sie auf eine neue Kennung umgelegt. Gruppen brauchen keine leeren
Container oder eigenen Zeichenobjekte. Der Tree erzeugt daraus einen Knoten **Gruppe N**
mit den einzelnen Formen als Kindern. Leere Gruppen verschwinden automatisch.

Das Modell unterstuetzt aktuell **eine Gruppenebene**. Werden vorhandene Gruppen
zusammen gruppiert, entsteht eine gemeinsame Gruppe; es entstehen keine Untergruppen.
Ein Klick auf eine Form oder ihre Gruppenzeile waehlt alle Mitglieder aus. Shift und
Rahmenauswahl behandeln die Gruppe ebenfalls als Einheit. Einzelbearbeitung erfolgt
nach dem Aufloesen. Eine Gruppe wird durch die gemeinsame Auswahl verschoben, skaliert
oder gedreht; Geometrie und Stil bleiben separate Werte ihrer Mitglieder.

Beim Gruppieren werden Mitglieder als zusammenhaengender Ebenenblock an der Position
des vordersten ausgewaehlten Objekts abgelegt. Die relative Reihenfolge bleibt erhalten;
beim Aufloesen bleiben Positionen und Reihenfolge unveraendert. Beide Befehle verwenden
die atomaren Struktursnapshots aus `object/clipboard.c` (Codec 6). Undo stellt auch die
vorherige Ebenenreihenfolge und Auswahl wieder her. Die bestehende Bewegungs-History
bleibt davon unabhaengig. Checkpoint `PVCKPT29` speichert die Gruppenwerte samt History.


## Bezier-Stift und nachtraegliche Knotenbearbeitung

Die Toolbar enthaelt `vector_pen` (`vector.tool_pen`) zum Zeichnen und `vector_nodes`
(`vector.tool_nodes`) zum Bearbeiten. Beide Werkzeuge sind wie Rechteck und Auswahl
gegenseitig exklusiv und werden beim Editor-/Dokumentwechsel entladen.

- Im Stiftwerkzeug setzt ein Klick einen Eckpunkt; Ziehen erzeugt zwei symmetrische
  Kontrollgriffe. Shift begrenzt die Griffrichtung auf 45-Grad-Schritte.
- Ein Klick auf den Startpunkt schliesst die Kurve. Enter, Rechtsklick oder Doppelklick
  beendet eine offene Kurve. Der Stift bleibt danach fuer die naechste Kurve aktiv.
- Escape oder Fokusverlust verwirft den aktuellen Entwurf. Backspace entfernt den letzten
  noch nicht uebernommenen Punkt. Es wird keine halbfertige Kurve gespeichert.
- Im Knotenwerkzeug wird eine Kurve angeklickt und dann an ihren Punkten oder Griffen
  gezogen. Punkte bewegen ihre Griffe mit. Griffe behalten normalerweise die gemeinsame
  Tangente; Alt erlaubt unabhaengige Griffe. Alt-Ziehen an einem Eckpunkt erzeugt einen Griff.
- Ein Doppelklick auf eine ausgewaehlte Kurve fuegt einen Knoten durch exakte kubische
  Teilung ein, ohne ihre Form zu aendern. Entf/Backspace loescht den ausgewaehlten Knoten,
  solange eine gueltige Kurve mit mindestens zwei Knoten uebrig bleibt.

`lib/pengdraw/src/geometry/bezier.c` enthaelt reine kubische Geometrie, analytische Extremwert-Bounds und
adaptive de-Casteljau-Unterteilung. `VectorShape.curve` speichert bis zu 32 echte Anker
mit eingehenden/ausgehenden Kontrollpunkten. Diese liegen wie Polygonpunkte normalisiert
im Formrechteck; Griffe duerfen ausserhalb der Box liegen. `VectorShape.path` ist bei
bestehenden Polygonen die Geometrie, bei geschlossenen Bezierkurven eine abgeleitete
Fuellkontur. Offene Kurven haben keine Fuellflaeche und werden als zentrierte einfarbige
Kontur dargestellt. Deren Farbe/Breite nutzt die vorhandenen Rahmenfelder im Inspector.
Geschlossene Kurven unterstuetzen auch Fuellfarbe und Rahmenplatzierung.

Fuer Darstellung und geometrische Abfragen werden hoechstens 128 Polygonpunkte erzeugt.
Bei sehr komplexen Kurven wird die Naeherung grober, die gespeicherten Kontrollpunkte
bleiben unveraendert. Flaechenoperationen sind auf geschlossene Kurven beschraenkt und
erzeugen Polygonpfade. **In Pfad** backt eine geschlossene Kurve ebenfalls zu Polygonen;
die normale Auswahl, Skalierung, Drehung und Spiegelung erhalten hingegen ihre Knoten.

`curve/curve.c` verwaltet Erzeugung und Aenderung mit History-Codec 8. `curve/tool.c`
haelt Mausgesten in parentlokalen Pixeln; `curve/render.c` zeichnet Kontur, Entwurf und
Knotengriffe. Erst Abschluss bzw. Loslassen erzeugt einen History-Schritt. Ein unveraenderter
Klick auf einen Knoten erzeugt keinen Schritt. Das Knotenwerkzeug kann auch eine Kurve
innerhalb einer Gruppe bearbeiten, ohne andere Mitglieder zu ersetzen. Beim Bearbeiten
einer gedrehten Kurve wird ihre Drehung in die Kontrollpunkte eingerechnet; die sichtbare
Geometrie bleibt erhalten und der Formwinkel wird auf 0 gesetzt.

Checkpoint `PVCKPT29` sowie Clipboard und strukturelle History-Snapshots enthalten die
kubischen Werte. Werkzeuge, Entwuerfe und laufende Griffbewegungen werden nicht gespeichert.


## Ellipsen und Kreise

Der Toolbar-Toggle `vector_add_ellipse` mit Aktion `vector.add_ellipse` aktiviert das
Ellipsenwerkzeug. Wie beim Rechteck wird die Form mit der Maus aufgezogen; Shift haelt
das Seitenverhaeltnis fuer einen Kreis, Alt zeichnet vom Mittelpunkt aus. Beide Tasten
koennen kombiniert werden. Raster, Zoom, Eltern-Clipping, Escape, Fokusverlust und das
aktive Werkzeug nach dem Anlegen verhalten sich wie beim Rechteck.

Die gemeinsame Aufzieh-Geste liegt nun in `workspace/box_tool.c`, ihre reinen Zustandswerte
in `workspace/box_tool_state.h`. Bestehende `VectorRectangleTool`-/`vector_rectangle_tool_*`-
Bezeichner bleiben erhalten. Der Modus waehlt lediglich zwischen den Fabriken
`vector_rectangle_add()` und `vector_ellipse_add()`; Mausverarbeitung wird nicht dupliziert.

`ellipse/ellipse.c` erzeugt eine geschlossene Kontur aus vier kubischen Viertelbögen
(Standard-Kreisfaktor 4/3*tan(pi/8)). `ellipse/render.c` zeichnet Fuellung und Rahmen
mit einer eigenen feinen Ellipsenabtastung, damit das Polygonlimit fuer Bool-Operationen
nicht die sichtbare Rahmenqualitaet reduziert. Ellipsen nutzen die vorhandenen Fuell-, Rahmen-,
Auswahl-, Transformations-, Gruppen- und History-Funktionen. Der Inspector zeigt **Ellipse**,
der Tree ein ovales Icon. Erst Loslassen fuehrt einen einzigen Erzeugungsbefehl aus;
die Vorschau aendert keine Dokumentdaten. Die Fuellfarbe entspricht neuen Rechtecken.

Die interne Kennung `VectorShape.curve_kind == VECTOR_CURVE_ELLIPSE` bleibt beim Skalieren,
Drehen, Spiegeln, Kopieren und Editorwechsel erhalten. Wird die Ellipse mit dem
Knotenwerkzeug frei veraendert, wird sie zu einer normalen Bezierkurve. **In Pfad** und
Flaechenoperationen erzeugen wie bisher Polygonpfade. Undo stellt die Ellipsenkennung
wieder her. Der gemeinsame Kurven-Erzeugungsbefehl (Codec 8) speichert Geometrie und Stil;
das temporaere Checkpoint-Schema lautet wegen der zusaetzlichen Formkennung `PVCKPT29`.


### Parametrische Polygone

`vector.add_polygon` aktiviert die gemeinsame Aufzieh-Geste aus `workspace/box_tool.c`.
Vorschau und Dokumenterzeugung nutzen `polygon/polygon.c`. Neue Polygone haben sechs
Ecken; `VECTOR_POLYGON_MAX_SIDES` begrenzt die Eckenzahl derzeit auf 32, entsprechend
der Kapazitaet von `VectorBezier`. Die Form speichert gerade Segmente als kubische
Kurve mit identischen Anker- und Kontrollpunkten sowie `VECTOR_CURVE_POLYGON` als
Identitaet. Es ist keine weitere Heap-Ressource oder separate Datei noetig.

Der Inspector laedt `ui/inspector/polygon/polygon.yaml` nur bei einem parametrischen Polygon
in den Slot `polygon_properties`. `vector.polygon.sides` validiert die ganzzahlige
Eingabe (3 bis 32) und ruft `vector_polygon_set_sides` auf. Die Operation ersetzt nur
die Geometrie, behaelt ID, Namen, Transformation, Fuellung und Rahmen bei und erzeugt
einen History-Schritt. Undo/Redo und temporaere Editor-Checkpoints enthalten auch
die Eckenzahl. Nach „In Pfad umwandeln“ oder freier Knotenbearbeitung entfaellt die
parametrische Identitaet und damit das Eckenzahl-Feld. Undo stellt beides wieder her.


### Textfelder und Typografie

Der aktuelle Checkpoint ist **PVCKPT29**; `VectorShape` enthält Textwerte und die Maskenrolle.
Der gemeinsame Wertebefehl liegt in `object/commands.c` statt in der Kurvendomaene.
[Textfelder im Vektoreditor](VEKTOR_TEXT.md) beschreibt Werkzeug, Schriftservice,
UTF-8-Eingabe, History-Gruppierung und den Textfluss ueber mehrere Spalten.

## Verlaufsfuellungen

`VectorShape.gradient` enthaelt den Verlaufstyp, relative Endpunkte und bis zu acht
Farbstopps. Das Canvas-Werkzeug haelt die laufende Geste getrennt in
`VectorDocument.gradient_tool`; erst Mouse-Up uebergibt den neuen Formwert an
`vector_shape_update`. Inspector-Felder verwenden denselben History-Befehl.
[Bedienung, Rendering und Speicherung](VEKTOR_VERLAEUFE.md) sind separat beschrieben.

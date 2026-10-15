# Deklarative Editoroberflächen mit YAML

**Status: Entwurf für den nächsten Ausbau.** Die aktuelle Engine erstellt ihre UI in C. Sie liest diese YAML-Dateien noch nicht ein und entlädt beim Wechsel noch nicht den vollständigen Editor. Dieses Dokument beschreibt den vereinbarten Zielaufbau; die folgenden Bezeichner sind ein Schemaentwurf, keine bereits verfügbare C-API. Die tatsächliche Implementierung steht in [IMPLEMENTIERUNG.md](IMPLEMENTIERUNG.md), die vorhandenen UI-Bausteine in [UI_BIBLIOTHEK.md](UI_BIBLIOTHEK.md).

YAML soll den Aufbau beschreiben: Panels, Tabs, Widgets, Toolbars, Gestaltung und Verknüpfungen. C stellt Widgets, Daten und Aktionen bereit. So lassen sich vorhandene Elemente selbst neu zusammenstellen; ein neues Zeichenwerkzeug oder ein völlig neues Widget benötigt weiterhin eine C-Implementierung. Der Ansatz entspricht der Trennung zwischen Layoutdatei und Controller etwa bei JavaFX/FXML.

## Drei getrennte Verantwortlichkeiten

| Teil | Aufgabe | Lebensdauer |
| --- | --- | --- |
| Anwendungshülle | Fenster, Renderer, Theme, globale Toolbar, Bereichsauswahl, Statusleiste, Spaltengriffe | Solange das Programm läuft |
| Aktiver Editor | Alle drei Spalten, eigene Toolbar, Widgets, Bindings, Ressourcen und Arbeitszustand | Nur solange dieser Editor aktiv ist |
| Sitzungsdienst | Dokumentzuordnung, Sicherungsdateien, Wiederherstellung und kleine Metadaten | Editorübergreifend; große inaktive Inhalte liegen auf Datenträger |

Die Auswahl oben entscheidet über **den Inhalt aller drei Spalten**. Ein Dialogeditor kann beispielsweise links Gespräche, mittig einen Graphen und rechts Knoteneigenschaften anzeigen. Ein Grafikeditor kann dieselben Flächen für Ebenen, Zeichenfläche und Werkzeuge verwenden. Gemeinsam ist der Rahmen, nicht ein immer geladener Dateibaum oder Inspector.

Auch ein gemeinsam aussehender Explorer wird pro Editor instanziiert und wieder zerstört. Gemeinsam bleiben lediglich die Implementierung und die kleinen Dienste der Anwendungshülle.

## Vollständiges Entladen beim Wechsel

Es gibt immer höchstens eine aktive Editorinstanz. Inaktive Editoren behalten keine versteckten Widgetbäume, Parser-Instanzen, Dokumentpuffer, Undo-Snapshots, Hintergrundjobs oder exklusiv gehaltenen Texturen im RAM. Ihre Zustände werden vor dem Entladen auf Datenträger gesichert. Ein Betriebssystem muss dadurch frei gewordenen Heap-Speicher nicht sofort als niedrigeren Prozessverbrauch anzeigen; entscheidend sind freigegebene Ressourcen und keine weiterlebenden Editorinstanzen.

Die Anwendungshülle, ihre Schrifttextur und tatsächlich gemeinsam benötigte Projektdienste bleiben bestehen. Das vollständige Entladen einer Editorinstanz ist kein Entladen des SDL-Fensters oder des C-Programmcodes. Dynamische Bibliotheken/DLL-Plugins sind für diesen Schritt nicht vorgesehen.

### Wechselablauf

1. Ziel-ID und ihre kleine YAML-Beschreibung prüfen, ohne bereits dessen schweren Editor zu erzeugen.
2. Laufende Eingabe abschließen oder nach den bisherigen Regeln abbrechen. Während Play bleibt der Wechsel gesperrt; Play zuerst stoppen.
3. Aktiven Editor für neue Eingaben sperren. Jobs beenden beziehungsweise abbrechen und deren Abschluss abwarten.
4. Dokument, ungespeicherte Änderungen, Undo/Redo und Wiederaufnahmezustand in eine temporäre Sitzungsdatei schreiben. Erst nach erfolgreichem Schreiben die bisherige Sicherung atomar ersetzen.
5. Bei Sicherungsfehlern **nicht entladen**: bisherigen Editor wieder freigeben, Meldung anzeigen, Wechsel abbrechen.
6. Bei erfolgreicher Sicherung Fokus, Maus-Capture, Callbacks und Bindings abmelden. Widgets und Editorzustand zerstören; Parser, Puffer und Ressourcen freigeben.
7. Ziel-Editor erzeugen, dessen Sitzung oder Projektzustand laden, alle drei Spalten aufbauen und Eingabe freigeben.

Schlägt Schritt 7 fehl, bleibt die Anwendungshülle mit einer Fehlermeldung und Wiederholungs-/Rückkehrmöglichkeit bedienbar. Der vorherige Editor wird bei Rückkehr aus seiner Sicherung neu geladen. Die alte und neue schwere Editorinstanz existieren nicht gleichzeitig.

Ein UI-Reload verwendet denselben Ablauf. Die neue YAML-Beschreibung wird vorab validiert; bei Syntax-/Schemafehlern bleibt die bisherige UI aktiv. Nach bereits erfolgtem Entladen kann die letzte gültige Beschreibung zur erneuten Wiederherstellung genutzt werden.

### Was gesichert werden muss

| Zustand | Wiederherstellung |
| --- | --- |
| Dokument | Vollständiger aktueller Inhalt und Bezug zur Quelldatei |
| Undo/Redo | Verlauf einschließlich aktuellem Schritt und gespeicherter Revision |
| Map | Kamera, Zoom, Auswahl über stabile Entity-ID, aktive Ebene, Werkzeug, Raster/Kollision-Anzeige |
| Scripts | Datei, Cursor, Auswahl, Scrollposition und Dokumentzustand |
| UI | Tab-Reihenfolge/-Auswahl, Scrollpositionen, offene Abschnitte und Spaltenbreiten |
| Flüchtige Ressourcen | Nicht speichern: neu aus Dokument/Assets erzeugen, z. B. Texturen oder Syntax-Token |

Sitzungsdaten enthalten keine C-Zeiger, SDL-Handles oder laufenden Threads. Sie benötigen ein eigenes versioniertes Speicherformat; YAML beschreibt das Layout und ist **nicht** das Format für große Dokumente und Undo-Verläufe. Die konkrete Serialisierung wird vor der Lebenszyklus-Implementierung festgelegt.

**Sitzungssicherung ist kein Speichern des Projekts.** Ctrl+S schreibt weiterhin bewusst die Quelldatei. Ein Editorwechsel darf ungespeicherte Änderungen nicht verwerfen, den Dirty-Status nicht zurücksetzen und keine Projektdatei automatisch überschreiben. Ist eine Wiederherstellung nicht möglich, muss das sichtbar gemeldet werden; kein stiller Rückfall auf einen älteren Dateistand.

Die Speicherung liegt in einem beschreibbaren, projektbezogenen Benutzer-Sitzungsverzeichnis, nicht zwingend im Git-Repository. Große Sicherungen werden nur zum Laden oder Schreiben in den RAM geholt. Editorübergreifende Dienste dürfen keine versteckten Kopien der inaktiven Dokumente/Undo-Historien behalten. Crash-Recovery, Versionsmigrationen und Bereinigung alter Sitzungen benötigen eigene Tests; dieses Dokument verspricht dafür noch keine fertige Implementierung.

## Dateien und erster Aufbau

Die vollständigen Beispielbeschreibungen liegen unter [examples/ui/shell.yaml](examples/ui/shell.yaml), [editors/map.yaml](examples/ui/editors/map.yaml) und [editors/scripts.yaml](examples/ui/editors/scripts.yaml). Es sind kleine Startbeispiele, keine vollständige Beschreibung aller heutigen Kartenwerkzeuge. Sie sind noch nicht an den Build oder Editorstart angeschlossen.

```text
ui/
  shell.yaml
  editors/
    map.yaml
    scripts.yaml
```

`file`-Pfade in der Shell sind relativ zur Shell-Datei. Die Modulnamen `map` und `scripts` bezeichnen registrierte, eingebaute C-Editoren. Ein Modul besitzt Aufbau, Eingabebehandlung, Aktionen, Dokumentanbieter, Sitzungssicherung und Zerstörung; YAML enthält keinen Pfad zu ausführbarem Code.

Jede Datei beginnt mit `schema: penguin.ui/v1`, `kind: shell` beziehungsweise `kind: editor` und einer stabilen `id`. Die Editor-ID muss zum Eintrag in der Shell passen. Nicht aufgeführte Editoren erscheinen nicht in der Auswahl und werden nicht instanziiert.

## Aufbau der Anwendungshülle

| Schlüssel | Bedeutung |
| --- | --- |
| `schema`, `kind`, `id` | Schemaversion, Dateityp und stabile Identität |
| `layout.gap` | Außen-/Panelabstand in logischen UI-Pixeln, zunächst 3 |
| `layout.tab_gap` | Abstand zwischen Tabs, zunächst 2 |
| `layout.columns` | Anfangs- und Mindestbreiten der drei Spalten |
| `theme` | Benannte Farbrollen; Farben immer als Zeichenfolge schreiben |
| `toolbar` | Geordnete Liste globaler UI-Elemente |
| `editors` | Liste aus `id`, `title`, `icon`, `module`, `file` |
| `editor_policy` | In v1 ausschließlich `unload_on_leave`; keine Option zum Behalten inaktiver Editoren |
| `statusbar` | `id`, `lines: 1` und geordnete `fields` mit Textbindings |

Die Mindestbreite der Mitte wird zuerst geschützt. Seitenspalten bleiben verstellbar; der Container begrenzt sie anhand ihrer Mindestwerte. Globale Dokumentaktionen werden über den Dokumentanbieter des aktiven Editors aufgelöst. Ohne aktives Dokument sind die zugehörigen Buttons deaktiviert.

Ein Button ohne `action` ist ein sichtbarer Platzhalter und verändert nichts. Damit kann etwa das Hamburger-Menü zunächst nur dargestellt werden.

## Aufbau einer Editorbeschreibung

```yaml
schema: penguin.ui/v1
kind: editor
id: map
document_provider: scene

toolbar:
  - type: button
    id: brush
    icon: brush
    tooltip: "Pinsel (B)"
    action: map.select_brush

columns:
  left:
    type: file_tree
    id: files
    binding: project.files
  center:
    type: map_canvas
    id: canvas
    binding: scene.active
  right:
    type: inspector
    id: inspector
    binding: selection.active
```

`toolbar` liegt direkt über dem mittleren Inhalt. Ihre Buttons brechen bei Platzmangel um. `columns` enthält exakt `left`, `center`, `right`; jede Spalte enthält einen Widgetknoten. Für eine absichtlich leere Spalte dient `type: spacer`. Der Editor darf keine Spalte unbemerkt vom zuvor aktiven Editor übernehmen.

`document_provider` ist ein registrierter Anbieter wie `scene` oder `script`. Er bindet globale Aktionen und Statuswerte an das aktuelle Dokument. Ein späterer Editor ohne Dokument kann den Schlüssel weglassen; Speichern/Undo/Redo sind dann deaktiviert. Ein unbekannter Anbieter ist dagegen ein Validierungsfehler.

## Gemeinsame Widgeteigenschaften

| Eigenschaft | Regel im Entwurf |
| --- | --- |
| `type` | Registrierter Widgettyp, verpflichtend |
| `id` | Stabile, sprechende ID, verpflichtend |
| `label` | Optionaler sichtbarer Text, z. B. für Buttons |
| `icon` | Optionaler registrierter Iconname |
| `tooltip` | Optionaler Hilfetext |
| `enabled` | Boolean oder `{binding: ...}`, Standard `true` |
| `visible` | Boolean oder Boolean-Binding, Standard `true` |
| `active` | Boolean oder Boolean-Binding, Standard `false`; für ausgewählte Buttons |
| `binding` | Datenquelle eines Inhaltswidgets; Typ wird vom Widget geprüft |
| `action` | Aktion eines Buttons, optional; unbekannte Namen sind Fehler |
| `children` | Geordnete Widgetliste für `row`, `column` und `section` |

Nicht jedes Widget akzeptiert jede Eigenschaft: etwa `children` bei `button` wird abgelehnt. `visible: false` bedeutet keinen Platz im Layout und keine Eingabe; die Instanz gehört weiterhin zum aktiven Editor und wird mit diesem entladen. Ein deaktivierter Button erhält keinen Fokus und löst keine Aktion aus.

IDs müssen innerhalb einer Editorbeschreibung einschließlich Tabs eindeutig sein. Die Shell hat ihren eigenen Namensraum. Die vollständige Identität ist beispielsweise `map/navigation/explorer_tab/files`. Wiederholte Namen in verschiedenen Editoren sind erlaubt. Beim Ändern einer ID kann die zugehörige gespeicherte UI-Einstellung nicht mehr automatisch zugeordnet werden; Dokument und Undo dürfen davon nicht abhängen.

Bei wiederholten Datenzeilen, etwa Entities, ergänzt der Widgetrenderer eine stabile Daten-ID. Zeilennummern und Bildschirmpositionen sind keine Identitäten.

## Erste Elemente

Alle folgenden YAML-Adapter sind geplant. Ein vorhandener C-Baustein bedeutet noch nicht, dass der entsprechende YAML-Typ bereits benutzt werden kann.

| Typ | Zusätzliche Felder / Aufgabe | Grundlage |
| --- | --- | --- |
| `button` | `label`, `icon`, `tooltip`, `action`, Zustandsbindings | Vorhandene UI-Buttons |
| `separator` | Schmaler Trennstrich mit Innenabstand in Toolbar/Zeile | Vorhandene Toolbar-Darstellung |
| `editor_switcher` | Nur Shell; `labels: false` zeigt Icons mit Titel als Tooltip | Vorhandene Bereichsauswahl |
| `tabs` | `tabs`-Liste mit `id`, `title`, optional `icon`, genau einem `content`-Knoten | Vorhandenes Tabmodell |
| `section` | `title`, `open` als anfänglicher Boolean, `children` | Klappbare Bereiche; generischer Adapter folgt |
| `row`, `column` | `children`, optional nichtnegativer `gap` | Allgemeine Container noch zu ergänzen |
| `label` | `label` als statischer Text oder `{binding: ...}` | Vorhandene Textdarstellung |
| `spacer` | Leerer Container, keine Eingabe | Einfacher neuer Baustein |
| `file_tree` | `binding: project.files` | Bisheriger Projektbaum |
| `entity_tree` | `binding: scene.entities` | Bisherige Scene-Liste |
| `map_canvas` | `binding: scene.active` | Vorhandener Map-Renderer und seine Eingaben |
| `script_editor` | `binding: document.active` | Vorhandener Texteditor mit Syntaxprüfung |
| `inspector` | `binding: selection.active` | Bisheriger Map-/Entity-Inspector |
| `scene_info`, `script_info` | Passendes Szenen-/Dokumentbinding | Bisherige Infoansichten |

Tabs belegen die gemessene Text-/Iconbreite mit Innenabstand; ein YAML-Autor muss keine Buchstabenbreiten ausrechnen. Der aktive Tab schließt am Panel an. Umsortierung und Scrollen bleiben Aufgabe des Containers. `tabs` darf nicht leer sein; ohne gespeicherten Zustand wird der erste Tab ausgewählt. `section.open` gilt nur bei der ersten Instanziierung ohne gespeicherten Zustand.

Die Shell liefert das Theme. Ein neuer Typ mit eigenen Eigenschaften bekommt später einen dokumentierten Vertrag und Beispiele. Slider, Textfelder, Dialoggraphen, Animationen und weitere Editoren sind bewusst nicht vorweg definiert.

## Aktionen und Bindings

YAML referenziert **registrierte Namen**, keine C-Funktionszeiger, Dateipfade zu Programmen oder frei auswertbaren Ausdrücke.

| Geplanter Aktionsname | Wirkung |
| --- | --- |
| `document.save` | Aktuelles Dokument bewusst in seine Quelldatei speichern |
| `document.undo`, `document.redo` | Verlauf des aktiven Dokumentanbieters bedienen |
| `scene.toggle_play` | Map starten/stoppen; Wechsel während Play sperren |
| `map.import_tileset` | Vorhandenen PNG-Importdialog öffnen |
| `map.select_brush`, `map.toggle_grid` | Kartenwerkzeug wählen beziehungsweise Raster umschalten |
| `script.check` | JavaScript des aktuellen Scripts nur auf Syntax prüfen |

Weitere heutige Werkzeuge bekommen bei Bedarf eigene registrierte Namen. Aktionen prüfen ihren Kontext auch beim Aufruf; ein optisch deaktivierter Button ist nicht die einzige Sperre. Dokumentändernde Aktionen laufen über die bestehende Command-/Undo-Schicht. UI-Layout und Editorwechsel werden nicht zu Szenen-Undo-Schritten.

| Binding | Datentyp |
| --- | --- |
| `document.can_save`, `document.can_undo`, `document.can_redo` | Boolean |
| `document.save_state`, `editor.mode`, `editor.details`, `editor.message` | Text |
| `map.grid_visible` | Boolean |
| `project.files` | Dateibaum-Modell |
| `scene.entities` | Entity-Liste mit stabilen IDs |
| `scene.active`, `selection.active`, `document.active` | Typgeprüfte Modellreferenz, ggf. leer |

Ein Widget darf nur passende Datentypen anbinden. Eine leere Auswahl zeigt einen leeren Inspector; ein unbekanntes Binding ist ein Fehler. Bindings lesen zunächst Zustand. Änderungen erfolgen über Aktionen oder über die dokumentierte Eingabelogik spezialisierter Widgets. Eine allgemeine zweiseitige Property-Bindung ist kein Bestandteil dieses ersten Entwurfs.

`enabled: {binding: document.can_undo}` ist zulässig. Ein Ausdruck wie `enabled: "scene.count > 0"` ist kein Boolean-Binding und wird abgelehnt. Tastenkürzel bleiben zunächst im C-Aktionssystem, nicht zusätzlich frei in YAML vergeben. Ein Tooltip wie „F7“ allein registriert keinen Shortcut.

## Selbst Änderungen vornehmen

1. Beispielstruktur in einen Layoutordner kopieren, sobald der Loader implementiert ist. Die Shell ist der Einstiegspunkt; ein CLI-Schalter dafür ist noch nicht festgelegt.
2. Titel, Icons oder Reihenfolge vorhandener Tabs/Buttons ändern. IDs für bestehende Elemente beibehalten.
3. Einen weiteren Tab mit neuer ID und einem bereits registrierten Inhaltswidget ergänzen. Widgets benötigen weiterhin ihre passende Datenquelle und ein Modul, das sie bereitstellt.
4. Änderungen erneut laden. Nur validierte Beschreibungen dürfen aktiv werden; ein Reload sichert und entlädt den aktuellen Editor wie ein Editorwechsel.

Beispiel für einen weiteren Informationstab im Map-Panel:

```yaml
- id: extra_info_tab
  title: "Szenendaten"
  icon: file
  content:
    type: scene_info
    id: extra_scene_info
    binding: scene.active
```

Diesen Eintrag unter `columns.right.tabs` in `map.yaml` ergänzen. Das erzeugt eine weitere Darstellung vorhandener Daten, keinen neuen Editor und keine zweite dauerhaft geladene Szene.

## Parser, Validierung und Fehlermeldungen

Als Parser ist **libyaml** in C vorgesehen. Der Parser liefert YAML-Knoten und Quellpositionen; unsere eigene Validierung prüft erst danach das UI-Schema, registrierte Namen und Bindingtypen. Der Parser allein kennt weder Buttons noch Engine-Aktionen. Es ist noch keine neue Bibliotheksabhängigkeit eingebaut.

Version 1 verwendet ein begrenztes YAML-Subset: UTF-8, Mappings, Listen, Zeichenfolgen, Booleans und Zahlen. Farben und mehrdeutige Titel als Strings in Anführungszeichen schreiben; Booleans als `true`/`false`. Doppelte Schlüssel, unbekannte Felder, unbekannte Schemaversionen sowie YAML-Tags, Anchors, Aliases und Merge-Keys werden abgelehnt. Es gibt zunächst keine Templates, Includes, Vererbung oder eingebettete Scripts. Der Bezug von Shell auf Editor-Dateien ist ein eigener, ausdrücklich definierter Mechanismus.

Dateigröße, Knotenzahl und Verschachtelungstiefe erhalten feste Loadergrenzen. Die konkreten Werte werden beim Loader festgelegt und anschließend dokumentiert. Das bestehende Tablimit von 16 ist zu beachten; Überschreitung liefert eine Fehlermeldung statt stiller Kürzung.

Fehlermeldungen nennen Datei, Zeile/Spalte, betroffenen Schlüssel und Ursache, zum Beispiel:

```text
editors/map.yaml:18:5: columns.left.type: unbekannter Widgettyp 'filetree'; erwartet 'file_tree'
```

Die Positionsangabe ist ein Beispiel. Validiert wird vor Aktivierung einer neuen Beschreibung. Bei Fehlern des ersten Starts muss die Anwendungshülle eine verständliche Fehlermeldung zeigen; bei Reload bleibt der alte Aufbau bedienbar.

## Umsetzung in kleinen Schritten

1. **Editorlebenszyklus:** Kontext aller drei Spalten, versionierte Sitzungssicherung einschließlich Undo, vollständige Freigabe und Wiederherstellung. Noch mit C-beschriebenen Layouts.
2. **YAML-Grundlage:** Parser und Validator, Knotenmodell, Registries für Komponenten/Aktionen/Bindings und die beschriebenen Shell-/Editor-Dateien.
3. **Erste Adapter:** Buttons, Toolbar, Tabs sowie die heutigen spezialisierten Map-/Script-/Explorer-/Inspector-Komponenten.
4. **Reload und weitere Elemente:** Transaktionaler Reload, zusätzliche Container und neue Widgets nach konkretem Bedarf.

Damit benutzt YAML denselben Renderer und denselben Lebenszyklus wie die C-Beschreibung. Die UI-Bibliothek bleibt in C; die Engine bekommt dadurch keine Python-/JavaScript-Pflicht für ihren Editoraufbau.

## Abnahme für die spätere Implementierung

- Map → Scripts → Map erhält Dokumentinhalt, Dirty-Status, Undo/Redo, Auswahl und Kamera; alle drei Spalten entsprechen dem Ziel-Editor.
- Speichern/Undo/Redo lösen ausschließlich Aktionen des aktiven Dokumentanbieters aus.
- Tracking zeigt nach Entladen keine lebenden editorgebundenen Widgets, Bindings, Jobs, Parser, Puffer oder Texturen; inaktive Historien werden nicht im RAM gehalten.
- Ein Sicherungsfehler lässt den bisherigen Editor unverändert bedienbar; ein Fehler beim Zielladen erlaubt Wiederholung oder Wiederherstellung des bisherigen Editors.
- Fehlerhafte YAML-Dateien überschreiben weder Layout noch Dokumente; Meldungen enthalten die Quellposition.
- Ein gültiger Reload erhält Dokument-/Undo-Daten; neue IDs erhalten Anfangszustände, fehlende UI-IDs beeinträchtigen keine Dokumentwiederherstellung.
- Doppelte IDs/Schlüssel, unbekannte Typen/Aktionen/Bindings, Typfehler und Loadergrenzen werden gezielt getestet.
- Editor-spezifische Texteingabe und Tastenkürzel sind nach dem Entladen abgemeldet. Beispieldateien werden durch Loader-Integrationstests geprüft, sobald der Loader existiert.

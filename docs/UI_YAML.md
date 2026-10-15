# Deklarative Editoroberflächen mit YAML

**Status: erste Implementierung von `penguin.ui/v1`.** Der Editor liest Shell- und Editor-Layouts aus YAML. Alle drei Spalten sind kontextabhängig; beim Wechsel wird die bisherige Editorinstanz vollständig entladen. Aktionen, Bindings und die vorhandenen Map-/Script-Komponenten bleiben in C. Die Umsetzung steht in [IMPLEMENTIERUNG.md](IMPLEMENTIERUNG.md), die UI-Bausteine in [UI_BIBLIOTHEK.md](UI_BIBLIOTHEK.md).

## Sofort ausprobieren

Die verwendeten Dateien sind [resources/editor/ui/shell.yaml](../resources/editor/ui/shell.yaml), [map.yaml](../resources/editor/ui/editors/map.yaml) und [scripts.yaml](../resources/editor/ui/editors/scripts.yaml). Der Build kopiert sie nach `build/ui-layouts`. Ohne Argument sucht der Editor diese Kopie neben der Programmdatei beziehungsweise eine Ebene darüber (Debug/Release-Konfigurationen).

```sh
cmake --build build --parallel
build/penguin_editor --ui resources/editor/ui/shell.yaml
```

Mit `--ui` kann eine eigene Shell-Datei gewählt werden; Editor-Dateipfade sind relativ zu dieser Datei. In CLion einen absoluten `--ui`-Pfad als Programmargument setzen, wenn die Arbeitsverzeichnisse variieren. **F6** lädt die Shell und den aktiven Editor erneut. Für dauerhafte eigene Änderungen einen eigenen Layoutordner oder direkt die Quelldateien unter `resources/editor/ui` verwenden: Ein weiterer Build überschreibt die Kopien unter `build/ui-layouts` mit den Repo-Vorgaben. Ein fehlerhafter Erststart meldet den Fehler auf stderr und beendet sich; ein fehlerhafter Reload lässt die laufende Oberfläche bestehen.

YAML beschreibt den Aufbau: Panels, Tabs, Widgets, Toolbars, Gestaltung und Verknüpfungen. C stellt Widgets, Daten und Aktionen bereit. So lassen sich vorhandene Elemente selbst neu zusammenstellen; ein neues Zeichenwerkzeug oder ein völlig neues Widget benötigt weiterhin eine C-Implementierung. Der Ansatz entspricht der Trennung zwischen Layoutdatei und Controller etwa bei JavaFX/FXML.

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

Die Anwendungshülle, ihre Schrifttextur und tatsächlich gemeinsam benötigte Projektdienste bleiben bestehen. Dazu gehört ausdrücklich die gemeinsame Shellkonsole: Sie ist nicht Teil einer Editorinstanz und läuft mit ihrem begrenzten Ausgabepuffer beim Editorwechsel weiter (siehe [KONSOLE.md](KONSOLE.md)). Das vollständige Entladen einer Editorinstanz ist kein Entladen des SDL-Fensters oder des C-Programmcodes. Dynamische Bibliotheken/DLL-Plugins sind für diesen Schritt nicht vorgesehen.

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

Sitzungsdaten enthalten keine C-Zeiger, SDL-Handles oder laufenden Threads. Sie werden im privaten binären Sitzungsformat Version 1 mit Längenbegrenzung, Typkennung, Größen-/ABI-Prüfung und Prüfsumme gespeichert. Die Prüfsumme wird vor dem Decodieren geprüft. Es ist ein nativer Cache für den aktuellen Build und die aktuelle Plattform, kein portables Projektformat. YAML beschreibt das Layout und ist **nicht** das Format für große Dokumente und Undo-Verläufe.

**Sitzungssicherung ist kein Speichern des Projekts.** Ctrl+S schreibt weiterhin bewusst die Quelldatei. Ein Editorwechsel darf ungespeicherte Änderungen nicht verwerfen, den Dirty-Status nicht zurücksetzen und keine Projektdatei automatisch überschreiben. Ist eine Wiederherstellung nicht möglich, muss das sichtbar gemeldet werden; kein stiller Rückfall auf einen älteren Dateistand.

Die Speicherung liegt im SDL-Benutzerverzeichnis `PenguinEngine/editor-sessions`, mit projekt- und laufbezogenen Dateinamen. Headless-Smoke-Tests verwenden dafür das Build-Verzeichnis. Große Sicherungen werden nur zum Laden oder Schreiben in den RAM geholt. Editorübergreifende Dienste dürfen keine versteckten Kopien der inaktiven Dokumente/Undo-Historien behalten. Beim regulären Beenden werden nach Speichern oder bewusstem Verwerfen die Dateien dieses Laufs entfernt. Nach einem Absturz eventuell verbleibende Dateien werden beim nächsten Start nicht automatisch geladen. Crash-Recovery, Versionsmigrationen und Bereinigung solcher Altdateien sind noch nicht implementiert. Atomarer Dateiaustausch schützt den bisherigen Checkpoint bei einem fehlgeschlagenen Schreibvorgang; eine Stromausfall-Dauerhaftigkeit wird nicht zugesichert.

## Dateien und erster Aufbau

Die vollständigen Beispielbeschreibungen liegen unter [examples/ui/shell.yaml](examples/ui/shell.yaml), [editors/map.yaml](examples/ui/editors/map.yaml) und [editors/scripts.yaml](examples/ui/editors/scripts.yaml). Es sind bewusst kleinere Startbeispiele, keine vollständige Beschreibung aller heutigen Kartenwerkzeuge. Sie können mit `--ui docs/examples/ui/shell.yaml` geladen werden. Die tatsächlich ausgelieferten Layouts liegen unter `resources/editor/ui`. Beide Beispielgruppen werden durch die C-Loader-Tests geprüft.

```text
ui/
  shell.yaml
  editors/
    map.yaml
    scripts.yaml
```

`file`-Pfade in der Shell sind relativ zur Shell-Datei. Die Modulnamen `map` und `scripts` bezeichnen registrierte, eingebaute C-Editoren. Die beiden registrierten Module `map` und `scripts` besitzen Aufbau, Eingabebehandlung, Aktionen, Dokumentanbieter, Sitzungssicherung und Zerstörung; YAML enthält keinen Pfad zu ausführbarem Code.

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

`document_provider` ist ein registrierter Anbieter wie `scene` oder `script`. Er bindet globale Aktionen und Statuswerte an das aktuelle Dokument. In dieser Version ist der Anbieter verpflichtend: `scene` für Map und `script` für Scripts. Andere Module oder Anbieter müssen zuerst in C ergänzt werden und werden sonst als Validierungsfehler abgelehnt.

Der vertikale Splitter und das untere Konsolen-Tabpanel sind ebenfalls vollständig in YAML beschrieben. Aufbau, Größenregeln, Bindings und Plattformgrenzen erklärt [KONSOLE.md](KONSOLE.md).

## Gemeinsame Widgeteigenschaften

| Eigenschaft | Regel |
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

Die folgenden Adapter sind implementiert. Spezialisierte Typen (`file_tree`, `entity_tree`, `map_canvas`, `script_editor`, `inspector`, `scene_info`, `script_info`, `terminal`) dürfen in dieser ersten Version jeweils nur einmal pro Editorbeschreibung vorkommen; ihre internen C-Zustände sind noch nicht für mehrere Instanzen ausgelegt. Allgemeine Labels, Buttons und Container können mehrfach mit unterschiedlichen IDs vorkommen. Spezialisierte Widgets verwenden weiterhin die bestehenden C-Eingabe- und Renderfunktionen; YAML ordnet sie an und bindet ihre Datenquelle.

| Typ | Zusätzliche Felder / Aufgabe | Grundlage |
| --- | --- | --- |
| `button` | `label`, `icon`, `tooltip`, `action`, Zustandsbindings | Vorhandene UI-Buttons |
| `separator` | Schmaler Trennstrich mit Innenabstand in Toolbar/Zeile | Vorhandene Toolbar-Darstellung |
| `editor_switcher` | Nur Shell; `labels: false` zeigt Icons mit Titel als Tooltip | Vorhandene Bereichsauswahl |
| `tabs` | `tabs`-Liste mit `id`, `title`, optional `icon`, genau einem `content`-Knoten | Vorhandenes Tabmodell |
| `section` | `title`, `open` als anfänglicher Boolean, `children` | Klappbarer Container |
| `split` | `axis`, `initial`, `minimum_first`, `minimum_second`, `gap`, `first`, `second` | Zwei verstellbare Bereiche; Details in [KONSOLE.md](KONSOLE.md) |
| `terminal` | `binding: terminal.shared` | Gemeinsame Shell der Anwendungshülle |
| `row`, `column` | `children`, optional nichtnegativer `gap` | Kinder werden gleichmäßig in verfügbare Breite/Höhe aufgeteilt |
| `label` | `label` als statischer Text oder `{binding: ...}` | Vorhandene Textdarstellung |
| `spacer` | Leerer Container, keine Eingabe | Leerer Platzhalter |
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

| Aktionsname | Wirkung |
| --- | --- |
| `document.save` | Aktuelles Dokument bewusst in seine Quelldatei speichern |
| `document.undo`, `document.redo` | Verlauf des aktiven Dokumentanbieters bedienen |
| `scene.toggle_play` | Map starten/stoppen; Wechsel während Play sperren |
| `map.import_tileset` | Vorhandenen PNG-Importdialog öffnen |
| `map.select_brush`, `map.toggle_grid` | Kartenwerkzeug wählen beziehungsweise Raster umschalten |
| `script.check` | JavaScript des aktuellen Scripts nur auf Syntax prüfen |

Weitere vorhandene Aktionen sind `map.select_eraser`, `map.select_rectangle`, `map.select_fill`, `map.select_entity` und `map.toggle_collision`. Der Boolean-Zustand dazu ist `map.collision_visible`. Aktionen prüfen ihren Kontext auch beim Aufruf; ein optisch deaktivierter Button ist nicht die einzige Sperre. Dokumentändernde Aktionen laufen über die bestehende Command-/Undo-Schicht. UI-Layout und Editorwechsel werden nicht zu Szenen-Undo-Schritten.

| Binding | Datentyp |
| --- | --- |
| `document.can_save`, `document.can_undo`, `document.can_redo` | Boolean |
| `document.save_state`, `editor.mode`, `editor.details`, `editor.message` | Text |
| `map.grid_visible` | Boolean |
| `project.files` | Dateibaum-Modell |
| `scene.entities` | Entity-Liste mit stabilen IDs |
| `terminal.shared` | Gemeinsamer Shellservice; unabhängig vom geladenen Editor |
| `scene.active`, `selection.active`, `document.active` | Typgeprüfte Modellreferenz, ggf. leer |

Ein Widget darf nur passende Datentypen anbinden. Eine leere Auswahl zeigt einen leeren Inspector; ein unbekanntes Binding ist ein Fehler. Bindings lesen zunächst Zustand. Änderungen erfolgen über Aktionen oder über die dokumentierte Eingabelogik spezialisierter Widgets. Eine allgemeine zweiseitige Property-Bindung ist kein Bestandteil dieser ersten Version.

`enabled: {binding: document.can_undo}` ist zulässig. Ein Ausdruck wie `enabled: "scene.count > 0"` ist kein Boolean-Binding und wird abgelehnt. Tastenkürzel bleiben zunächst im C-Aktionssystem, nicht zusätzlich frei in YAML vergeben. Ein Tooltip wie „F7“ allein registriert keinen Shortcut.

## Selbst Änderungen vornehmen

1. Beispielstruktur in einen eigenen Layoutordner kopieren und mit `--ui /pfad/zur/shell.yaml` öffnen. Die Shell ist der Einstiegspunkt.
2. Titel, Icons oder Reihenfolge vorhandener Tabs/Buttons ändern. IDs für bestehende Elemente beibehalten.
3. Einen weiteren Tab mit neuer ID und einem bereits registrierten Inhaltswidget ergänzen. Widgets benötigen weiterhin ihre passende Datenquelle und ein Modul, das sie bereitstellt.
4. Änderungen erneut laden. Nur validierte Beschreibungen dürfen aktiv werden; ein Reload sichert und entlädt den aktuellen Editor wie ein Editorwechsel.

Beispiel für einen weiteren Informationstab im Map-Panel:

```yaml
- id: extra_info_tab
  title: "Szenendaten"
  icon: file
  content:
    type: label
    id: extra_scene_info
    label: {binding: editor.details}
```

Diesen Eintrag unter `columns.right.tabs` in `map.yaml` ergänzen. Das erzeugt eine weitere Darstellung vorhandener Daten, keinen neuen Editor und keine zweite dauerhaft geladene Szene.

## Parser, Validierung und Fehlermeldungen

Der Parser ist **libyaml 0.2.5**, im Repository mit MIT-Lizenz mitgeliefert. Der Parser liefert YAML-Knoten und Quellpositionen; unsere eigene Validierung prüft erst danach das UI-Schema, registrierte Namen und Bindingtypen. Der Parser allein kennt weder Buttons noch Engine-Aktionen. Zum Bauen ist kein zusätzliches Systempaket und kein Download des Parsers erforderlich.

Version 1 verwendet ein begrenztes YAML-Subset: UTF-8, Mappings, Listen, Zeichenfolgen, Booleans und Zahlen. Farben und mehrdeutige Titel als Strings in Anführungszeichen schreiben; Booleans als `true`/`false`. Doppelte Schlüssel, unbekannte Felder, unbekannte Schemaversionen sowie YAML-Tags, Anchors, Aliases und Merge-Keys werden abgelehnt. Es gibt zunächst keine Templates, Includes, Vererbung oder eingebettete Scripts. Der Bezug von Shell auf Editor-Dateien ist ein eigener, ausdrücklich definierter Mechanismus.

Die Grenzen betragen 256 KiB pro Datei, 2048 YAML-Knotenereignisse einschließlich Schlüssel und 32 Verschachtelungsebenen. Pro Editor sind höchstens 64 zustandsbehaftete `tabs`-/`section`-Container und ein interaktiver `map_canvas` oder `script_editor` erlaubt. `layout.gap` ist eine ganze Zahl von 0 bis 12, `tab_gap` von 0 bis 16. Container ohne eigene Scrollbar beschneiden überlaufende Inhalte; sie sind noch kein Flexbox-/Docking-System. Das bestehende Tablimit von 16 ist zu beachten; Überschreitung liefert eine Fehlermeldung statt stiller Kürzung.

Fehlermeldungen nennen Datei, Zeile/Spalte, betroffenen Schlüssel und Ursache, zum Beispiel:

```text
editors/map.yaml:18:5: columns.left.type: unbekannter Widgettyp 'filetree'; erwartet 'file_tree'
```

Die Positionsangabe ist ein Beispiel. Validiert wird vor Aktivierung einer neuen Beschreibung. Bei Fehlern des ersten Starts erscheint eine verständliche Meldung auf stderr; bei Reload bleibt der alte Aufbau bedienbar. Schemafehler nennen Datei und Position sowie die Ursache bzw. das unbekannte Feld; vollständige JSON-Pfade werden derzeit nicht für jeden Fehler ausgegeben.

## Implementierung und Erweiterungsstellen

- `src/editor/layout/ui_layout.c`: YAML → Knotenmodell, Quellpositionen, Schema, Typ-/Aktions-/Binding-Prüfung.
- `src/editor/app/main.c`: Start und Hauptschleife; `app/shell.c` zeichnet die Anwendungshülle.
- `src/editor/layout/renderer.c` und `bindings.c`: YAML-Darstellung sowie Bindings und Aktionsverteilung. Map-/Script-Aktionen liegen bei den jeweiligen Editoren.
- `src/editor/workspace/lifecycle.c`: Editorwechsel, UI-Snapshots und F6-Reload. Wechsel aus einem UI-Callback werden bis zum Frameende aufgeschoben, damit keine gerade benutzten YAML-Knoten freigegeben werden.
- `src/editor/editors/map/checkpoint.c` und `scripts/checkpoint.c`: Dokument- und Undo-Codecs ohne Abhängigkeit vom globalen Editorzustand.
- `src/editor/workspace/session.c`: begrenzter Checkpoint-Stream, Prüfsumme und atomarer Austausch. Dokument-/Undo-Codec und UI-Snapshots werden vom Editor bereitgestellt.
- `src/editor/layout/ui_layout.h` und `src/editor/workspace/session.h`: interne C-Schnittstellen für Loader und Checkpoints, unabhängig von SDL testbar.
- `src/ui/`: wiederverwendbare Eingabe-/Layoutbausteine und SDL-Darstellung; das Theme kann zur Laufzeit gesetzt werden.

Ein neuer Widgettyp benötigt zunächst Validatorvertrag und C-Adapter, ein neuer Editor zusätzlich Modul-/Dokument-Lebenszyklus und Eingaben. Die Engine bleibt in C und benötigt für den UI-Aufbau weder Python noch JavaScript. Die Parsernamen sind eine begrenzte interne Registry; dynamische DLL-Plugins sind noch nicht vorgesehen.

Die globale Toolbar enthält in v1 genau einen `editor_switcher` als letztes Element. `labels: false` zeigt nur Icons, `true` zusätzlich Titel. Andere Elemente davor lassen sich umordnen. Spezifische Werkzeuge bleiben im Editor-`toolbar`-Abschnitt. Die beiden heutigen Editoren sind registriert; ein unbekannter Modulname wird nicht als Platzhalter geladen.

Noch offen sind automatische Dateibeobachtung statt F6, allgemeines Zweiwege-Binding, Gewichte für beliebige Row-/Column-Kinder, zusätzliche Widgettypen, neue Editormodule und Crash-Recovery über Programmstarts hinweg. Die Sitzungssicherung speichert vollständige Undo-Snapshots; dadurch kann ein Wechsel je nach Verlauf spürbar Datenträger-I/O verursachen.

## Abnahme und Tests

- Map → Scripts → Map erhält Dokumentinhalt, Dirty-Status, Undo/Redo, Auswahl und Kamera; alle drei Spalten entsprechen dem Ziel-Editor.
- Speichern/Undo/Redo lösen ausschließlich Aktionen des aktiven Dokumentanbieters aus.
- Tracking zeigt nach Entladen keine lebenden editorgebundenen Widgets, Bindings, Jobs, Parser, Puffer oder Texturen; inaktive Historien werden nicht im RAM gehalten.
- Ein Sicherungsfehler lässt den bisherigen Editor unverändert bedienbar; ein Fehler beim Zielladen erlaubt Wiederholung oder Wiederherstellung des bisherigen Editors.
- Fehlerhafte YAML-Dateien überschreiben weder Layout noch Dokumente; Meldungen enthalten die Quellposition.
- Ein gültiger Reload erhält Dokument-/Undo-Daten; neue IDs erhalten Anfangszustände, fehlende UI-IDs beeinträchtigen keine Dokumentwiederherstellung.
- Doppelte IDs/Schlüssel, unbekannte Typen/Aktionen/Bindings, Typfehler und Loadergrenzen werden gezielt getestet.
- Editor-spezifische Texteingabe und Tastenkürzel sind nach dem Entladen abgemeldet. Beispieldateien werden durch die Loader-Tests geprüft.


## Eigener Fensterrahmen

`window_controls` steht in der Shell neben `toolbar`. Die Standardliste enthält die drei Buttons Minimieren, Maximieren/Wiederherstellen und Schließen. Die Liste unterstützt höchstens vier Buttons und wird rechtsbündig in der oberen Leiste gezeichnet. Die Editor-Navigation lässt diesen Platz frei. Ohne diese Liste bleibt der native Rahmen erhalten. Wenn SDL auf der Plattform keinen Fenster-Hit-Test unterstützt, bleibt der native Rahmen ebenfalls als Rückfall erhalten.

```yaml
window_controls:
  - {type: button, id: window_minimize, icon: minimize, tooltip: "Minimieren", action: window.minimize}
  - {type: button, id: window_maximize, icon: maximize, tooltip: "Maximieren / Wiederherstellen", action: window.maximize}
  - {type: button, id: window_close, icon: close, tooltip: "Schließen", action: window.close}
```

Freie Flächen der oberen Leiste verschieben das Fenster über den Betriebssystem-Hit-Test. Ein fünf Pixel breiter Rand erlaubt Größenänderungen, einschließlich der Ecken. Maximierte Fenster haben keine Resize-Zonen. Toolbuttons und Editor-Tabs bleiben anklickbar.

Schließen durchläuft denselben Ablauf für ungespeicherte Dokumente wie das native Schließen. Minimieren verwendet die Betriebssystemfunktion und kann über die Taskleiste bzw. das Dock rückgängig gemacht werden.

Die Fensterlogik liegt in `src/editor/app/window.c`; die UI-Lib stellt die Icons bereit. Die SDL-Dummy-Smokes prüfen die Fensterbutton-Anordnung, die Hit-Test-Zonen und die Größenwiederherstellung. Fensterbewegung, Snap-Verhalten und Darstellung mit einem echten Window-Manager müssen zusätzlich manuell auf der Zielplattform geprüft werden.


## Werkzeuge des Scene-Graphs

Der spezialisierte Map-Knoten `entity_tree` rendert den aufklappbaren Entity-Baum. Seine optionale `tools`-Liste enthält bis zu vier Buttons. Sie werden oberhalb des Baums gezeichnet und bei langen Labels oder schmalen Panels umgebrochen. IDs müssen wie alle Widget-IDs eindeutig bleiben.

```yaml
content:
  type: entity_tree
  id: entities
  binding: scene.entities
  tools:
    - {type: button, id: entity_root, icon: entity, tooltip: "Neue Wurzel", action: scene.add_root}
    - {type: button, id: entity_sibling, icon: plus, tooltip: "Geschwister hinzufügen", action: scene.add_sibling}
    - {type: button, id: entity_child, icon: layers, tooltip: "Kind hinzufügen", action: scene.add_child}
    - {type: button, id: entity_delete, icon: delete, tooltip: "Unterbaum löschen", action: scene.delete}
```

Wurzel-Erstellen benötigt keine Auswahl. Geschwister, Kind und Löschen benötigen eine gültige Auswahl; Kind-Erstellen setzt einen Transform voraus. Dokumentänderungen sind während Play oder laufenden Gesten deaktiviert. Pfeile steuern nur die Navigation und ändern das Szenendokument nicht. Aufklappzustände werden mit dem Map-Editor ausgelagert und beim Wechsel/F6 wiederhergestellt. Ohne `tools` wird nur der Baum gezeichnet; dieselben Aktionen lassen sich auch in anderen Map-Toolbars platzieren.

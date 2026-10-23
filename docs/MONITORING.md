# Prozess-Monitor

Die Statusleiste zeigt RAM, CPU und die bekannte GPU-Effekttexturkapazität des laufenden Programms.
Im Vektoreditor liegt **Monitor** nach **Vorschau**, **Klonen** und **Raster**.
Die drei laufenden Diagramme zeigen die letzten 60 Sekunden.

- Messintervall: 500 ms. Der Verlauf enthält höchstens 120 Messungen.
- RAM: aktuell residenter Prozessspeicher in MiB (1 MiB = 1.048.576 Bytes).
- CPU: Differenz der gesamten Prozess-CPU-Zeit geteilt durch die tatsächlich
  vergangene Zeit. 100 % entsprechen einem ausgelasteten CPU-Kern; mehrere Threads
  können zusammen mehr als 100 % erreichen. Die Diagrammskala passt sich an.
- Die erste CPU-Anzeige benötigt zwei Messungen. Fehlende Werte erscheinen als
  `--` in der Statusleiste und als Lücken im Diagramm.
- Die Aufzeichnung läuft auch bei verborgenem Monitor weiter und bleibt beim
  Editorwechsel erhalten. Ein neuer Programmstart beginnt einen neuen Verlauf.

## In anderen Editoren verwenden

Das Panel ist Teil der gemeinsamen App-Infrastruktur und benötigt keinen eigenen
C-Code im jeweiligen Editor. In dessen YAML genügt ein Tab mit dem reservierten Slot:

```yaml
- type: tab
  id: my_monitor_tab
  text: Monitor
  padding: 0
  children:
    - {type: slot, id: process_monitor_content}
```

Der Host füllt diesen Slot mit `resources/monitor.yaml`. Die Canvas-IDs
`process_monitor_cpu`, `process_monitor_memory` und `process_monitor_gpu_memory` werden vom Host gezeichnet;
andere Canvas-Aufrufe gehen weiterhin an den aktiven Editor. Pro Editor-Layout
wird ein Monitor-Panel unterstützt. Die Statusanzeige verwendet das gemeinsame
Shell-Label `process_status`.

## Zuständigkeiten

- `apps/demo/platform/process_metrics.c`: native Zähler. Linux nutzt die
  Prozess-CPU-Uhr und `/proc/self/statm`, Windows `GetProcessTimes` und
  `GetProcessMemoryInfo`, macOS `getrusage` und Mach `task_info`.
- `apps/demo/monitor/history.c`: reine Werte, Berechnung und begrenzter Ringpuffer.
  Zeit und Zähler werden übergeben, wodurch sich die Berechnung deterministisch
  mit Unity prüfen lässt.
- `apps/demo/monitor/monitor.c`: Abtastrate, Statusanzeige und YAML-Komposition.
- `apps/demo/monitor/render.c`: drei Diagramme mit Zeitachse und eigener Skalierung.
- `EditorHost`: besitzt den Verlauf für die gesamte Sitzung und leitet die
  Canvas-Aufrufe weiter. Messung und Zeichnen erfolgen auf dem UI-Thread; die
  nativen Abfragen werden nur zweimal pro Sekunde ausgeführt.

pengUI bleibt unabhängig von Prozessmetriken und konkreten Editoren. Für eine
weitere App lässt sich derselbe Monitor über den Host oder über
`process_monitor_tick`, `process_monitor_layout` und `process_monitor_draw` anbinden.

## Grafikkarte und GPU-Effekttexturen

Der Monitor zeigt außerdem Gerätename, SDL-Renderer, GPU-Backend/Treiber und den
aktiven Effektpfad. Die Angaben stammen aus SDLs Geräte-Eigenschaften; ein
Softwaregerät wie llvmpipe wird mit seinem tatsächlichen Namen angezeigt.
Fehlt ein SDL-GPU-Gerät, wird kein Kartenname erfunden.

Das dritte Diagramm `process_monitor_gpu_memory` zeichnet die von pengDraw
verwaltete Kapazität der drei Schatten-Effekttexturen in MiB auf. Derselbe Wert
steht als **GPU-FX** in der Statuszeile. Ein gültiger Nullwert vor dem ersten Effekt
unterscheidet sich von einer nicht verfügbaren Messung (`--`). Die Information
wird ohne Pixel-Readback oder Warten auf die GPU ausgelesen.

**GPU-FX ist kein gesamter VRAM-Verbrauch:** Artwork-Kachelcache, UI-/Bild-/
Texttexturen, Swapchain und Treiber-Overhead sind nicht enthalten. Bei integrierten
GPUs oder Softwaregeräten kann der Speicher physisch aus dem Arbeitsspeicher
stammen. Die Kapazität kann zur Wiederverwendung erhalten bleiben, auch wenn das
Objekt bereits gelöscht wurde.

Eine echte GPU-Auslastung in Prozent wird ausdrücklich als **nicht erfasst**
gekennzeichnet. SDL liefert dafür keinen einheitlichen portablen Zähler. Weder
CPU-Auslastung noch Shader-Aufrufzahlen werden als GPU-Auslastung ausgegeben.
Solche Werte benötigen eine gesonderte native beziehungsweise treiberspezifische
Anbindung. Die Plattform-Messung für CPU/RAM bleibt unverändert.

Das YAML-Panel scrollt, damit Metadaten und drei Diagramme auch in einem kleinen
Monitorbereich erreichbar sind. Der Host beobachtet den gemeinsamen Renderer beim
Canvas-Zeichnen und speichert nur Werte, keine geliehenen GPU-Zeiger. Geräteangaben,
Verlauf und Darstellung bleiben damit unabhängig vom ausgewählten Editor.


## Performance und Aktionen im Projekt aufzeichnen

1. Ein Projekt öffnen oder neu anlegen.
2. Im Tab **Monitor** den Toggle **Performance loggen** einschalten.
3. Den problematischen Ablauf durchführen, etwa Klone anlegen und verschieben.
4. Den Toggle wieder ausschalten. Im Projektordner liegt die Datei
   `performance-YYYYMMDD-HHMMSS-NNN.jsonl`; Datum und Uhrzeit sind UTC.
   Der Dateiname steht während der Aufzeichnung im Monitor. Der Explorer wird
   beim Umschalten aktualisiert. Die Datei kann anschließend zur Analyse geteilt werden.

Jeder Start erzeugt eine neue Datei und überschreibt keine bestehende Aufzeichnung.
Ohne geöffnetes Projekt wird der Start mit einer Meldung abgewiesen. Ein
Projektwechsel beendet die Aufzeichnung im bisherigen Projekt; im neuen Projekt
muss sie ausdrücklich wieder eingeschaltet werden. Editorwechsel lassen sie
weiterlaufen. Beim Beenden der Anwendung wird die Datei geschlossen. Schreibfehler
beenden die Aufzeichnung, setzen den Toggle zurück und erscheinen im Monitor.

### Inhalt und Messgrenzen

Das Format ist **JSON Lines**, Schema `penguengine/performance/v1`: jede Zeile ist
für sich ein JSON-Objekt. Enthalten sind:

- UTC-Zeit und monotone Sekunden seit Aufzeichnungsbeginn.
- Ereignistyp, Editor, Dokument-ID, UI-Aktionsname und Widget-ID.
- Auswahl: `selected_ids` (bis zu 64 Objekt-IDs), `active_object` und `active_artboard`.
  `selection.changed` erfasst Auswählen, Abwählen und Wechsel zwischen Objekten,
  auch bei unveränderter Anzahl. Das funktioniert in Zeichenfläche und Ebenen-Tree.
- Ansicht: `viewport_x`/`viewport_y` als Versatz in Bildschirmpixeln und `zoom` als Faktor
  (1 = 100 %); bei Editoren ohne diese Diagnosewerte steht dort `null`.
  Beim Verschieben der Zeichenfläche steht `viewport.pan` in den Start-/Endeinträgen
  und in den periodischen Samples. Ein direkter Ansichtswechsel ohne laufende Geste
  wird zusätzlich als `viewport.changed` erfasst.
- Aktuelle Interaktion, etwa `selection.move`, `selection.resize`, `selection.rotate`,
  `viewport.pan`, `viewport.zoom` oder `inspector.scrub`.
- Anzahl tatsächlicher Formobjekte, dargestellbarer Instanzen inklusive Originalen,
  ausgewählter Objekte und Position des Undo-Cursors. Diese optionalen Kontextwerte
  liefert aktuell der Vektoreditor über seinen Adapter; sie zählen auch ausgeblendete
  beziehungsweise außerhalb des Viewports liegende Objekte.
- Frisch abgefragter Prozess-RAM in MiB, CPU-Prozent des letzten Monitorintervalls,
  dessen Alter in Sekunden und die bekannte GPU-Effekttexturkapazität in MiB.
- Im Starteintrag zusätzlich Plattform, GPU-Gerät, Renderer, Treiber und der begrenzte
  Umfang der GPU-Speichermessung. Nicht verfügbare Messwerte sind `null`, nicht Null.

`sample`-Zeilen folgen den 500-ms-Messungen, auch wenn der Monitor-Tab nicht sichtbar
ist. `action` protokolliert UI-Aktionsanforderungen; das ist kein Erfolgsnachweis für
Dateidialoge oder Befehle. Erkannte Tastenkürzel werden ohne eingegebenen Text erfasst.
`interaction.begin` und `interaction.end` markieren Gesten. `document.changed`
kennzeichnet Änderungen am Undo-Stand oder an den Objekt-/Instanzzahlen;
`context.changed` einen Dokument- oder Editorwechsel. Abbruch und Undo können deshalb
an Operation und Undo-Cursor nachvollzogen werden. Eine Kopieraktion verändert den
Undo-Cursor dagegen nicht. Start und Ende heißen `session.start` und `session.stop`.

CPU 100 % entspricht weiterhin einem ausgelasteten Kern. `metrics_age_s` beschreibt
das Alter des letzten Monitorintervalls; RAM wird für jede Zeile erneut gelesen.
GPU-Effekttexturen sind weiterhin **kein gesamter VRAM-Verbrauch** und keine
GPU-Auslastung. Der Log enthält keine Feldwerte, eingegebenen Texte oder Dokumentinhalte.

### Architektur und Aufwand

`monitor/log.c` streamt und leert seinen Dateipuffer nach jeder Zeile. Es hält nur
Dateihandle, Pfade und den letzten Kontext im RAM. `monitor/recording.c` bindet den
YAML-Toggle an. Der Host sammelt Aktionen und vergleicht die optionalen
`EditorDiagnostics` seiner Adapter. Der Vektoreditor übersetzt seine Werte in
`workspace/diagnostics.c`; der Host greift dafür nicht in sein Modell ein.

Es wird nicht jede Mausbewegung geschrieben. Innerhalb einer unveränderten Geste
bleibt es bei den periodischen Samples. Die Dateiausgabe und Messabfragen verursachen
einen kleinen zusätzlichen Aufwand; auf langsamen Laufwerken kann insbesondere das
Leeren des Dateipuffers messbar werden. Die Aufzeichnung startet standardmäßig ausgeschaltet.

### Render-Ressourcen im Performance-Log

Jede Zeile enthält zusätzlich zwei Diagnoseblöcke. Sie sind rein lesend; die
Abfrage wartet nicht auf die GPU und liest keine Pixel zurück.

`render_resources` gehört zum SDL-Renderer und zählt seine gesamte Laufzeit:

| Feld | Bedeutung |
| --- | --- |
| `available`, `complete` | Zähler vorhanden und vollständig; bei fehlenden Daten stehen Zahlen auf `null`. `complete: false` bedeutet, dass Diagnose-Metadaten nicht vollständig angelegt werden konnten. |
| `age_s` | Alter des letzten Ressourcen-Snapshots; er wird nach dem Canvas-Zeichnen aktualisiert. |
| `textures_created`, `textures_released` | Erfolgreich erfasste pengDraw-Textur-Erzeugungen und SDL-Texturfreigaben. |
| `texture_failures` | Fehlgeschlagene Textur-Erzeugungen. |
| `live_textures`, `live_bytes`, `peak_bytes` | Aktueller Bestand, dessen rechnerische Pixeldaten und bisheriger Höchststand in Bytes. |
| `artwork_bytes`, `shadow_bytes`, `mask_bytes`, `gradient_bytes`, `auxiliary_bytes` | Aktuelle Pixeldaten nach Zweck; zusammen ergeben sie `live_bytes`. |
| `gpu_shadow_draws`, `gpu_gradient_draws` | Erfolgreiche GPU-Effektaufrufe, bei nicht vorhandenem Effektpfad `null`. |
| `gpu_shadow_failures`, `gpu_gradient_failures` | Fehlgeschlagene GPU-Effektaufrufe. |
| `shadow_fallback_attempts`, `gradient_fallback_attempts` | Begonnene CPU-Ersatzpfade; auch beim Software-Renderer. Das ist kein Erfolgszähler. |
| `shadow_width`, `shadow_height`, `shadow_targets` | Vorhandene GPU-Schattenpuffer: Kapazität in Pixeln und Anzahl. |

`artwork_cache` gehört zur **aktiven Editorinstanz** und wird beim Entladen
zurückgesetzt. Der Vektoreditor meldet `tile_bytes`, `scratch_bytes`, `tiles`,
`hits`, `misses`, `repaints` und `evictions`. `repaints` zählt die Neuzeichnung
bestehender Kacheln; `evictions` nur tatsächlich entfernte Kacheltexturen.
Andere Editoren ohne diese Anbindung melden `available: false` und `null`-Werte.
Die Cache-Bytes sind bereits in `render_resources.artwork_bytes` enthalten,
sie dürfen nicht noch einmal zum Gesamtwert addiert werden.

**Messgrenze:** Gezählt werden ausschließlich von pengDraw angelegte
SDL-Textur-Handles. UI-Bilder, importierte Bilder, Schrifttexturen, die Swapchain,
Treiberpools und interne beziehungsweise noch verwendete GPU-Kopien sind nicht
enthalten. Die Größe ist `Breite × Höhe × Bytes pro Pixel`, ohne Alignment oder
Treiber-Overhead. Diese Zahlen sind weder Prozess-RAM noch eine vollständige
VRAM-Messung. `textures_released` bedeutet, dass SDL den Textur-Handle freigegeben
hat; der Treiber kann zugehörige GPU-Ressourcen noch länger behalten.

Normalerweise gilt `textures_created - textures_released == live_textures`.
Wachsen diese Bestände und Bytes beim RAM-Anstieg mit, lässt sich der betreffende
pengDraw-Bereich eingrenzen. Bleiben sie konstant, ist der zusätzliche Verbrauch
mit diesen Texturen nicht erklärt; daraus allein folgt noch kein Treiberfehler.
Die Diagnose legt keine eigenen Texturen an und hält nur einen kleinen Zählerblock
pro Renderer sowie einen Eintrag pro aktuell erfasster Textur im RAM.

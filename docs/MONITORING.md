# Prozess-Monitor

Die Statusleiste zeigt den aktuellen RAM- und CPU-Verbrauch des laufenden Programms.
Im Vektoreditor liegt **Monitor** nach **Vorschau**, **Klonen** und **Raster**.
Die beiden laufenden Diagramme zeigen die letzten 60 Sekunden.

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
`process_monitor_cpu` und `process_monitor_memory` werden vom Host gezeichnet;
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
- `apps/demo/monitor/render.c`: zwei Diagramme mit Zeitachse und eigener Skalierung.
- `EditorHost`: besitzt den Verlauf für die gesamte Sitzung und leitet die
  Canvas-Aufrufe weiter. Messung und Zeichnen erfolgen auf dem UI-Thread; die
  nativen Abfragen werden nur zweimal pro Sekunde ausgeführt.

pengUI bleibt unabhängig von Prozessmetriken und konkreten Editoren. Für eine
weitere App lässt sich derselbe Monitor über den Host oder über
`process_monitor_tick`, `process_monitor_layout` und `process_monitor_draw` anbinden.

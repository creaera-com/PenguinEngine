# Gemeinsame Shell-Konsole

Unter Map und Scripts steht in der mittleren Spalte ein vertikal teilbarer Bereich. Oben bleibt der Editor, unten befindet sich ein Panel mit dem Tab **Konsole**. Die drei waagerecht angeordneten Punkte dazwischen lassen sich nach oben/unten ziehen. Jede Editoransicht merkt sich ihre Panelhöhe und Tabreihenfolge beim Wechsel und bei F6.

## Eine gemeinsame Sitzung

Beide Panels zeigen **dieselbe Shell**. Arbeitsverzeichnis, Ausgabe und laufende Befehle bleiben beim Wechsel zwischen Map und Scripts und beim YAML-Reload erhalten. Die Shell gehört zur Anwendungshülle, während die eigentlichen Editorinstanzen weiterhin entladen werden. Das Terminal startet beim ersten sichtbaren Konsolenwidget im Projektverzeichnis mit der ausführbaren `$SHELL`, ansonsten `/bin/sh`. Das ist eine interaktive Shell ohne zusätzliches Terminalfenster.

Die Sitzung lebt nur bis zum Beenden des Editors. Shellprozess und Vordergrund-Prozessgruppe werden dann geschlossen; bewusst abgekoppelte Hintergrundprogramme besitzen wie in anderen Terminals gegebenenfalls eine eigene Lebensdauer. Ein beendetes Terminal wird über das Refresh-Symbol rechts unten neu gestartet; dabei wird sein Ausgabepuffer zurückgesetzt. Es gibt keine automatische Wiederaufnahme nach einem Programmneustart und keine Änderung von Projektdateien durch den Editorwechsel. Shellbefehle arbeiten selbstverständlich mit dem echten Dateisystem und den Rechten des Benutzers.

## Bedienung

- In den Textbereich klicken, um den Terminalfokus zu setzen. Danach gehören Texteingabe, Pfeile, Tab, Home/End, Funktionstasten und Ctrl-Tastenkombinationen der Shell.
- **Ctrl+C** unterbricht ein Vordergrundprogramm. **Ctrl+D** sendet Dateiende; **Ctrl+L** kann die Shellanzeige leeren. Globale Dokumentkürzel und WASD-Spielereingaben werden beim Terminalfokus nicht ausgelöst.
- Mit der Maus Text markieren; **Ctrl+Shift+C** kopiert, **Ctrl+Shift+V** fügt ein. Auf macOS sind zusätzlich Cmd+C/V vorgesehen. Paste berücksichtigt Bracketed Paste, sofern das Shellprogramm es aktiviert hat.
- Mausrad, Scrollbar oder **Shift+PageUp/PageDown** bewegen sich durch maximal 512 Ausgabezeilen. Neue Tastatureingabe springt zur aktuellen Ausgabe zurück.
- Außerhalb der Konsole klicken, um wieder den Editor zu bedienen. Auch F6 gehört bei aktivem Terminalfokus zum Terminal; für einen YAML-Reload vorher den Fokus wechseln.

Der VT-Parser unterstützt ANSI-Farben, Cursorsteuerung, Unicode und Alternativbildschirme. Die bestehende eingebettete UI-Schrift stellt allerdings nur ASCII/Latin-1 vollständig dar; weitere Glyphen erscheinen als Platzhalter. Unicode-Eingabe und Kopieren bleiben UTF-8. Mausreporting an Terminalprogramme, Terminalbilder und zusätzliche Schriftstile sind noch nicht implementiert. Mausgesten dienen zunächst der Textauswahl und dem Scrollen.

## YAML-Aufbau

Die Dateien `resources/editor/ui/editors/map.yaml` und `scripts.yaml` beschreiben jeweils ihren eigenen unteren Tabcontainer. Für Map sieht der mittlere Bereich so aus:

```yaml
center:
  type: split
  id: center_split
  axis: vertical
  initial: 210
  minimum_first: 160
  minimum_second: 100
  gap: 8
  first: {type: map_canvas, id: canvas, binding: scene.active}
  second:
    type: tabs
    id: bottom_tools
    tabs:
      - id: console_tab
        title: "Konsole"
        icon: terminal
        content: {type: terminal, id: console, binding: terminal.shared}
```

Scripts verwendet als `first` stattdessen `{type: script_editor, id: document, binding: document.active}`. Weitere kontextabhängige Tools lassen sich später als zusätzliche Einträge unter `second.tabs` ergänzen. Bereits registrierte Labels oder Widgets können dort sofort verwendet werden; ein neues Tool benötigt weiterhin seinen C-Adapter.

`split` besitzt genau zwei Kinder, `first` und `second`. `axis` ist `vertical` (oben/unten) oder `horizontal` (links/rechts). `initial` ist die Anfangsgröße des zweiten Panels in Pixeln; `minimum_first` und `minimum_second` sind die Mindestgrößen. Diese drei Zahlen müssen ganze Werte von 1 bis 2000 sein. `gap` ist der ziehbare Zwischenraum von 4 bis 16 Pixeln. Bei Platzmangel reduziert die UI Mindestgrößen proportional, ohne negative Bereiche zu erzeugen. Die gespeicherte Größe wird an den aktuell verfügbaren Bereich angepasst.

`terminal` verwendet ausschließlich `binding: terminal.shared` und darf einmal je Editorbeschreibung vorkommen. Ein ausgeblendetes oder deaktiviertes Konsolenwidget erhält keine Eingaben; eine bereits gestartete gemeinsame Shell läuft weiter. YAML enthält keine automatisch auszuführenden Shellbefehle.

## Plattformen und Aufbau

| Teil | Verantwortung |
| --- | --- |
| `src/editor/console/terminal.*` | SDL-unabhängiges VT-Modell, Farben, Tastencodierung und begrenzter Scrollback; verwendet libvterm 0.3.3. |
| `src/editor/console/view.*` | Darstellung, Auswahl, Fokus, Clipboard und pro Frame begrenzte Shellkommunikation. |
| `src/platform/terminal/process.h` | Plattformvertrag für Start, nichtblockierendes Lesen/Schreiben, Resize und Stop. |
| `src/platform/terminal/posix.c` | PTY, kontrollierendes Terminal und Shellprozess unter Linux/POSIX. |
| `src/platform/terminal/unavailable.c` | Kompilierbarer Fallback für Plattformen ohne Backend. |
| `src/ui/core/core.c` | Wiederverwendbare Split-Geometrie; keine Terminalabhängigkeit. |

**Linux ist lokal getestet.** Der POSIX-Backend wird auch für macOS gebaut; die Desktop-CI bzw. ein echter Mac müssen ihn dort bestätigen. **Unter Windows ist der ConPTY-Backend noch offen.** Das Konsolenpanel wird dort bereits gebaut und zeigt eine verständliche Meldung statt eines Terminalprozesses. Die Plattformgrenze ist für diese spätere Ergänzung vorbereitet.

Parser und Screen erhalten höchstens 100 Zeilen und 240 Spalten; der Scrollback hält maximal 512 Zeilen, die ausstehende Eingabe höchstens 64 KiB. Pro Frame werden höchstens 64 KiB Shellausgabe gelesen. Größenänderungen erreichen sowohl den VT-Bildschirm als auch den PTY; laufende Programme erhalten die neue Terminalgröße.

GUI-Smoke-Tests und `--smoke` starten absichtlich keine Shell. Sie prüfen Splitter, Fokus, gemeinsamen Zustand und Reload mit einem lokalen VT-Modell. Die separaten Terminal-Unit-Tests prüfen auf POSIX zusätzlich echte Befehle, Terminalgröße, Arbeitsverzeichnis, Ctrl+C und Prozessende. Die vollständige interaktive Bedienung sollte auf dem jeweiligen Desktop getestet werden.

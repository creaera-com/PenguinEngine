# 01 – Einstieg und Projekte

Dieses Handbuch richtet sich an Anwender des Vector Tools in der Editor-App
`pengui_editor`. Die separate Widget-Demo ist nicht das Zeichenprogramm.
Die folgenden Kapitel führen von einem Testprojekt bis zu einer gespeicherten
Zeichnung und einem Bildexport. Arbeite für die Übungen in einem eigenen Ordner;
Dateioperationen wie Export und Ordneranlage gehören nicht zur Dokument-History.

## Start und Projektanlage

1. Starte `pengui_editor` aus deiner Build-Umgebung, beispielsweise als Run-Target in CLion.
2. Wähle bei Bedarf oben das Symbol für den Vector Editor.
3. Öffne links oben das Hamburger-Menü und wähle **Neues Projekt**.
4. Wähle einen vorhandenen, möglichst leeren Ordner als Projektverzeichnis.

Dort entsteht `project.pengui.yaml`. Eine bereits vorhandene Projektdatei wird nicht
überschrieben. **Projekt öffnen** im selben Menü öffnet diese Datei über den
Systemdialog. Ein Projekt ist ein Arbeitsverzeichnis; es ist nicht mit einem
Zeichendokument gleichzusetzen. Du kannst auch ohne Projekt zeichnen und Dokumente
speichern. Der Bildimport benötigt hingegen ein Projekt.

## Wiederaufnahme und Sicherheit

Beim nächsten Programmstart wird das zuletzt geöffnete Projekt erneut geöffnet,
sofern es noch erreichbar ist. Unter Linux steht der gemerkte Pfad in
`$XDG_STATE_HOME/penguengine/last-project.txt`, bei nicht gesetztem beziehungsweise
nicht absolutem XDG-Pfad unter `~/.local/state/penguengine/last-project.txt`.
Windows und macOS verwenden den SDL-Benutzerdatenordner für `penguengine`.

Das merkt **nur das Projekt**, keine ungespeicherten Zeichnungen. Beim Wechsel
zwischen Editoren werden Dokumente samt History innerhalb desselben Prozesses
temporär gesichert. Diese temporären Checkpoints sind kein Wiederherstellen nach
Programmende oder Absturz. Speichere wichtige Arbeit als `.pengvec`.
Ein Projektwechsel speichert und schließt offene Zeichnungen nicht automatisch.

## Übungen und Abnahme

### VEC-START-01: Projekt erstellen und erneut öffnen

Lege ein Projekt in einem leeren Testordner an. Kontrolliere die Datei `project.pengui.yaml`. Beende und starte die App anschließend erneut.

**Erwartet:** Der Explorer zeigt wieder denselben Projektordner. Eine neue Sitzung stellt dadurch keine ungespeicherten Dokumente wieder her.

### VEC-START-02: Editorwechsel

Erzeuge in einem Dokument ein Rechteck. Wechsle oben zum Script Editor und zurück zum Vector Editor.

**Erwartet:** Die Zeichnung bleibt in derselben Sitzung erhalten. Undo kann die zuvor bestätigte Rechteckerstellung weiterhin zurücknehmen.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

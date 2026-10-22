# 03 – Dokumente und Dateien

## Neu, öffnen und speichern

Beim ersten Einstieg ohne wiederhergestellte Dokumente entsteht ein Dokument mit
einem Artboard von **1920 × 1080 Pixeln**. Der Button **Dokument hinzufügen**
(Datei mit Plus) legt weitere Dokument-Tabs an. In einem ausdrücklich neu angelegten
Dokument kannst du anschließend selbst Artboards hinzufügen.

Klicke eine `.pengvec`-Datei im Projekt-Explorer an, um sie in einem eigenen Tab zu
öffnen. Ist dieselbe Datei bereits geöffnet, wird ihr vorhandener Tab aktiviert.
Sie wird dadurch nicht erneut geladen; ungespeicherte Änderungen bleiben erhalten.

- **Speichern** in der Shell oder Ctrl+S; unter macOS entsprechend Cmd+S.
- Ohne bisherigen Dateipfad erscheint **Speichern unter**, zunächst im Projektordner.
  Ohne Projekt wird auf den persönlichen Dokumentordner zurückgegriffen.
- **Speichern unter** im Menü beziehungsweise Ctrl/Cmd+Shift+S wählt einen neuen Pfad.
- Ein Stern am Tab kennzeichnet neue oder seit dem Speichern geänderte Dokumente.
  Undo bis zum gespeicherten Inhalt kann den Stern wieder entfernen.

Ein `.pengvec` ist das eigene bearbeitbare Dokumentformat, derzeit Version 9.
Ältere unterstützte Versionen 1–8 werden beim Lesen übernommen. Die Datei enthält
Artboards, Objekte, Stile, Text und Verknüpfungen. Importierte Bilder bleiben
separate Projektdateien. Ein PNG-/JPEG-Export ersetzt diese Dokumentdatei nicht.

## Schließen und History

Das X am Dokument-Tab schließt das Dokument. Bei ungespeicherten Änderungen bietet
die Rückfrage Speichern, Verwerfen oder Abbrechen. Wird ein nötiger Speicherdialog
abgebrochen, bleibt das Dokument offen. Auch der letzte Tab kann geschlossen werden.
Vor dem Beenden der App solltest du wichtige Dokumente ausdrücklich speichern.

Undo/Redo gehört jeweils zum aktiven Dokument. Das Öffnen einer gespeicherten Datei
beginnt mit einer neuen History; die History steht nicht in `.pengvec`.
Zoom, Raster und Vorschau sind Ansichtszustände und werden ebenfalls nicht in dieser
Datei gesichert. Die temporäre Sicherung beim Editorwechsel ist davon getrennt.

## Übungen und Abnahme

### VEC-DOK-01: Speichern und wieder öffnen

Erzeuge ein Rechteck, speichere als `handbuch.pengvec` im Testprojekt und schließe den Tab. Öffne die Datei zweimal nacheinander im Explorer.

**Erwartet:** Das Objekt ist erhalten; es gibt nur einen Tab für diese Datei. Der gespeicherte Tab trägt keinen Änderungsstern.

### VEC-DOK-02: Änderungen und Abbruch

Ändere die Rechteckbreite und prüfe den Stern. Nimm die Änderung zurück. Ändere erneut, schließe den Tab und wähle Abbrechen. Schließe danach mit Verwerfen und öffne die Datei wieder.

**Erwartet:** Undo zum gespeicherten Inhalt entfernt den Stern. Abbrechen lässt den Tab offen. Nach Verwerfen erscheint beim erneuten Öffnen der zuletzt gespeicherte Inhalt.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

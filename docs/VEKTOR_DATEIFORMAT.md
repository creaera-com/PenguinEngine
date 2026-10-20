# Vektordokumente speichern und öffnen

Der Speichern-Button speichert den aktiven Dokument-Tab. Ohne bisherigen Dateipfad
öffnet sich ein nativer **Speichern unter**-Dialog im aktuellen Projektverzeichnis.
Ohne Projekt wird das persönliche Dokumentverzeichnis verwendet. Ein Dateiname
mit der Endung `.pengvec` wird vorgeschlagen. Eine fehlende/andere Endung wird ergänzt;
entsteht dadurch ein bereits vorhandener Zielname, wird der Vorgang abgelehnt und
zum erneuten expliziten Wählen der vollständigen Zieldatei aufgefordert.

- **Strg/Cmd+S:** speichern.
- **Strg/Cmd+Shift+S** oder **Speichern unter** im Hamburger-Menü: anderen Namen wählen.
- **Klick auf eine `.pengvec`-Datei im Explorer:** im Vektoreditor als eigenen Tab öffnen.
  Bei Bedarf wird aus dem Script-/Game-Editor automatisch zum Vektoreditor gewechselt.
- Abbrechen des Dialogs verändert weder Dokument noch Dateipfad.

Nach erfolgreichem Speichern heißt der Tab wie die Datei. Weitere Speichervorgänge
verwenden denselben Pfad. Der Explorer wird aktualisiert. Der Dialog merkt sich
seinen ursprünglichen Dokument-Tab: Ein zwischenzeitlich anderer aktiver Tab wird
nicht versehentlich gespeichert. Bei einem Wechsel des gesamten Editors wird
stattdessen zum erneuten Speichern aufgefordert.

Mehrmaliges Öffnen derselben Datei erzeugt derzeit weitere unabhängige Tabs. Es gilt
die bestehende Grenze von 15 Dokumenten; Änderungen werden nicht automatisch zwischen
diesen Tabs synchronisiert. Ein fehlerhafter Ladevorgang fügt keinen Tab hinzu.

## Inhalt und Speichersicherheit

Gespeichert werden Artboards mit Namen/Geometrie/Zuständen und die geordnete Liste
aller Formen: Rechtecke, Polygone, Ellipsen, freie Pfade/Bézierkurven und Textfelder.
Dazu gehören IDs/Parent- und Gruppenbezüge, Namen, Sichtbarkeit/Sperren, Position,
Größe, Winkel, Füllfarbe, Rahmenfarbe/-breite/-ausrichtung und Textformatierung.
Fontpfade sind Referenzen: Fontdateien selbst werden nicht eingebettet. Fehlende
Fonts nutzen wie bisher die mitgelieferte Schrift als Darstellungs-Fallback.

Viewport, Rastereinstellungen, aktuelle Auswahl, laufende Mausgesten, GPU-Ressourcen
und Undo-/Redo-Verlauf gehören nicht zur Dokumentdatei. Sie werden beim Öffnen
frisch initialisiert. Insbesondere werden unfertige Aufzieh-/Skaliergesten nicht
versehentlich als fertige Geometrie gespeichert.

Dateien werden zuerst vollständig in eine temporäre Geschwisterdatei geschrieben,
geschlossen und erst bei Erfolg auf den Zielnamen umbenannt. Bei Schreib-/Validierungs-
fehlern bleiben die bestehende Zieldatei und die bisherige Dokumentidentität erhalten.
Dies ist kein automatisches Recovery-System und keine Garantie gegen Stromausfall.

## Format `PENGVEC`, Version 3

Das eigene binäre Format ist unabhängig von C-Struct-Padding und nativer Byte-Reihenfolge.
Es gibt keinen SVG-Mapping-Schritt und keine gespeicherten Zeiger.

Grundtypen:

| Typ | Darstellung |
| --- | --- |
| `u32` | 4 Bytes, unsigned, Little Endian |
| `f32` | IEEE-754 binary32, Bits als Little-Endian-u32; nur endliche Werte |
| `string(N)` | u32 Byte-Länge, danach UTF-8-Bytes ohne Nullterminator; Länge kleiner N |
| `point` | x, y als f32 |
| `rect` | x, y, Breite, Höhe als f32 |

Datei-Reihenfolge:

1. Acht Magic-Bytes `50 45 4E 47 56 45 43 00` (`PENGVEC` plus Nullbyte).
2. Version `u32` = 3.
3. Artboardanzahl und Formanzahl als u32, jeweils maximal 64.
4. Pro Artboard: ID, Flags, Name `string(64)`, Rechteck.
5. Pro Form, in Ebenenreihenfolge:
   - ID, Parent-ID, Flags, Gruppen-ID, Maskenrolle, Füllfarbe (je u32).
   - Rahmenfarbe (u32), Rahmenbreite (f32), Rahmenausrichtung (u32).
   - Kurvenkennung (u32), Name `string(64)`, Rechteck, Winkel (f32).
   - Bézier-Knotenanzahl, geschlossen (u32); je Knoten Anker, eingehender und
     ausgehender Kontrollpunkt. Maximal 32 Knoten; Koordinaten sind normalisiert.
   - Pfadpunktanzahl, Konturanzahl (u32); Punkte (max. 128), danach exklusive
     Endindizes jeder Kontur (max. 32).
   - Text aktiviert (u32). Falls aktiviert: Inhalt `string(2048)`, Fontpfad
     `string(512)`, Face-Index, Fett, Kursiv, Ausrichtung, Spalten (u32), dann
     Schriftgröße, Zeilenhöhenfaktor, Zeichenabstand und Spaltenabstand (f32).
   - Bild aktiviert (u32). Falls aktiviert: relativer Projektpfad `string(4096)`,
     Originalbreite/-höhe und Spiegelung X/Y (je u32). Nur `assets/images/<Dateiname>`
     ist zulässig. Der interne aufgelöste Pfad ist auf 1023 UTF-8-Bytes begrenzt.
6. Dateiende; zusätzliche Bytes werden abgelehnt.

Flags und Kennungen verwenden die dokumentierten Modell-Enums: Kurvenkennung
0=frei/Rechteck, 1=Ellipse, 2=parametrisches Polygon; Rahmenausrichtung 0=mittig,
1=innen, 2=außen; Textausrichtung 0=links, 1=zentriert, 2=rechts, 3=Blocksatz.
Farben sind gepackte Werte `0xRRGGBBAA`. Aktive Daten sind maßgeblich; ungenutzte
Array-Puffer werden nicht gespeichert.

Die Maskenrolle ist 0 für normale Inhalte und 1 für das spezielle Masken-Kind einer
Schnittgruppe. Pro Schnittgruppe sind genau ein Masken-Kind und mindestens ein
Inhaltsobjekt zulässig. Alle Kinder teilen dasselbe Artboard. Masken müssen geschlossene
geometrische Formen sein; Textfelder und offene Kurven sind keine Masken.

Der Reader akzeptiert weiterhin Version 1 und 2. Version 1 hat weder Maskenrolle noch
Bildfeld; Version 2 enthält die Maskenrolle, aber kein Bildfeld. Fehlende Werte werden
auf 0 gesetzt. Neue Dateien werden als Version 3 geschrieben.

Bilddateien liegen separat im Projekt unter `assets/images`. Der Reader sucht das
nächste übergeordnete `project.pengui.yaml` als Pfadbasis; außerhalb eines Projekts
gilt der Dokumentordner. Beim Speichern werden benötigte Dateien auch für „Speichern
unter“ ins Zielprojekt kopiert. Bestehende gleichnamige Dateien werden nur wiederverwendet,
wenn der Inhalt identisch ist; sonst wird der Speichervorgang abgelehnt.

Der Reader begrenzt die Dateigröße auf 4 MiB und prüft Version, Kapazitäten,
Geometrie, Stilwerte, IDs und Parent-/Gruppenbezüge vor dem Einsetzen ins Dokument.
Unbekannte Versionen werden mit Fehlermeldung abgelehnt. Künftige strukturelle
Änderungen benötigen eine neue Versionsnummer und gegebenenfalls einen gezielten
Reader für ältere Versionen.

## Fachliche Aufteilung und History

- `apps/demo/documents/controller.c`: app-weite Save-/Save-As- und Explorer-Weiterleitung,
  native Dialoge und Rückgabe ihrer Ergebnisse an den Hauptthread.
- `editor_host/editor.h`: allgemeiner Dateivertrag (`accepts_file`, `file_info`,
  `open_file`, `save_file`). Nur die Registry kennt konkrete Editor-Adapter.
- `editors/vector/workspace/files.c`: Vektordokumente anlegen, Tabs einsetzen und beschriften.
- `editors/vector/persistence/document_file.c`: expliziter Format-Codec und Dateitransaktionen.
- `object/clipboard.c`: validierte Anfangsbelegung als geschützter History-Ausgangszustand.

Geladene Dokumente beginnen mit einer geschützten Basis (`history.floor`). Undo
macht spätere Änderungen rückgängig, aber nicht den Ladevorgang selbst. Der interne
Sitzungs-Checkpoint heißt aktuell **PVCKPT26** und enthält auch den Vorschaubereich. Er erhält
beim Editorwechsel auch den gespeicherten Dateinamen und die späteren Undo-Schritte.
Die dauerhafte Dokumentdatei und dieser private Checkpoint sind getrennte Formate.

Ein App-Neustart öffnet weiterhin zunächst das zuletzt verwendete Projektverzeichnis.
Die zuletzt geöffneten Dokument-Tabs werden noch nicht automatisch wieder geöffnet;
gespeicherte Dokumente lassen sich über den Explorer laden.

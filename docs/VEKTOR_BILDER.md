# PNG und JPEG im Vektoreditor

Ein Projekt öffnen und ein Artboard auswählen. Der **Bild-plus-Button** rechts neben
„Zum Artboard zurück“ öffnet den nativen Dateidialog für PNG, JPG und JPEG.
Das Bild wird mittig im gewählten Artboard platziert und ausgewählt. Es beginnt mit
seiner natürlichen Pixelgröße; größere Bilder werden proportional auf das Artboard
verkleinert. Anschließend lassen sich Position, Größe und Winkel mit Auswahlwerkzeug,
Greifern und Inspector ändern. Shift behält beim Skalieren das Seitenverhältnis bei.

Bilder sind eigene Objekte im Dokumentbaum, unterstützen Sperren/Sichtbarkeit,
Gruppen, Kopieren zwischen Tabs, Spiegeln, Drehen, Löschen und Undo/Redo. Sie können
Inhalt einer Schnittgruppe sein und erscheinen auch in der 1:1-Vorschau. Als
Maskenpfad oder Eingabe für boolesche Pfadoperationen sind Rasterbilder nicht geeignet.
Der Bild-Inspector zeigt nur die passenden Geometrie- und Ebeneneinstellungen.

## Dateien und Pfade

Der Import legt eine eigenständige Kopie in `assets/images` im geöffneten Projekt an.
Der Dateiname wird eindeutig vergeben; das Original bleibt unverändert. Importieren
desselben Bilds erzeugt eine weitere Kopie. Der sichtbare Objektname stammt vom
Original-Dateinamen und lässt sich unabhängig davon ändern.

`.pengvec` speichert **relative Projektpfade**, keine eingebetteten Bildbytes. Beim
Öffnen wird die nächste übergeordnete `project.pengui.yaml` gesucht. Damit lassen
sich Projekt und Asset-Ordner zusammen verschieben. Dokumente ohne übergeordnete
Projektdatei verwenden ihren eigenen Ordner als Pfadbasis. „Speichern unter“ kopiert
benötigte Assets bei Bedarf in die Zielstruktur; gleichnamige andere Dateien werden
nicht überschrieben. Dokument und zugehöriger Asset-Ordner müssen zusammen weitergegeben werden.

Undo oder Löschen entfernt das Bildobjekt, aber nicht die kopierte Datei: Andere
Dokumente oder Undo-Schritte könnten sie noch benötigen. Es gibt noch keine automatische
Bereinigung unbenutzter Assets. Fehlende/nicht lesbare Bilddateien erscheinen als
Platzhalter mit Kreuz. Nach manuellen Dateikorrekturen den Editor neu laden (F6).

Der Dialog merkt sich Dokument, Artboard und Projekt beim Öffnen. Ein zwischenzeitlich
anderer Dokument-Tab wird nicht versehentlich verändert; ein Editor-/Projektwechsel
bricht den Import ab. Ohne geöffnetes Projekt ist kein Import möglich.

## Grenzen und Architektur

- Nur PNG und JPEG; PNG-Transparenz bleibt erhalten. Keine Filter, Farbprofile,
  animierten Bilder oder EXIF-Ausrichtungskorrektur.
- Maximal 32 MiB pro Quelldatei, 8192 Pixel pro Achse und insgesamt 16 Millionen Pixel.
  Aufgelöste interne Bildpfade sind auf 1023 UTF-8-Bytes begrenzt.
- Der editor-eigene Texture-Cache hält höchstens acht Einträge bzw. 64 MiB Pixeltexturen
  und wird beim Entladen des Editors freigegeben. Dekodierung erfolgt beim Import
  bzw. erstmaligen Anzeigen; große Bilder können den UI-Thread kurz beanspruchen.
- `image/state.*`: reine Bildreferenz/Metadaten; keine Pixel, SDL-Objekte oder Heap-Zeiger.
- `image/assets.*`: Projektkopien und Auflösen relativer Asset-Pfade.
- `image/import.*`: validieren, kopieren und über einen Shape-History-Befehl platzieren.
- `image/render.*`: begrenzter Texture-Cache und Rendering mit Rotation/Spiegelung.
- `apps/demo/platform/image_decode.c`: privater PNG-/JPEG-Decoder auf Basis des bereits
  mitgelieferten stb_image; pengUI selbst bleibt unabhängig vom Editor.
- Der App-Dokumentcontroller besitzt den nativen Dialog. Er erreicht den Editor über
  den optionalen Adapter-Vertrag `import_image`; konkrete Editor-Header sind dort unnötig.
- `ui/inspector/image.yaml`: separates, deklaratives Bildformular.

Dauerhafte Dateien verwenden jetzt **PENGVEC Version 5**, weiterhin mit Reader für
Version 1, 2, 3 und 4. Temporäre Sitzungsdaten inklusive History verwenden **PVCKPT28**.

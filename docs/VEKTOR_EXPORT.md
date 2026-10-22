# PNG- und JPEG-Export

Das neue **Export-Symbol** in der Toolbar des Vektoreditors öffnet ein eigenes,
in YAML beschriebenes Exportformular.

## Verwendung

1. **Aktives Artboard**, **Alle Artboards** oder **Aktuelle Auswahl** wählen.
2. PNG oder JPEG auswählen.
3. Zielordner eingeben oder über den Ordner-Button einen nativen Ordnerdialog öffnen.
4. Dateinamen bzw. gemeinsamen Namensanfang eingeben.
5. Bei PNG **Transparenter Hintergrund** ein-/ausschalten. Ohne Transparenz wird
   Weiß verwendet. JPEG besitzt keinen Alpha-Kanal und wird immer auf Weiß ausgegeben.
6. Optional JPEG-Qualität von 1 bis 100 einstellen (Standard: 90).
7. **Exportieren (1:1)** drücken. Die Statuszeile meldet Erfolg oder Fehler.

Die Bildgröße entspricht Dokumentpixeln, unabhängig von Zoom, Fenstergröße oder
Position der Zeichenfläche. Ein 1920 × 1080 Artboard ergibt ein 1920 × 1080 Bild.
Pro Bild gelten maximal 8192 Pixel je Achse und 16 Millionen Pixel insgesamt.

## Bereiche und Dateinamen

- **Aktives Artboard:** exportiert das ganze aktive Artboard nach `Name.png` bzw.
  `Name.jpg`. Die Artboardgrenze beschneidet überstehende Inhalte.
- **Alle Artboards:** exportiert jedes Artboard des aktiven Dokuments einzeln,
  auch wenn das Artboard auf der Zeichenfläche ausgeblendet ist. Ausgeblendete
  Objekte bleiben unsichtbar. Namen folgen `Name-ID-Artboardname.png` bzw. `.jpg`.
  Die stabile ID verhindert Konflikte bei gleich benannten Artboards.
- **Aktuelle Auswahl:** exportiert ausschließlich ausgewählte Objekte/Gruppen,
  begrenzt auf deren gemeinsame geometrische Bildgrenze einschließlich Rahmen,
  Schatten und Wiederholungen. Die Auswahl darf mehrere Artboards umfassen.
  Benötigte Schnittmasken bleiben wirksam; nicht ausgewählte Objekte fehlen.
  Die Auswahl muss mindestens ein sichtbares Objekt enthalten.

Eine ausgewählte Schnittmaske ohne ihre Inhalte wird als eigene Form exportiert.
Bei einer vollständig ausgewählten Schnittgruppe bestimmt deren Inhalt die
Exportgrenze, nicht das unsichtbare Masken-Kind. Die Auswahlgrenzen werden auf
volle Pixel nach außen gerundet. Bei Masken sind es geometrische Begrenzungen;
bei unregelmäßigen Konturen kann transparenter Rand innerhalb dieser Box bleiben.

Dateiendungen werden automatisch ergänzt. Bereits eingegebenes `.png`, `.jpg`
oder `.jpeg` wird dabei entfernt. Nicht zulässige Dateinamenzeichen werden ersetzt.
**Bestehende gleichnamige Dateien werden beim Exportieren ersetzt.** Jede neue
Datei wird zunächst vollständig in eine temporäre Nachbardatei geschrieben und
erst danach umbenannt. Bei einem Fehler im Stapel bleiben bereits erfolgreich
exportierte Artboards erhalten; die Fehlermeldung nennt die Anzahl.

Der Zielordner startet beim ersten Laden mit dem Ordner des bereits gespeicherten
Dokuments, ansonsten mit dem Betriebssystem-Dokumentordner. Die Exportoptionen
gehören zum geöffneten Editor und werden nicht in das Kunstwerk geschrieben.

## Darstellung und Datenfluss

Exportiert wird der bestätigte Dokumentzustand zum Zeitpunkt des Export-Klicks.
Noch laufende Gesten oder Farbvorschauen werden nicht eingebrannt. Raster,
Lineale, Vorschau-Markierungen, Auswahlrahmen, Greifer und Textcursor fehlen.
Füllungen, Verläufe, Rahmen, Text, Bilder, Schnittmasken und Schlagschatten verwenden
dieselben Renderer wie die Zeichenfläche.

- `ui/export/dialog.yaml`: eigenständiges YAML-Formular, über
  `vector_export_toolbar` in die Toolbar eingebunden.
- `ui/export/dialog.c`: liest Optionen, plant den Export und erstellt einen eigenen
  Snapshot aus reinen Dokumentwerten. Der native Ordnerdialog kommuniziert über
  eine unabhängige, mit Mutex und Referenzzählung geschützte Mailbox. Sein Callback
  fasst weder Editor noch Widgets an; er darf auch nach dem Editorwechsel zurückkehren.
- `export/plan.c`: bestimmt Bereiche und Pixelgrenzen ohne SDL, Widgets oder Dateidialoge.
- `export/render.c`: zeichnet den Snapshot bei Zoom 1 in ein transparentes oder weißes
  Renderziel und stellt danach Renderziel, Viewport, Skalierung, Clip, Farbe und
  Blendmodus des Editors wieder her. Für PNG wird premultipliziertes Render-Alpha
  in das vom Dateiformat erwartete gerade Alpha zurückgerechnet.
- `editor_host/images.h`: gemeinsamer Bildcodec-Vertrag.
- `apps/demo/platform/image_encode.c`: PNG-/JPEG-Encoder mit privater
  `stb_image_write`-Implementierung und SDL-Dateistreams, auch für Unicode-Pfade.

Das Zeichnen/Schreiben erfolgt derzeit auf dem Hauptthread, sobald der Renderer
verfügbar ist. Ein großer Stapel kann deshalb die Bedienung vorübergehend blockieren.
Die History, Dateinamen der `.pengvec`-Dokumente und der Gespeichert-Zustand bleiben
unverändert. Für das eigene Dokumentformat und die Checkpoint-Signatur ist keine
Änderung erforderlich.

Der Export-Button öffnet ein wiederverwendbares pengUI-Modal über den Dokument-Tabs.
Die übrige Oberfläche ist währenddessen gesperrt. X oder Escape schließen den Dialog;
ein Klick außerhalb lässt ihn offen. Der native Zielordnerdialog schließt das Modal nicht.
Die Konfiguration liegt in `editors/vector/ui/export/dialog.yaml`; allgemeine
Modal-Eigenschaften und die C-API stehen in [pengUI-Architektur](ARCHITEKTUR.md).

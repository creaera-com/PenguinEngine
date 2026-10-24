# Farbschema-Designer

Rechts neben **Inspector** öffnet **Farbschema** einen eigenständigen Paletten-Designer.
Er verändert keine Objekte und erzeugt keine History-Einträge.

- Im Farbrad ziehen: Winkel bestimmt Farbton, Abstand zur Mitte die Sättigung.
- Die Markierungen zeigen die erzeugten Harmoniefarben.
- Monochromatisch erzeugt Helligkeitsvarianten; benachbarte Farben, Triade,
  Tetrade, Komplementärfarben, geteilte Komplementärfarben und Quadrat variieren den Farbton.
- Winkelabstand (5–90 Grad) beeinflusst benachbarte Farben, Tetrade und geteilte Komplementärfarben.
- Unter Basisfarbe lassen sich HSL, RGBA, Hex und Deckkraft einstellen.
- Ein Farbfeld kopiert seinen Hex-Wert mit Alpha; **Palette kopieren** kopiert alle Werte.

Die Harmonien verwenden den HSL-Farbkreis. Deckkraft bleibt in allen Farben erhalten.
Die Vorschau zeigt die RGB-Farben deckend; kopierte Werte enthalten die eingestellte Deckkraft.
Die Einstellungen gehören zur aktuellen Editor-Instanz und werden beim Entladen zurückgesetzt.
Sie werden derzeit weder im Projekt noch im Dokument gespeichert.

## Aufbau

`editors/vector/palette/` enthält die reine Berechnung ohne Editorzustand.
`editors/vector/ui/palette/` enthält YAML, Darstellung und Eingabeanbindung.
`editor.c` leitet nur Aktionen, Canvas-Aufrufe und Ereignisse weiter.

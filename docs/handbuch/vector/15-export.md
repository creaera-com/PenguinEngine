# 15 – PNG- und JPEG-Export {#vector_manual_15}

Der Exportbutton am rechten Ende der Vector-Toolbar öffnet ein Modal über der
abgedunkelten Zeichenfläche. Export erzeugt ein gerastertes Bild; speichere die
bearbeitbare Zeichnung zusätzlich als `.pengvec`.

## Bereich und Format

1. Wähle **aktives Artboard**, **alle Artboards** oder **aktuelle Auswahl**.
2. Wähle PNG oder JPEG und einen Zielordner sowie Dateinamen.
3. Für PNG kannst du einen transparenten Hintergrund wählen. Ohne Transparenz
   wird weiß hinterlegt. JPEG unterstützt keine Transparenz und verwendet Weiß.
4. Stelle bei JPEG bei Bedarf die Qualität ein; der Standard ist 90.
5. Starte den Export und prüfe anschließend die erzeugte Datei mit einem Bildbetrachter.

Ein Dokumentpixel entspricht einem Ausgabepixel. Der aktuelle Canvas-Zoom verändert
die Ausgabegröße nicht: Ein Artboard 1920×1080 ergibt 1920×1080 Bildpixel.
Alle Artboards werden als getrennte Dateien ausgegeben, auch ausgeblendete Artboards;
ausgeblendete Objekte werden nicht gezeichnet. Bei mehreren Dateien entstehen
unterscheidbare Namen aus Name, ID und Artboardbezeichnung.

Die Auswahl wird nach ihren gemeinsamen Grenzen ausgegeben, einschließlich
relevanter Kontur, Schatten und Wiederholung. Gruppen und Schnittmasken bleiben
in der gerasterten Darstellung berücksichtigt. Griffe, Raster, Lineale,
Auswahlrahmen und Textcursor erscheinen nicht im Bild.

## Grenzen und Dateisicherheit

Die Ausgabe ist auf 8192 Pixel je Achse und 16 Megapixel begrenzt. Ein ungültiger
oder zu großer Bereich kann deshalb nicht exportiert werden. Bestehende
Zieldateien können überschrieben werden: Verwende für die Übungen einen eigenen
Exportordner und eindeutige Namen. Ein Export ist nicht mit Undo rückgängig zu machen.
Exportiert wird der bestätigte Dokumentzustand; beende laufende Eingaben und
Mausgesten vorher. SVG/PDF sind in diesem Arbeitsstand keine Exportformate.

## Übungen und Abnahme

### VEC-EXP-01: Transparentes PNG und JPEG

Erzeuge ein kleines Artboard mit einer halbtransparenten Form. Exportiere es bei 50 Prozent Canvas-Zoom einmal als transparentes PNG und einmal als JPEG in einen leeren Testordner.

**Erwartet:** Beide Dateien haben die Artboard-Pixelgröße. Das PNG hat außerhalb der Form Transparenz, JPEG einen weißen Hintergrund. Bearbeitungshilfen fehlen.

### VEC-EXP-02: Alle Artboards und Auswahl

Lege zwei Artboards mit verschiedenen Namen an. Exportiere alle Artboards. Wähle danach nur ein Objekt und exportiere die aktuelle Auswahl unter einem neuen Namen.

**Erwartet:** Es entstehen getrennte Artboardbilder. Das Auswahlbild umfasst die ausgewählten Inhalte statt des gesamten Artboards.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

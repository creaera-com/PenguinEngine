# 07 – Pfade und Bézierkurven

## Kurve zeichnen

1. Aktiviere das Stiftwerkzeug.
2. Ein Klick setzt einen geraden Anker. Klicken und Ziehen setzt einen Anker mit Tangenten.
3. Setze weitere Anker; ziehe die Tangenten in die gewünschte Richtung.
4. Enter beendet einen offenen Pfad. Ein Klick auf den ersten Anker schließt den Pfad.
5. Esc verwirft den noch nicht abgeschlossenen Entwurf.

Shift beschränkt die Richtung beim Zeichnen. Mit Alt lassen sich Tangenten unabhängig
statt gekoppelt behandeln. Ein offener Pfad hat keine geschlossene Füllfläche.

## Nachbearbeitung

Wechsle zum Knotenwerkzeug und wähle den Pfad. Verschiebe Anker oder ihre Tangentengriffe.
Ein Doppelklick auf ein Segment fügt einen Knoten ein. Delete/Backspace entfernt
im Knotenmodus den ausgewählten Knoten; außerhalb dieses Modus betrifft Löschen
normalerweise das ausgewählte Objekt. Prüfe deshalb zuerst das aktive Werkzeug.

Über **Flaechen → In Pfad** im radialen Menü wandelst du geeignete parametrische
Formen um. Danach bearbeitest du Geometrie statt Rechteckradius, Ellipsensegment
oder Polygon-Eckenzahl. Undo stellt die ursprüngliche Form wieder her.
**Kontur zu Pfad** ist eine andere Aktion: Sie macht aus der Kontur eine gefüllte
Bandfläche. Das ist zum Beispiel nötig, wenn die Kontur selbst an einer
Flächenoperation teilnehmen soll.

Die erste Version begrenzt Kurven auf 32 Bézierknoten. Komplexe Flächenoperationen
können Kurven in angenäherte Konturen überführen; sie sind kein verlustfreier
Austausch mit beliebigen SVG-Pfaden. Text kann derzeit nicht in Konturpfade umgewandelt werden.

## Übungen und Abnahme

### VEC-PFAD-01: Kurve bearbeiten

Zeichne mit mindestens einem gezogenen Anker eine geschlossene Kurve. Wechsle zum Knotenwerkzeug, verschiebe eine Tangente und füge per Doppelklick auf ein Segment einen Knoten hinzu. Nutze Undo.

**Erwartet:** Die Kurve folgt der Tangente. Der zusätzliche Knoten wird mit Undo entfernt, ohne die vorherige Kurve zu verlieren.

### VEC-PFAD-02: Form umwandeln

Erzeuge ein Polygon. Wandle es über das Kontextmenü in einen Pfad um, bearbeite einen Knoten und nimm beide Änderungen wieder zurück.

**Erwartet:** Nach der Umwandlung sind Knoten verfügbar; nach Undo der Umwandlung ist wieder die Polygon-Eckenzahl einstellbar.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

# 06 – Rechtecke, Ellipsen und Polygone {#vector_manual_06}

Aktiviere das entsprechende Formsymbol und ziehe die gewünschte Form innerhalb
eines Artboards auf. Beim Rechteck bleibt das Werkzeug für weitere Rechtecke aktiv;
wechsle zum Zeiger, um die Formen zu bearbeiten. Der Inspector zeigt die Parameter
der ausgewählten Form. Sichtbarkeit und Sperre können ihre Bearbeitung verhindern.

## Rechteck

Neben Position und Abmessungen kannst du den Eckenradius in Pixeln einstellen.
Der Radius ist durch die halbe kürzere Seite begrenzt. Die Form bleibt ein Rechteck
mit bearbeitbarem Radius, bis sie in einen Pfad umgewandelt wird.

## Ellipse und Segmente

Der Abschnitt **Ellipsensegment** steht am Ende des Ellipsen-Inspectors. Neben der
vollen Ellipse gibt es einen offenen Bogen, ein geschlossenes Tortenstück und einen
Kreisabschnitt mit gerader Verbindung der Endpunkte. Start- und Endwinkel bestimmen
den Ausschnitt; null Grad liegt rechts, positive Winkel laufen im Uhrzeigersinn.
Die Einstellung erlaubt auch negative Winkel. Ein vollständiger Umlauf und ein
Teilsegment sind bewusst verschieden.

Ein offener Bogen hat keine Fläche zum Füllen. Falls bisher keine Kontur vorhanden
war, wird für die Darstellung eine schmale Kontur verwendet. Tortenstück und Abschnitt
können dagegen gefüllt werden. Eine Objektdrehung dreht das gesamte Ergebnis und
verändert nicht seine Segmentparameter.

## Polygon

Das Polygonwerkzeug erzeugt ein regelmäßiges Polygon im aufgezogenen Bereich.
Im Inspector lässt sich die Eckenzahl zwischen 3 und 32 ändern. Nach **In Pfad**
sind die Knoten einzeln bearbeitbar; die parametrische Eckenzahl entfällt.

## Übungen und Abnahme

### VEC-FORM-01: Parametrische Formen

Erzeuge ein Rechteck mit Radius 20, eine Ellipse und ein Polygon. Stelle beim Polygon fünf Ecken ein. Wechsle bei der Ellipse zwischen Bogen, Tortenstück und Abschnitt mit Start 0 und Ende 120 Grad. Speichere und öffne die Datei erneut.

**Erwartet:** Rundung, Eckenzahl und Segmenttyp bleiben erhalten. Der offene Bogen hat keine gefüllte Innenfläche; die beiden geschlossenen Varianten schon.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

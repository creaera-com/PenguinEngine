# 08 – Füllung, Kontur, Verlauf und Schatten

## Einfarbige Gestaltung

Die Farbfelder im Inspector zeigen die aktuelle Farbe. Anklicken öffnet den
Farbdialog mit HSL-, RGBA- und Hex-Eingabe; Alpha bestimmt die Deckkraft.
Ein Schachbrett macht Transparenz erkennbar. Eine geschlossene Form oder ein
 geschlossener Pfad kann eine Füllung erhalten. Offene Bögen besitzen keine Innenfläche.

Für die Kontur stellst du Farbe und Breite ein. Breite null deaktiviert sie.
Die Buttongruppe wählt **innen**, **zentriert** oder **außen** relativ zum Pfadrand.
Konturbreite ist eine Dokumentgröße, nicht die sichtbare Anzahl Bildschirmpixel bei jedem Zoom.

## Verläufe

Im Füllungsbereich wählst du einfarbig, linear, radial, konisch oder reflektiert.
Ein Verlauf besitzt zwei bis acht geordnete Farbstopps. Der erste/letzte liegen bei
0/100 Prozent; weitere Stopps geben Zwischenfarben vor. Wähle einen Stopp, ändere
Farbe/Deckkraft und Position. Hinzufügen ergänzt einen Stopp, Entfernen lässt
mindestens zwei übrig. Die Umkehrfunktion vertauscht den Farbverlauf.

Das Verlaufswerkzeug in der Toolbar macht die Verlaufspunkte auf der Zeichenfläche
bearbeitbar. Ziehe Start/Ende und Stopps; bestätige durch Loslassen. Esc oder
Rechtsklick bricht die laufende Änderung ab. Text verwendet in dieser Version eine
einfarbige Schriftfarbe; ein Verlauf ist kein allgemeiner Filter für Bildpixel.

## Schatten

Aktiviere im Effektbereich den Schatten einer Ebene. Stelle Schattenfarbe,
Versatz X/Y, Weichheit und Ausdehnung ein. Der Schatten gehört zum Objekt und
wird mit gespeichert und exportiert. Er ist von dem dezenten UI-Schatten des
Artboards zu unterscheiden. Große Weichheit und viele Objekte können die Darstellung verlangsamen.

## Farbschema

Der eigene Tab **Farbschema** unterstützt monochromatische, benachbarte, triadische,
tetradische, komplementäre, geteilte komplementäre und quadratische Schemata.
Am Rad wählst du Farbton und Sättigung; die weiteren Felder beeinflussen das Schema.
Ein Klick auf ein Farbfeld kopiert seinen Hex-Wert, die Kopierfunktion die Palette.
Füge den Wert anschließend in einem Objekt-Farbdialog ein. Das Schema färbt die
Auswahl nicht automatisch. Es ist ein Werkzeugzustand, kein gespeichertes Dokumentelement.

## Übungen und Abnahme

### VEC-STIL-01: Kontur und Verlauf

Gib einem Rechteck eine rote Füllung und eine blaue Kontur mit Breite 10. Vergleiche innen, zentriert und außen. Wähle einen linearen Verlauf, ergänze einen dritten Stopp und ziehe seine Griffe mit dem Verlaufswerkzeug.

**Erwartet:** Die Kontur liegt entsprechend der Wahl zum Pfadrand. Zwischenfarben und Verlaufsausrichtung sind sichtbar; Undo nimmt die letzte bestätigte Änderung zurück.

### VEC-STIL-02: Schatten speichern

Aktiviere einen Schatten mit gut sichtbarem Versatz. Speichere, schließe und öffne die Datei erneut.

**Erwartet:** Farbe, Versatz und Weichheit des Schattens sind erhalten.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

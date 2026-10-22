# 14 – Raster, Einrasten und numerische Eingabe

## Raster und Magnet sind unabhängig

Der Gitter-Button zeigt nur das Raster. Erst der Magnet-Button aktiviert das
Einrasten daran. Im Tab **Raster** stellst du rechtwinkliges Standardraster,
isometrisches Raster oder hexagonales Raster sowie Breite, Höhe, Farbe und
Einrasttoleranz ein. Die Ausgangswerte sind 16 × 16 Pixel und eine Toleranz von 5 Pixeln.
Du kannst das Raster sichtbar lassen und Einrasten ausschalten, oder umgekehrt.
Teste das Einrasten zunächst mit einem ungedrehten Rechteck.

Ein separater Button für **Element-Einrasten** unterstützt das Ausrichten an anderen
Objekten: gestrichelte Hilfslinien zeigen übereinstimmende Kanten/Mittelpunkte und
gleiche Zwischenräume. Das sind keine gespeicherten Zeichenobjekte. Die Rasterwahl
und die Anzeigehilfen gehören zum Ansichtszustand, nicht zum exportierten Artwork.

## Werte direkt bearbeiten

Klicke in ein numerisches Inspector-Feld, tippe den Wert und bestätige mit Enter
oder durch Tab in das nächste Feld. Ctrl/Cmd+A markiert den ganzen Feldinhalt;
Doppelklick markiert ein Wort beziehungsweise den entsprechenden Textabschnitt.
Mit der Maus kannst du auch einen Teil markieren. Verwende für Dezimalzahlen im
Zweifelsfall einen Punkt als Dezimaltrenner.

Numerische Felder unterstützen Rechenausdrücke mit Addition, Subtraktion,
Multiplikation, Division und Klammern. Relative Änderungen verwendest du mit `+=`, `-=`, `*=` oder `/=`: Bei Breite 100
ergibt `+=20` den Wert 120. `100+20` berechnet ebenfalls 120 als absoluten Ausdruck.
Ein bloßes `+20` ist dagegen der absolute Wert 20. Negative Werte sind nur bei
geeigneten Eigenschaften wie Position oder Versatz zulässig.

## Zahlen mit der Maus verändern

Halte die linke Maustaste in einem geeigneten numerischen Feld **0,5 Sekunden**
gedrückt. Nach dem sichtbaren Moduswechsel erhöht eine Bewegung nach rechts den
Wert, nach links verringert sie ihn; ungefähr 6 UI-Pixel entsprechen einem Schritt
von 1. Vorhandene Nachkommastellen bleiben dabei erhalten.

Im Objekt-Inspector zeigt die Zeichenfläche die laufende Änderung direkt an.
Erst das Loslassen bestätigt die Änderung als einen History-Schritt. Esc bricht
sie ab. Das gilt für unterstützte numerische Felder, nicht pauschal für jedes Feld,
das zufällig eine Ziffer enthält. Die Vorschau verändert während des Ziehens noch
nicht das dauerhaft bestätigte Dokument.

## Übungen und Abnahme

### VEC-SNAP-01: Raster einrasten

Stelle ein rechtwinkliges Raster 16×16 ein. Schalte zunächst nur das Gitter ein und verschiebe ein Rechteck. Aktiviere dann zusätzlich den Magneten und bewege es langsam nahe eine Rasterposition.

**Erwartet:** Nur das sichtbare Gitter erzwingt kein Einrasten. Mit aktivem Magneten wird innerhalb der Toleranz eingerastet.

### VEC-NUM-01: Rechnen und Live-Vorschau

Setze die Rechteckbreite auf 100, gib danach `+=20` ein und verlasse das Feld mit Tab. Halte anschließend das Breitenfeld 0,5 Sekunden gedrückt und ziehe nach rechts. Lass los und nutze Undo; wiederhole den Zug mit Esc.

**Erwartet:** Die Breite wird zunächst 120. Während des Ziehens ändert sich das Artwork live. Ein Undo nimmt den gesamten bestätigten Zug zurück; Esc verwirft den zweiten Zug.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

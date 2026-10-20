# Klonen / Wiederholen

Im rechten **Tools**-Tab den Abschnitt **Klonen / Wiederholen** öffnen und ein einzelnes,
freies Objekt auswählen. Der Name des Ausgangsobjekts erscheint im Formular.
Das Tools-Panel lässt sich mit dem Mausrad oder seiner Scrollbar vertikal scrollen;
Vorschau und Raster bleiben unabhängig zuklappbar.

1. Modus **Verknüpft** oder **Unabhängige Kopien** wählen.
2. Spalten, Zeilen und den Abstand X/Y in Dokumentpixeln einstellen.
3. Im Eigenschaftsmenü die gewünschte Änderung auswählen und ihren Schritt je Spalte
   und Zeile eingeben. Alle Eigenschaften haben eigene, gleichzeitig wirksame Werte;
   ein Wechsel der angezeigten Eigenschaft setzt die anderen Schritte nicht zurück.
4. **Anwenden** bestätigt die gesamte Konfiguration in einem Undo-Schritt.

**5 Spalten × 4 Zeilen bedeutet 20 Exemplare einschließlich des Originals.**
Für eine fortlaufende Drehung von 0° bis 19° bei fünf Spalten: Winkel auswählen,
Spaltenschritt **1**, Zeilenschritt **5**. Für jeweils gleich gedrehte Zeilen:
Spaltenschritt **0**, Zeilenschritt zum Beispiel **10**.

## Beide Modi

**Verknüpft:** Das Ausgangsobjekt speichert nur eine Wiederholungsbeschreibung.
Es bleibt genau ein echtes Objekt; zusätzliche Zellen werden aus den aktuellen
Werten gezeichnet. Änderungen an Geometrie, Pfaden, Text, Füllung oder Rahmen des
Originals wirken auf alle Zellen. Der Baum kennzeichnet die Ebene etwa als
`Original [5x4]`. Ein Klick auf ein gezeichnetes Exemplar wählt das Original.
Greifer und Inspector bearbeiten dessen Geometrie, nicht einzelne virtuelle Zellen.
Verschieben des Originals verschiebt das gesamte Raster.

**Unabhängige Kopien:** Es entstehen echte Objekte mit eigenen IDs. Das Original
bleibt in der ersten Zelle erhalten. Änderungen an einer Kopie verändern die anderen
nicht. Die neuen Objekte werden gemeinsam ausgewählt. **Lösen** wandelt eine bereits
verknüpfte Wiederholung mit ihren gespeicherten Einstellungen in solche Objekte um.
**Wiederholung entfernen** behält nur das Original. Beide Aktionen sind rückgängig machbar.

## Berechnung der Schritte

Für Spalte `c` und Zeile `r`, jeweils ab 0, gilt:

`Eigenschaft = Originalwert + c × Spaltenschritt + r × Zeilenschritt`

Unterstützt werden:

- Breite und Höhe in Pixeln, Winkel in Grad, Rahmenbreite in Pixeln.
- Füll- und Rahmenfarbton in Grad.
- Füll- und Rahmensättigung, Helligkeit und Deckkraft in **Prozentpunkten**.
  Beispiel: Deckkraft 100 % und Schritt −10 ergibt 100 %, 90 %, 80 % usw.

Farbton und Winkel laufen zyklisch weiter. Sättigung/Helligkeit/Deckkraft werden auf
0–100 % begrenzt; Breite/Höhe auf mindestens 1 px, Rahmenbreite auf mindestens 0 px.
Die vorhandenen maximalen Geometriegrenzen gelten ebenfalls. Das Original bleibt unverändert.
Farbänderungen gelten für die bestehenden Vektor-/Textfüllungen und Rahmen; sie sind
keine Farbfilter für importierte Rasterbilder.

Die Rasterpositionen verwenden feste Abstände auf den Artboardachsen:

`x = Original.x + c × (Originalbreite + Abstand X)`

`y = Original.y + r × (Originalhöhe + Abstand Y)`

Schrittweise Größenänderungen ändern diesen Rasterabstand nicht; wachsende Exemplare
können sich dadurch überlappen. Wird die Originalgröße geändert, ändert sich der
Rasterabstand entsprechend. Drehungen erfolgen um die Mitte jeder einzelnen Zelle.
Es gibt kein automatisch kumuliertes Anwachsen der Spaltenabstände.

## Speicherung und Grenzen

Bestätigte verknüpfte Einstellungen gehören zum Objekt: Sie bleiben bei Kopieren,
Duplizieren, Speichern/Laden und temporärem Entladen des Editors erhalten. Eine
normale Duplizierung eines solchen Originals erzeugt eine neue, unabhängige Quelle
mit derselben Wiederholungsbeschreibung. Noch nicht angewendete Formularwerte sind
UI-Entwürfe und werden nicht in die Dokumentdatei geschrieben.

Die erste Version unterstützt einzelne unabhängige Objekte, keine Gruppe als
gemeinsame Quelle. Bereits gruppierte Objekte vor dem Konfigurieren entgruppieren.
Rasterbilder und Text können wiederholt werden. Schnittmasken und boolesche
Flächenoperationen auf einer Wiederholung benötigen zunächst **Lösen**.

- Je Achse höchstens 64, je Quelle insgesamt höchstens 1024 verknüpfte Zellen.
- Je Dokument höchstens 4096 berechnete Exemplare einschließlich normaler Objekte.
- Für unabhängige Kopien gilt die bestehende Grenze von 64 echten Objekten im Dokument.
  Ein zu großes Raster wird atomar abgelehnt; das Original bleibt erhalten.
- Sichtbarkeit und Sperren des Originals gelten für alle virtuellen Zellen.
- Die Zeichenfläche und 1:1-Vorschau verwenden dieselbe Ableitung, auch bei Live-Transformationen.

Das Dateiformat ist **PENGVEC Version 5** mit Reader für Version 1–4. Der temporäre
Checkpoint heißt **PVCKPT28**. Bereits gespeicherte ältere Dokumente bleiben lesbar.

## Architektur

- `repeat/state.*`: reine Werte und Validierung; stabile Enum-Reihenfolge der Schritte.
- `repeat/repeat.*`: Ableitung einer Zelle und atomare Apply-/Lösen-/Entfernen-Befehle.
  Verknüpfungen verwenden die vorhandenen Shape-History-Befehle, unabhängige Kopien
  die strukturelle History. Modell und History kennen keine UI oder SDL-Eingaben.
- `repeat/render.*`: übernimmt Live-Geometrie/Farbvorschauen und übergibt kurzlebige
  Zellenwerte an die bestehenden Shape-Renderer. Kein zusätzliches Dokumentmodell.
- `ui/tools/repeat.*`: Formularentwurf und Übersetzung von YAML-Aktionen ins Modell.
- `ui/tools/repeat.yaml`: separat geladener, deklarativer Abschnitt im Tools-Panel.
- `object/shape.c`: Trefferprüfung virtueller Zellen ohne neue Entity-IDs.
- `persistence/document_file.c`: explizite, plattformunabhängige Wiederholungsfelder.

pengUI ergänzt hierfür allgemein wiederverwendbares `scroll: true` auf Panels.
Die feste Layoutkapazität wurde für die gleichzeitig montierten Werkzeuge und den
Text-Inspector von 256 auf 512 Knoten erhöht.

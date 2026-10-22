# 05 – Auswahl und Transformation

Aktiviere den Zeiger in der Vector-Toolbar. Klicke ein Objekt an, um es auszuwählen.
Shift+Klick ergänzt beziehungsweise entfernt ein Objekt aus der Auswahl. Ziehe von
einem freien Bereich aus einen Auswahlrahmen auf, um mehrere Objekte zu erfassen.
Achte darauf, nicht versehentlich ein vorhandenes Objekt zu greifen.

Ziehe ein ausgewähltes Objekt an seiner Fläche, um es zu bewegen. Eine Mehrfachauswahl
bekommt gemeinsame Griffe. Eck- und Kantengriffe skalieren die Auswahl; der runde
Drehgriff dreht sie. Die Vorschau erfolgt während der Mausbewegung, die bestätigte
Transformation zählt nach dem Loslassen als eine Undo-Aktion. Esc bricht eine laufende
Geste ab. Gesperrte Objekte und Kinder gesperrter Artboards sind nicht frei bearbeitbar.

## Zusatztasten

| Geste | Zusatztaste | Wirkung |
|---|---|---|
| Form aufziehen / skalieren | Shift | Seitenverhältnis festhalten beziehungsweise gleichmäßige Form |
| Form aufziehen / skalieren | Alt | Vom Mittelpunkt aus verändern |
| Form aufziehen / skalieren | Shift+Alt | Beide Einschränkungen kombinieren |
| Drehen | Shift | In Schritten von 15 Grad drehen |
| Auswahl anklicken | Shift | Zur Auswahl hinzufügen / daraus entfernen |

Die Winkelanzeige verwendet Grad. Drehung ist von den Start-/Endwinkeln eines
Ellipsensegments unabhängig. Im radialen Menü unter **Transformieren** kannst du
horizontal oder vertikal spiegeln sowie um 90 Grad nach links/rechts drehen.
Verknüpfte Instanzen müssen vor nicht unterstützten Spiegelungen entkoppelt werden.

## Übungen und Abnahme

### VEC-AUS-01: Mehrfachauswahl und History

Erzeuge zwei Rechtecke, wähle beide mit Shift aus und verschiebe sie gemeinsam. Skaliere über einen Eckgriff, drehe am runden Griff mit Shift und führe dreimal Undo aus.

**Erwartet:** Beide Objekte verändern sich zusammen. Die Drehung rastet in 15-Grad-Schritten. Jede abgeschlossene Geste lässt sich einzeln zurücknehmen.

### VEC-AUS-02: Abbrechen und Mittelpunkt

Beginne eine Skalierung und drücke vor dem Loslassen Esc. Ziehe anschließend mit Alt eine neue Form auf; wiederhole mit Shift+Alt.

**Erwartet:** Die abgebrochene Skalierung bleibt nicht im Dokument. Alt hält den Ausgangspunkt als Mitte; Shift+Alt erzeugt eine gleichmäßig proportionierte Form.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

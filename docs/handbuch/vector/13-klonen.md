# 13 – Wiederholungen und verknüpfte Instanzen {#vector_manual_13}

Es gibt zwei verwandte Werkzeuge: ein regelmäßiges Wiederholungsraster im Tab
**Klonen** und frei platzierbare verknüpfte Instanzen im Kontextmenü.

## Rasterförmig wiederholen

Wähle ein einzelnes freies Objekt und öffne **Klonen**. Spalten und Zeilen geben
die Gesamtzahl einschließlich Original vor: **5 × 4 ergibt 20 Elemente**.
Abstand X/Y bezeichnet zusätzliche Lücken zur ursprünglichen Objektgröße, nicht
den Abstand von Mittelpunkt zu Mittelpunkt. Anwenden bestätigt den gesamten Vorgang.

- **Entkoppelt:** Es entstehen selbständige Objekte, anschließend einzeln bearbeitbar.
- **Verknüpft:** Die Wiederholung bleibt am Original. Zusätzliche Zellen werden
  gezeichnet, sind aber keine einzelnen Tree-Objekte. Anklicken wählt die Quelle.

Pro Eigenschaft können Schritte je Spalte und je Zeile eingestellt werden, etwa
Größe, Konturbreite, Farbe oder Winkel. Die Wirkung ist additiv:
`Wert = Original + Spalte × Spaltenschritt + Zeile × Zeilenschritt`.
Für Winkel 0 bis 19 Grad in einem 5×4-Raster verwendest du 1 Grad pro Spalte und
5 Grad pro Zeile. Farb- und Größenwerte werden auf gültige Bereiche begrenzt.
Bei wachsenden Objekten können Überlappungen entstehen, weil die Zellabstände von
der ursprünglichen Größe ausgehen.

**Loesen** materialisiert eine verknüpfte Wiederholung in unabhängige Objekte.
**Wiederholung entfernen** lässt das Original übrig. Die Wiederholungsparameter
werden mit dem Dokument gespeichert und gehören zur History.

## Frei platzierbare Instanzen

Wähle ein Objekt beziehungsweise eine Auswahl und nutze im radialen Menü
**Kopien → Verknuepfte Instanz**. Die Instanz hat eigene Position, Drehung, Größe,
Name, Sichtbarkeit und Sperre. Ihre Form und Gestaltung folgen dem Original.
Ändere die Quelle, um die verknüpften Darstellungen zu ändern. **Instanz entkoppeln**
macht die ausgewählte Instanz unabhängig. Wird die Quelle gelöscht, erhalten
verbleibende Instanzen ihren aktuellen Inhalt als selbständige Werte; Undo kann
die Verknüpfung wiederherstellen. Spiegelungen können vorheriges Entkoppeln erfordern.

## Übungen und Abnahme

### VEC-KLON-01: 20 Wiederholungen

Erzeuge ein kleines Rechteck, wähle im Klonen-Tab 5 Spalten und 4 Zeilen mit sichtbarem Abstand. Nutze verknüpft und ändere danach die Originalfüllung. Teste Loesen und Undo.

**Erwartet:** 20 Darstellungen einschließlich Original sind sichtbar. Die Füllung folgt der Quelle; nach Lösen sind unabhängige Objekte vorhanden.

### VEC-INST-01: Freie Instanz

Erzeuge über Kopien eine verknüpfte Instanz und verschiebe/drehe sie. Ändere die Füllung des Originals. Entkopple dann die Instanz und ändere erneut das Original.

**Erwartet:** Vor dem Entkoppeln folgt die Farbe der Quelle, während Position/Drehung unabhängig bleiben. Danach bleibt die Instanzfarbe unverändert.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

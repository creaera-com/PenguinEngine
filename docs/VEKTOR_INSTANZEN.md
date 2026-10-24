# Verknüpfte Instanzen

Eine Form oder mehrere Formen auswählen, per Rechtsklick das radiale Menü öffnen
und **Kopien → Verknüpfte Instanz** wählen. Pro ausgewählter Form entsteht ein
selektierbarer Eintrag mit Link-Icon im Dokumentbaum. Gruppen behalten ihre Struktur.
Die gesamte Erzeugung ist ein einziger Undo-Schritt.

Instanzen besitzen eigene Position, Drehung, Größe, Namen, Sichtbarkeit und Sperre.
Die Größe skaliert relativ zur Quellform. Formkontur, Füllung, Verlauf, Rahmen,
Eckenradius, Text/Bild und Schatten werden vom Original gelesen. Gestaltungsänderungen
werden am Original vorgenommen. Die eigene Drehung und Position folgen dem Original
nicht; mehrere ausgewählte Originale erzeugen jeweils eine eigene Instanz.

**Kopien → Instanz entkoppeln** übernimmt das aktuelle Aussehen und entfernt die
Verknüpfung. Danach lässt sich die Gestaltung unabhängig ändern. Auch das ist rückgängig
machbar. Beim Löschen des Originals werden verbleibende Instanzen mit ihrem aktuellen
Aussehen entkoppelt; Undo stellt die vorherigen Beziehungen wieder her.

Verknüpfungen gelten innerhalb eines Dokuments. Beim Einfügen in einen anderen Tab
werden mitkopierte Quellobjekte auf die neuen IDs umgebogen. Fehlt die Quelle in der
Zwischenablage, wird die Instanz als unabhängige Kopie des aktuellen Aussehens eingefügt.

## Architektur und Persistenz

`editors/vector/instance/` enthält referenzbasierte Auflösung und Befehle. Die Auflösung
schreibt in einen vom Aufrufer bereitgestellten Wert und verändert niemals Dokument
oder History. Dadurch bleiben Undo/Redo-Snapshots stabil. Canvas, Vorschau, Export,
Hit-Tests, Inspector und Transform-Geometrie lesen dieselben aufgelösten Werte.

`instance_source` referenziert eine stabile Form-ID. Basisdimensionen und das eigene
Rechteck beschreiben die relative Skalierung. Die bestehende feste Formstruktur hält
zusätzlich eine Fallback-Darstellung; eine Instanz besitzt keine unabhängig editierbare
Gestaltung. `.pengvec` Version 8 speichert Referenz und Basisdimensionen explizit;
Versionen 1–7 bleiben lesbar. Der private Session-Checkpoint besitzt eine neue Signatur
und speichert auch die History. Quellen werden nach dem Laden über IDs aufgelöst,
nicht über gespeicherte Speicheradressen.

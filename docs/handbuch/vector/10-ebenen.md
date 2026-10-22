# 10 – Explorer, Ebenen und Gruppen

## Projekt-Explorer

Der Explorer zeigt Verzeichnisse und Dateien des Projekts; Ordner stehen vor Dateien.
Öffne Ordner über ihre Tree-Zeilen. **Aktualisieren** liest externe Änderungen neu ein.
Das Suchfeld sucht nach Dateinamen im Projekt, auch in geschlossenen Ordnern.
Die Suche arbeitet schrittweise im Hintergrund der laufenden Oberfläche; bei großen
Projekten können Treffer zeitversetzt erscheinen. Es ist keine Volltextsuche in Dateiinhalten.
Bei großen Verzeichnissen gibt es weitere Seiten statt einer unbegrenzten Liste.

Über den Ordner-plus-Button öffnest du **Neuer Ordner**, gibst einen einfachen
Ordnernamen ein und klickst **Anlegen**. Ein ausgewählter Ordner wird zum Ziel, bei einer ausgewählten Datei deren
übergeordneter Ordner. Ohne passende Auswahl wird das Projektverzeichnis verwendet.
Prüfe die Rückmeldung und den entstandenen Pfad. Die Ordneranlage ist
keine rückgängig machbare Änderung der Zeichnung.

## Document-Tree

Artboards sind die oberste Ebene. Darunter erscheinen Objekte und Gruppen.
Das Suchfeld hilft beim Filtern nach Namen. Anklicken wählt das zugehörige Element
und zeigt seine Einstellungen im Inspector. Auge und Schloss stehen am Anfang:
Das Auge blendet aus/ein, das Schloss verhindert Bearbeitung. Die entsprechenden
Inspector-Einstellungen wirken auf denselben Zustand. Ausgeblendete Objekte werden
auch beim Export nicht gezeichnet. Elternzustände gelten für ihre Kinder.

Das radiale Menü **Ebenen** verschiebt ausgewählte Elemente ganz nach oben,
eine Ebene hoch, ganz nach unten oder eine Ebene herunter. Das verändert ihre
Zeichenreihenfolge innerhalb des jeweiligen Elternkontexts, nicht den Artboardbesitz.

Mit **Gruppieren** fasst du eine Auswahl zusammen; **Gruppe aufloesen** trennt sie
wieder. Gruppen ermöglichen gemeinsame Auswahl und Transformation. Die aktuelle
Version hat flache Gruppen, keine beliebig verschachtelte Gruppenhierarchie.
Schnittgruppen haben zusätzlich eine Maskenfunktion; dazu siehe Kapitel 12.

## Übungen und Abnahme

### VEC-TREE-01: Sichtbarkeit und Sperre

Benenne zwei Objekte unterschiedlich. Filtere im Document-Tree nach einem Namen und lösche den Filter wieder. Blende ein Objekt per Auge aus, dann ein; sperre es und versuche es zu verschieben.

**Erwartet:** Die Filterung ändert nicht die Zeichnung. Das versteckte Objekt verschwindet, das gesperrte lässt sich nicht verschieben. Nach Entsperren ist es wieder bearbeitbar.

### VEC-TREE-02: Gruppieren und Reihenfolge

Lege zwei überlappende Formen an. Ändere über Ebenen ihre Reihenfolge. Wähle beide, gruppiere sie und verschiebe die Gruppe. Löse sie wieder auf und teste Undo.

**Erwartet:** Die Überdeckung folgt der Reihenfolge. Die Gruppe bewegt ihre Kinder gemeinsam; Auflösen und Gruppieren sind rückgängig machbar.

### VEC-DIR-01: Ordner erstellen

Wähle im Explorer den gewünschten Zielordner, öffne Ordner plus und lege `Tests` an. Prüfe den Pfad mit deinem Dateimanager und aktualisiere den Explorer.

**Erwartet:** Der neue Ordner liegt im beabsichtigten Projektverzeichnis und erscheint im Explorer.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

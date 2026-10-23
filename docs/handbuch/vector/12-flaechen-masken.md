# 12 – Flächenoperationen und Schnittmasken {#vector_manual_12}

## Flächen kombinieren

Wähle überlappende geeignete Formen und öffne **Flaechen** im radialen Menü:

| Aktion | Ergebnis |
|---|---|
| Vereinigen | Gemeinsame Außenfläche |
| Subtrahieren | Obere ausgewählte Flächen werden aus der unteren entfernt |
| Schnittmenge | Nur gemeinsam überdeckter Bereich |
| XOR | Überdeckungsbereiche werden ausgespart |
| Aufteilen | Getrennte Teilflächen entlang der Überschneidungen |

Bei diesen Operationen entsteht Pfadgeometrie; parametrische Formeinstellungen
gehen dabei verloren. Die Ausgangsgestaltung orientiert sich an der unteren
Ausgangsform. Prüfe bei Subtraktion deshalb zuerst die Ebenenreihenfolge.
Eine Kontur ist nicht automatisch zusätzliche Füllfläche: Wandle sie bei Bedarf
vorher mit **Kontur zu Pfad** um. Text und Bilder sind keine geometrischen Operanden.
Übermäßig komplexe Ergebnisse werden abgelehnt, statt die Ausgangsobjekte teilweise
zu verändern. Kurven werden für Bool-Operationen angenähert; Details können sich ändern.

## Schnittmaske nach Illustrator-Prinzip

1. Lege den geschlossenen Maskenpfad über den zu beschneidenden Inhalt.
2. Wähle Maskenpfad und mindestens ein Inhaltsobjekt aus.
3. Öffne **Ebenen → Schnittmaske**.

Das oberste geeignete ausgewählte Objekt wird zur Maske. Seine geschlossene Fläche
bestimmt den sichtbaren Bereich der Schnittgruppe. Maskenfarbe, Kontur und Alpha
sind dafür nicht maßgeblich: Dies ist keine Luminanz- oder Transparenzmaske.
Auch Bildobjekte können Inhalt sein, aber nicht selbst der Maskenpfad.

Im Tree kannst du die Maske gezielt auswählen und bearbeiten. Wird sie ausgeblendet,
verschwindet der maskierte Inhalt. **Maske loesen** stellt Maskenobjekt und Inhalt
als normale Gruppe wieder zur Verfügung. Löschen nur der Maske gibt den verbleibenden
Inhalt frei. Undo stellt die Verbindung wieder her. Verschachtelte Schnittmasken
sind derzeit nicht vorgesehen; löse eine Schnittgruppe vor Flächenoperationen auf.

## Übungen und Abnahme

### VEC-BOOL-01: Flächen vergleichen

Erzeuge zwei überlappende Rechtecke in unterschiedlichen Farben. Teste nacheinander Vereinigen, Subtrahieren, Schnittmenge, XOR und Aufteilen; gehe vor jedem nächsten Versuch per Undo zur Ausgangslage zurück.

**Erwartet:** Die fünf Ergebnisse entsprechen der Tabelle. Undo stellt jeweils beide Originale wieder her; die Reihenfolge bestimmt die Subtraktion.

### VEC-MASK-01: Maske speichern

Lege eine Ellipse über ein großes Rechteck oder Bild. Erzeuge eine Schnittmaske, speichere und öffne neu. Löse anschließend die Maske und nutze Undo.

**Erwartet:** Nur der Inhalt innerhalb der Ellipse ist sichtbar. Die gespeicherte Datei erhält die Schnittgruppe; Lösen und Undo funktionieren ohne Verlust des Inhalts.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

# 11 – Kontextmenü, Zwischenablage und Ausrichten {#vector_manual_11}

Ein Rechtsklick auf der Zeichenfläche öffnet das radiale Kontextmenü.
Einträge mit Untermenü sind optisch gekennzeichnet. Nach dem Anklicken entsteht ein
zusätzlicher äußerer Ring; die innere Ebene bleibt erreichbar. Esc oder ein Klick
außerhalb schließt das Menü. Nicht jede Aktion ist für jede Auswahl verfügbar.

## Kopieren und Löschen

- Kopieren: Ctrl/Cmd+C.
- Ausschneiden: Ctrl/Cmd+X.
- Einfügen: Ctrl/Cmd+V.
- Duplizieren: Ctrl/Cmd+D beziehungsweise **Kopien → Duplizieren**.
- Löschen: Delete/Backspace außerhalb aktiver Text-/Knoteneingabe.

Die gleichen Grundaktionen stehen im Kontextmenü. Die Objekt-Zwischenablage arbeitet
auch zwischen Dokument-Tabs. Das ist von der Systemzwischenablage für markierten Text
zu unterscheiden. Ausschneiden entfernt die Auswahl; Undo stellt sie wieder her.
Bei verknüpften Objekten muss beim dokumentübergreifenden Kopieren gegebenenfalls
der aktuelle Inhalt materialisiert werden, wenn das Original im Zieldokument fehlt.

## Ausrichten und verteilen

Unter **Transformieren → Ausrichten** gibt es links, horizontal zentriert, rechts,
oben, vertikal mittig und unten. Mindestens zwei Objekte sind nötig. Bezugsrahmen
ist die gemeinsame Begrenzung der Auswahl, nicht automatisch die Artboardkante.
Bei gedrehten Formen werden ihre achsenparallelen geometrischen Grenzen verwendet;
Konturbreiten zählen für diese Ausrichtung nicht dazu.

**H. Abstand** und **V. Abstand** benötigen mindestens drei Objekte und machen die
Zwischenräume gleich groß. Sie gleichen nicht die Abstände der Objektmittelpunkte an.
Die äußeren Grenzen bleiben die Bezugspunkte. Große Objekte können daher auch
negative Zwischenräume und Überlappung ergeben.

## Übungen und Abnahme

### VEC-EDIT-01: Zwischen Tabs kopieren

Kopiere zwei ausgewählte Formen, lege ein weiteres Dokument mit Artboard an und füge sie dort ein. Dupliziere die neue Auswahl und lösche das Duplikat. Nutze Undo.

**Erwartet:** Die Formen erscheinen im anderen Tab. Duplikat und Löschung lassen sich im Zieldokument rückgängig machen.

### VEC-ALIGN-01: Gleiche Abstände

Lege drei unterschiedlich breite Rechtecke nebeneinander an. Wähle alle und nutze Transformieren → Ausrichten → H. Abstand. Richte sie danach oben aus.

**Erwartet:** Die freien horizontalen Zwischenräume sind gleich groß; danach liegen die oberen geometrischen Kanten auf einer Höhe.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

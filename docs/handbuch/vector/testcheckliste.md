# Vector Tool – Manuelle Testcheckliste

Alle Tests sind zunächst **nicht durchgeführt**. Die Erstellung dieser Anleitung
und ein erfolgreicher Doxygen-Build sind keine interaktive Funktionsabnahme.
Kopiere diese Seite für jeden Durchlauf und protokolliere deine tatsächlichen Ergebnisse.

## Testumgebung

- Datum / Tester:
- Git-Commit / Branch:
- Betriebssystem / Version:
- Build-Typ (Debug/Release) und Startziel (`pengui_editor`):
- Monitorauflösung / Skalierung:
- Projektverzeichnis / Testdateien:
- Besonderheiten (GPU, Eingabegerät, installierte Testschrift):

## Ergebnisliste

Status nach Durchführung: **bestanden**, **fehlgeschlagen** oder **unklar**.
Bei Abweichungen Beobachtung, Datei und gegebenenfalls Screenshot ergänzen.
Die genauen Schritte und Soll-Ergebnisse stehen im jeweils verlinkten Kapitel.

| Test | Aufgabe / Anleitung | Status | Beobachtung / Nachweis |
|---|---|---|---|
| VEC-START-01 | [Projekt erstellen und erneut öffnen](01-einstieg.md) | nicht durchgeführt | — |
| VEC-START-02 | [Editorwechsel](01-einstieg.md) | nicht durchgeführt | — |
| VEC-UI-01 | [Layout verändern](02-oberflaeche.md) | nicht durchgeführt | — |
| VEC-DOK-01 | [Speichern und wieder öffnen](03-dokumente.md) | nicht durchgeführt | — |
| VEC-DOK-02 | [Änderungen und Abbruch](03-dokumente.md) | nicht durchgeführt | — |
| VEC-ART-01 | [Artboard bearbeiten](04-artboards.md) | nicht durchgeführt | — |
| VEC-ART-02 | [Zoom und Vorschau](04-artboards.md) | nicht durchgeführt | — |
| VEC-AUS-01 | [Mehrfachauswahl und History](05-auswahl.md) | nicht durchgeführt | — |
| VEC-AUS-02 | [Abbrechen und Mittelpunkt](05-auswahl.md) | nicht durchgeführt | — |
| VEC-FORM-01 | [Parametrische Formen](06-formen.md) | nicht durchgeführt | — |
| VEC-PFAD-01 | [Kurve bearbeiten](07-pfade.md) | nicht durchgeführt | — |
| VEC-PFAD-02 | [Form umwandeln](07-pfade.md) | nicht durchgeführt | — |
| VEC-STIL-01 | [Kontur und Verlauf](08-gestaltung.md) | nicht durchgeführt | — |
| VEC-STIL-02 | [Schatten speichern](08-gestaltung.md) | nicht durchgeführt | — |
| VEC-TEXT-01 | [Textfluss](09-text-bilder.md) | nicht durchgeführt | — |
| VEC-BILD-01 | [Projektbild sichern](09-text-bilder.md) | nicht durchgeführt | — |
| VEC-TREE-01 | [Sichtbarkeit und Sperre](10-ebenen.md) | nicht durchgeführt | — |
| VEC-TREE-02 | [Gruppieren und Reihenfolge](10-ebenen.md) | nicht durchgeführt | — |
| VEC-DIR-01 | [Ordner erstellen](10-ebenen.md) | nicht durchgeführt | — |
| VEC-EDIT-01 | [Zwischen Tabs kopieren](11-bearbeiten.md) | nicht durchgeführt | — |
| VEC-ALIGN-01 | [Gleiche Abstände](11-bearbeiten.md) | nicht durchgeführt | — |
| VEC-BOOL-01 | [Flächen vergleichen](12-flaechen-masken.md) | nicht durchgeführt | — |
| VEC-MASK-01 | [Maske speichern](12-flaechen-masken.md) | nicht durchgeführt | — |
| VEC-KLON-01 | [20 Wiederholungen](13-klonen.md) | nicht durchgeführt | — |
| VEC-INST-01 | [Freie Instanz](13-klonen.md) | nicht durchgeführt | — |
| VEC-SNAP-01 | [Raster einrasten](14-praezision.md) | nicht durchgeführt | — |
| VEC-NUM-01 | [Rechnen und Live-Vorschau](14-praezision.md) | nicht durchgeführt | — |
| VEC-EXP-01 | [Transparentes PNG und JPEG](15-export.md) | nicht durchgeführt | — |
| VEC-EXP-02 | [Alle Artboards und Auswahl](15-export.md) | nicht durchgeführt | — |
| VEC-REF-01 | [Sitzung und Datei unterscheiden](16-referenz.md) | nicht durchgeführt | — |

## Fehlerbericht zum Kopieren

- Test-ID:
- Erwartetes Ergebnis:
- Tatsächliches Ergebnis:
- Reproduktionsschritte ab Programmstart:
- Häufigkeit (immer / gelegentlich):
- Letzte Statusmeldung:
- Testdatei einschließlich benötigter Assets:
- Screenshot oder kurzes Video:
- Funktioniert Undo / Speichern / erneutes Öffnen noch?

## Zusätzliche Regressionen

Nach einem Fix den betroffenen Test und angrenzende Funktionen wiederholen.
Besonders wichtig: Undo/Redo, Speichern/Öffnen, Editorwechsel, mehrere Dokumente,
versteckte/gesperrte Eltern und Dateidialog-Abbruch. Erst nach einem echten
Durchlauf den Status ändern; nicht getestete Betriebssysteme bleiben offen.

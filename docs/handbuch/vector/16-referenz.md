# 16 – Referenz, Grenzen und Fehlersuche {#vector_manual_16}

## Tastatur und Maus

Unter macOS ist bei den folgenden Dokument-/Zwischenablagekürzeln Cmd statt Ctrl
vorgesehen. Aktive Eingabefelder und Werkzeuge können dieselbe Taste anders verwenden.

| Eingabe | Kontext und Wirkung |
|---|---|
| Ctrl/Cmd+S | Dokument speichern |
| Ctrl/Cmd+Shift+S | Speichern unter |
| Ctrl/Cmd+Z | Undo |
| Ctrl/Cmd+Shift+Z / Ctrl/Cmd+Y | Redo |
| Ctrl/Cmd+C / X / V | Objekt- oder Textzwischenablage, je nach Fokus |
| Ctrl/Cmd+D | Objektauswahl duplizieren |
| Delete / Backspace | Objekte beziehungsweise ausgewählte Knoten löschen; im Text Zeichen löschen |
| Tab / Shift+Tab | Fokus zwischen Formularfeldern wechseln und Änderung übernehmen |
| Ctrl/Cmd+A | Text im aktiven Eingabefeld markieren |
| Mittlere Maustaste ziehen | Zeichenansicht verschieben |
| Mausrad | Weicher Zeichenflächenzoom |
| Rechtsklick | Kontextmenü; bei laufenden Werkzeugen gegebenenfalls Abbruch |
| Esc | Menü schließen / Werkzeug abbrechen; Texteingabe beenden und Inhalt behalten |
| Enter | Formular bestätigen / offenen Kurvenentwurf beenden; im Text neuer Absatz |
| Ctrl/Cmd+Enter | Canvas-Texteingabe abschließen |
| Shift | Mehrfachauswahl beziehungsweise Proportionen/15-Grad-Drehung je nach Geste |
| Alt | Vom Mittelpunkt skalieren/zeichnen beziehungsweise Tangenten unabhängig bewegen |
| F6 | Aktiven Editor neu laden; vorher wichtige Arbeit speichern |

## Was wird wo erhalten?

| Zustand | `.pengvec` | Editorwechsel in laufender App |
|---|---|---|
| Bestätigte Formen, Artboards, Stile, Texte, Masken und Verknüpfungen | Ja | Ja |
| Bildpixel | Separate Projekt-Assets, nur Pfad in Datei | Ressourcen werden neu aufgebaut |
| Undo/Redo-History | Nein | Ja, temporärer Checkpoint |
| Zoom, Raster, Vorschau-Bereich | Nein | Sitzungszustand |
| Unfertige Mausgeste / Texteingabecursor | Nein | Nicht als laufende Interaktion |
| Zuletzt geöffnetes Projekt | Separate Benutzereinstellung | Unabhängig vom Editor |

Linux/macOS verwenden für temporäre Editor-Checkpoints `tmpfile`, Windows temporäre
Dateien mit Löschen beim Schließen. Daraus entsteht kein dauerhaftes Projektbackup.
Speichere regelmäßig und sichere das gesamte Projekt einschließlich Assets extern.

## Häufige Fragen

| Beobachtung | Prüfen / Vorgehen |
|---|---|
| Rechteck rastet trotz sichtbarem Raster nicht ein | Magnet zusätzlich einschalten; Toleranz und Rastertyp prüfen |
| Objekt lässt sich nicht greifen | Auswahlwerkzeug aktiv? Objekt oder Eltern gesperrt/ausgeblendet? |
| Artboard lässt sich nicht verschieben | Am Namen über dem Artboard ziehen, nicht am weißen Innenbereich |
| Änderung erscheint nicht dauerhaft | Feld mit Enter/Tab bestätigen, Geste loslassen, Statusmeldung prüfen |
| Löschen betrifft Text statt Objekt | Texteingabe abschließen und zum Auswahlwerkzeug wechseln |
| Bild fehlt nach Weitergabe | `assets/images` mitgeben und relativen Projektpfad erhalten |
| Schrift sieht auf anderem Rechner anders aus | Gespeicherte Schrift dort installieren; sonst greift der Ersatzfont |
| Vorschau zeigt nur einen Ausschnitt | Region größer als Panel; Ansicht verschieben oder Panel vergrößern |
| Datei öffnet sich nicht nochmals | Ein vorhandener Tab derselben Datei wird absichtlich aktiviert |
| Nach Neustart fehlen ungespeicherte Tabs | Projektwiederaufnahme ist keine Dokument-/Absturzwiederherstellung |
| Operation wird nicht ausgeführt | Auswahlzahl, Sperren, Formtyp und Statusmeldung prüfen; Kapazitätsgrenzen beachten |

## Grenzen dieses Arbeitsstands

Die Arbeitsfläche ist kein vollständiger Ersatz für Illustrator oder Affinity.
Aktuell gelten unter anderem maximal 15 Dokumente, 64 Artboards und 64 Objekte pro
Dokument. Wiederholungen können zusätzliche Darstellung erzeugen, ohne zusätzliche
Objekte zu besitzen. Materialisieren und komplexe Operationen können an Grenzen
stoßen. Bézierkurven, Konturkomplexität und History sind ebenfalls begrenzt.

Es gibt keine beliebige Gruppenverschachtelung, keine allgemeinen SVG-/PDF-Importe
und keine vollständige professionelle Satz- oder Farbmanagementpipeline.
Speicher- und Exportfehler dürfen nicht als Erfolg interpretiert werden: Prüfe die
Statusmeldung und die tatsächlich erzeugte Datei. Die Testcheckliste hilft,
Abweichungen mit reproduzierbaren Schritten zu melden.

## Übungen und Abnahme

### VEC-REF-01: Sitzung und Datei unterscheiden

Speichere eine Zeichnung, ändere Zoom und Vorschau, wechsle den Editor und zurück. Schließe dann das Dokument und öffne die gespeicherte Datei erneut.

**Erwartet:** Beim Editorwechsel bleibt die laufende Sitzung erhalten. Die erneut geladene Datei enthält Artwork, aber keine gespeicherte Undo-History oder Vorschau-Region.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

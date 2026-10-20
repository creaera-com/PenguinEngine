# Textfelder im Vektoreditor

## Bedienung

Der Text-Button (`vector.add_text`, Tabler `typography`) aktiviert die gemeinsame
Aufzieh-Geste. Im Artboard ein Rechteck aufziehen, danach direkt schreiben. Der
anfängliche Platzhalter „Text“ ist ausgewählt. Shift und Alt nutzen dieselben
Aufzieh-Regeln wie die anderen Box-Werkzeuge.

- **Doppelklick auf ein Textfeld:** Schreiben wieder aufnehmen.
- **Enter:** neuer Absatz; **Tab:** Tabulator im Text.
- **Strg/Cmd+Enter oder Escape:** Texteingabe beenden. Bereits geschriebener Text bleibt erhalten.
- **Pfeiltasten, Pos1/Ende und Shift:** Cursor bewegen beziehungsweise Auswahl erweitern.
- **Maus ziehen:** Text auswählen; Doppelklick während der Bearbeitung markiert ein Wort.
- **Strg/Cmd+A/C/X/V:** auswählen, kopieren, ausschneiden und einfügen. CRLF wird zu LF normalisiert.
- **Strg/Cmd+Z, Strg/Cmd+Shift+Z oder Strg/Cmd+Y:** Undo/Redo im Dokument.

Zum Verschieben, Drehen oder Ändern des Rahmens das Auswahlwerkzeug aktivieren.
Beim Ändern der Rahmengröße fließt der Text neu um; die Schriftgröße bleibt gleich.
Ein roter Rahmen und der Hinweis im Typografie-Popup kennzeichnen Textüberlauf.
Der überlaufende Inhalt bleibt im Modell erhalten und wird nach Vergrößern wieder sichtbar.

Im Inspector öffnet **Typografie** die Textformatierung. Der Schriftfilter wird mit
Enter oder Fokuswechsel übernommen. Die Liste zeigt installierte Schriftschnitte
und die mitgelieferte JetBrains Mono. **Weitere Schriften** blättert durch die
gefilterte Liste, ohne das Node-Limit der YAML-Oberfläche zu überschreiten.

Schriftgröße und Zeichenabstand sind Pixelwerte. Die Zeilenhöhe ist ein Faktor:
`1.2` bedeutet einen Zeilenabstand von 120 Prozent der Schriftgröße. Ausrichtung:
links, zentriert, rechts oder Blocksatz. Blocksatz dehnt Wortzwischenräume, aber
nicht die letzte Zeile eines Absatzes. Fett/Kursiv wählen möglichst einen echten
Schnitt derselben Familie. Fehlt dieser, wird er synthetisch dargestellt.

**Spalten** (1–16) und **Spaltenabstand** teilen einen Textkasten in gleich breite
Spalten. Der Text fließt oben nach unten und anschließend in die nächste Spalte.
Absätze bleiben erhalten. Die Füllfarbe des Objekts bestimmt die Schriftfarbe;
ein optionaler Rahmen umrandet den Textkasten.

## Programmfluss und fachliche Grenzen

1. `ui/editor.yaml` beschreibt den Werkzeug-Button; `workspace/box_tool.c` führt
   die Aufzieh-Geste aus. Die Vorschau enthält noch keine Dokumentänderung.
2. Beim Loslassen erzeugt `text/text.c::vector_text_add` einen `VectorShape` mit
   aktivem `VectorText`. `object/commands.c` legt ihn über die History an.
3. `text/tool.c` verarbeitet native Texteingabe. Cursor und Auswahl sind temporär;
   die eigentlichen Textwerte werden sofort durch validierte Befehle geändert.
4. Aufeinanderfolgende Eingaben werden innerhalb einer Schreibsitzung zu einem
   History-Schritt zusammengefasst. Werkzeugwechsel, Formatänderungen, Undo und
   erneutes Aktivieren beginnen eine neue Gruppe. Andere Befehle werden nicht vermischt.
5. `ui/inspector/text.yaml` wird nur bei ausgewählten Textfeldern in den Slot
   `text_properties` eingesetzt. `ui/inspector/text.c` übersetzt Formularaktionen
   in `vector_text_update`; reine Modelldateien kennen keine Widgets.
6. `text/layout.c` berechnet Wortumbruch, Ausrichtung und Spalten mit einer
   austauschbaren Metrik-Funktion. Es gibt dort keine SDL-, Fontdatei- oder GPU-Abhängigkeit.
7. `text/render.c` nutzt den App-Schriftservice und einen begrenzten Texture-Cache.
   Der Renderer behandelt Text an derselben Position in der Ebenenreihenfolge wie
   geometrische Objekte. Rahmen und Artboard-Clipping bleiben wirksam.

Der gemeinsame Wertebefehl für Kurven und Text liegt jetzt in `object/commands.c`
(Checkpoint-Codec 8). Kurvennormalisierung und Knotenbearbeitung bleiben in `curve/`.

## Schriftservice und Build

Der gemeinsame App-Vertrag exportiert `editor_host/fonts.h` über `editor.h`.
`apps/demo/platform/fonts.c` implementiert die Suche in System- und Benutzer-
Schriftverzeichnissen unter Linux, Windows und macOS. Unterstützt werden TTF,
OTF und TTC (Fontdateien bis 32 MiB, Face-Indizes 0–255). Familien/Schnitte stammen aus den Fonttabellen. Der Katalog wächst
dynamisch; geladene Fontdateien und Frame-Texturen haben getrennte, begrenzte Caches.

Rasterisierung und Kerning übernimmt **stb_truetype 1.26**, über einen festen Git-
Commit und SHA-256 in `cmake/TextFonts.cmake` gebunden. CMake lädt den Header einmal
ins Build-Verzeichnis. Er wird nicht ins Repository aufgenommen; die Lizenz steht
im Header (MIT oder Public Domain). `PENGUI_TEXT_DEP_DIR` kann auf einen bereits
vorbereiteten Cache zeigen, etwa für Offline-Builds. Die mitgelieferte Schrift
wird vom Resource-Target nach `fonts/` neben das Programm kopiert.

## Speicher, Persistenz und aktueller Umfang

`VectorShape.text` enthält ausschließlich Werte: UTF-8-Inhalt, Fontpfad und
Face-Index, Schriftgröße, Fett/Kursiv, Ausrichtung, Zeilen-/Zeichenabstand sowie
Spaltenanzahl und Spaltenabstand. Leerer Fontpfad bedeutet JetBrains Mono.
Gelöschte oder nicht mehr installierte Fonts werden zur Darstellung durch die
mitgelieferte Schrift ersetzt; der gespeicherte Fontpfad bleibt erhalten.

Es gibt keine Font-, Textur- oder Heap-Zeiger im Dokument, Clipboard oder in
History-Payloads. Der aktuelle Checkpoint **PVCKPT24** speichert auch die Textwerte und die
History. Beim Editorwechsel werden native Schriftressourcen freigegeben und beim
Wiederherstellen neu aufgebaut. Texteingabezustand und GPU-Caches werden nicht gespeichert.

Die erste Fassung formatiert jeweils ein ganzes Textfeld einheitlich und speichert
bis zu **2047 UTF-8-Bytes** pro Feld. Gemischte Schriftgrößen/Schnitte innerhalb eines
Felds, automatische Silbentrennung, komplexes Schrift-Shaping (z. B. Arabisch),
OpenType-Features/variable Achsen, verknüpfte Textrahmen, Text auf Pfaden und
Umwandeln von Text in Konturpfade gehören noch nicht zu dieser Fassung.
Geometrische Bool-Operationen werden für ausgewählten Text gesperrt, damit Text
nicht versehentlich in das Rechteck seines Rahmens umgewandelt wird.

# 09 – Text und importierte Bilder {#vector_manual_09}

## Textkasten

Aktiviere das Textwerkzeug und ziehe einen Kasten auf. Der Platzhalter ist ausgewählt;
Tippen ersetzt ihn. Ein Doppelklick auf vorhandenen Text startet die Bearbeitung
wieder. Enter fügt einen Absatz ein, Tab innerhalb der Canvas-Texteingabe einen Tabulator.
Ctrl/Cmd+Enter oder Esc beendet die Eingabe **und behält bereits geschriebenen Text**.
Das unterscheidet sich vom Abbrechen einer Transformationsgeste.

Maus und Shift+Pfeiltasten markieren Text; Ctrl/Cmd+A markiert alles. Kopieren,
Ausschneiden und Einfügen gelten während der Texteingabe für den Text. Zum
Verschieben/Skalieren des Kastens wechsle zum Auswahlwerkzeug.

## Typografie im Inspector

Bei ausgewähltem Text steht der Typografiebereich direkt im Inspector. Wähle die
mitgelieferte JetBrains Mono oder eine installierte Schrift. Die Suche wird mit
Enter beziehungsweise beim Verlassen übernommen; weitere Treffer sind über die
Schriftliste erreichbar. Fett/Kursiv verwendet passende Schnitte, soweit vorhanden,
sonst eine angenäherte Darstellung.

Schriftgröße und Buchstabenabstand sind Pixelwerte. Die Zeilenhöhe ist ein Faktor,
also 1,2 für 120 Prozent. Ausrichtungen sind links, zentriert, rechts und Blocksatz;
die letzte Absatzzeile wird beim Blocksatz nicht gestreckt. Bis zu 16 gleich breite
Spalten können mit einem Spaltenabstand angelegt werden. Text fließt erst nach unten,
dann in die nächste Spalte. Eine Größenänderung des Kastens ändert den Umbruch,
nicht die Schriftgröße. Überlauf wird rot markiert; der unsichtbare Rest bleibt gespeichert.

Die Formatierung gilt für den ganzen Kasten. Gemischte Schriftschnitte innerhalb
eines Kastens, Silbentrennung, komplexes Schrift-Shaping und Text auf Pfaden fehlen.
Pro Textfeld stehen bis zu 2047 UTF-8-Bytes zur Verfügung. Schriften werden nicht
in die Dokumentdatei eingebettet; fehlende Fonts fallen auf JetBrains Mono zurück.

## PNG und JPEG platzieren

Öffne ein Projekt und wähle ein Artboard. Der Bildimport-Button öffnet den Dateidialog.
PNG/JPEG werden nach `assets/images` im Projekt kopiert, bei Namenskollision mit
einem freien Dateinamen. Das Original bleibt unverändert. Das Bild wird im Artboard
zentriert, bei Bedarf verkleinert, und lässt sich danach transformieren.

Die Zeichnung speichert einen relativen Assetpfad, keine eingebetteten Bildbytes.
Gib deshalb Projekt, `.pengvec` und Assets gemeinsam weiter. Löschen/Undo des
Bildobjekts entfernt die kopierte Bilddatei nicht. Bei fehlender Datei erscheint
ein Platzhalter. Stelle sie am erwarteten Pfad wieder her und lade neu.
PNG-Transparenz ist möglich; Bilddateien sind derzeit auf 32 MiB, 8192 Pixel pro
Achse und 16 Megapixel begrenzt. EXIF-Drehung und Farbprofilkonvertierung werden nicht zugesichert.

## Übungen und Abnahme

### VEC-TEXT-01: Textfluss

Ziehe einen Textkasten auf, gib mehrere Absätze ein und beende mit Ctrl/Cmd+Enter. Stelle zwei Spalten und einen Spaltenabstand ein. Verkleinere den Kasten bis zum Überlauf, vergrößere ihn wieder.

**Erwartet:** Der Text fließt in die zweite Spalte. Überlauf wird angezeigt und der Text erscheint beim Vergrößern wieder.

### VEC-BILD-01: Projektbild sichern

Importiere in einem Testprojekt ein PNG mit Transparenz. Speichere die Zeichnung, schließe und öffne sie. Kontrolliere anschließend `assets/images`.

**Erwartet:** Das Bild bleibt sichtbar und transparent. Im Assetordner liegt die Kopie; das ursprüngliche Bild wurde nicht verändert.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

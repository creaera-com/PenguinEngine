# Manueller Prüfablauf

Zuerst `cmake --build build` und `ctest --test-dir build --output-on-failure` ausführen. Für manuelle Tests eine Kopie des Beispielprojekts außerhalb des Build-Verzeichnisses verwenden.

1. Editor öffnen. Prüfen, dass Dateibaum, Karte und acht Beispiel-Tiles erscheinen.
2. Assets und Szenen auf-/zuklappen, `Refresh` verwenden und eine PNG auswählen.
3. Ein anderes PNG mit zum Raster passender Größe importieren; falsche Bildgröße ausprobieren. Ein fehlerhafter Import darf die Szene nicht verändern.
4. Tilegröße des PNGs ändern. Ungültige Zahlen, null und nicht passende Raster müssen abgelehnt werden.
5. Mausrad an mehreren Kartenpositionen verwenden: derselbe Weltpunkt bleibt unter dem Cursor. Mittlere Maustaste verschiebt die Kamera.
6. Pinsel schnell ziehen: keine Lücken. Einmal Undo entfernt den gesamten Strich. Redo stellt ihn wieder her.
7. Radierer, rechte Maustaste, Rechteck in beiden Richtungen und Füllen an getrennten Bereichen ausprobieren.
8. Eine Ebene hinzufügen, umbenennen, verschieben, ausblenden und löschen. Jede Änderung einzeln rückgängig machen und wiederholen.
9. `Collision` aktivieren, Wände malen. Mit F5 spielen: die Figur bleibt an roten Feldern und am Kartenrand stehen.
10. Eine Entity erstellen, mit `Select` ziehen, Position/Größe im Inspector ändern, Spielersteuerung umschalten und löschen. Undo stellt das Objekt wieder her.
11. Beim Ziehen Escape drücken und den Fokus wechseln: laufende Änderung wird verworfen.
12. Im Play-Modus bewegen, dann Stop: die bearbeitete Ausgangsposition bleibt erhalten.
13. Speichern, ändern, Undo: Änderungsanzeige entspricht dem gespeicherten Zustand. Nach Undo neu malen: Redo ist nicht mehr möglich.
14. Andere Szene öffnen und Fenster schließen, jeweils mit ungespeicherten Daten: Speichern/Verwerfen/Abbrechen funktionieren.
15. Projekt erneut öffnen: Ebenen, Tiles, Kollisionen, Entity-Werte und Tileset entsprechen den gespeicherten Daten.
16. `penguin_game` mit demselben Projekt starten: Bewegung funktioniert ohne Editor.
17. Fenstergröße ändern, Datei-/Palette-Scrollen prüfen. Text-/Modal-Eingaben dürfen keine Weltaktionen auslösen.

Die automatisierten Tests wurden in der Entwicklungsumgebung headless ausgeführt. Der vollständige Desktop-Ablauf muss auf dem jeweiligen Zielsystem zusätzlich geprüft werden.

## Scripts-Tab

1. Zwischen Map und Scripts wechseln: Kamera, Karte, Cursor und Text bleiben erhalten.
2. `scripts/player.js` öffnen: Keywords, Kommentare, Strings und Zahlen haben unterschiedliche Farben.
3. `const value = ;` einfügen: nach kurzer Pause erscheint ein Fehler mit Zeile. F7 prüft sofort; Klick auf die Meldung bewegt den Cursor.
4. Fehler korrigieren: Anzeige wechselt auf Syntax OK. Unbekannte Funktionsnamen sind keine Syntaxfehler.
5. `cons` schreiben und Strg+Leertaste drücken, `const` übernehmen. `Math.fl` soll `floor` anbieten. Eigene Bezeichner testen. In Kommentaren und Strings keine Vorschläge.
6. Auswahl ziehen, Shift+Pfeile, Strg+A/C/X/V, Backspace/Delete und Undo/Redo prüfen. Ein Paste ist ein Undo-Schritt.
7. Mit vielen Zeilen vertikal und mit einer langen Zeile horizontal scrollen. Gutter bleibt sichtbar.
8. Script ändern und eine andere `.js` öffnen oder beenden: Speichern/Verwerfen/Abbrechen testen. Ungültige UTF-8- oder zu große Dateien dürfen den Text nicht ersetzen.
9. Ctrl+S im Scripts-Tab speichert nur das Script; im Map-Tab nur die Szene. Ungespeicherte Änderungen beider Dokumente werden beim Beenden berücksichtigt.
10. Play im Map-Tab testen: Scripts werden noch nicht ausgeführt.

## Gemeinsame UI und Flat-Layout

1. Panel-Abstände prüfen: Toolbar, Dateibaum und Inspector heben sich mit hellerem Grau und 3 px dunklem Zwischenraum ab; beim Spaltenziehen/Fensterresize bleiben die Abstände erhalten. Map-Pinsel, Zoom und Skript-Auswahl treffen nach dem Einrücken weiterhin korrekt. Buttons/Tabs prüfen: gerade Kanten, flächige Farben, aktives Orange und gut lesbare Beschriftung. Save und Play/Stop zeigen nur Icons; Tooltips und Ctrl+S/F5 funktionieren weiterhin. Außerhalb der Karte ist die Arbeitsfläche heller grau; beim Zoomen/Verschieben bleiben leere und transparente Tiles innerhalb der Karte dunkel.
2. Beide Spaltentrenner ziehen, auch über ihren ursprünglichen Bereich hinaus. Mindestbreiten erhalten die mittlere Arbeitsfläche. Keine Tiles oder Texte dürfen dabei verändert werden.
3. Rechts Inspector-Bereiche ein-/ausklappen und die Tile-Palette bei geänderter Spaltenbreite prüfen. Rename und Sortieraktionen bleiben erreichbar.
4. Mit vielen Dateien, vier Ebenen/ausgewählter Entity und langen Scripts Scrollbars prüfen: Mausrad, Thumb-Ziehen und Track-Klicks. Ausgeblendete Controls dürfen nicht anklickbar sein.
5. Map-/Scripts-Buttons in der globalen Toolbar umsortieren. Der Inhalt bleibt seinem Tab zugeordnet, Wechsel erhält Text und Kamera.
6. Button drücken und außerhalb loslassen: keine Aktion. Fokusverlust während Drag beendet Capture.
7. `penguin_ui_demo` starten und dieselben UI-Komponenten ohne Engine-/Editorfunktionen prüfen.

8. Links Explorer/Scene und rechts Inspector/Info wechseln, auch im Scripts-Tab. Die Gruppen schalten unabhängig voneinander; ihre Tabs lassen sich innerhalb der Gruppe umsortieren. Die Inhalte folgen der Tab-ID.
9. Scene: Entity anklicken. Map und Inspector werden aktiv, die passende Entity ist ausgewählt. Auswahl/Tabwechsel erzeugen keine Undo-Schritte. Löschen/Undo im Scene-Tab aktualisieren die Scene-Liste. Leere Szenen zeigen einen Hinweis.
10. Viele Entities anlegen und Scene scrollen; dann Explorer öffnen und zurückwechseln. Beide Scrollpositionen bleiben separat erhalten. Info zeigt aktuelle Szenenzahlen. Bei minimalen Spaltenbreiten passen die Tabtitel weiterhin.
11. Zwischenräume der Spalten prüfen: keine sichtbaren Trennstriche, auch bei Hover. Jeweils drei senkrecht angeordnete Punkte sitzen mittig, folgen beim Resize und werden bei Hover/Ziehen orange. Resize-Cursor und Ziehen funktionieren weiterhin.

12. Reiter prüfen: kurze Titel belegen weniger Breite als lange Titel, neben dem letzten Tab bleibt die dunkle Grundfläche frei. Der aktive Tab geht bündig ins Panel über. Unterschiedlich breite Tabs ziehen, Maus dabei kurz halten: keine springende Reihenfolge. Bei schmalen Seitenpanels über der Tabreihe mit dem Mausrad scrollen; abgeschnittene Tabs bleiben erreichbar. Umlaut-Titel zählen als einzelne Zeichen.

13. Globale Toolbar: Menü, Trennstrich, Save, Trennstrich, Undo/Redo, Trennstrich, Map-/Scripts-Icons. Hamburger anklicken: noch keine Aktion. Tooltips benennen die Arbeitsbereiche. Save/Undo/Redo gehören zum aktiven Dokument. Zwischen den Bereichen wechseln; Werkzeuge wechseln passend zum Inhalt. Im Scripts-Bereich fehlen Play, Import und Kartenwerkzeuge. Die Syntaxprüfung erscheint ausschließlich in seiner Werkzeugleiste, nicht nochmals im Dokumentkopf.
14. Map-Spalte stark verengen. Werkzeugleiste bricht um; auch Raster und Kollision bleiben erreichbar. Klicks in beiden Toolbar-Zeilen verändern keine Tiles, Textauswahl oder Zoom. Bei breiterer Spalte entsteht wieder eine Zeile.
15. Scripts wählen, F5 drücken: kein Play. Map wählen, Play starten: Wechsel zu Scripts gesperrt; Stop stellt die bearbeitete Szene wieder her. Tooltips, Ctrl+S, Ctrl+Z und Ctrl+Shift+Z gelten für das aktive Dokument, auch nach Fokus auf einen Toolbar-Button.


## YAML, Entladen und Sitzungssicherung

1. Eine eigene Kopie von `resources/editor/ui` mit `--ui /absoluter/pfad/shell.yaml` öffnen. Texte, Iconreihenfolge und Panelinhalte ändern, F6 drücken. Alle drei Spalten folgen dem jeweiligen Editor. Layouts, die noch nicht registrierte Typen verwenden, werden mit Quellposition abgelehnt.
2. Map ändern, Kamera/Zoom/Auswahl verstellen und zwischen Map/Scripts wechseln. Undo/Redo, Dirty-Anzeige und Ansicht bleiben erhalten. Im Script Text ändern, Undo ausführen, wechseln und zurückkehren: auch die Redo-Stufe bleibt erhalten.
3. F6 mit geänderter Farbe oder Abstand testen. Danach einen YAML-Syntaxfehler bzw. unbekannte Aktion einbauen und erneut F6: bisherige UI und ungespeichertes Dokument bleiben bedienbar. Fehler korrigieren und erneut laden.
4. Bei beiden Editoren ungespeicherte Änderungen anlegen und schließen: Speichern berücksichtigt auch den inaktiven Editor. Verwerfen verändert keine Quelldatei; Abbrechen behält die Sitzung. Play verhindert einen Wechsel.
5. Speicherprüfung: Nach einem Wechsel existiert nur die aktive Payload/Layoutinstanz; die andere Payload und ihre Historien/Texturen sind freigegeben. Ein gleicher Heap-Adresswert nach erneuter Allokation oder ein nicht sofort fallender RSS-Wert beweist keinen Fehler.
6. Die automatisierten Editor-Smokes simulieren außerdem ein nicht beschreibbares Checkpointziel und eine beschädigte Zielsitzung. Im ersten Fall bleibt der Quelleditor stehen, im zweiten wird er aus seiner gerade gesicherten Sitzung wiederhergestellt. Kein Test schreibt dabei die Projektdateien.

Der native Sitzungscache ersetzt keine Backups und ist noch keine Crash-Recovery über Programmstarts hinweg. Eigene Layoutkopien bearbeiten: Ein Build kopiert die Repo-Vorgaben erneut nach `build/ui-layouts`.

## Fachliche Modultrennung

- Im SDL-Build laufen insgesamt unter Linux 183 Unity-Tests und 16 CTest-Prüfungen. `editor_document_unit_tests` prüft Map- und Script-Checkpoints ohne Anwendungssingleton oder geöffnetes Fenster: Text/Tile-Zustand, UTF-8, Undo/Redo, gespeicherte Revisionen und unabhängige Dokumentkopien bleiben erhalten.
- `tests/editor_smoke.c` verwendet weiterhin die echten Eingabehandler, Editorwechsel und F6-Reloads. Die Tests liegen außerhalb des produktiven Quellcodes und werden nur bei `BUILD_TESTING=ON` eingebunden.
- Mit `PENGUIN_BUILD_EDITOR=OFF` bleiben die acht SDL-freien Testgruppen verfügbar. `BUILD_TESTING=OFF` muss ebenfalls bauen; `penguin_editor --smoke` führt dann nur den kurzen Start-/Rendercheck aus.
- Für das Strukturrefactoring zusätzlich die Map-/Scripts-Aufnahmen gegen den vorherigen Stand vergleichen und einen vollständigen ASan-/UBSan-Testlauf ausführen.

## Konsolenpanel und gemeinsame Shell

1. Unter Linux Map und Scripts öffnen: jeweils mittig unten einen Konsole-Tab prüfen. Dreipunktgriff vertikal ziehen und in kleinen Fenstern die Größenbegrenzung prüfen.
2. `pwd`, `printf` und `cd` ausführen. Zwischen Editoren wechseln und F6 außerhalb der Konsole auslösen: dieselbe Shell, dasselbe Arbeitsverzeichnis und dieselbe Ausgabe bleiben bestehen; die Höhe pro Editor wird wiederhergestellt.
3. Ein Vordergrundprogramm starten und mit Ctrl+C abbrechen. Bei Konsolenfokus dürfen Ctrl+S/Z und WASD keine Map-/Scriptänderungen oder Spielerbewegung auslösen.
4. Text markieren und mit Ctrl+Shift+C/V kopieren/einfügen. Mausrad, Scrollbar und Shift+PageUp/PageDown bei längerer Ausgabe testen. Fenster- und Panelgröße ändern; `stty size` muss folgen.
5. Shell mit `exit` schließen und per Refresh-Symbol neu starten. Editor beenden und prüfen, dass seine Shell und normale Vordergrundprogramme beendet sind.
6. Unter Windows zunächst die Backend-Meldung prüfen; dort fehlt noch ConPTY. Der macOS-POSIX-Backend benötigt zusätzlich einen echten Desktoptest.

Automatisch: sieben Terminaltests unter Linux/POSIX (unter Windows Parser- plus Fallbacktests), drei Splittertests, zwei zusätzliche YAML-Schematests und GUI-Integration für Fokus, Resize, Reload und gemeinsame Sitzung. In GUI-Smokes werden keine Shellprozesse gestartet; die dedizierten PTY-Tests verwenden `/bin/sh`.


## Scene-Graph

`hierarchy_unit_tests` enthält zehn SDL-freie Unity-Tests für lokale und globale Positionen, Reparenting mit/ohne Erhalt der Weltposition, ungültige oder veraltete Parents, Selbstreferenzen und Zyklen, Tiefe bis 128 Entities, Unterbaum-Löschen mit Slot-Wiederverwendung, Undo/Redo, Tilekollision in Weltkoordinaten, parentabhängige Runtime-Reihenfolge sowie Szene-v2-Roundtrip und v1-Kompatibilität. Fehlende/falsche Parent-Felder, gebrochene Referenzen und Zyklen dürfen das vorhandene Dokument beim Laden nicht verändern.

Der Dokument-Checkpoint-Test erhält Parent-Beziehungen und Aufklappzustände zusätzlich zum Undo-Verlauf. Die GUI-Smokes bedienen die YAML-Buttons für Wurzel/Geschwister/Kind, klappen Knoten ein/aus, ziehen einen Parent auf dem Canvas, prüfen die Position des Enkels, löschen den Unterbaum und prüfen Undo/Redo sowie F6 und Map → Scripts → Map.

Manuell: `examples/topdown/scenes/hierarchy.json` über den Explorer öffnen, den Scene-Tab auswählen und den Player ziehen. Seine drei Nachkommen folgen; die zweite Wurzel bleibt stehen. Kinder separat auswählen und ziehen, Local-X/Y im Inspector ändern, Parent löschen und mit Ctrl+Z wiederherstellen, speichern und neu laden. Hierarchie-Physikgruppen sowie Rotation/Skalierung sind noch nicht implementiert.


## Komponenten-Inspector

Neun SDL-freie Unity-Tests (`component_unit_tests`) prüfen Standardwerte und automatisch ergänzte Abhängigkeiten, abgewiesene ungültige/mehrere Bits und veraltete Handles, Schutz bestehender Hierarchien beim Entfernen von Transform, Wiederhinzufügen mit Standardwerten, Undo/Redo sowie Sprite-Farbe und Body-Geschwindigkeiten im Szenenformat. Fehlende Geschwindigkeitsfelder ergeben 0; falsche Feldtypen verändern die aktive Szene nicht. Nicht endliche oder übergroße Geschwindigkeiten werden vor dem Schreiben abgelehnt.

Die Editor-Smokes bedienen das Hinzufügen-Menü und die Entfernen-Buttons, klappen Transform ein/aus, bearbeiten RGBA-Farbe (einschließlich ungültiger Eingabe) und Velocity X, prüfen deaktivierte abhängige Löschaktionen sowie Undo/Redo, F6 und Map → Scripts → Map. Komponentenänderungen während Play werden abgewiesen. Der Dokument-Checkpoint-Test prüft zusätzlich die entitybezogenen Klappzustände.


## Map- und Tiles-Tabs

Ein zusätzlicher YAML-Test prüft die Bindings der neuen Adapter, ihre Beschränkung auf den Map-Editor und die Einmaligkeitsregel für `tile_palette`. Die aktualisierten Layout-Smokes klappen die Layerverwaltung im linken Map-Tab statt im Inspector.

Der neue Panel-Smoke wählt Map und Tiles per Tab-Klick, erstellt/benennt/sortiert/löscht Layer und prüft Undo, Sichtbarkeit sowie Redo beim Umbenennen. Er öffnet Import und Source-Tilegröße aus Tiles, wählt ein Palette-Tile und erhält Tabs, Layernamen und Undo-Verlauf über F6 sowie Map → Scripts → Map. Manuell zusätzlich Mausrad und Scrollbars bei großen Tilesets und schmalen Panels testen.

### Dropdown und sichtbare Auswahl

- Add component: alle vier Typen sichtbar, vorhandene deaktiviert; fehlende per Maus oder Pfeiltasten/Enter hinzufügen. Escape und Außenklick schließen ohne Kartenverschiebung oder Weiterreichen des Klicks.
- Entity-Erstellen/Löschen nur im Scene-Tab; Komponenten-Entfernen weiterhin an der Karte.
- Entity im Scene-Baum wählen: Map-Rahmen und ID erscheinen auch für Kinder und Transform ohne Sprite. Außerhalb der Ansicht wird die Auswahl sichtbar; erneute Auswahl innerhalb der Ansicht verschiebt die Kamera nicht.
- Auswahl ziehen: keine Kamerabewegung während der Geste. Entfernte Position im Inspector eingeben: Auswahl wird wieder sichtbar. Ohne Transform wird der Hinweis angezeigt.

Diese Abläufe werden zusätzlich im Editor-Smoke geprüft. Bei `--smoke --screenshot /tmp/editor.bmp` entsteht neben dem normalen Bild `/tmp/editor.bmp.dropdown.bmp` mit geöffnetem Komponenten-Dropdown.

Panel-Aktionen: Scene-Werkzeuge und Layer-Aktionsgruppen sitzen rechts; Add component und Import PNG verwenden Inhaltsbreite rechts. Spalten verbreitern/verkleinern und bei langen YAML-Labels den mehrzeiligen Umbruch prüfen. Inspector zeigt Entity-ID und Karten direkt, ohne äußere Entity-Klappüberschrift.

Komponenten-Eigenschaften: Bezeichnung links, Wert rechts. Klick auf Bezeichnung startet keine Bearbeitung; Wertefeld von X, Farbe und Velocity öffnet das richtige Feld. Änderungen, ungültige Farbe und Undo/Redo prüfen. Beim Verbreitern des Inspectors bleiben die Spalten bündig; lange Labels dürfen nicht in Werte hineinzeichnen. Zwei UI-Unity-Tests prüfen bündige Spalten, Resize und begrenzte Geometrie bei Platzmangel.

### Direkte Eingabefelder

- X anklicken: orange Fokusrahmen und blinkender Cursor direkt in der Wertspalte, kein Dialog. Anfangswert ist markiert und kann sofort ersetzt werden.
- Pfeile/Home/End, Shift-Auswahl, Mausposition/Mausauswahl, Backspace/Delete und Ctrl/Cmd+A/C/X/V prüfen. Langer Text scrollt im Feld, ohne andere Spalten zu überzeichnen.
- Enter oder Außenklick übernimmt genau einen gültigen Wert; Undo stellt den vorherigen Wert wieder her. Escape und Fenster-Fokusverlust verwerfen nur den unbestätigten Entwurf.
- Ungültige Zahl/Farbe: roter Rahmen, Statushinweis, Dokument unverändert; Außenklick führt keine darunterliegende Aktion aus. Korrigieren und Enter funktioniert.
- Während Fokus im Textfeld liegt, dürfen Map-Kürzel und gemeinsame Shell keine Tastatureingabe erhalten. Import-/Layer-Dialoge behalten Apply/Cancel und nutzen ebenfalls Cursor/Auswahl.
- Fünf zusätzliche Unity-Tests prüfen Einfügen am Cursor, UTF-8-Grenzen, Auswahl, atomare Puffergrenzen und abgewiesene Steuerzeichen/ungültiges UTF-8. Der Editor-Smoke prüft direkte Eingabe, Cursor/Delete, Bestätigen, Abbrechen, Außenklick, Validierung und Undo.
- Bei `--smoke --screenshot /tmp/editor.bmp` entsteht zusätzlich `/tmp/editor.bmp.input.bmp` mit aktivem Inspector-Feld.

Entity-Namen: im Inspector umbenennen, Scene-Anzeige und unveränderte ID prüfen. Enter/Außenklick, Escape, leere/zu lange Namen, Undo/Redo, Speichern/Laden sowie F6 und Map → Scripts → Map testen. Drei zusätzliche Engine-Tests prüfen Namen, UTF-8/Grenzen, stabile Identität, Undo und alte/ungültige JSON-Dateien.

Tree-Verbindungen: Explorer-Ordner und Scene-Unterbäume öffnen/schließen. Gestrichelte Linien verbinden Parents und Kinder, enden beim letzten Kind und setzen sich für spätere Geschwister korrekt fort. Bei Scrollen bleiben sie an den Zeilen ausgerichtet, auch wenn Parent oder nächste Geschwister außerhalb des Viewports liegen. Tiefe Szenen/Spaltenresize dürfen keine Linien über das Panel hinaus zeichnen; Hover/Auswahl bleiben sichtbar. Zwei UI-Tests prüfen Geschwister, letzte Kinder, eingeklappte Listen und ungültige Indizes.

## Vector Art

Neunzehn SDL-freie `vector_unit_tests` prüfen primitive Formen, Bézier-Konvertierung/-Teilung, Move/Spiegeln, Artboard-Koordinaten und Grenzen, die vier booleschen Operationen, gemeinsame Kanten/Berührungen, Löcher, identische und disjunkte Konturen, große Koordinaten, Nonzero/Even-Odd, SVG-Import/Rundlauf, abgewiesene Dateien und Verlauf mit gespeicherter Revision/Eviction.

`tests/editor_vector_smoke.c` verwendet echte Editor-Ereignisse für Rechteckzeichnen, Verschieben, Ctrl+Z/Y, direkte Inspector-Eingabe, Ankerziehen, Segmentteilung, Artboardverschieben und Zuordnungswechsel beim Ziehen. Der Objects-Tree wird mit echten Klicks auf Pfad-/Artboard-Pfeile und Segmentzeilen geprüft: Aufklappen, Nachfahren ausblenden, Segmentwahl ohne Dokumentänderung sowie Erhalt des Aufklappzustands beim Editorwechsel. Es speichert und öffnet ein temporäres SVG, prüft Speicherrückfragen und erhält Dokument/Verlauf über F6 sowie Vector → Map → Vector → Scripts → Map bei jeweils entladenen anderen Payloads.

Manuell `--vector-demo` starten: Zoom/Pan und Pixel-Lineale, Pfad-/Kontrollpunktgriffe, Circle/Rectangle mit Shift, Artboard-Hintergrund und Konturfarben einschließlich Alpha, zwei Objekte mit Shift auswählen und Union/Intersection/Difference/XOR samt Undo prüfen. SVG aus einem anderen Programm öffnen und Grenzen des unterstützten Teilumfangs beachten. Beim Schließen mit Änderungen in allen drei Editoren Save/Discard/Cancel einschließlich erstem Save-As-Pfad prüfen. Screenshots und Bedienung: [VECTOR_EDITOR.md](VECTOR_EDITOR.md).

Tools-Tab: Rechts Transform/Paths/Boolean/Objects auf- und zuklappen, Aktionen ausführen und Undo prüfen. Die Zeichen-Toolbar bleibt einzeilig. Bei schmaler Spalte brechen Aktionszeilen rechtsbündig um, bei niedriger Fensterhöhe scrollen nur die Tools (nicht Properties). Der GUI-Smoke bedient Flip X, Klappüberschrift, Mausrad, F6 und Editorwechsel; ein Unity-Schematest prüft Tool-Panel-Abschnitte und weist fremde Kindtypen ab.

Artboard-Lineale: Bei `--vector-demo` besitzt jedes Artboard eigene Pixel-Lineale oben/links mit lokalem Nullpunkt. Zweites Artboard aktivieren, verschieben, zoomen und pannen: beide Skalen bleiben ihrem Artboard zugeordnet. Teilweise aus dem Canvas schieben und Fit bei großen Dimensionen prüfen; keine Lineale oder Zahlen dürfen in Nachbarpanels zeichnen. SVG speichern/erneut öffnen: Lineale sind UI, keine SVG-Geometrie.

Vector-Zoom: Der GUI-Smoke prüft Mausrad-Zoom mit stabilem Weltpunkt unter dem Cursor, den Plus-Toolbarbutton mit stabilem Ansichts-Mittelpunkt, Minus/100%, 2–3200%-Grenzen und Fit für beide Demo-Artboards. Navigation verändert den Dokumentverlauf nicht. Manuell müssen die Lineale bei gleichem lokalen Pixelwert mit der Geometrie skalieren, während Leisten und Zahlen ihre Bildschirmgröße behalten.

Preview: Drei zusätzliche Unity-Fälle prüfen kleine zentrierte Bereiche, übergroße Ausschnitte mit Folgebewegung/Clamping, exakt gleiche Quell-/Zielmaße und atomar abgewiesene ungültige Eingaben. Ein Schematest prüft Height-Grenzen und Modulzuordnung. Der GUI-Smoke zieht rückwärts, begrenzt auf Artboards, bricht mit Escape/Fokusverlust ab, rendert auch das zweite Artboard (bekannter RGBA-Pixel), leert die Auswahl und prüft kameraabhängiges Cropping ohne Größenänderung sowie Sitzungserhalt. Manuell 25/100/400%-Zoom und große Auswahlen mit Pan prüfen; die Vorschau bleibt 1:1 und enthält keine Lineale/Griffe.

Grid: Drei zusätzliche Unity-Fälle prüfen 16-Pixel-Linien mit lokalem Ursprung und beschnittenem Pan-Bereich, 32×16-Isometrie, sichere Linienanzahl bei dichten/extremen Rastern und ungültige Eingaben. Ein Schematest prüft Modul- und Höhengrenzen. Der GUI-Smoke klickt den Toolbar-Schalter und das Breitenfeld, ändert Farbe und Typ, weist ungültige Maße/Farben ab und erhält Einstellungen über F6/Editorwechsel ohne Dokumentänderung. Manuell Standard/Isometrie, Zoom/Pan und farbige/halbtransparente Linien prüfen; SVG und Preview bleiben ohne Grid.

Grid-Snapping: Drei weitere Unity-Fälle prüfen Kreisradius/Grenztreffer, negative Koordinaten, ungültige Eingaben ohne Ausgabeänderung, nächste isometrische Kreuzungen auch bei extremem Seitenverhältnis, Zoom-unabhängige Bildschirmtoleranz und dichtes logisches Raster. Der GUI-Smoke prüft freies/gerastetes Verschieben, Loslösen bei mehreren Mausereignissen, Undo/Redo, Mehrfachauswahl mit unveränderten Abständen, Wechsel des Rasterursprungs beim Artboardwechsel, neue Pen-Punkte, Toleranzvalidierung und Erhalt des Schalters/Fangradius beim vollständigen Entladen und Wiederherstellen. Manuell die Preview als erste Tools-Karte, Mehrfachauswahl und artboardübergreifendes Einrasten prüfen.

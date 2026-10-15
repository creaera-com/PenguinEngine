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
9. Scene: Entity anklicken. Map und Inspector werden aktiv, die passende Entity ist ausgewählt. Auswahl/Tabwechsel erzeugen keine Undo-Schritte. Löschen/Undo im Inspector aktualisieren die Scene-Liste. Leere Szenen zeigen einen Hinweis.
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

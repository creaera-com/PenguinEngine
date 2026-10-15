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

1. Buttons/Tabs prüfen: gerade Kanten, flächige Farben, aktives Orange und gut lesbare Beschriftung.
2. Beide Spaltentrenner ziehen, auch über ihren ursprünglichen Bereich hinaus. Mindestbreiten erhalten die mittlere Arbeitsfläche. Keine Tiles oder Texte dürfen dabei verändert werden.
3. Rechts Inspector-Bereiche ein-/ausklappen und die Tile-Palette bei geänderter Spaltenbreite prüfen. Rename und Sortieraktionen bleiben erreichbar.
4. Mit vielen Dateien, vier Ebenen/ausgewählter Entity und langen Scripts Scrollbars prüfen: Mausrad, Thumb-Ziehen und Track-Klicks. Ausgeblendete Controls dürfen nicht anklickbar sein.
5. Map-/Scripts-Tabs umsortieren. Der Inhalt bleibt seinem Tab zugeordnet, Wechsel erhält Text und Kamera.
6. Button drücken und außerhalb loslassen: keine Aktion. Fokusverlust während Drag beendet Capture.
7. `penguin_ui_demo` starten und dieselben UI-Komponenten ohne Engine-/Editorfunktionen prüfen.

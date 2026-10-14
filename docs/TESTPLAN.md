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

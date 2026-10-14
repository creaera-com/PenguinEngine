# PenguinEngine 0.1: So funktioniert die Implementierung

Die erste Basis ist implementiert. Dieses Dokument beschreibt den tatsächlichen Stand; `ENGINE_KONZEPT.md` bleibt der ursprüngliche Plan.

## Module und Zuständigkeiten

| Datei | Aufgabe |
| --- | --- |
| `engine/include/penguin/engine.h` | Öffentliche, von SDL unabhängige C-Datentypen und Funktionen. |
| `engine/src/core.c` | Entities, Tile-Werkzeuge, Undo/Redo, Kamera und Bewegung/Kollision. |
| `engine/src/scene.c` | Validiertes JSON-Laden, Speichern und Projektmanifest. |
| `engine/include/penguin/render.h` | SDL-Rendering und Textur-Lebenszyklus. |
| `engine/src/render.c` | PNG-Laden, Tile-Zeichnung, Entity-Darstellung, Raster und Kollisionsanzeige. |
| `editor/src/main.c` | Editor-Oberfläche, Eingabe, Dateibaum und Bearbeitungsabläufe. |
| `examples/topdown/main.c` | Eigenständige Spielruntime ohne Editor. |
| `tests/test_engine.c` | Unity-Tests der Kernregeln. |

CMake erzeugt die Bibliotheken `penguin`, `penguin_sdl` und `unity` sowie die Programme `penguin_editor`, `penguin_game` und `penguin_tests`.

## Das ECS

Die Szene besitzt 128 Entity-Slots. Eine `PeEntity` besteht aus Slot-Index und Generation. Beim Löschen wird die Generation erhöht; alte Kennungen sind dadurch ungültig. Erschöpfte Kapazität wird mit einer ungültigen Entity gemeldet. `pe_entity_get` liefert für ungültige Kennungen `NULL`.

Die erste kompakte Implementierung speichert Komponentendaten gemeinsam in `PeObject`. Die Maske entscheidet, welche Daten verwendet werden:

- `PE_TRANSFORM`: Position und Größe.
- `PE_SPRITE`: Darstellung mit einer RGBA-Farbe.
- `PE_BODY`: Geschwindigkeit und Kollision gegen das Raster.
- `PE_PLAYER`: Geschwindigkeit aus Spieler-Eingabe ableiten.

Systeme sind C-Funktionen mit Schleifen über passende Slots. Es gibt keine Vererbung und keine Archetypen. Eine spätere Aufteilung in getrennte Komponentenarrays bleibt möglich; sie ist in dieser Version noch nicht umgesetzt.

```c
PeScene scene;
pe_scene_init(&scene, 32, 24, 16);
PeEntity entity = pe_entity_create(&scene, PE_TRANSFORM | PE_SPRITE | PE_BODY | PE_PLAYER);
PeObject *player = pe_entity_get(&scene, entity);
if (player) {
    player->x = 64;
    player->y = 64;
}
```

Komponentenpointer verweisen in die jeweilige Szene. Nach Löschen, Laden, Undo/Redo oder Wechsel der Welt müssen sie neu aufgelöst werden. Der Editor hält deshalb eine stabile Objekt-ID, keinen dauerhaften Komponentenpointer. Undo stellt auch Entity-Lebenszyklen wieder her; Laufzeitkennungen werden nicht als externe dauerhafte Referenzen verwendet.

## Tilemaps

Eine Szene besitzt genau eine endliche Tilemap mit bis zu 64 × 64 Feldern und vier Ebenen. Eine Ebene enthält Name, Sichtbarkeit und Tile-IDs. Das interne Raster hat stets einen Zeilenabstand von `PE_MAP_MAX`; im JSON werden nur die aktiven Felder zeilenweise gespeichert.

Tile-ID 0 ist leer. IDs ab 1 beziehen sich auf das PNG-Raster: links nach rechts, anschließend von oben nach unten. Es gibt zunächst ein Tileset pro Szene. PNG-Abmessungen müssen durch die eingestellte Source-Tilegröße teilbar sein. Bilder sind auf 8192 Pixel je Achse und Tilesets auf 65535 Tiles begrenzt.

Die Welt-Tilegröße und die Source-Tilegröße sind getrennt. Ein 16-Pixel-Bildausschnitt kann beispielsweise auf einem 32-Pixel-Weltraster dargestellt werden. Source-Tilegröße lässt sich im Inspector ändern. Ränder und Abstände zwischen PNG-Tiles unterstützt diese Version noch nicht.

Kollisionen liegen in einem separaten Raster aus 0 und 1. Die `Collision`-Schaltfläche schaltet Pinsel, Radierer, Rechteck und Füllen auf dieses Raster um. Unsichtbare Ebenen entfernen keine Kollisionen.

## Commands und Undo/Redo

Die erste Version nutzt **Szenen-Snapshots pro Command**, statt Änderungen für jedes Werkzeug einzeln zu kodieren. Die begrenzte Szenengröße macht das überschaubar und erfasst Entity-, Ebenen-, Tile-, Kollisions- und Tileset-Änderungen über denselben Weg.

1. `pe_history_begin` kopiert den Zustand vor der Aktion.
2. Der Editor verändert die Szene.
3. `pe_history_commit` speichert den Zustand danach.
4. Undo stellt „vorher“, Redo „nachher“ wieder her.

```c
if (pe_history_begin(&history, &scene)) {
    pe_rectangle(&scene, 0, 2, 2, 5, 5, 3);
    pe_history_commit(&history, &scene);
}
```

Ein Command ohne Änderung wird nicht eingetragen. Pinselbewegungen bleiben bis zum Loslassen in einer Transaktion; zwischen Mauspositionen werden fehlende Felder interpoliert. Escape oder Fokusverlust verwirft laufende Änderungen. Schlägt das Anlegen des Nachher-Snapshots fehl, wird die Änderung zurückgenommen.

Die Historie enthält maximal 64 Commands. Alte Einträge werden freigegeben. Neue Bearbeitung nach Undo verwirft den Redo-Zweig. Eindeutige Revisionsnummern markieren den gespeicherten Zustand, damit eine neue Verzweigung nicht versehentlich als gespeichert gilt.

Ansichtszoom, Auswahl und Spielsimulation gehören nicht zur Szenenhistorie. Import-Undo stellt die vorherige Tileset-Referenz wieder her; die importierte Datei bleibt erhalten, damit Redo und andere Referenzen funktionieren. Eine Historie über Programmstarts hinweg und allgemeine Dateisystem-Undo-Aktionen gibt es noch nicht.

## Bildimport und Dateibaum

PNG-Dateien können ins Fenster gezogen oder über `Import PNG` mit einem vollständigen Dateipfad importiert werden. Die Datei wird unter einem neuen Namen nach `assets/` kopiert, geprüft und als Tileset ausgewählt. Vorhandene PNGs im Dateibaum können direkt ausgewählt werden. Ein ungültiger neuer Import wird entfernt und verändert die Szene nicht.

Der Dateibaum nutzt die plattformübergreifenden SDL3-Dateifunktionen. Ordner lassen sich auf-/zuklappen; Mausrad scrollt den Baum. `Refresh` liest Änderungen durch externe Programme neu ein. Die Anzeige ist auf 256 Einträge und acht Verzeichnisebenen begrenzt.

In dieser Version sind Asset-Referenzen relative Pfade, **noch keine Asset-UUIDs oder ein Asset-Katalog**. Dateien extern umzubenennen bricht Referenzen. Bei fehlenden oder ungültigen Tiles werden Platzhalter gezeichnet und ein Fehler angezeigt. Umbenennen und Löschen von Assets im Editor wird erst mit einem Referenzsystem ergänzt.

## Speichern und Laden

`project.json` enthält `version`, `name` und `start_scene`. Nur `version` und `start_scene` werden für den Start benötigt. Projekt- und Szenenformat haben Version 1.

Eine Szene speichert Rastergröße, Welt-/Source-Tilegröße, PNG-Pfad, Ebenen, Kollisionsraster und Entities. Eine Entity speichert ihre stabile ID, Maske, Position, Größe und Farbe. Geschwindigkeit ist Laufzeitzustand und wird beim Laden auf null gesetzt.

Beim Laden werden Typen, Wertebereiche, Arraylängen und doppelte Entity-IDs geprüft. Asset- und Startszene-Pfade müssen relativ sein und dürfen keine `..`-Segmente oder Laufwerksangaben enthalten. Der Loader baut eine temporäre Szene und ersetzt das Ziel erst nach erfolgreicher Prüfung.

Speichern schreibt zunächst eine temporäre Datei neben das Ziel. Nach erfolgreichem Schließen wird sie durch `rename` beziehungsweise `MoveFileExA` an die Zielposition verschoben. Schreibfehler werden gemeldet, die ursprüngliche Datei bleibt erhalten. Diese Basis ist nicht für paralleles Bearbeiten derselben Datei durch mehrere Prozesse vorgesehen und garantiert keine Wiederherstellung nach Stromausfall.

Der Editor fragt beim Öffnen einer anderen Szene und beim Beenden nach ungespeicherten Änderungen. Speichern, Verwerfen und Abbrechen sind möglich. Eine neue Szene wird durch Kopieren einer vorhandenen Szenendatei angelegt; ein grafischer Projekt-/Szenenassistent folgt später.

## Kamera und Rendering

Weltkoordinaten sind Pixel, X zeigt nach rechts und Y nach unten. Die Kamera speichert die obere linke Weltposition und einen Zoom von 25 bis 800 Prozent. Zum Zoomen wird die Weltposition unter dem Cursor vor und nach der Änderung gleich gehalten.

Der Renderer zeichnet sichtbare Tile-Ebenen, danach Entity-Rechtecke. Die erste Version hat noch keine frei konfigurierbare Sprite-Sortierung. Die Szene ist auf den mittleren Viewport begrenzt. PNG-Texturen verwenden nearest-neighbor filtering. Subpixel-Zoom ist im Editor erlaubt; eine feste interne Spielauflösung mit ganzzahliger Skalierung ist noch nicht umgesetzt.

Die eigene Editor-GUI verwendet SDL-Rechtecke und `SDL_RenderDebugText`. Sie benötigt keine zusätzliche GUI-Abhängigkeit. Textfelder akzeptieren SDL-Texteingabe; Backspace entfernt ein UTF-8-Zeichen, aber die Debug-Schrift stellt nicht alle Unicode-Zeichen dar. Textauswahl, Zwischenablage, Docking und frei verschiebbare Panels sind noch nicht vorhanden.

## Play-Modus und Kollision

Play kopiert die gesamte bearbeitete Szene in eine unabhängige Laufzeitwelt. Bildtexturen werden unverändert gemeinsam verwendet. Stop verwirft die Laufzeitwelt. Tests prüfen, dass Bearbeitungsdaten durch Bewegung im Play-Modus unverändert bleiben.

Die Hauptschleife sammelt vergangene Zeit und führt Simulation mit 1/60 Sekunde aus. Pro Frame werden maximal 0,1 Sekunden aufgeholt. Diagonale Spielerbewegung wird normalisiert. Eine Entity mit Body bewegt sich gegen das Kollisionsraster und die Kartengrenzen.

Bewegung wird in kleine Schritte unterteilt und pro Achse geprüft, damit Körper an Wänden entlanggleiten können. Extrem große oder nicht endliche Bewegungen werden verworfen. Bei einfachen Wandkontakten wird die Bewegung schrittweise blockiert; es gibt noch keine exakte Kontaktauflösung. Kollision zwischen Entities, Trigger-Ereignisse, Schwerkraft, Sprünge und dynamische Physikkörper sind noch nicht implementiert.

## Tests und CI

Unity 2.6.1 prüft 32 Regeln, unter anderem Entity-Lebenszyklen, Kapazität, Rastergrenzen, Füllen, gruppierte Commands, Historienbegrenzung, gespeicherte Revisionen, Kameraanker, Kollision, Dateiformate und abgelehnte Szenendaten.

Der Editor-Smoke-Test verwendet dieselben Eingabehandler wie Maus und Tastatur: Malen, Undo, Redo und Play/Stop werden ohne Desktop geprüft. Ein weiterer Smoke-Test startet und rendert das Beispielspiel. SDL-Dummy-Tests ersetzen keine manuelle Desktop-Bedienungsprüfung.

In der lokalen verwalteten Umgebung verhindert die Prozessüberwachung LeakSanitizer (ptrace-Einschränkung). Hier werden AddressSanitizer und UndefinedBehaviorSanitizer mit `ASAN_OPTIONS=detect_leaks=0` ausgeführt; der CI-Job deaktiviert die Leak-Prüfung nicht.

GitHub Actions baut auf Linux, Windows und macOS und führt CTest aus. Ein zusätzlicher Linux-Job prüft den Kern mit AddressSanitizer und UndefinedBehaviorSanitizer. Ergebnisse anderer Betriebssysteme gelten erst als verifiziert, wenn die jeweiligen CI-Jobs bestanden haben.

## Nächste sinnvolle Erweiterungen

1. Eigene Projekte und Karten im Editor anlegen, Rastergröße ändern und Tile-Bereiche kopieren.
2. Bildbasierte Sprite-Komponente, Animationen und Zeichenreihenfolge.
3. Asset-Katalog mit stabilen Kennungen und sicherem Umbenennen.
4. Getrennte Komponentenarrays und dynamische Kapazitäten bei konkretem Bedarf.
5. Shooter-Beispiel mit Projektilen, Trefferregeln und Kamera-Scrolling.
6. Plattformbewegung mit Bodenprüfung, Sprung, Schwerkraft und Übergängen zwischen Räumen.
7. QuickJS und dokumentierte Script-Lebenszyklen.
8. Dialogeditor auf derselben Command- und Asset-Basis.

Diese Erweiterungen brauchen eigene Datenmodelle und Tests und gehören nicht zum bereits ausgelieferten Funktionsumfang.

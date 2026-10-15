# PenguinEngine 0.1: So funktioniert die Implementierung

Die erste Basis ist implementiert. Dieses Dokument beschreibt den tatsächlichen Stand; `ENGINE_KONZEPT.md` bleibt der ursprüngliche Plan.

## Module und Zuständigkeiten

| Datei | Aufgabe |
| --- | --- |
| `engine/include/penguin/engine.h` | Öffentliche, von SDL unabhängige C-Datentypen und Funktionen. |
| `engine/src/core.c` | Entities, Tile-Werkzeuge, Undo/Redo, Kamera und Bewegung/Kollision. |
| `engine/src/text.c` | UTF-8-Textpuffer, Auswahl, Undo/Redo und sichere Script-Dateioperationen. |
| `engine/src/javascript.c` | QuickJS-ng-Syntaxprüfung, lexikalisches Highlighting und einfache Vervollständigung. |
| `editor/src/script_editor.c` | Textansicht, Eingaben, Zeilennummern, Vorschläge und Fehleranzeige. |
| `engine/src/project.c` | Projektsuche relativ zur Programmdatei und explizite Projektpfade. |
| `engine/src/scene.c` | Validiertes JSON-Laden, Speichern und Projektmanifest. |
| `engine/include/penguin/render.h` | SDL-Rendering und Textur-Lebenszyklus. |
| `engine/src/render.c` | PNG-Laden, Tile-Zeichnung, Entity-Darstellung, Raster und Kollisionsanzeige. |
| `ui/src/core.c`, `ui/src/sdl.c` und öffentliche UI-Header | Gemeinsame Widget-Logik, Flat-Rendering, Icons und Themes für Editor und Spiele. |
| `editor/src/main.c` | Editor-Oberfläche, Eingabe, Dateibaum und Bearbeitungsabläufe. |
| `examples/topdown/main.c` | Eigenständige Spielruntime ohne Editor. |
| `tests/test_engine.c` | Unity-Tests der Kernregeln. |

CMake erzeugt die Bibliotheken `penguin`, `penguin_sdl` und `unity` sowie die Programme `penguin_editor`, `penguin_game`, `penguin_tests` und `penguin_script_tests`. `penguin_script` enthält die von SDL unabhängigen Script-Werkzeuge und nutzt die vendorte `penguin_js_parser`-Bibliothek. Das eigenständige Beispielspiel bindet diese Bibliotheken nicht ein.

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

## Projektsuche beim Start

Ein expliziter `--project`-Pfad hat Vorrang. Ist er ungültig, wird ein Fehler mit Bedienhinweis gemeldet; es wird kein anderes Projekt geöffnet. Ohne Argument suchen beide Programme zuerst `example/` neben der ausführbaren Datei, danach `../example/` (für Visual-Studio-Konfigurationen wie `Debug/`). Erst dann folgen `example/`, `examples/topdown/` und ein Projekt direkt im Arbeitsverzeichnis. `SDL_GetBasePath` liefert den Programmpfad; die Suchfunktion selbst bleibt SDL-unabhängig und wird mit Unity geprüft.

Die Build-Targets für Editor und Spiel hängen ausdrücklich von der erstmaligen Beispielinitialisierung ab. Existierende Projekte werden dabei nicht überschrieben. Zwei zusätzliche Smoke-Tests starten beide Programme ohne Projektargument aus einem unbeteiligten Arbeitsverzeichnis.

## Speichern und Laden

`project.json` enthält `version`, `name` und `start_scene`. Nur `version` und `start_scene` werden für den Start benötigt. Projekt- und Szenenformat haben Version 1.

Eine Szene speichert Rastergröße, Welt-/Source-Tilegröße, PNG-Pfad, Ebenen, Kollisionsraster und Entities. Eine Entity speichert ihre stabile ID, Maske, Position, Größe und Farbe. Geschwindigkeit ist Laufzeitzustand und wird beim Laden auf null gesetzt.

Beim Laden werden Typen, Wertebereiche, Arraylängen und doppelte Entity-IDs geprüft. Asset- und Startszene-Pfade müssen relativ sein und dürfen keine `..`-Segmente oder Laufwerksangaben enthalten. Der Loader baut eine temporäre Szene und ersetzt das Ziel erst nach erfolgreicher Prüfung.

Speichern schreibt zunächst eine temporäre Datei neben das Ziel. Nach erfolgreichem Schließen wird sie durch `rename` beziehungsweise `MoveFileExA` an die Zielposition verschoben. Schreibfehler werden gemeldet, die ursprüngliche Datei bleibt erhalten. Diese Basis ist nicht für paralleles Bearbeiten derselben Datei durch mehrere Prozesse vorgesehen und garantiert keine Wiederherstellung nach Stromausfall.

Der Editor fragt beim Öffnen einer anderen Szene und beim Beenden nach ungespeicherten Änderungen. Speichern, Verwerfen und Abbrechen sind möglich. Eine neue Szene wird durch Kopieren einer vorhandenen Szenendatei angelegt; ein grafischer Projekt-/Szenenassistent folgt später.

## Kamera und Rendering

Weltkoordinaten sind Pixel, X zeigt nach rechts und Y nach unten. Die Kamera speichert die obere linke Weltposition und einen Zoom von 25 bis 800 Prozent. Zum Zoomen wird die Weltposition unter dem Cursor vor und nach der Änderung gleich gehalten.

Der Renderer zeichnet sichtbare Tile-Ebenen, danach Entity-Rechtecke. Die erste Version hat noch keine frei konfigurierbare Sprite-Sortierung. Die Szene ist auf den mittleren Viewport begrenzt. PNG-Texturen verwenden nearest-neighbor filtering. Subpixel-Zoom ist im Editor erlaubt; eine feste interne Spielauflösung mit ganzzahliger Skalierung ist noch nicht umgesetzt.

Die Editor-GUI verwendet die gemeinsame `penguin_ui`-/`penguin_ui_sdl`-Bibliothek: dunklere graue Panels, orange Akzente, gerade Kanten und einfarbige Buttons/Tabs ohne Rundungen. Text nutzt eingebettete JetBrains Mono über `ui_text`; Standardfarben und `UiTheme` sind in `ui/include/penguin/ui_sdl.h` definiert. Komponenten, API und Verwendung in Spielen erklärt [UI_BIBLIOTHEK.md](UI_BIBLIOTHEK.md). Die Icons werden mit SDL-Linien und Flächen gezeichnet, benötigen keine Icon-Schrift oder Bilddateien und folgen den Hover-, Auswahl- und Deaktiviert-Zuständen. Werkzeugbuttons zeigen Tooltips mit Namen und Tastenkürzeln. Die kompakte Toolbar enthält keinen App-Titel; EDIT/PLAY und Saved/Modified stehen unten in der Statuszeile zusammen mit Zoom, Tile-Auswahl und Undo-Verlauf. Panel-Positionen richten sich nach der Toolbar-Höhe und den ziehbaren Spaltentrennern. Inspector-Bereiche sind klappbar, Tabs umsortierbar und Scrollbars werden bei überlaufendem Inhalt angezeigt. Die Seitenpanels bieten eigene Tabgruppen (Explorer/Scene und Inspector/Info). Scene ermöglicht die Entity-Auswahl; Info zeigt Szenenstatistik und Tastenkürzel. Die verstellbaren Spalten nutzen durch drei mittige Punkte markierte Griffe im Zwischenraum ohne gezeichnete Trennlinie. Sie benötigt keine zusätzliche GUI-Abhängigkeit. Textfelder akzeptieren SDL-Texteingabe; Backspace entfernt ein UTF-8-Zeichen, aber die Schrift stellt ASCII und Latin-1 dar, weitere Unicode-Zeichen erscheinen als Platzhalter. Der Script-Editor ergänzt Textauswahl und Zwischenablage. Die kleinen Inspector-/Modal-Felder bleiben einfache Eingaben. Docking und frei verschiebbare Panels sind noch nicht vorhanden.

## Play-Modus und Kollision

Play kopiert die gesamte bearbeitete Szene in eine unabhängige Laufzeitwelt. Bildtexturen werden unverändert gemeinsam verwendet. Stop verwirft die Laufzeitwelt. Tests prüfen, dass Bearbeitungsdaten durch Bewegung im Play-Modus unverändert bleiben.

Die Hauptschleife sammelt vergangene Zeit und führt Simulation mit 1/60 Sekunde aus. Pro Frame werden maximal 0,1 Sekunden aufgeholt. Diagonale Spielerbewegung wird normalisiert. Eine Entity mit Body bewegt sich gegen das Kollisionsraster und die Kartengrenzen.

Bewegung wird in kleine Schritte unterteilt und pro Achse geprüft, damit Körper an Wänden entlanggleiten können. Extrem große oder nicht endliche Bewegungen werden verworfen. Bei einfachen Wandkontakten wird die Bewegung schrittweise blockiert; es gibt noch keine exakte Kontaktauflösung. Kollision zwischen Entities, Trigger-Ereignisse, Schwerkraft, Sprünge und dynamische Physikkörper sind noch nicht implementiert.

## Map-/Scripts-Tabs und Texteditor

Die Tabs liegen über der mittleren Arbeitsfläche. Ein Wechsel erhält Karte, Kamera, Dokument, Cursor und Scrollposition. Szene und Script haben getrennte Undo-Verläufe; die Toolbar und Saved/Modified-Anzeige beziehen sich auf den aktiven Tab. Während Play bleiben Scripts und Textbearbeitung gesperrt.

Der Script-Editor hält zunächst ein Dokument. Beim Start wird `scripts/player.js` oder ein vorhandenes `scripts/new_script.js` geladen. Ohne beides beginnt ein leeres Dokument, das beim ersten Speichern als `scripts/new_script.js` angelegt wird. Andere `.js`-Dateien werden über Dateibaum oder Drag-and-drop geöffnet. Wechsel und Beenden berücksichtigen ungespeicherten Text; beim Beenden können Szene und Script gemeinsam gespeichert werden.

`PeText` speichert maximal 64 KiB UTF-8, Cursor und Auswahlanker. Einfügen, Löschen, Paste und Completion sind atomare Text-Commands. Jeder SDL-Texteingabevorgang ist ein Undo-Schritt; die Historie umfasst 64 Schritte. Cursor-Navigation erzeugt keine Änderungen. UTF-8-Zeichen werden vollständig entfernt, Tabulatoren werden in vier Spalten dargestellt und CRLF wird beim Laden/Paste normalisiert. Der eingebettete Schriftatlas unterstützt ASCII und Latin-1; nicht unterstützte Zeichen bleiben im Text erhalten und erscheinen als Platzhalter.

QuickJS-ng wird mit `JS_EVAL_FLAG_COMPILE_ONLY` verwendet. Nach erfolgreichem Parsen bricht ein eigener Module-Loader die Import-Auflösung absichtlich ab; es werden keine fremden Module geladen. Syntaxfehler bleiben Fehler, fehlende Importdateien sind für diese Prüfung irrelevant. Das Prüf-Runtime ist auf 64 MiB Speicher und 256 KiB JavaScript-Stack begrenzt. Die Prüfung erfolgt nach 300 ms Schreibpause im sichtbaren Tab oder explizit mit F7. Sie kennt keine Gameplay-Bindings und führt keine Statements aus.

Highlighting ist ein eigener einfacher Lexer für Keywords, Standardobjekte, Zahlen, Strings und Kommentare. Template-Strings werden als ein String eingefärbt; Regex-Literale und Template-Interpolation haben noch kein vollständiges eigenes Highlighting. Die Syntaxprüfung verwendet unabhängig davon den echten JavaScript-Parser.

Completion schlägt Keywords, Standardobjekte, `Math`-/`JSON`-/`console`-Methoden und Bezeichner aus dem aktuellen Text vor. Vorschläge berücksichtigen keine Scopes oder Typen und werden in Strings/Kommentaren unterdrückt. Cursor und Dokumentrevision werden vor dem Übernehmen geprüft, sodass veraltete Vorschläge keinen anderen Text ersetzen. Strg+Leertaste öffnet die Liste, Pfeiltasten wählen und Enter/Tab übernimmt.

Für eine spätere umfangreiche Completion wäre eine LSP-Anbindung, etwa an einen TypeScript/JavaScript Language Server, passend. Das bräuchte einen zusätzlichen Serverprozess, Dokument-Synchronisation und dessen Laufzeit/Abhängigkeiten. Die Engine selbst kann dabei in C bleiben. Dies ist noch nicht eingebaut.

## Tests und CI

Unity 2.6.1 prüft 33 Engine-Regeln, unter anderem Entity-Lebenszyklen, Kapazität, Rastergrenzen, Füllen, gruppierte Commands, Historienbegrenzung, gespeicherte Revisionen, Kameraanker, Kollision, Dateiformate und abgelehnte Szenendaten.

32 weitere Unity-Tests prüfen Textbearbeitung, UTF-8-Grenzen, Dateioperationen, Parserfehler mit Zeilenangabe, Compile-only-Verhalten, Imports, Highlighting und Vervollständigung. 26 zusätzliche Unity-Tests prüfen die SDL-unabhängige UI-Bibliothek. Insgesamt laufen 91 Unity-Tests in drei Suiten und zehn CTest-Prüfungen.

Ein zusätzlicher CMake-Test prüft, dass die Beispielkopie korrekt eingerichtet wird und erneute Builds bearbeitete Dateien nicht überschreiben.

Der Editor-Smoke-Test verwendet dieselben Eingabehandler wie Maus und Tastatur: Malen, Undo, Redo und Play/Stop werden ohne Desktop geprüft. Die Editor-Smoke-Tests prüfen zusätzlich Spalten-Resize, Tab-Reihenfolge, klappbare Bereiche und Script-Scrollbar-Capture ohne Änderungen an Szene/Text sowie Script-Eingabe, Completion, Undo/Redo, Syntaxfehler, den Schutz ungespeicherter Scripts und die Trennung von Szenenänderungen. Ein Test rendert ausdrücklich den Scripts-Tab. Ein weiterer Smoke-Test startet und rendert das Beispielspiel. SDL-Dummy-Tests ersetzen keine manuelle Desktop-Bedienungsprüfung.

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

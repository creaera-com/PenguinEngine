# PenguinEngine

Eine kleine 2D-Engine in **C17 mit SDL3**, einem einfachen ECS und einem Tilemap-Editor. Die erste Version enthält ein spielbares Top-down-Beispiel und **Unity-Unit-Tests**.

## Enthalten

- Entities mit Generationen und Komponentenmasken; feste, geprüfte Kapazitäten.
- Tilemaps mit bis zu vier Ebenen und separatem Kollisionsraster.
- Projekt-Dateibaum, PNG-Import per Drag-and-drop oder Dateipfad, Tile-Palette.
- Pinsel, Radierer, Rechteck, Füllen, Entity-Auswahl und Verschieben.
- Ebenen hinzufügen, löschen, umbenennen, sortieren und ausblenden.
- Inspector für Position, Größe und Spielersteuerung.
- Undo/Redo für Szenenänderungen, gruppierte Pinselstriche und Änderungsanzeige.
- JSON-Speichern/Laden, Schema-Prüfung und Rückfrage bei ungespeicherten Änderungen.
- Play-/Stop-Modus mit separater Laufzeitwelt; unabhängiges Beispielspiel.
- Bewegung mit festem 60-Hz-Zeitschritt und einfacher Tilemap-Kollision.

**Noch nicht enthalten:** JavaScript/Python, Dialogeditor, Shooter-Logik, Plattformphysik, Animationen, Audio, Asset-Umbenennung und Tile-Auswahl/Kopieren. Diese Erweiterungen bleiben im Entwicklungsplan.

## Bauen und starten

Voraussetzungen: C17-Compiler, CMake >= 3.20 und Git. SDL3 >= 3.2 wird verwendet, wenn es als CMake-Paket installiert ist. Andernfalls lädt CMake **SDL3 3.2.10** mit geprüftem SHA-256 herunter. Dafür ist beim ersten Konfigurieren Internetzugriff nötig.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Linux benötigt zusätzlich SDL-Entwicklungsvoraussetzungen, mindestens ein Desktop-Backend. Für Debian/Ubuntu genügt für einen X11-/Software-Renderer-Build beispielsweise:

```sh
sudo apt-get install build-essential cmake pkg-config libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev
```

Start von Linux/macOS aus dem Repository-Ordner:

```sh
./build/penguin_editor --project build/example
./build/penguin_game --project build/example
```

Unter Windows mit Visual Studio 2022 und installierter C++-Desktop-Workload:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\penguin_editor.exe --project build/example
.\build\Debug\penguin_game.exe --project build/example
```

Das Beispiel wird in `build/example` kopiert. **Eigene Änderungen dort vor einem erneuten Build in einen eigenen Projektordner sichern**, denn der Build kopiert die Vorlage erneut. Dauerhafte eigene Projekte außerhalb des Build-Ordners öffnen:

```sh
./build/penguin_editor --project /pfad/zu/meinem-projekt
```

Zum Anlegen eines eigenen Projekts zunächst `examples/topdown/project.json`, `assets/` und `scenes/` in einen eigenen Ordner kopieren. Projektname und Startszene lassen sich in `project.json` ändern. Kartenabmessungen und Welt-Tilegröße werden in dieser Version über die Szenendatei festgelegt.

## Tests ohne SDL oder Fenster

Unity 2.6.1 ist im Repo enthalten; die Kern-Tests benötigen keinen Download.

```sh
cmake -S . -B build-core -DPENGUIN_BUILD_EDITOR=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

Mit GCC/Clang lassen sich zusätzliche Speicher- und Undefined-Behavior-Prüfungen aktivieren:

```sh
cmake -S . -B build-asan -DPENGUIN_BUILD_EDITOR=OFF -DPENGUIN_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

Die vollständige Testsuite enthält außerdem Headless-Smoke-Tests mit dem SDL-Dummy-Treiber. In einer Linux-Umgebung ohne X11-/Wayland-Entwicklungsbibliotheken kann ausschließlich für Headless-Prüfungen `-DSDL_UNIX_CONSOLE_BUILD=ON` verwendet werden; dieser Build ersetzt keinen Desktop-Build.

## Bedienung

| Aktion | Eingabe |
| --- | --- |
| Malen / aktives Werkzeug | Linke Maustaste |
| Löschen / Kollision entfernen | Rechte Maustaste |
| Ansicht verschieben | Mittlere Maustaste ziehen |
| Zoom am Mauszeiger | Mausrad über der Szene |
| Pinsel / Radierer / Rechteck / Füllen / Entity-Auswahl | B / E / R / F / V |
| Speichern | Strg+S, unter macOS auch Command+S |
| Undo / Redo | Strg+Z / Strg+Umschalt+Z oder Strg+Y |
| Play / Stop | F5 oder Schaltfläche |
| Spielfigur bewegen | WASD oder Pfeiltasten |
| Entity löschen | Entf im Editor |
| Eingabe bestätigen / abbrechen | Enter / Escape |
| Textfeld leeren | Strg+A / Command+A |

`Collision` schaltet die Werkzeuge auf das separate Kollisionsraster um. Rote Felder blockieren die Figur; sichtbare Wand-Tiles und Kollisionen sind unabhängig. `Select` wählt Entities und erlaubt Ziehen. Ein Pinselstrich oder Ziehen ist ein Undo-Schritt. Escape verwirft eine laufende Bearbeitung.

## Dokumentation

- [Bedienung, Architektur und Erweiterungen](docs/IMPLEMENTIERUNG.md)
- [Ursprüngliches Konzept und Entwicklungsplan](docs/ENGINE_KONZEPT.md)
- [Manueller Prüfablauf](docs/TESTPLAN.md)
- [Abhängigkeiten und Lizenzen](third_party/README.md)

Die Engine ist eine ausbaubare Basis. Sprite-Entities werden zunächst als farbige Rechtecke dargestellt; Bildausschnitte werden bei Tilemaps unterstützt. Die Undo-Historie liegt im Speicher und umfasst maximal 64 Änderungen. Importierte Dateien bleiben beim Undo erhalten. Weitere Grenzen und Abweichungen vom ursprünglichen Konzept beschreibt die Implementierungsdokumentation.

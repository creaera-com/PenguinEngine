# PenguinEngine: Konzept und Architektur

Stand: 14. Oktober 2026

**Status: Ursprüngliches Planungsdokument. Die erste Basis ist inzwischen implementiert; den tatsächlichen Stand beschreibt [IMPLEMENTIERUNG.md](IMPLEMENTIERUNG.md).** Die hier beschriebenen APIs, Ordner und Funktionen sind Vorschläge für die gemeinsame Umsetzung. Entscheidungen können wir beim Aufbau des Repositories anpassen.

## 1. Ziel

Wir bauen eine überschaubare 2D-Engine in C mit SDL3. Dazu gehört ein Editor, mit dem sich Szenen und Tilemaps erstellen lassen. Die Engine soll langfristig drei Arten von Spielen unterstützen:

- Vertikale oder horizontale Shooter im Stil von TwinBee.
- Top-down-Abenteuer im Stil der klassischen Zelda-Spiele.
- Metroidvanias mit seitlicher Ansicht, Plattformen und verbundenen Räumen.

Diese Spiele teilen viele Grundlagen: Bilder, Animationen, Eingabe, Szenen, Entities, Tilemaps, Kollisionen und Audio. Unterschiede wie Schwerkraft, Kamera, Gegnerverhalten und Raumwechsel gehören in spezialisierte Systeme oder später in Spielscripts.

Wir bauen zunächst eine kleine durchgängig nutzbare Basis. Dialoge, umfangreiche Physik und Scripting folgen, wenn das Bearbeiten, Speichern und Spielen einer Szene zuverlässig funktioniert.

## 2. Grundentscheidungen

| Bereich | Vorgeschlagene Lösung | Zweck |
| --- | --- | --- |
| Sprache | C17 | Engine und Editor bleiben in C. |
| Plattform | SDL3 | Fenster, Eingabe, Zeit, Rendering und später Audio. |
| Rendering | Zunächst SDL_Renderer | Einfache 2D-Darstellung ohne eigenen Grafik-Backend-Aufbau. |
| Build | CMake | Gemeinsamer Build für unterstützte Betriebssysteme. |
| Editor-GUI | Nuklear, Entscheidung beim ersten Editor-Meilenstein | Eine C-basierte Oberfläche für Dateibaum, Inspector und Werkzeuge. |
| Bildimport | SDL3_image, zunächst PNG | Tilesets und Sprites laden. |
| Projektdateien | JSON mit Formatversion | Lesbare, überprüfbare Projekt- und Szenendaten. |
| Scripting | Später bevorzugt QuickJS | JavaScript für Gameplay, eingebettet in C. |
| Physik | Zunächst eigene einfache Kollisionen | Bewegung gegen Tilemaps und einfache Formen. |

Die SDL3-Anbindung der Editor-GUI müssen wir selbst integrieren und prüfen. Eine reine SDL-Oberfläche wäre für einen Prototyp möglich, aber Dateibaum, Texteingabe und Inspector profitieren von einer GUI-Bibliothek.

JSON wird über eine kleine C-Bibliothek verarbeitet. Welche Bibliothek wir verwenden, entscheiden wir beim Einrichten der Abhängigkeiten. Versionsstände und Lizenzen werden im Repo festgehalten.

## 3. Engine, Editor und Spiel

Die **Engine** enthält wiederverwendbare Funktionen: Assets laden, eine Szene verwalten, Entities aktualisieren und die Welt zeichnen.

Der **Editor** verwendet diese Engine und ergänzt Dateibrowser, Werkzeuge, Auswahl, Inspector, Speichern sowie Undo/Redo.

Das **Spiel** verwendet die Engine ohne Editor. Es lädt die erstellten Daten und führt Spiellogik aus. Dadurch braucht ein später veröffentlichtes Spiel keine Editor-Oberfläche.

Abhängigkeitsrichtung:

```text
Editor ──> Engine ──> SDL3 und Hilfsbibliotheken
Spiel  ──> Engine
```

Die Engine kennt keine Editor-Fenster oder Editor-Werkzeuge. Editor-Code darf Engine-Funktionen aufrufen; Engine-Code darf keine Editor-Funktionen voraussetzen.

## 4. Ursprünglich vorgeschlagene Repository-Struktur

Die aktuelle Struktur unter `src/` und `resources/` beschreibt [PROJEKTSTRUKTUR.md](PROJEKTSTRUKTUR.md). Der folgende Baum dokumentiert die ursprüngliche Planung.

```text
repository/
  CMakeLists.txt
  README.md
  docs/
    ENGINE_KONZEPT.md
  engine/
    include/          Öffentliche C-Schnittstellen
    src/
      core/           Zeit, Speicherhilfen, Logging
      platform/       SDL3, Fenster und Eingabe
      ecs/            Entities und Komponenten
      assets/         Bilder und Asset-Referenzen
      scene/          Szenen und Serialisierung
      render/         Sprites, Tilemaps und Kamera
      collision/      Später Kollision und Bewegung
      scripting/      Später Script-Anbindung
  editor/
    src/
      ui/             Dateibaum, Inspector und Palette
      tools/          Tile-Werkzeuge und Auswahl
      commands/       Undo/Redo-Befehle
  examples/
    topdown/
      project.json
      scenes/
      assets/
  tests/
  third_party/        Falls Bibliotheken mitgeliefert werden
```

Diese Struktur beschreibt Zuständigkeiten. Wir erstellen Ordner erst, wenn sie tatsächlich gebraucht werden.

## 5. Das einfache ECS

ECS steht für Entity, Component und System.

### Entity: die Identität eines Objekts

Eine Entity ist eine Kennung. Sie enthält selbst keine Spiellogik. Ein Spieler, ein Gegner, eine Tür und ein Kameraobjekt können jeweils eine Entity sein.

Die Laufzeitkennung enthält einen Index und eine Generation. Beim Löschen einer Entity wird ihre Kennung ungültig. Wird der Platz später wiederverwendet, verhindert die andere Generation, dass eine alte Referenz versehentlich auf das neue Objekt zeigt.

Gespeicherte Szenen erhalten zusätzlich stabile Objektkennungen. Laufzeitkennungen werden beim Laden neu aufgebaut; Referenzen auf andere Objekte werden dabei aufgelöst.

### Component: die Daten eines Objekts

Eine Komponente beschreibt eine Eigenschaft:

| Komponente | Beispielinhalt |
| --- | --- |
| Transform2D | Position, Rotation und Skalierung |
| Sprite | Bildreferenz, Bildausschnitt, Farbe und Zeichenreihenfolge |
| Camera2D | Sichtbereich und Zoom |
| Tilemap | Rastergröße, Tilegröße, Ebenen und Tileset-Referenz |
| Collider2D, später | Kollisionsform und Kollisionsgruppe |
| Body2D, später | Geschwindigkeit und Bewegungseigenschaften |
| Script, später | Script-Asset und konfigurierbare Eigenschaften |

Zu Beginn liegen Komponenten in einfachen Arrays mit festgelegten, geprüften Kapazitäten. Eine Überschreitung erzeugt einen verständlichen Fehler. Wir brauchen zunächst keine Archetypen, automatische Registrierung oder komplizierte Abfragesprache.

### System: die Verarbeitung

Ein System ist eine normale C-Funktion. Es durchläuft passende Entities und verarbeitet deren Komponenten. Beispielsweise zeichnet das Sprite-System Entities mit Transform und Sprite.

Eine Spieler-Entity könnte Transform, Sprite, Collider und ein Bewegungsverhalten besitzen. Beim Top-down-Spiel reagiert dieses Verhalten auf Bewegung in zwei Achsen. Beim Metroidvania berücksichtigt es Sprung und Schwerkraft. Die gemeinsame Engine bleibt dieselbe.

## 6. Ablauf eines Frames

Ein Frame ist ein Durchlauf der Hauptschleife:

1. SDL-Ereignisse lesen: Tastatur, Maus, Fenster und Beenden.
2. Eingabe für Editor oder Spiel zuordnen.
3. Editor-Aktionen als Befehle ausführen.
4. Im Play-Modus die Simulation aktualisieren.
5. Welt mit der aktiven Kamera zeichnen.
6. Editor-Oberfläche zeichnen und das Bild anzeigen.

Die Simulation soll mit einem festen Zeitschritt laufen, beispielsweise 60 Updates pro Sekunde. Dafür sammelt die Hauptschleife vergangene Zeit und führt bei Bedarf mehrere Updates aus. Eine Obergrenze verhindert endloses Aufholen nach einer langen Pause. Rendering kann unabhängig davon erfolgen.

Diese Trennung macht Bewegung weniger abhängig von der Bildrate. Sie garantiert für sich allein noch keine deterministische Simulation.

## 7. Koordinaten und Kamera

Die Welt verwendet zunächst Pixel als Einheit. Die X-Achse zeigt nach rechts, die Y-Achse nach unten. Tilepositionen sind ganzzahlige Rasterkoordinaten; Objektpositionen verwenden Fließkommazahlen.

Die Kamera bestimmt, welcher Weltbereich im Editor sichtbar ist. Für die erste ungedrehte Kamera gilt vereinfacht:

```text
Bildschirmposition = Viewport-Ursprung + (Weltposition - Kameraposition) * Zoom
Weltposition = Kameraposition + (Bildschirmposition - Viewport-Ursprung) / Zoom
```

Die Mausposition muss zuerst in die Koordinaten der Szenenansicht umgerechnet werden. Danach lässt sich die Weltposition auf das Tile-Raster abbilden.

Beim Zoomen bleibt die Weltposition unter dem Mauszeiger erhalten. Dafür passen wir beim Ändern des Zooms auch die Kameraposition an. Zoomen und Verschieben der Editor-Ansicht verändern keine Szenendaten.

Für Pixel-Art verwenden wir zunächst nearest-neighbor filtering. Ob das Spiel eine feste interne Auflösung mit ganzzahliger Skalierung bekommt, legen wir für das Beispielprojekt fest.

## 8. Tilemaps und Bildimport

Eine Tilemap ist ein Raster aus Tile-IDs. Ein Tile wird keine eigene Entity. Die Tilemap selbst kann als Komponente an einer Entity hängen und damit eine Position und andere Eigenschaften besitzen.

Ein Tileset beschreibt ein Bild und dessen Aufteilung. Zunächst unterstützen wir regelmäßige Raster mit:

- Tilebreite und Tilehöhe.
- Optionalem Rand und Abstand zwischen Tiles.
- Einer stabilen Asset-Referenz auf das Bild.

Eine Tile-ID bezeichnet einen Ausschnitt im Tileset. Eine reservierte ID bedeutet „leeres Feld“. Die genaue Kodierung legen wir in der Implementierung fest.

### Beispiel: ein Tileset benutzen

1. Eine PNG-Datei importieren. Der Editor kopiert sie in den Projektordner.
2. Tilegröße einstellen, zum Beispiel 16 × 16 Pixel.
3. Der Editor prüft die Rasterangaben und zeigt die Tiles in einer Palette.
4. Ein Tile auswählen und auf der aktiven Ebene malen.
5. Szene speichern. Gespeichert werden Tile-IDs und Asset-Referenzen, keine SDL-Texturzeiger.

Mehrere Ebenen ermöglichen Boden, Dekoration und Vordergrund. Ebenen besitzen Namen, Sichtbarkeit und eine definierte Zeichenreihenfolge. Kollisionsdaten werden gesondert modelliert: Ein gezeichnetes Tile ist nicht automatisch eine Wand.

Zunächst sind Tilemaps endlich groß. Unendliche Karten, Chunk-Streaming, Autotiling und animierte Tiles sind spätere Erweiterungen.

## 9. Der Editor

Die geplante Oberfläche besteht aus vier Bereichen:

| Bereich | Aufgabe |
| --- | --- |
| Dateibrowser | Projektordner als Baum anzeigen und Assets oder Szenen öffnen. |
| Szenenansicht | Welt anzeigen, zoomen, verschieben, auswählen und bearbeiten. |
| Tileset-Palette | Ein Tile oder später eine Tile-Gruppe auswählen. |
| Inspector | Eigenschaften der Auswahl und Tilemap-Ebenen bearbeiten. |

Werkzeuge werden schrittweise ergänzt: zunächst Pinsel und Radierer, danach Rechteck, Füllen und Auswahl. Ein Werkzeug berechnet die betroffenen Änderungen und gibt sie an das Befehlssystem weiter.

Vorgesehene Bedienung:

- Mausrad: Zoom in der Szenenansicht.
- Mittlere Maustaste: Ansicht verschieben.
- Linke Maustaste: aktives Werkzeug verwenden.
- Strg+Z: Undo.
- Strg+Umschalt+Z: Redo; zusätzlich nach Plattformkonvention Strg+Y.
- Strg+S: Speichern.

Auf macOS verwenden wir gegebenenfalls die Command-Taste. Eingaben über GUI-Feldern dürfen nicht gleichzeitig die Welt bearbeiten oder Spielfiguren bewegen.

## 10. Undo und Redo

Alle Bearbeitungen an Projektdaten laufen über Commands. Ein Command enthält ausreichend Informationen, um eine Änderung auszuführen und rückgängig zu machen.

Beispiel für Tile-Änderungen:

```text
Ziel: Szene, Tilemap und Ebene
Änderungen:
  Feld (4, 8): vorher leer, danach Tile 12
  Feld (5, 8): vorher Tile 3, danach Tile 12
```

Undo stellt die alten Werte wieder her. Redo setzt die neuen Werte erneut.

Ein vollständiger Pinselstrich wird als ein Command zusammengefasst. Wird dasselbe Feld dabei mehrfach getroffen, speichern wir den ursprünglichen und den abschließenden Wert. Gruppen aus mehreren Aktionen können als Transaktion behandelt werden.

Das Verlaufssystem besitzt eine aktuelle Position. Nach Undo und einer neuen Änderung wird der bisherige Redo-Zweig verworfen. Der gespeicherte Zustand wird markiert, damit der Editor ungespeicherte Änderungen anzeigen kann. Historiengröße und Speicherverbrauch bekommen Grenzen; die Historie bleibt zunächst nur für die laufende Sitzung erhalten.

Auch das Löschen einer Entity benötigt eine Kopie aller Daten, die zur Wiederherstellung nötig sind. Referenzen müssen beim Wiederherstellen korrekt zugeordnet werden.

### Was „Undo für alles“ zunächst bedeutet

Undo gilt für Bearbeitungen an Szene, Komponenten, Tilemaps, Ebenen und unterstützte Projektaktionen. Kamerabewegung, Auswahl und laufende Spielsimulation werden getrennt behandelt und landen zunächst nicht in diesem Verlauf.

Dateioperationen erfordern besondere Regeln. Eine importierte Datei darf beim Undo nicht entfernt werden, wenn andere Assets sie benötigen. Für spätere Löschaktionen müssen Dateien erhalten bleiben, solange sie wiederherstellbar sein sollen. Beliebige Änderungen durch externe Programme kann der Editor nicht automatisch rückgängig machen.

## 11. Projekte, Assets und Speichern

Ein Projekt besteht aus einer Projektbeschreibung, Szenen und Assets. Alle Pfade werden relativ zum Projektordner gespeichert, damit sich ein Projekt zwischen Rechnern und über Git übertragen lässt.

Assets bekommen stabile Kennungen. Dadurch können Szenen ein Bild weiterhin finden, wenn es innerhalb des Projekts über eine unterstützte Editor-Aktion umbenannt wird. Ein Asset-Verzeichnis ordnet Kennungen den Dateien zu.

Dateiformate erhalten eine Versionsnummer. Beim Laden prüfen wir Werte, Größen, Referenzen und die Version. Bei einer unbekannten Version wird ein verständlicher Fehler angezeigt. Sobald sich ein veröffentlichtes Format ändert, brauchen wir eine ausdrückliche Migrationsstrategie.

Speichern erfolgt zunächst in eine temporäre Datei im selben Verzeichnis. Erst nach erfolgreichem Schreiben und Schließen wird die Zieldatei ersetzt. Details unterscheiden sich nach Betriebssystem; vorhandene Dateien dürfen bei einem fehlgeschlagenen Schreibvorgang nicht einfach zerstört werden.

Texturen und andere Laufzeitressourcen gehören dem Asset-System. Szenen speichern Referenzen darauf. Fehlende Bilder werden mit einem Platzhalter dargestellt und als Fehler gemeldet.

## 12. Play-Modus und Beispielprojekt

Beim Start des Play-Modus wird aus den bearbeiteten Szenendaten eine separate Laufzeitwelt erstellt. Beide Welten dürfen unveränderliche Bildressourcen teilen, aber keine veränderlichen Komponenten oder Tile-Raster.

Der Editor zeigt während des Spielens die Laufzeitwelt. Stop beendet diese Welt und zeigt wieder die bearbeitete Szene. Positionen und Script-Änderungen aus dem Testlauf werden zunächst nicht zurückgeschrieben.

Das erste Beispiel wird ein kleiner Top-down-Raum mit selbst erstellten oder passend lizenzierten Platzhaltergrafiken. Es soll zeigen, wie man eine Karte bearbeitet, speichert, lädt und eine Figur darin bewegt. Einfache Wandkollisionen folgen als eigener Schritt.

Wir verwenden keine Grafiken aus TwinBee, Zelda oder Metroid. Diese Spiele dienen als Orientierung für Spielarten und Bediengefühl.

## 13. Spätere Physik und Gameplay-Systeme

Für die drei Spielarten brauchen wir unterschiedliche Bewegungsmodelle:

| Spielart | Typische Anforderungen |
| --- | --- |
| Shooter | Frei bewegliche Figuren, Projektile, Treffer, Spawn-Wellen und scrollende Kamera. |
| Top-down-Abenteuer | Wandkollisionen, Trigger, Interaktion, Türen und Raumwechsel. |
| Metroidvania | Schwerkraft, Bodenprüfung, Sprünge, Plattformen und Kameraübergänge. |

Wir beginnen mit achsenparallelen Rechtecken und Kollisionen gegen Tilemaps. Kollisionsgruppen bestimmen, welche Objekte sich beeinflussen. Trigger melden Kontakte, ohne Bewegung zu blockieren.

Schnelle Projektile und Plattformbewegung brauchen gesonderte Lösungen; eine einfache Überlappungsprüfung reicht dafür nicht immer aus. Falls Rotation, komplexe Körper oder umfangreiche physikalische Interaktionen nötig werden, prüfen wir eine geeignete Bibliothek und ihre C-Schnittstelle.

## 14. Späteres Scripting

Die bevorzugte Lösung ist QuickJS. Die Engine bleibt C; JavaScript beschreibt Gameplay. Python bleibt eine Alternative, falls deine Arbeitsweise dafür spricht, würde aber zusätzliche Anforderungen an Auslieferung und Laufzeit mitbringen.

Die Script-API wird klein gehalten. Denkbare Funktionen sind Position lesen oder setzen, Eingabe abfragen, Entities erzeugen und Ereignisse empfangen. Direkte Zeiger auf internen C-Speicher werden nicht an Scripts gegeben.

Vorgeschlagene Lebenszyklusfunktionen sind „beim Erstellen“, „Update mit festem Zeitschritt“, „Kontakt“ und „beim Entfernen“. Die konkreten Namen sind noch offen.

Scriptfehler sollen mit Datei und Zeile gemeldet werden. Ausführungsgrenzen sollen verhindern, dass ein endloses Script den Editor blockiert. Hot Reload kommt erst, wenn Lebenszyklus und Zustandsübernahme klar geregelt sind.

## 15. Dialoge und weitere Editoren

Ein späterer Dialogeditor kann Figuren, Texte, Antwortmöglichkeiten, Bedingungen und Ereignisse als eigene Daten bearbeiten. Die Spielruntime liest diese Daten über einen Dialogdienst oder Scripts.

Der Dialogeditor nutzt das bestehende Asset-, Speicher- und Command-System. Er ergänzt seine eigenen Commands, beispielsweise „Antwort hinzufügen“ oder „Verbindung ändern“. So gelten dieselben Undo/Redo-Regeln wie für Tilemaps.

Weitere mögliche Erweiterungen sind Animationen, Audio, Inventar, Speicherstände, Lokalisierung und Raumnavigation. Wir bauen sie bei konkretem Bedarf, statt vorab ein großes allgemeines Framework zu erstellen.

## 16. Entwicklungsplan und Abnahmekriterien

### Meilenstein 1: Grundgerüst

CMake-Projekt, SDL3-Fenster, Logging, Eingabe, Kamera, Sprite-Rendering und einfaches ECS.

Fertig, wenn das Beispiel auf dem vereinbarten Zielsystem gebaut und gestartet werden kann, Entities dargestellt werden und Ressourcen sauber freigegeben werden.

### Meilenstein 2: Durchgängiger Tilemap-Editor

Dateibaum, PNG-Import, Tileset-Palette, Pinsel, Radierer, Zoom, Speichern/Laden und Undo/Redo.

Fertig, wenn ein Bild importiert, eine Karte gemalt, ein Pinselstrich rückgängig gemacht und wiederholt sowie die gespeicherte Szene identisch erneut geladen werden kann.

### Meilenstein 3: Beispielspiel und Editor-Ausbau

Inspector, mehrere Ebenen, weitere Tile-Werkzeuge und Play-/Stop-Modus mit steuerbarer Figur. Ein eigenständiges Beispielprogramm lädt dieselben Szenendaten.

Fertig, wenn der Testlauf die bearbeitete Szene unverändert lässt und die Szene auch ohne Editor angezeigt und gespielt werden kann.

### Meilenstein 4: Kollision und Bewegung

Tilemap-Kollision, Trigger und zunächst das Top-down-Bewegungsmodell. Danach Shooter- und Plattform-Beispiele nach deiner Priorität.

### Meilenstein 5: Scripting

QuickJS-Anbindung, dokumentierte Gameplay-API, Fehlermeldungen und ein kleines Beispielscript.

Dialogeditor und weitere Werkzeuge folgen danach nach Bedarf.

## 17. Wartbarkeit und Prüfung

Öffentliche Module bekommen kleine, dokumentierte C-Schnittstellen. Für jede Funktion muss klar sein, wer Speicher besitzt, wer ihn freigibt und wie Fehler gemeldet werden. Compilerwarnungen werden aktiviert; externe Abhängigkeiten werden auf konkrete Versionen festgelegt.

Automatische Tests konzentrieren sich auf fehleranfällige Regeln: ungültige Entity-Kennungen, Speichern und Laden, Command-Gruppierung, Undo/Redo-Verzweigungen sowie später Kollisionen. Editor-Bedienung und Darstellung werden mit einem dokumentierten manuellen Testablauf geprüft.

Build-Dateien und erzeugte Artefakte werden nicht ins Repository aufgenommen. Beispielbilder und andere Assets erhalten dokumentierte Herkunft und Lizenz. README und Dokumentation werden mit den tatsächlich implementierten Funktionen aktualisiert.

## 18. Was vor dem Implementieren noch offen ist

- Dein Entwicklungssystem: Windows, Linux oder macOS.
- Die zunächst unterstützten Zielplattformen.
- Projektname und GitHub-Repository sind festgelegt: PenguinEngine, https://github.com/creaera-com/PenguinEngine.
- Gewünschte Auflösung und Tilegröße für das erste Beispiel.

Als Vorschlag starten wir mit dem Top-down-Beispiel und 16 × 16 Pixel großen Tiles. Diese Werte sind konfigurierbar und beschränken spätere Shooter oder Metroidvanias nicht.

Sobald das Repo bereitsteht, richten wir zuerst Build und Grundgerüst ein. Diese Dokumentation dient als Ausgangspunkt; sie ist keine Zusage, dass alle beschriebenen Erweiterungen bereits zur ersten Version gehören.

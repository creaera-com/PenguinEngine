# Programmablauf: vom Start zum ersten Editor

Diese Anleitung verfolgt den **erfolgreichen normalen Start** des aktuellen Programms. Alle benötigten Dateien sind vorhanden, SDL und Speicherreservierungen funktionieren. Wir starten ohne `--smoke`, `--scripts` oder `--vector` und verwenden die mitgelieferte `shell.yaml`. Damit wird zuerst der **Map-Editor** geöffnet.

Die folgenden Schritte beschreiben die tatsächliche Reihenfolge im Code. Zur Einordnung der Module dienen [Projektstruktur](PROJEKTSTRUKTUR.md) und [Architektur](ARCHITEKTUR.md).

## 1. main.c übergibt an die Anwendung

Der Einstieg liegt in `src/main.c`:

```c
int main(int argc, char **argv) {
    return pe_application_run(argc, argv);
}
```

`argc` ist die Anzahl der Programmargumente, `argv` das zugehörige Textarray. `main()` reicht beides unverändert an `pe_application_run()` aus `src/application/application.c` weiter. Dessen Rückgabewert wird später zum Exitcode des Prozesses. Im Erfolgsfall ist er 0.

Die Aufgaben sind bewusst getrennt: `main.c` ist nur der Programmeinstieg; `application.c` koordiniert Start, Hauptschleife und Freigabe.

Die private Variable `app` ist in `src/application/state.c` als globale Instanz definiert. C initialisiert sie vor `main()` mit Nullen. Insbesondere sind anfangs keine Payloads oder Layouts geladen, `loaded_view` ist 0 und alle `session_exists`-Einträge sind false. Die Funktionen arbeiten im hier beschriebenen Startablauf auf dem Hauptthread.

## 2. Argumente lesen und das Projekt finden

`pe_application_run()` geht zunächst die Argumente durch. Zum Beispiel setzt `--project /mein/spiel` die gewünschte Projektwurzel; `--ui /mein/layout/shell.yaml` bestimmt die Shellbeschreibung. Ohne solche Argumente greifen die Standardsuchen.

Danach folgen zwei unterschiedliche Schritte:

| Funktion | Datei | Ergebnis |
| --- | --- | --- |
| `pe_project_resolve()` | `src/engine/project/project.c` | Findet das Verzeichnis, das `project.json` enthält; schreibt `app.project.root`. |
| `pe_project_load()` | `src/engine/project/manifest.c` | Liest und validiert das Manifest; schreibt den Pfad der Startszene nach `app.project.scene_path`. |

Ohne expliziten Projektpfad sucht `pe_project_resolve()` zuerst `example` und `../example` relativ zum Programmverzeichnis aus `SDL_GetBasePath()`. Anschließend versucht es `example`, `examples/topdown` und `.` relativ zum Arbeitsverzeichnis. Der Build kopiert das Beispielprojekt in das Build-Verzeichnis; deshalb funktioniert der Start normalerweise auch aus einem anderen Arbeitsverzeichnis.

Das Manifest enthält unter anderem `version` und `start_scene`. `pe_project_load()` verbindet die Projektwurzel mit diesem relativen Szenenpfad.

**Zu diesem Zeitpunkt steht fest, welche Szene später geladen werden soll. Die Szene selbst ist noch nicht eingelesen und es gibt noch keinen Map-Payload.** Auch bei einem später gewünschten Script-/Vector-Start wird dieses Projektmanifest zunächst benötigt.

## 3. SDL, Fenster und UI-Kontext vorbereiten

Als Nächstes richtet `pe_application_run()` die technische Umgebung ein:

1. `SDL_Init(SDL_INIT_VIDEO)` initialisiert SDL für die Fensterdarstellung.
2. `SDL_CreateWindow()` erzeugt das 1280×800 große, veränderbare Hauptfenster. Es ist zunächst **unsichtbar** (`SDL_WINDOW_HIDDEN`).
3. `SDL_CreateRenderer()` erzeugt den dazugehörigen Renderer. Er wird später auch die Texturen besitzen.
4. Die Anwendung setzt Fenster-Mindestgröße und VSync.
5. `ui_init(&app.shell.ui)` initialisiert den gemeinsamen UI-Kontext, etwa für Fokus und Eingaben.

Das Fenster und der Renderer stehen jetzt unter `app.platform`, der UI-Kontext unter `app.shell`. Noch ist kein Editor geladen und kein vollständiger Editorframe gezeichnet.

## 4. shell.yaml laden und den ersten Editor auswählen

Falls `--ui` nicht gesetzt wurde, sucht die Anwendung `ui-layouts/shell.yaml` neben der Programmdatei, danach `../ui-layouts/shell.yaml`. Das berücksichtigt auch Build-Konfigurationen mit einem zusätzlichen Unterordner für die ausführbare Datei.

```text
pe_application_run()
  -> pe_ui_layout_load(shell_path, "shell", ...)
  -> editor_apply_shell_layout()
```

`pe_ui_layout_load()` in `src/editor/layout/ui_layout.c` liest YAML, erzeugt den Knotenbaum und prüft das Schema. Das Ergebnis wird in `app.shell.shell_layout` gespeichert. **Ein geladenes YAML-Modell wird noch nicht automatisch gezeichnet.**

`editor_apply_shell_layout()` in `src/editor/layout/renderer.c` übernimmt daraus Theme, Panelabstände, Spaltenvorgaben und die Liste der verfügbaren Editoren. Im aktuellen Startcode wird die Funktion zweimal aufgerufen: einmal vor und einmal nach den expliziten anfänglichen Spaltenbreiten. Das ist der vorhandene Ablauf, kein zwingendes allgemeines Initialisierungsmuster.

Für die erste Auswahl ist diese Regel entscheidend: Wenn noch kein Editor aktiv ist, wird der **erste Eintrag aus `editors:`** ausgewählt. Für die erste Auswahl sind diese Angaben entscheidend (gekürzter Ausschnitt):

```yaml
editors:
  - id: map
    title: "Map"
    module: map
    file: editors/map.yaml
```

`module: map` wird der internen ID `UI_MAP_VIEW` zugeordnet. Die Auswahl landet in `app.shell.workspaces.selected`. **Ausgewählt** bedeutet hier zunächst nur: Dieser Editor soll geladen werden. `app.workspace.loaded_view` bleibt bis zur erfolgreichen Aktivierung 0.

Die YAML-Datei beschreibt die Oberfläche und verweist auf ein Editorlayout. Sie enthält keine C-Funktionszeiger. Die Zuordnung zum ausführbaren Editor-Code ist separat in `src/application/editors.c` festgelegt:

```text
UI_MAP_VIEW
  -> EditorView für Map
     -> Zeichenfunktionen der Map
     -> &pe_map_backend
```

`pe_map_backend` ist eine statische Operationstabelle in `src/editors/game/map/adapter.c`. Das Bereitstellen dieser Tabelle führt ihre Funktionen noch nicht aus. Sie werden erst durch den Workspace aufgerufen.

## 5. Shell und temporäre Sitzungen vervollständigen

Vor der Aktivierung ergänzt die Anwendung noch die gemeinsame Umgebung:

- Sie initialisiert die linken und rechten Tabgruppen.
- Sie erzeugt die Resize-Cursor, setzt Fenstermaße und ruft `editor_window_init()` für die eigene Fensterbedienung auf.
- Sie legt einen vorläufigen Canvasbereich in `app.shell.viewport` fest. Beim Zeichnen wird dessen genaue Geometrie neu berechnet.
- Über `SDL_GetPrefPath()` ermittelt sie das Verzeichnis für temporäre Sitzungsdateien und erzeugt darin drei Laufzeitpfade für Map, Script und Vector.
- Sie setzt `app.runtime.running` und den ersten Zeitstempel.
- Sie bereitet aufgeklappte Projektordner vor, aktualisiert den Dateibaum und setzt die Willkommensmeldung.

Die Sitzungsdateipfade sind zunächst nur Namen. Da `session_exists` beim frischen Programmstart false ist, wird **keine Sitzung einer früheren Programmausführung wiederhergestellt**. Die Checkpoints werden später für Editorwechsel innerhalb dieser Anwendungslaufzeit verwendet.

Die gemeinsame Konsole wird hier ebenfalls noch nicht erzeugt. Sie entsteht bei Bedarf beim Zeichnen eines sichtbaren YAML-Terminalknotens.

## 6. editor_activate() lädt den Map-Editor

Jetzt kommt der eigentliche Übergang zur Editor-Infrastruktur:

```c
editor_activate(app.shell.workspaces.selected, error, sizeof error);
```

`editor_activate()` liegt in `src/editor/workspace/lifecycle.c`. Für unseren Map-Start läuft Folgendes ab:

### 6.1 Das Editorlayout laden

`editor_load_editor_layout(UI_MAP_VIEW, ...)` sucht den passenden Eintrag in der bereits geladenen Shell. Über dessen `file: editors/map.yaml` entsteht ein Pfad **relativ zur shell.yaml**.

Die Funktion lädt diese Datei mit `pe_ui_layout_load(..., "editor", ...)` und prüft, ob ihre ID zur Shellregistrierung passt. Das Modell landet in `app.workspace.active_layout`.

Damit existieren nun zwei verschiedene YAML-Modelle:

| Zustand | Bedeutung |
| --- | --- |
| `app.shell.shell_layout` | Gemeinsame Toolbar, Editor-Auswahl, Theme und Rahmen |
| `app.workspace.active_layout` | Layout des aktiven Editors, beispielsweise seine drei Spalten und Werkzeuge |

### 6.2 Backend auswählen und Payload reservieren

`editor_session_index()` bestimmt den Sitzungsslot für Map. `editor_view(UI_MAP_VIEW)` liefert den registrierten `EditorView`; dessen `backend` zeigt auf `pe_map_backend`.

Der Workspace ruft `backend->create(error, size)` auf. Bei Map zeigt dieser Rückruf auf die dateilokale Funktion `create()` in `src/editors/game/map/adapter.c`.

Diese Funktion reserviert mit `calloc()` einen leeren `MapPayload` und speichert ihn unter `app.workspace.map`. Anschließend merkt sich der Workspace die Operationstabelle in `app.workspace.backend`.

Ein Payload ist hier die besitzende Datenstruktur des konkreten Editors: Die Map wird darin Szene, Verlauf, Navigation und Darstellungscaches verwalten. Die gemeinsame Lebenszyklussteuerung verwendet zum Erzeugen und Freigeben den Backend-Vertrag.

### 6.3 Beim frischen Start initialisieren

Da für Map noch kein Laufzeitcheckpoint existiert, ruft der Workspace zunächst `default_editor_ui(false)` auf. Das richtet die anfänglichen Panelbreiten, Tabgruppen, Scrollpositionen und Auswahl ein.

Danach folgt `backend->initialize(error, size)`. Der Map-Adapter führt dabei aus:

```text
initialize()
  -> MapPayload.view mit Standardwerten belegen
     Kamera: x=-16, y=-16, Zoom=1.4
     Tile: 1, Werkzeug: BRUSH, Raster sichtbar
  -> pe_history_init(&map->history)
  -> pe_scene_load(&map->scene, app.project.scene_path, ...)
```

**Erst hier wird die in Schritt 2 bestimmte Startszene tatsächlich eingelesen.** Das Szenenmodell befindet sich anschließend in `app.workspace.map->scene`; der Undo-/Redo-Verlauf ist initialisiert.

Bei einem späteren Wechsel zu einem bereits verwendeten Editor wäre die Alternative `pe_session_load()` mit dem jeweiligen `checkpoint`-Rückruf. Beim hier beschriebenen Erststart wird dieser Zweig nicht genommen.

### 6.4 Editor als geladen markieren und Assets vorbereiten

Nach erfolgreicher Initialisierung setzt `editor_activate()`:

```text
app.workspace.loaded_view = UI_MAP_VIEW
app.shell.needs_layout = true
```

Danach aktualisiert es den Dateibaum und ruft, falls vorhanden, `backend->ready()` auf. Für Map ist das `editor_assets_sync()` in `src/editors/game/map/document.c`.

`editor_assets_sync()` verwendet `pe_assets_load()` aus `src/platform/sdl/scene_renderer.c`, um das zur Szene gehörende Tileset relativ zur Projektwurzel zu laden und für den SDL-Renderer vorzubereiten. Im hier betrachteten Erfolgsfall ist danach auch der Map-Assetcache bereit.

`editor_activate()` gibt true zurück. Der erste Editor ist nun fachlich geladen: Layout, Payload, Szene, Verlauf und Assets sind vorhanden. Die anderen beiden Editor-Payloads bleiben NULL.

## 7. Fenster anzeigen und den ersten Frame zeichnen

Zurück in `pe_application_run()` macht `SDL_ShowWindow()` das zuvor versteckte Fenster sichtbar. Ohne besondere Startargumente geht es anschließend in die Hauptschleife:

```text
while (app.runtime.running)
  -> SDL_PollEvent() und editor_event()
  -> vergangene Zeit berechnen und auf 0.1 Sekunden begrenzen
  -> pe_console_tick()
  -> editor_map_update()
  -> editor_draw()
  -> SDL_Delay(1)
```

`editor_map_update()` verändert die separate Spielsimulation nur im Play-Modus. Beim normalen Start ist Play aus.

`editor_draw()` in `src/shell/shell.c` berechnet die Flächen und zeichnet den gemeinsamen Rahmen sowie die Inhalte des aktiven YAML-Layouts. Die registrierte Zeichenfunktion liefert die Editor-Werkzeugleiste; `editor_draw_yaml_node()` verteilt die Inhaltsknoten an die spezialisierten Ansichten. Beim Knoten `map_canvas` wird die Mapdarstellung aufgerufen. `SDL_RenderPresent()` zeigt den fertigen Frame.

Der erste Aufruf von `editor_draw()` kann bereits innerhalb von `editor_event()` erfolgen: Wenn `needs_layout` gesetzt ist, bereitet der Eingabehandler vor der Ereignisverarbeitung die aktuelle Geometrie vor. Deshalb ist nicht garantiert, dass ausschließlich der explizite Aufruf am Ende der Hauptschleife den allerersten Frame zeichnet.

PNG-Icons werden beim ersten tatsächlichen Zeichnen aus dem passenden Layoutcache geladen. Ein sichtbarer Terminalknoten erzeugt bei Bedarf den gemeinsamen Konsolendienst. Das sind zusätzliche, verzögert erzeugte Darstellungsressourcen; die Szene ist zu diesem Zeitpunkt bereits geladen.

## Der vollständige Aufrufweg zum Nachverfolgen

Der folgende Baum ist ein vereinfachter Ausschnitt des erfolgreichen Erststarts. Die Pfeile hinter den Rückrufen zeigen, welche konkrete Funktion für Map in der Operationstabelle eingetragen ist.

```text
src/main.c: main()
└── src/application/application.c: pe_application_run()
    ├── pe_project_resolve()
    ├── pe_project_load()
    ├── SDL_Init(), SDL_CreateWindow(), SDL_CreateRenderer()
    ├── ui_init()
    ├── pe_ui_layout_load(..., "shell", ...)
    ├── editor_apply_shell_layout()     -> erste Editor-ID auswählen
    ├── Tabs, Fensterbedienung und Sitzungsdateipfade vorbereiten
    ├── editor_activate(UI_MAP_VIEW, ...)
    │   ├── editor_load_editor_layout()
    │   │   └── pe_ui_layout_load(..., "editor", ...)
    │   ├── editor_view(UI_MAP_VIEW)    -> pe_map_backend finden
    │   ├── backend->create()          -> Map-Adapter: create()
    │   ├── default_editor_ui(false)
    │   ├── backend->initialize()      -> Map-Adapter: initialize()
    │   │   ├── Map-Navigation initialisieren
    │   │   ├── pe_history_init()
    │   │   └── pe_scene_load()
    │   ├── loaded_view / needs_layout setzen, Dateibaum aktualisieren
    │   └── backend->ready()           -> editor_assets_sync()
    │       └── pe_assets_load()
    ├── SDL_ShowWindow()
    └── Hauptschleife
        ├── Ereignisse und Zeit verarbeiten
        ├── Konsole und gegebenenfalls Spielsimulation aktualisieren
        └── editor_draw()
            ├── Shell und aktive YAML-Inhalte zeichnen
            └── SDL_RenderPresent()
```

## Abweichende Startargumente richtig einordnen

`--scripts` und `--vector` ändern im aktuellen Code **nicht** unmittelbar die erste Auswahl aus der Shellbeschreibung. Sie setzen beim Argumentelesen zunächst nur Merker. Erst **nach** der ersten erfolgreichen Aktivierung und `SDL_ShowWindow()` folgt der Wechsel über `editor_tab_select(true)` beziehungsweise `editor_select_workspace(UI_VECTOR_VIEW)`.

Dieser Wechsel nutzt den normalen Workspace-Lebenszyklus: Der erste Editor wird gesichert und entladen, dann der gewünschte Editor angelegt. `--vector-demo` befüllt anschließend das Vector-Dokument mit Beispieldaten. Wenn du dagegen die Reihenfolge von `editors:` in der Shellbeschreibung änderst, änderst du tatsächlich die erste Auswahl.

`--smoke` ist ein eigener Prüfpfad. In einem Testbuild führt die Anwendung vor den Startansichts-Merkern zusätzlich die Editor-Smokes aus. Im Produktionsbuild gibt es diese Testfunktionen nicht; die Hauptschleife beendet sich im Smoke-Modus nach wenigen Frames. Dieser Prüfpfad gehört nicht zur oben beschriebenen normalen Bedienung.

## In CLion Schritt für Schritt mitlaufen

Zum Nachvollziehen ohne Testumwege ohne `--smoke` starten. Diese Haltepunkte führen durch den beschriebenen Weg:

| Reihenfolge | Haltepunkt | Worauf schauen? |
| --- | --- | --- |
| 1 | `main()` in `src/main.c` | Übergabe von `argc` und `argv` |
| 2 | `pe_application_run()` | Noch leere Zustandsbereiche, gelesene Argumente |
| 3 | `pe_project_load()` | Projektwurzel und berechneter Start-Szenenpfad |
| 4 | `editor_apply_shell_layout()` | `workspaces.selected` wird beim Erststart auf den ersten YAML-Eintrag gesetzt |
| 5 | `editor_activate()` | Erstes Editorlayout und ausgewählter Backend-Zeiger |
| 6 | `create()` im **Map-Adapter** | `app.workspace.map` wird reserviert |
| 7 | `initialize()` im **Map-Adapter** | Navigation, Verlauf und Szene werden angelegt |
| 8 | `editor_assets_sync()` | Tileset und SDL-Texturen werden vorbereitet |
| 9 | `editor_draw()` | Aktive ID, Panelgeometrie und erste sichtbare Ausgabe |

Die Adapter enthalten jeweils eigene statische Funktionen namens `create()` und `initialize()`. Setze diese Haltepunkte deshalb in der konkreten Datei `src/editors/game/map/adapter.c`.

Nach Rückkehr aus `editor_activate()` sind beim normalen Map-Start besonders diese Werte interessant:

```text
app.shell.workspaces.selected == UI_MAP_VIEW
app.workspace.loaded_view    == UI_MAP_VIEW
app.workspace.backend        == &pe_map_backend
app.workspace.map            != NULL
app.workspace.script         == NULL
app.workspace.vector         == NULL
app.workspace.active_layout  != NULL
```

Ab hier bearbeitet die Hauptschleife die Eingaben. Änderungen an der Map gehen später über ihre Werkzeuge und Dokumentoperationen in den Verlauf; die gemeinsame Toolbar spricht dafür den in [ARCHITEKTUR.md](ARCHITEKTUR.md) beschriebenen Dokumentvertrag an.

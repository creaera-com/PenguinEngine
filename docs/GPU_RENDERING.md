# GPU-Rendering mit SDL3

## Ziel und Zuständigkeiten

Die Desktop-Apps bevorzugen den **SDL GPU Renderer** aus SDL **3.4**. Er führt
UI-Zeichenaufrufe, Texturen, Dreiecke, Masken und Artwork-Kacheln über SDL_GPU aus.
Je nach Plattform verwendet SDL Vulkan, Metal oder Direct3D 12. pengUI bleibt von
pengDraw unabhängig; die App wählt den Renderer und übergibt ihn an beide Libraries.

pengDraw besitzt die Effekt-Shader und ihre Ressourcen. Editoren übersetzen ihre
Dokumentwerte in Zeichenwerte. Dokumente, History, YAML und der SDL-freie Kern
enthalten weder GPU-Handles noch Shaderwissen. Dieselbe Schnittstelle kann später
von einem Spiele- oder Diagrammeditor verwendet werden.

GPU-beschleunigte Ausgabe bedeutet nicht, dass jede Aufgabe auf der GPU läuft:
Layout, Eingabe, Szenenverwaltung, Bool-Operationen, Kurvenaufbereitung und
Triangulierung bleiben auf der CPU. Fonts werden derzeit auf der CPU rasterisiert
und als zwischengespeicherte Texturen hochgeladen; Bilddateien werden auf der CPU
dekodiert. Speichern/Exportieren darf Bilddaten bewusst zur CPU zurücklesen.

## Renderer anlegen und freigeben

```c
SDL_Renderer *renderer = pdraw_create_renderer(window);
if (!renderer) {
    /* SDL_GetError() an die Anwendung weiterreichen. */
    return false;
}
/* An pengUI und pengDraw übergeben, Ressourcen benutzen. */
/* Zuerst Views, Texturen und PDrawRenderer-Caches freigeben. */
pdraw_destroy_renderer(renderer);
```

Das Paar übernimmt zusätzlich die shaderbezogenen Ressourcen. Nicht mit einem
bloßen `SDL_DestroyRenderer()` mischen. Für eigene Renderziele kann ein GPU-Renderer
auch ohne Fenster angelegt werden; anschließend vor dem Zeichnen eine Zieltextur
setzen. Alle SDL-/GPU-Aufrufe bleiben auf dem Renderer-Thread.

Die Anwendung kontrolliert den benutzerdefinierten GPU-Renderzustand. pengDraw
aktiviert ihn nur innerhalb seiner Effektaufrufe und setzt ihn danach auf NULL.
Fremde benutzerdefinierte Shader nicht über einen pengDraw-Aufruf hinweg aktiv lassen.
Renderziel, Viewport, Clip, Skalierung, Farbe und Blendmodus werden wiederhergestellt.

## Schatten und Farbverläufe

Ein Schatten wird ohne `SDL_RenderReadPixels()` berechnet:

1. Die Objekt-Silhouette wird in eine transparente GPU-Zieltextur gezeichnet.
2. Optional folgen horizontale/vertikale Maximumfilter für die Ausbreitung.
3. Drei horizontale/vertikale Boxfilter-Durchläufe erzeugen die Weichzeichnung.
4. Ein Shader färbt die Alpha-Silhouette ein und zeichnet sie mit Versatz hinter dem Objekt.

Die Renderer-Ressourcen enthalten drei wiederverwendete Texturen. Ihre Kapazität
wächst bei Bedarf, schrumpft innerhalb der Sitzung nicht bei jedem Objektwechsel
und wird beim Zerstören des Renderers freigegeben. Die vorhandene Bereichsplanung
begrenzt die Texturen auf 4096 Pixel pro Achse; drei volle RGBA-Ziele können zusammen
bis zu 192 MiB belegen. Der Artwork-Kachelcache hat zusätzlich sein eigenes Budget.
Verschiedene Objektgrößen erzwingen dadurch nicht fortlaufend neue Allokationen.

Linearer, radialer, konischer und reflektierter Verlauf werden im Fragment-Shader
innerhalb der triangulierten Form berechnet. Bis zu acht Stops mit Alpha und
Interpolation über vormultiplizierte Farbwerte entsprechen dem reinen CPU-Sampler.
GPU-Rundungen können sich an einzelnen Pixeln geringfügig unterscheiden.
Es wird keine vollständige Verlaufsbitmap pro Objekt auf der CPU erstellt.

Der vorhandene Artwork-Cache überspringt auch GPU-Effektdurchläufe bei unverändertem
Inhalt. Während Bearbeitung und Zoom werden betroffene Kacheln neu gezeichnet.
Die Editor-App begrenzt ihre Zeichenschleife auf maximal 60 FPS; das ist keine
Vorgabe für spätere Spiele, die ihre eigene Frame-Steuerung besitzen können.

## Diagnose und Kompatibilität

Beim Start protokolliert die App beispielsweise:

```text
pengDraw: GPU renderer (vulkan), GPU shadow and gradient shaders enabled
```

Wenn GPU-Erstellung oder Shaderinitialisierung fehlschlagen, meldet der Standardpfad
das und versucht einen kompatiblen SDL-Renderer. Dort bleiben die bisherigen
CPU-Effektpfade verfügbar. Ein fehlgeschlagener GPU-Effekt wird ebenfalls gemeldet;
die Diagnosezähler unterscheiden erfolgreiche GPU-Aufrufe und Fehler.

- `pdraw_renderer_gpu_effects(renderer)` prüft die eingerichtete Effektpipeline.
- `pdraw_renderer_gpu_stats(renderer)` liest Aufruf-/Fehlerzähler und Schatten-
  Texturkapazität ohne GPU-Synchronisierung aus.
- `SDL_RENDER_DRIVER=gpu` verlangt den GPU-Pfad ausdrücklich und bricht bei einem
  Initialisierungsfehler ab. Das eignet sich zur Prüfung statt eines stillen Fallbacks.
- `SDL_GPU_DRIVER=vulkan` kann zusätzlich das GPU-Backend vorgeben.
- `SDL_RENDER_DRIVER=software` erzwingt die CPU-Variante für Vergleiche/Headless-Tests.
  Andere explizite SDL-Renderer verwenden ebenfalls den Kompatibilitätspfad.

Unter Linux beispielsweise:

```sh
SDL_RENDER_DRIVER=gpu ./build/pengui_editor
```

Ein Vulkan-Softwaretreiber wie Lavapipe führt die GPU-API auf der CPU aus. Ein
aktiver Vulkan-Pfad allein belegt daher keine Beschleunigung durch echte Hardware.
Für Performancebeurteilungen GPU-Gerät/Treiber, Szene, Auflösung und FPS mit erfassen.
Große Schatten, viele Objekte oder Treiberprobleme können auch GPU-Zeit beanspruchen.

## Texturcache beim Bearbeiten

Der Artwork-Kachelcache behält bis zu 16 Kacheln beziehungsweise 64 MiB an
Textur-Pixeldaten. Ändert sich nur der Inhalt desselben Ausschnitts, wird die
vorhandene Textur neu gezeichnet. Alte Zwischenstände einer Verschiebegeste werden
nicht als weitere Kacheln aufbewahrt. Unveränderte Inhalte bleiben ohne erneutes
Zeichnen nutzbar; andere Ausschnitte unterliegen weiterhin dem LRU-Speicherlimit.

Das Limit beschreibt die eigenen Textur-Pixeldaten, nicht den gesamten Prozess-RAM:
Treiberressourcen, Undo-Daten, Bilder, Schrift- und Effektressourcen kommen hinzu.
Ein Software-GPU-Treiber kann seine Texturen ebenfalls im Prozess-RAM halten.

## Build, Shader und Tests

CMake benötigt SDL mindestens 3.4 und lädt ohne passende Installation die mit
SHA-256 gepinnte Version **3.4.18**. Nach einem Versionswechsel vorhandene
SDL-Pfadüberschreibungen im CMake-Profil prüfen. Die unbenötigte optionale
XTEST-Erweiterung ist beim mitgebauten SDL deaktiviert, auch in vorhandenen CMake-Caches.
Andere X11-/Wayland-Entwicklungspakete werden für das jeweilige Desktop-Backend weiterhin benötigt.
Shader liegen unter
`lib/pengdraw/src/gpu/shaders/`: GLSL für Vulkan, HLSL für Direct3D 12 und MSL für
Metal. Die eingebetteten `.inc`-Dateien werden beim normalen Build direkt verwendet.
Es ist kein Shadercompiler beim Anwender und keine zusätzliche Runtime-Library nötig.

Bei Shaderänderungen alle drei Quellvarianten angleichen und mit glslangValidator
und Microsoft DXC die eingebetteten Daten neu erzeugen:

```sh
python3 tools/build_pengdraw_shaders.py --glslang glslangValidator --dxc dxc
```

Die Artefakte sind eigene Shader, keine zusätzliche Fremdsoftware-Runtime.
Die Compiler werden nur zur Entwicklung/Regeneration benötigt.

`pengdraw_gpu` prüft die wirkliche GPU-API mit Pixelvergleichen für Schatten,
Weichheit, Ausbreitung, alle Verlaufstypen und Alpha. Weitere Prüfungen sichern
Renderzustand, Ressourcenwiederverwendung und das Zusammenspiel mit dem Kachelcache.
Ohne verfügbaren GPU-Treiber werden diese Tests ausdrücklich als ignoriert gemeldet.
`PDRAW_REQUIRE_GPU=1` macht daraus einen Fehler. Linux-CI installiert Mesa und
verlangt diesen Test zusätzlich auf Vulkan. Die normalen Software-Tests bleiben
für den Kompatibilitätspfad bestehen.

```sh
PDRAW_REQUIRE_GPU=1 ctest --test-dir build -R '^pengdraw_gpu$' --output-on-failure
```

Die Linux-Tests können auf einem Vulkan-Softwaregerät laufen. Metal und Direct3D
benötigen zur tatsächlichen Ausführungsprüfung einen passenden Rechner; vorhandene
Shaderdateien allein sind kein Nachweis einer dort getesteten Ausgabe.

## Ressourcen-Diagnose

`pdraw_renderer_resource_stats(renderer)` liefert rendererweite Lebenszeit-Zähler
für pengDraw-eigene Texturen, deren Freigaben, aktuelle und maximale logische
Pixelgröße sowie CPU-Fallback-Versuche. Die Registrierung hängt an SDL-Properties;
auch eine direkte SDL-Texturfreigabe entfernt den registrierten Bestand.
`pdraw_renderer_cache_stats(cache)` liefert dagegen den aktuellen Kachel-/Scratch-
Bestand und Cache-Treffer, Neuzeichnungen und Verdrängungen. Die GPU-Effektstatistik
enthält zusätzlich Anzahl und Kapazität der Schatten-Zwischentexturen.

Die App übernimmt diese Werte über Monitor und Editor-Adapter in jede Zeile der
Projekt-Aufzeichnung. Umfang, Felder und Messgrenzen stehen in
[Prozess-Monitor](MONITORING.md#render-ressourcen-im-performance-log).
Insbesondere ist eine SDL-Handle-Freigabe kein Nachweis, dass der Treiber bereits
sämtliche Kopien physisch freigegeben hat. Es findet keine GPU-Synchronisierung
für die Messung statt.

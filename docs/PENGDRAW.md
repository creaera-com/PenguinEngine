# pengDraw: unabhängiger Zeichenkern

pengDraw ist eine C17-Library neben pengUI. Ihre öffentliche Schnittstelle liegt
in `lib/pengdraw/include/pengdraw/pengdraw.h`. Sie kennt keine Dokumente,
Artboards, IDs, Ebenen, Werkzeuge, History, YAML oder UI-Themes. Die Bibliotheken
hängen nicht voneinander ab. Ein UML-Tool oder eine Spieleengine kann pengDraw
verwenden, ohne den Vektoreditor oder pengUI zu linken.

## Zuständigkeiten und Programmfluss

```text
Dokument, Spielwelt oder UML-Modell
    -> eigener Adapter: sichtbare Objekte und aktuelle Transformationen auflösen
    -> pengDraw: Geometrie, Füllung, Kontur, Textur und Compositing
    -> SDL3
```

Im Vektoreditor bildet `rendering/drawing.h` die Grenze: Ein `VectorShape` wird
in einen `PDrawShape` kopiert. Die Kopie enthält nur Geometrie und Darstellung,
keine Dokumentreferenzen. `rendering/objects.c` entscheidet weiterhin über
Ebenenreihenfolge, Sichtbarkeit, Gruppen, verknüpfte Instanzen, Wiederholungen und
Werkzeugvorschauen. Die bisherigen `render.c`-Schnittstellen der Objekttypen
übersetzen noch Auswahlfarben und Koordinaten in die allgemeine Zeichen-API.
Die eigentlichen Pfad-/Kurven-/Ellipsenalgorithmen existieren nur in pengDraw.

`rendering/artwork.c` löst laufende Gesten beim tatsächlichen Zoom einmal auf.
Es übergibt einen unveränderlich vorbereiteten Zeichenstand über einen Callback
an `pdraw_render`. pengDraw zeichnet begrenzte Kacheln mit zweifacher Auflösung
und skaliert sie zurück. Vorschau und PNG/JPEG-Export verwenden denselben Weg.
Der Callback bekommt Ursprung und Skalierung für die jeweilige Kachel. Er darf
keine Änderungen am fachlichen Modell auslösen, weil er mehrfach aufgerufen wird.

Editor-Markierungen (Greifer, Caret, Auswahl, Raster, Lineale) bleiben außerhalb
der Library. Optionales `outline` an den einfachen Zeichenfunktionen ist nur
eine explizite RGBA-Farbe, keine Kenntnis einer Editor-Auswahl.

## Aufbau und CMake

| Bereich | Verantwortung |
| --- | --- |
| `src/geometry/` | Pfade, Bézierkurven, Rotation, Konturen, Triangulation und Flächenoperationen |
| `src/paint/` | Verläufe, Schattenwerte, Textvalidierung und Textumbruch/Spaltenfluss |
| `src/sdl/` | Formen, Texturen, Gradientrasterisierung, Masken, Schatten und Supersampling |

```cmake
target_link_libraries(my_app PRIVATE pengDraw::sdl)
# Nur Berechnung, ohne SDL:
target_link_libraries(my_geometry_tool PRIVATE pengDraw::core)
```

Der Kern benötigt nur die C-Laufzeit, Mathematik und das mitgelieferte libtess2.
Das SDL-Backend ergänzt SDL3. Es kommen keine neuen externen Abhängigkeiten hinzu.
Beim normalen Projektbuild wird pengDraw automatisch gebaut. Auch separat:

```sh
cmake -S lib/pengdraw -B build-draw -DPENGDRAW_BUILD_SDL=OFF
cmake --build build-draw
```

Für einen separaten Build mit Backend `PENGDRAW_BUILD_SDL=ON` verwenden;
ein installiertes SDL3 mit CMake-Paket muss dann verfügbar sein. Die Library lässt
sich mit `add_subdirectory` aus diesem Repository einbinden. Sie wird derzeit als
statische Library gebaut; ein Install-/Paketformat ist noch nicht definiert.

## Einfache Zeichenaufträge

```c
#include "pengdraw/pengdraw.h"

static void paint(void *context, SDL_Renderer *renderer,
                  float origin_x, float origin_y, float scale) {
    (void)context;
    PDrawShape shape = {
        .corner_radius = 12,
        .stroke = { .color = 0x202020ff, .width = 2,
                    .alignment = PDRAW_STROKE_INSIDE }
    };
    pdraw_shape_draw(renderer, &shape, (PDrawRect){20, 30, 160, 80},
                     15, (SDL_FRect){origin_x, origin_y, 0, 0},
                     scale, 0x5b9bd5ff);
}

/* Renderer und Fenster legt die Anwendung an. Cache mit null initialisieren. */
PDrawRenderer drawing = {0};
pdraw_render(&drawing, renderer, (SDL_FRect){0, 0, width, height}, paint, NULL);
/* Vor SDL_DestroyRenderer: */
pdraw_renderer_destroy(&drawing);
```

Farben sind gerade RGBA-Werte `0xRRGGBBAA`. Koordinaten laufen nach rechts/unten;
positive Winkel drehen im Uhrzeigersinn. Pfad-/Bézierwerte einer Form sind relativ
zu ihrer Rechteckbox (gewöhnlich 0 bis 1). `origin` und `zoom` bilden diese Werte
auf Rendererkoordinaten ab. Ein leerer Pfad mit leerer Kurve bedeutet Rechteck;
`ellipse=true` wählt eine volle Ellipse. Ellipsensegmente werden als Béziergeometrie
übergeben, ohne parametrische Editorwerte in den Zeichenkern einzuschleusen.

Der Editor kann weiterhin eigene Koordinatentypen verwenden. Seine Adapter
konvertieren `PUIRect` ausdrücklich zu `PDrawRect`. Die bisherigen Geometrieheader
enthalten vorerst Aliasnamen/kleine Konvertierungen, keine zweite Implementierung.
Das Dokumentformat und das Speicherlayout der fachlichen Daten ändern sich durch
diese Auslagerung nicht.

## Text, Bilder und Ressourcen

`pdraw_text_layout` berechnet UTF-8-Umbruch, Ausrichtung, Zeilenabstand und
mehrspaltigen Textfluss anhand eines vom Aufrufer gelieferten Metrik-Callbacks.
Die Library sucht selbst keine installierten Schriften und lädt keine Dateien.
Glyphrasterisierung, Schriftkatalog und Cache-Besitz bleiben aktuell im
Schrift-Service bzw. im Textadapter des Vektoreditors.

`pdraw_texture_draw` zeichnet Bilder oder bereits rasterisierte Textframes mit
Position, Drehung, Spiegelung und Farbe/Deckkraft. Die Textur gehört weiterhin dem
Aufrufer; ihre Farb-/Alphamodulation wird wiederhergestellt. So kann ein anderes
Programm seinen eigenen Font- oder Asset-Service verwenden.

`pdraw_mask_render` zeichnet über einen Callback durch beliebige geschlossene
Pfade, einschließlich Löchern. `pdraw_shadow_draw` erzeugt aus einem
Silhouetten-Callback den Schatten. Gruppierung und die Entscheidung, welches
Objekt als Maske dient, sind Aufgaben des jeweiligen Programms.

SDL-Aufrufe und GPU-Ressourcen gehören auf den Renderer-Thread. Reine Geometrie
kann auf unabhängigen Werten in anderen Threads berechnet werden. Keine Library
übernimmt die Lebensdauer eines fremden Modells oder Renderers.

## Grenzen und Prüfungen

Geometrie bleibt bewusst begrenzt: 128 Pfadpunkte, 32 Konturen, 32 Bézierknoten,
8 Verlaufsstopps und 2048 Textbytes pro Textframe. Fehler bei begrenzten
Geometrieoperationen werden über Rückgabewerte gemeldet. Abgeleitete Kurven können
zum Einhalten der Konturgrenze gröber approximiert werden.

`pdraw_render` verwendet eine wiederverwendbare 2048×2048-RGBA-Arbeitstextur
(bis 16 MiB). `pdraw_render_cached` ergänzt einen LRU-Cache für fertig gezeichnete
Kacheln: höchstens 16 Einträge mit zusammen 64 MiB Texturdaten. Diese Grenze gilt
pro `PDrawRenderer`; Vorschau und Canvas können eigene Renderer besitzen.
Der Aufrufer liefert eine Inhaltsrevision, die bei jeder sichtbaren Änderung
wechseln muss. Bounds, Clipping, Renderziel und Renderer werden zusätzlich geprüft.
Unveränderte Kacheln werden als Textur ausgegeben, ohne Geometrie, Schattenfilter
oder GPU-Readback erneut auszuführen. Neue/ungültige Kacheln werden weiterhin
synchron gezeichnet. Änderungen und Kamerabewegungen sind deshalb teurer als eine
ruhende Ansicht. Bei fehlendem Speicher wird unmittelbar gezeichnet.

Der Vector-Adapter bildet die Revision aus aufgelösten Formen, Artboard, Ansicht,
Auswahl und Farbvorschau. Für Texteingabe und laufende Formerstellung verwendet er
den unmittelbaren Pfad, damit Cursor und vorläufige Formen aktuell bleiben.
Nach Editorwechsel beziehungsweise beim Freigeben werden auch die Kacheln zerstört.
Der Cache gehört nicht zu Dokument, History oder Projektdatei.

Er stellt Renderziel, Viewport, Clipping, Skalierung, Farbe und Blendmodus wieder
her. Bei nicht unterstützten Zieltexturen wird nativ gezeichnet. Masken und
Schatten besitzen ihre eigenen temporären Ressourcen; die genannten Cache-Grenzen sind keine
Gesamtgrenze für alle Effekte oder Texturen.

`pengdraw_core` und `pengdraw_sdl` testen die Library direkt ohne Editor-/UI-Link.
Die vorhandenen Editor-, Vorschau-, Export-, Text-, Masken- und Bildtests prüfen
zusätzlich die Adapter. `tools/check_architecture.py` verbietet Abhängigkeiten
von pengDraw auf UI- oder Editor-Code.

## GPU-Ausgabe

Die Desktop-Apps bevorzugen seit SDL 3.4 den GPU-Renderer mit eigenen Schatten- und Verlaufs-Shadern. Rendererwahl, Lebensdauer, CPU-Restaufgaben und Tests beschreibt [GPU-Rendering](GPU_RENDERING.md).

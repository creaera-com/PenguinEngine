/** @file
 * @brief Vector-Art-Arbeitsbereich; vollständig auslagerbarer UI-/Dokumentzustand. */
#ifndef PENGUIN_VECTOR_EDITOR_H
#define PENGUIN_VECTOR_EDITOR_H
#include "editors/vector/document.h"
#include "penguin/ui_sdl.h"
#include "workspace/session.h"

/** @brief Interaktive Vektorwerkzeuge. */
typedef enum { VT_SELECT, VT_RECT, VT_ELLIPSE, VT_PEN, VT_NODE, VT_BOARD } VeTool;

/** @brief Persistente Navigation des Vektorarbeitsbereichs. */
typedef struct {
    char path[1024];                    ///< Dateipfad; leer für ein unbenanntes Dokument.
    uint64_t selected;                  ///< Bitmaske ausgewählter Objekt-Slots.
    uint32_t expanded_paths[VE_SHAPES]; ///< Stabile IDs aufgeklappter Pfade; 0 ist frei.
    int board;                          ///< Index des zugehörigen aktiven Artboards.
    int tool;                           ///< Aktives Werkzeug aus VeTool.
    int node;                           ///< Ausgewähltes Pfadsegment; -1 bedeutet keines.
    float x;                            ///< Horizontale Position in Pixeln.
    float y;                            ///< Vertikale Position in Pixeln.
    float zoom;                         ///< Zoomfaktor; 1 entspricht einem Pixel pro Dokumentpixel.
    uint8_t expanded[VE_BOARDS];        ///< Aufklappzustand je Artboard, 0 oder 1.
    uint8_t exists;                     ///< Ob das Dokument bereits als Datei existiert.
} VeView;

/** @brief Besitzender Editor; Render-Caches werden beim Verlassen freigegeben. */
typedef struct {
    VeDocument document;               ///< Besitzendes Vektordokument.
    VeHistory history;                 ///< Besitzender Undo-/Redo-Verlauf.
    VeView view;                       ///< Auslagerbare Navigation.
    SDL_Texture *texture;              ///< Besitzende SDL-Vorschautextur.
    struct NSVGrasterizer *rasterizer; ///< Interner Feldzustand.
    unsigned char *pixels;             ///< Interner Feldzustand.
    int width;                         ///< Breite des Rastercaches.
    int height;                        ///< Höhe des Rastercaches.
    float cached_x;                    ///< Kameraposition beim letzten Rasterlauf.
    float cached_y;                    ///< Kameraposition beim letzten Rasterlauf.
    float cached_zoom;                 ///< Zoom beim letzten Rasterlauf.
    bool render_dirty;                 ///< Ob der SVG-Rastercache erneuert werden muss.
    bool dragging;                     ///< Laufende, noch nicht bestätigte Dokumentgeste.
    bool panning;                      ///< Laufende Kameraverschiebung.
    int drag_shape;                    ///< Objekt-Slot der aktuellen Geste; -1 für keinen.
    int drag_handle;                   ///< 0 für Anker, 1/2 für Bézier-Kontrollpunkte.
    int drag_node;                     ///< Pfadsegment der Geste; -1 für keines.
    int drag_board;                    ///< Artboard der Geste; -1 für keines.
    int after_save;                    ///< Aufgeschobene Request-Aktion nach Save As.
    float last_x;                      ///< Letzte Zeigerkoordinate in Dokumentpixeln.
    float last_y;                      ///< Letzte Zeigerkoordinate in Dokumentpixeln.
    float start_x;                     ///< Startkoordinate der Dokumentgeste.
    float start_y;                     ///< Startkoordinate der Dokumentgeste.
    SDL_FRect canvas;                  ///< Sichtbare Zeichenfläche ohne Lineale.
} VectorEditor;

/** @brief Initialisiert ein leeres Dokument und Verlauf. */
bool vector_init(VectorEditor *editor);
/** @brief Gibt Verlauf, Rasterizer, Pixel und Textur frei. */
void vector_free(VectorEditor *editor);
/** @brief Codec für temporäre native Sitzungen einschließlich Undo/Redo. */
bool vector_checkpoint(VectorEditor *editor, PeSession *session);
/** @brief Prüft den aktiven Arbeitsbereich. */
bool editor_vector_active(void);
/** @brief Zeichnet YAML-Werkzeugliste des Vector-Editors. */
void vector_draw_tools(SDL_FRect area);
/** @brief Zeichnet Artboards, SVG, Pixel-Lineale und Griffe. */
void vector_draw_canvas(void);
/** @brief Zeichnet Artboard-/Objektbaum. */
void vector_draw_tree(void);
/** @brief Zeichnet kontextbezogene Eigenschaften. */
void vector_draw_inspector(void);
/** @brief Verarbeitet Canvas-/Werkzeugereignisse; true bedeutet verbraucht. */
bool vector_event(SDL_Event *event);
/** @brief Verarbeitet registrierte vector.*-Aktionen. */
bool vector_action(const char *action);
/** @brief Übernimmt ein Inspector-/Dateifeld; field ist ein Field-Enumwert. */
void vector_apply_field(int field);
/** @brief Speichert oder öffnet den Save-As-Dialog. */
bool vector_save(void);
/** @brief Öffnet SVG atomar und setzt den Verlauf zurück. */
bool vector_open(const char *path);
/** @brief Erzeugt ein neues leeres Dokument. */
void vector_new(void);
/**
 * @brief Setzt 2..3200 Prozent Zoom und erhält den Weltpunkt unter der Bildschirmposition.
 * @param zoom Gewünschter Faktor; 1 bedeutet 100 Prozent.
 * @param x Horizontale Bildschirmposition als Zoom-Anker.
 * @param y Vertikale Bildschirmposition als Zoom-Anker.
 */
void vector_zoom(float zoom, float x, float y);
/** @brief Undo/Redo einschließlich Navigation-Synchronisation. */
void vector_undo(bool redo);
/** @brief Erstellt ausschließlich für Vorschau/Tests eine Beispielzeichnung. */
void vector_demo(void);
#endif

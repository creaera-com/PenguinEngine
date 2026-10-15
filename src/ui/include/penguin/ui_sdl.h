/**
 * @file
 * @brief SDL-Darstellung, Schrift, Symbole und zur Laufzeit veränderbares UI-Theme.
 * @ingroup ui_api
 */

/** @addtogroup ui_api
 * @{
 */
#ifndef PENGUIN_UI_SDL_H
#define PENGUIN_UI_SDL_H
#include "penguin/ui.h"
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
/** @brief Aktueller Theme-Farbwert UI_CANVAS als RGBA. */
#define UI_CANVAS (ui_theme.canvas)
/** @brief Aktueller Theme-Farbwert UI_PANEL als RGBA. */
#define UI_PANEL (ui_theme.panel)
/** @brief Aktueller Theme-Farbwert UI_SURFACE als RGBA. */
#define UI_SURFACE (ui_theme.surface)
/** @brief Aktueller Theme-Farbwert UI_BORDER als RGBA. */
#define UI_BORDER (ui_theme.border)
/** @brief Aktueller Theme-Farbwert UI_HOVER als RGBA. */
#define UI_HOVER (ui_theme.hover)
/** @brief Aktueller Theme-Farbwert UI_ACCENT als RGBA. */
#define UI_ACCENT (ui_theme.accent)
/** @brief Aktueller Theme-Farbwert UI_ACCENT_BG als RGBA. */
#define UI_ACCENT_BG (ui_theme.accent)
/** @brief Aktueller Theme-Farbwert UI_TEXT als RGBA. */
#define UI_TEXT (ui_theme.text)
/** @brief Aktueller Theme-Farbwert UI_MUTED als RGBA. */
#define UI_MUTED (ui_theme.muted)
/** @brief Aktueller Theme-Farbwert UI_DISABLED als RGBA. */
#define UI_DISABLED (ui_theme.disabled)

/**
 * @brief Zeichnet gestrichelte Baumverbindungen für eine Zeile; erhält den aktiven Clip.
 * @param renderer Geliehener SDL-Renderer.
 * @param depths Preorder-Tiefen aller aufgeklappten Zeilen, auch außerhalb des Viewports.
 * @param count Anzahl Einträge in depths.
 * @param row Nullbasierter aktueller Zeilenindex.
 * @param area Zeilenbereich; x ist der Ursprung der Wurzel-Einrückung, h die Zeilenhöhe.
 * @param indent Abstand pro Hierarchieebene in Pixeln.
 * @param marker_center Horizontale Mitte des Aufklappsymbols relativ zum Knotenanfang.
 * @param max_indent Maximale Einrückung; tiefere Linien werden begrenzt.
 * @param branch_end Horizontales Ende des Asts in Fensterpixeln, vor Symbol oder Label.
 * @param rgba Linienfarbe aus dem aktuellen Theme.
 */
void ui_draw_tree_guides(SDL_Renderer *renderer, const int *depths, int count, int row, UiRect area,
                         float indent, float marker_center, float max_indent, float branch_end,
                         Uint32 rgba);

/**
 * @brief Zeichnet ein einzeiliges Eingabefeld mit Auswahl, horizontalem Scrollen und Cursor.
 * @param renderer Geliehener SDL-Renderer.
 * @param rect Feldbereich in Fensterpixeln; der bisherige Render-Clip bleibt erhalten.
 * @param text Nullterminierter UTF-8-Text.
 * @param edit Feldzustand; scroll wird für den sichtbaren Cursor angepasst.
 * @param focused Ob Auswahl und orange Fokusumrandung sichtbar sind.
 * @param enabled Ob das Feld bedienbar ist.
 * @param invalid Ob eine abgewiesene Eingabe rot markiert wird.
 * @param cursor_visible Blinkphase des Cursors; wird vom Aufrufer gesteuert.
 */
void ui_draw_text_input(SDL_Renderer *renderer, SDL_FRect rect, const char *text, UiTextEdit *edit,
                        bool focused, bool enabled, bool invalid, bool cursor_visible);

/** @brief Interne Layout-/Darstellungsvorgabe UI_FONT_WIDTH. */
#define UI_FONT_WIDTH 8
/** @brief Interne Layout-/Darstellungsvorgabe UI_FONT_HEIGHT. */
#define UI_FONT_HEIGHT 16
/* UTF-8, Latin-1 glyphs; unsupported codepoints use ?. Texture cached per renderer. */
/**
 * @brief Misst UTF-8-Text mit der eingebetteten Schrift in Pixeln.
 *
 * @param text Nullterminierter UTF-8-Text.
 */
float ui_text_width(const char *text);

/**
 * @brief Zeichnet UTF-8-Text mit Latin-1-Glyphen; unbekannte Zeichen erscheinen als Fragezeichen.
 *
 * Die Schrifttextur wird pro Renderer zwischengespeichert und von SDL mit dem Renderer freigegeben.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param text Nullterminierter UTF-8-Text.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
bool ui_text(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color);

/** @brief UI-Palette als gepackte RGBA-Werte im Format 0xRRGGBBAA. */
typedef struct {
    Uint32 canvas;   ///< Hintergrundfarbe der Zeichenfläche als RGBA.
    Uint32 panel;    ///< Panelfarbe als RGBA.
    Uint32 surface;  ///< Farbe angehobener Bedienflächen als RGBA.
    Uint32 hover;    ///< Hoverfarbe als RGBA.
    Uint32 accent;   ///< Akzentfarbe als RGBA.
    Uint32 text;     ///< Standardtextfarbe als RGBA.
    Uint32 muted;    ///< Farbe zurückhaltender Beschriftungen als RGBA.
    Uint32 disabled; ///< Farbe deaktivierter Elemente als RGBA.
    Uint32 border;   ///< Rahmenfarbe als RGBA.
} UiTheme;

/** @brief Gemeinsame UI-Palette; über ui_set_theme aktualisieren. */
extern UiTheme ui_theme;

/**
 * @brief Kopiert ein Theme in den gemeinsam verwendeten UI-Farbzustand.
 *
 * @param theme Palette; Werte werden beim Setzen kopiert.
 */
void ui_set_theme(const UiTheme *theme);

/**
 * @brief Liefert die dunkelgraue Standardpalette mit gedämpftem orangefarbenem Akzent.
 */
UiTheme ui_dark_theme(void);

/** @brief Eingebautes, mit SDL-Primitiven gezeichnetes UI-Symbol. */
typedef enum {
    ICON_NONE,
    ICON_SAVE,
    ICON_UNDO,
    ICON_REDO,
    ICON_PLAY,
    ICON_STOP,
    ICON_IMPORT,
    ICON_BRUSH,
    ICON_ERASE,
    ICON_RECTANGLE,
    ICON_FILL,
    ICON_SELECT,
    ICON_COLLISION,
    ICON_GRID,
    ICON_REFRESH,
    ICON_FOLDER,
    ICON_FILE,
    ICON_IMAGE,
    ICON_LAYERS,
    ICON_ENTITY,
    ICON_PLUS,
    ICON_DELETE,
    ICON_EDIT,
    ICON_UP,
    ICON_DOWN,
    ICON_RIGHT,
    ICON_EYE,
    ICON_EYE_OFF,
    ICON_CHECK,
    ICON_CLOSE,
    ICON_MENU,
    ICON_TERMINAL, ///< Shell-/Konsolensymbol.
    ICON_MINIMIZE, ///< Fenster minimieren.
    ICON_MAXIMIZE, ///< Fenster maximieren.
    ICON_RESTORE,  ///< Fenstergröße wiederherstellen.
    ICON_COLLAPSE  ///< Fenster auf Titelleiste reduzieren.
} UiIcon;

/**
 * @brief Zeichnet einen flachen Icon-/Textbutton mit expliziter Palette.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param theme Palette; Werte werden beim Setzen kopiert.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_button_themed(SDL_Renderer *renderer, const UiTheme *theme, SDL_FRect rect,
                           UiIcon icon, const char *label, bool active, bool enabled, bool hover);

/**
 * @brief Füllt eine rechteckige Panelfläche mit einer RGBA-Farbe.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
void ui_panel(SDL_Renderer *renderer, SDL_FRect rect, Uint32 color);

/**
 * @brief Zeichnet ein skalierbares Symbol aus SDL-Linien und -Flächen.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param size Symbolkantenlänge in Pixeln.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
void ui_icon(SDL_Renderer *renderer, UiIcon icon, float x, float y, float size, Uint32 color);

/**
 * @brief Zeichnet einen flachen Icon-/Textbutton mit der aktuellen Palette.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_button(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                    bool active, bool enabled, bool hover);

/**
 * @brief Berechnet die benötigte Tabbreite für Icon, Beschriftung und Innenabstände.
 *
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 */
float ui_tab_content_width(UiIcon icon, const char *label);

/**
 * @brief Zeichnet einen Panel-Tab mit aktuellem Theme.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_tab(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                 bool active, bool enabled, bool hover);

/**
 * @brief Zeichnet Track und Griff passend zum Interaktionszustand.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param scroll Scrollzustand in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
void ui_draw_scrollbar(SDL_Renderer *renderer, const UiContext *context, UiId id, UiRect track,
                       const UiScroll *scroll, UiAxis axis);

/**
 * @brief Überträgt den aktuellen UI-Clip auf den SDL-Renderer.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_apply_clip(SDL_Renderer *renderer, const UiContext *context);
/** @} */
#endif

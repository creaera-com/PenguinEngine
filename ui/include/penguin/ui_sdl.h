#ifndef PENGUIN_UI_SDL_H
#define PENGUIN_UI_SDL_H
#include "penguin/ui.h"
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
#define UI_CANVAS (ui_theme.canvas)
#define UI_PANEL (ui_theme.panel)
#define UI_SURFACE (ui_theme.surface)
#define UI_BORDER (ui_theme.border)
#define UI_HOVER (ui_theme.hover)
#define UI_ACCENT (ui_theme.accent)
#define UI_ACCENT_BG (ui_theme.accent)
#define UI_TEXT (ui_theme.text)
#define UI_MUTED (ui_theme.muted)
#define UI_DISABLED (ui_theme.disabled)

#define UI_FONT_WIDTH 8
#define UI_FONT_HEIGHT 16
/* UTF-8, Latin-1 glyphs; unsupported codepoints use ?. Texture cached per renderer. */
float ui_text_width(const char *text);
bool ui_text(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color);

typedef struct {
    Uint32 canvas, panel, surface, hover, accent, text, muted, disabled, border;
} UiTheme;
extern UiTheme ui_theme;
void ui_set_theme(const UiTheme *theme);
UiTheme ui_dark_theme(void);
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
    ICON_MENU
} UiIcon;
void ui_draw_button_themed(SDL_Renderer *renderer, const UiTheme *theme, SDL_FRect rect,
                           UiIcon icon, const char *label, bool active, bool enabled, bool hover);
void ui_panel(SDL_Renderer *renderer, SDL_FRect rect, Uint32 color);
void ui_icon(SDL_Renderer *renderer, UiIcon icon, float x, float y, float size, Uint32 color);
void ui_draw_button(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                    bool active, bool enabled, bool hover);
float ui_tab_content_width(UiIcon icon, const char *label);
void ui_draw_tab(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                 bool active, bool enabled, bool hover);
void ui_draw_scrollbar(SDL_Renderer *renderer, const UiContext *context, UiId id, UiRect track,
                       const UiScroll *scroll, UiAxis axis);
void ui_apply_clip(SDL_Renderer *renderer, const UiContext *context);
#endif

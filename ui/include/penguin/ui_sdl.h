#ifndef PENGUIN_UI_SDL_H
#define PENGUIN_UI_SDL_H
#include "penguin/ui.h"
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
#define UI_CANVAS 0x101010ffu
#define UI_PANEL 0x202020ffu
#define UI_SURFACE 0x292929ffu
#define UI_BORDER 0x303030ffu
#define UI_HOVER 0x393939ffu
#define UI_ACCENT 0xc28e5cffu
#define UI_ACCENT_BG 0xc28e5cffu
#define UI_TEXT 0xe9e9e9ffu
#define UI_MUTED 0xa2a2a2ffu
#define UI_DISABLED 0x686868ffu

#define UI_FONT_WIDTH 8
#define UI_FONT_HEIGHT 16
/* UTF-8, Latin-1 glyphs; unsupported codepoints use ?. Texture cached per renderer. */
float ui_text_width(const char *text);
bool ui_text(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color);

typedef struct {
    Uint32 canvas, panel, surface, hover, accent, text, muted, disabled, border;
} UiTheme;
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
    ICON_CLOSE
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

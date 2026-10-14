#ifndef PENGUIN_EDITOR_UI_H
#define PENGUIN_EDITOR_UI_H
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
#define UI_CANVAS 0x171717ffu
#define UI_PANEL 0x212121ffu
#define UI_SURFACE 0x292929ffu
#define UI_BORDER 0x3c3c3cffu
#define UI_HOVER 0x383838ffu
#define UI_ACCENT 0xff932bffu
#define UI_ACCENT_BG 0x49301fffu
#define UI_TEXT 0xe9e9e9ffu
#define UI_MUTED 0xa2a2a2ffu
#define UI_DISABLED 0x686868ffu

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
void ui_round_rect(SDL_Renderer *renderer, SDL_FRect rect, float radius, Uint32 color);
void ui_icon(SDL_Renderer *renderer, UiIcon icon, float x, float y, float size, Uint32 color);
void ui_draw_button(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                    bool active, bool enabled, bool hover);
#endif

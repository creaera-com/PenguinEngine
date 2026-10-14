#include "ui.h"
#include <math.h>
#include <string.h>
static void color(SDL_Renderer *r, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}
void ui_round_rect(SDL_Renderer *r, SDL_FRect rect, float radius, Uint32 c) {
    color(r, c);
    float rad = fminf(radius, fminf(rect.w, rect.h) * .5f);
    SDL_FRect middle = {rect.x, rect.y + rad, rect.w, rect.h - 2 * rad};
    SDL_RenderFillRect(r, &middle);
    for (int y = 0; y < (int)ceilf(rad); y++) {
        float dy = rad - y - .5f;
        float inset = rad - sqrtf(fmaxf(0, rad * rad - dy * dy));
        SDL_FRect top = {rect.x + inset, rect.y + y, rect.w - 2 * inset, 1};
        SDL_FRect bottom = {rect.x + inset, rect.y + rect.h - y - 1, rect.w - 2 * inset, 1};
        SDL_RenderFillRect(r, &top);
        SDL_RenderFillRect(r, &bottom);
    }
}
typedef struct {
    float x1, y1, x2, y2;
} Line;
static void lines(SDL_Renderer *r, float x, float y, float size, const Line *v, int n) {
    for (int i = 0; i < n; i++)
        SDL_RenderLine(r, x + v[i].x1 * size / 16, y + v[i].y1 * size / 16, x + v[i].x2 * size / 16,
                       y + v[i].y2 * size / 16);
}
#define DRAW(...)                                                                                  \
    do {                                                                                           \
        const Line v[] = {__VA_ARGS__};                                                            \
        lines(r, x, y, s, v, (int)(sizeof v / sizeof v[0]));                                       \
    } while (0)
void ui_icon(SDL_Renderer *r, UiIcon icon, float x, float y, float s, Uint32 c) {
    color(r, c);
    switch (icon) {
    case ICON_SAVE:
        DRAW({2, 2, 12, 2}, {12, 2, 14, 4}, {14, 4, 14, 14}, {14, 14, 2, 14}, {2, 14, 2, 2},
             {5, 2, 5, 6}, {5, 6, 11, 6}, {11, 6, 11, 2}, {5, 14, 5, 10}, {5, 10, 11, 10},
             {11, 10, 11, 14});
        break;
    case ICON_UNDO:
        DRAW({6, 3, 2, 7}, {2, 7, 6, 11}, {2, 7, 10, 7}, {10, 7, 13, 9}, {13, 9, 13, 13});
        break;
    case ICON_REDO:
        DRAW({10, 3, 14, 7}, {14, 7, 10, 11}, {14, 7, 6, 7}, {6, 7, 3, 9}, {3, 9, 3, 13});
        break;
    case ICON_PLAY:
        for (int i = 0; i < 10; i++)
            SDL_RenderLine(r, x + 4 * s / 16 + i * s / 16, y + (2 + i * .6f) * s / 16,
                           x + 4 * s / 16 + i * s / 16, y + (14 - i * .6f) * s / 16);
        break;
    case ICON_STOP: {
        SDL_FRect rect = {x + 3 * s / 16, y + 3 * s / 16, 10 * s / 16, 10 * s / 16};
        SDL_RenderFillRect(r, &rect);
        break;
    }
    case ICON_IMPORT:
        DRAW({8, 1, 8, 10}, {4, 6, 8, 10}, {8, 10, 12, 6}, {2, 10, 2, 14}, {2, 14, 14, 14},
             {14, 14, 14, 10});
        break;
    case ICON_BRUSH:
        DRAW({10, 2, 14, 6}, {14, 6, 7, 13}, {7, 13, 3, 9}, {3, 9, 10, 2}, {3, 9, 1, 15},
             {1, 15, 7, 13}, {8, 4, 12, 8});
        break;
    case ICON_ERASE:
        DRAW({9, 2, 15, 8}, {15, 8, 9, 14}, {9, 14, 5, 14}, {5, 14, 1, 10}, {1, 10, 9, 2},
             {5, 6, 11, 12}, {3, 15, 15, 15});
        break;
    case ICON_RECTANGLE:
        DRAW({2, 2, 14, 2}, {14, 2, 14, 14}, {14, 14, 2, 14}, {2, 14, 2, 2}, {1, 1, 4, 1},
             {1, 1, 1, 4}, {12, 15, 15, 15}, {15, 12, 15, 15});
        break;
    case ICON_FILL:
        DRAW({7, 2, 14, 9}, {14, 9, 8, 15}, {8, 15, 1, 8}, {1, 8, 7, 2}, {2, 9, 13, 9},
             {4, 1, 8, 5}, {15, 11, 15, 14});
        break;
    case ICON_SELECT:
        DRAW({3, 1, 3, 13}, {3, 13, 7, 10}, {7, 10, 10, 15}, {10, 15, 12, 14}, {12, 14, 9, 9},
             {9, 9, 14, 9}, {14, 9, 3, 1});
        break;
    case ICON_COLLISION:
        DRAW({8, 1, 14, 4}, {14, 4, 13, 10}, {13, 10, 8, 15}, {8, 15, 3, 10}, {3, 10, 2, 4},
             {2, 4, 8, 1}, {8, 5, 8, 9}, {8, 12, 8, 12});
        break;
    case ICON_GRID:
        DRAW({2, 2, 14, 2}, {14, 2, 14, 14}, {14, 14, 2, 14}, {2, 14, 2, 2}, {6, 2, 6, 14},
             {10, 2, 10, 14}, {2, 6, 14, 6}, {2, 10, 14, 10});
        break;
    case ICON_REFRESH:
        DRAW({13, 6, 11, 3}, {11, 3, 5, 3}, {5, 3, 2, 6}, {2, 6, 2, 10}, {2, 10, 5, 13},
             {5, 13, 11, 13}, {11, 13, 14, 10}, {13, 1, 13, 6}, {13, 6, 8, 6});
        break;
    case ICON_FOLDER:
        DRAW({1, 4, 1, 14}, {1, 14, 15, 14}, {15, 14, 15, 5}, {15, 5, 7, 5}, {7, 5, 5, 2},
             {5, 2, 1, 2}, {1, 2, 1, 4}, {1, 7, 15, 7});
        break;
    case ICON_FILE:
        DRAW({3, 1, 10, 1}, {10, 1, 14, 5}, {14, 5, 14, 15}, {14, 15, 3, 15}, {3, 15, 3, 1},
             {10, 1, 10, 5}, {10, 5, 14, 5}, {6, 9, 11, 9}, {6, 12, 11, 12});
        break;
    case ICON_IMAGE:
        DRAW({2, 2, 14, 2}, {14, 2, 14, 14}, {14, 14, 2, 14}, {2, 14, 2, 2}, {3, 12, 7, 7},
             {7, 7, 10, 11}, {10, 11, 12, 9}, {12, 9, 14, 12}, {11, 5, 11, 5});
        break;
    case ICON_LAYERS:
        DRAW({8, 1, 15, 5}, {15, 5, 8, 9}, {8, 9, 1, 5}, {1, 5, 8, 1}, {1, 8, 8, 12},
             {8, 12, 15, 8}, {1, 11, 8, 15}, {8, 15, 15, 11});
        break;
    case ICON_ENTITY:
        DRAW({8, 1, 14, 4}, {14, 4, 14, 12}, {14, 12, 8, 15}, {8, 15, 2, 12}, {2, 12, 2, 4},
             {2, 4, 8, 1}, {2, 4, 8, 7}, {8, 7, 14, 4}, {8, 7, 8, 15});
        break;
    case ICON_PLUS:
        DRAW({8, 3, 8, 13}, {3, 8, 13, 8});
        break;
    case ICON_DELETE:
        DRAW({3, 4, 13, 4}, {5, 4, 5, 14}, {5, 14, 11, 14}, {11, 14, 11, 4}, {6, 1, 10, 1},
             {6, 1, 6, 4}, {10, 1, 10, 4}, {8, 7, 8, 11});
        break;
    case ICON_EDIT:
        DRAW({11, 1, 15, 5}, {15, 5, 5, 15}, {5, 15, 1, 15}, {1, 15, 1, 11}, {1, 11, 11, 1},
             {9, 3, 13, 7});
        break;
    case ICON_UP:
        DRAW({3, 10, 8, 5}, {8, 5, 13, 10});
        break;
    case ICON_RIGHT:
        DRAW({6, 3, 11, 8}, {11, 8, 6, 13});
        break;
    case ICON_DOWN:
        DRAW({3, 6, 8, 11}, {8, 11, 13, 6});
        break;
    case ICON_EYE:
    case ICON_EYE_OFF:
        DRAW({1, 8, 5, 4}, {5, 4, 11, 4}, {11, 4, 15, 8}, {15, 8, 11, 12}, {11, 12, 5, 12},
             {5, 12, 1, 8}, {7, 7, 9, 7}, {9, 7, 9, 9}, {9, 9, 7, 9}, {7, 9, 7, 7});
        if (icon == ICON_EYE_OFF)
            DRAW({2, 1, 14, 15});
        break;
    case ICON_CHECK:
        DRAW({2, 8, 6, 12}, {6, 12, 14, 3});
        break;
    case ICON_CLOSE:
        DRAW({3, 3, 13, 13}, {13, 3, 3, 13});
        break;
    case ICON_NONE:
        break;
    }
}
void ui_draw_button(SDL_Renderer *r, SDL_FRect rect, UiIcon icon, const char *label, bool active,
                    bool enabled, bool hover) {
    Uint32 border = enabled && active ? UI_ACCENT : enabled && hover ? 0x575757ffu : UI_BORDER;
    ui_round_rect(r, rect, 5, border);
    SDL_FRect inner = {rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
    ui_round_rect(r, inner, 4,
                  !enabled ? 0x242424ffu
                  : active ? UI_ACCENT_BG
                  : hover  ? UI_HOVER
                           : UI_SURFACE);
    Uint32 foreground = !enabled ? UI_DISABLED : active ? UI_ACCENT : UI_TEXT;
    bool label_present = label && *label;
    if (icon != ICON_NONE)
        ui_icon(r, icon, label_present ? rect.x + 9 : rect.x + (rect.w - 16) * .5f,
                rect.y + (rect.h - 16) * .5f, 16, foreground);
    if (label_present) {
        color(r, foreground);
        SDL_RenderDebugText(r, rect.x + (icon == ICON_NONE ? 9 : 31), rect.y + (rect.h - 8) * .5f,
                            label);
    }
}

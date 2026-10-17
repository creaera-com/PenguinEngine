/** @file @brief Vector icon drawing and YAML icon-name lookup. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

typedef struct {
    float x1, y1, x2, y2;
} Line;

static void lines(SDL_Renderer *r, float x, float y, float size, const Line *v, int n) {
    for (int i = 0; i < n; i++) {
        SDL_RenderLine(r, x + v[i].x1 * size / 16, y + v[i].y1 * size / 16, x + v[i].x2 * size / 16,
                       y + v[i].y2 * size / 16);
    }
}

#define DRAW(...)                                                                                  \
    do {                                                                                           \
        const Line v[] = {__VA_ARGS__};                                                            \
        lines(r, x, y, s, v, (int)(sizeof v / sizeof v[0]));                                       \
    } while (0)

void pui_icon(SDL_Renderer *r, PUIIcon icon, float x, float y, float s, Uint32 c) {
    pui_render_color(r, c);
    switch (icon) {
    case ICON_MINIMIZE:
        DRAW({3, 12, 13, 12});
        break;
    case ICON_MAXIMIZE:
        DRAW({3, 3, 13, 3}, {13, 3, 13, 13}, {13, 13, 3, 13}, {3, 13, 3, 3});
        break;
    case ICON_RESTORE:
        DRAW({5, 2, 14, 2}, {14, 2, 14, 11}, {14, 11, 11, 11}, {2, 5, 11, 5}, {11, 5, 11, 14},
             {11, 14, 2, 14}, {2, 14, 2, 5});
        break;
    case ICON_COLLAPSE:
        DRAW({2, 3, 14, 3}, {5, 11, 8, 8}, {8, 8, 11, 11});
        break;
    case ICON_TERMINAL:
        SDL_RenderLine(r, x + s * .15f, y + s * .25f, x + s * .45f, y + s * .5f);
        SDL_RenderLine(r, x + s * .45f, y + s * .5f, x + s * .15f, y + s * .75f);
        SDL_RenderLine(r, x + s * .55f, y + s * .75f, x + s * .85f, y + s * .75f);
        break;
    case ICON_MENU:
        DRAW({2, 4, 14, 4}, {2, 8, 14, 8}, {2, 12, 14, 12});
        break;
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
        for (int i = 0; i < 10; i++) {
            SDL_RenderLine(r, x + 4 * s / 16 + i * s / 16, y + (2 + i * .6f) * s / 16,
                           x + 4 * s / 16 + i * s / 16, y + (14 - i * .6f) * s / 16);
        }
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
        if (icon == ICON_EYE_OFF) {
            DRAW({2, 1, 14, 15});
        }
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

PUIIcon pui_icon_from_name(const char *name) {
    static const struct {
        const char *name;
        PUIIcon value;
    } icons[] = {
        {"minimize", ICON_MINIMIZE}, {"maximize", ICON_MAXIMIZE}, {"restore", ICON_RESTORE},
        {"menu", ICON_MENU},         {"save", ICON_SAVE},         {"undo", ICON_UNDO},
        {"redo", ICON_REDO},         {"folder", ICON_FOLDER},     {"file", ICON_FILE},
        {"plus", ICON_PLUS},         {"delete", ICON_DELETE},     {"edit", ICON_EDIT},
        {"check", ICON_CHECK},       {"close", ICON_CLOSE},       {"grid", ICON_GRID},
        {"refresh", ICON_REFRESH},   {"image", ICON_IMAGE},       {"settings", ICON_LAYERS}};

    for (size_t i = 0; i < sizeof icons / sizeof icons[0]; i++) {
        if (!strcmp(name, icons[i].name)) {
            return icons[i].value;
        }
    }
    return ICON_NONE;
}

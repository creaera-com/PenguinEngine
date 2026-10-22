#include "editors/vector/document/document.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/rendering/objects.h"
#include "unity.h"
#include <stdlib.h>
static SDL_Window *window;
static SDL_Renderer *renderer;
static VectorDocument *d;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("Theme highlighting", 160, 120, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    d = calloc(1, sizeof *d);
    TEST_ASSERT_NOT_NULL(d);
    vector_document_init(d, 1);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){0, 0, 100, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 40, 30}));
    vector_document_select(d, 1);
    d->viewport.zoom = 1;
}

void tearDown(void) {
    vector_history_destroy(&d->history);
    free(d);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_selection_uses_central_theme_without_modifying_artwork(void) {
    int cursor = d->history.cursor;
    uint32_t fill = d->rectangles[0].fill;
    PUITheme saved = pui_theme;
    uint32_t colors[] = {0x28a4e8ff, 0x35aa66ff};
    for (unsigned i = 0; i < 2; i++) {
        PUITheme theme = saved;
        theme.accent = colors[i];
        pui_set_theme(&theme);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        vector_objects_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 100, 100}, NULL, NULL);
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        Uint8 r, g, b, a;
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 20, 40, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_HEX32(colors[i],
                                ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 40, 45, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_HEX32(fill,
                                ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a);
        SDL_DestroySurface(surface);
    }
    pui_set_theme(&saved);
    TEST_ASSERT_EQUAL_HEX32(fill, d->rectangles[0].fill);
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_selection_uses_central_theme_without_modifying_artwork);
    return UNITY_END();
}

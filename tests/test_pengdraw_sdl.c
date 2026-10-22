/** @file @brief Standalone pengDraw pixels, replay, masking and resource-state contracts. */
#include "pengdraw/pengdraw.h"
#include "unity.h"
static SDL_Window *window;
static SDL_Renderer *renderer;
static PDrawRenderer cache;
static int calls;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("pengDraw only", 160, 120, SDL_WINDOW_HIDDEN);
    TEST_ASSERT_NOT_NULL(window);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    calls = 0;
}

void tearDown(void) {
    pdraw_renderer_destroy(&cache);
    pdraw_renderer_destroy(&cache);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static uint32_t pixel(int x, int y) {
    SDL_Surface *s = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(s);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
    SDL_DestroySurface(s);
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

static void paint(void *context, SDL_Renderer *r, float x, float y, float scale) {
    const PDrawShape *shape = context;
    calls++;
    pdraw_shape_draw(r, shape, (PDrawRect){20, 20, 80, 60}, 0, (SDL_FRect){x, y, 0, 0}, scale,
                     0x336699ff);
}

static void test_standalone_supersampling_and_clipping(void) {
    PDrawShape shape = {.ellipse = true};
    SDL_Rect clip = {30, 0, 130, 120};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    pdraw_render(&cache, renderer, (SDL_FRect){0, 0, 160, 120}, paint, &shape);
    TEST_ASSERT_TRUE(calls > 0);
    TEST_ASSERT_NOT_NULL(cache.target);
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(60, 50));
    TEST_ASSERT_EQUAL_HEX32(0x000000ff, pixel(25, 50));
    TEST_ASSERT_NULL(SDL_GetRenderTarget(renderer));
    SDL_Rect restored;
    SDL_GetRenderClipRect(renderer, &restored);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &restored, sizeof clip);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    TEST_ASSERT_EQUAL_INT(SDL_BLENDMODE_ADD, blend);
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(7, r);
    TEST_ASSERT_EQUAL_UINT8(10, a);
}

static void test_borrowed_texture_modulation_is_restored(void) {
    SDL_Surface *s = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
    TEST_ASSERT_NOT_NULL(s);
    SDL_FillSurfaceRect(s, NULL, SDL_MapSurfaceRGBA(s, 255, 255, 255, 255));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, s);
    SDL_DestroySurface(s);
    TEST_ASSERT_NOT_NULL(texture);
    SDL_SetTextureColorMod(texture, 11, 22, 33);
    SDL_SetTextureAlphaMod(texture, 44);
    SDL_FRect destination = {10, 10, 30, 30};
    TEST_ASSERT_TRUE(
        pdraw_texture_draw(renderer, texture, NULL, &destination, 0, SDL_FLIP_NONE, 0xff0000ff));
    TEST_ASSERT_EQUAL_HEX32(0xff0000ff, pixel(20, 20));
    Uint8 r, g, b, a;
    SDL_GetTextureColorMod(texture, &r, &g, &b);
    SDL_GetTextureAlphaMod(texture, &a);
    TEST_ASSERT_EQUAL_UINT8(11, r);
    TEST_ASSERT_EQUAL_UINT8(22, g);
    TEST_ASSERT_EQUAL_UINT8(33, b);
    TEST_ASSERT_EQUAL_UINT8(44, a);
    SDL_DestroyTexture(texture);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_standalone_supersampling_and_clipping);
    RUN_TEST(test_borrowed_texture_modulation_is_restored);
    return UNITY_END();
}

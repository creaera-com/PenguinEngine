#include "editor_host/host.h"
#include "editors/vector/grid/render.h"
#include "editors/vector/internal.h"
#include "shell/shell.h"
#include "unity.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("grid", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_preferences_validate_isolate_and_survive_editor_unload(void) {
    VectorDocument *doc = document();
    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_FALSE(doc->grid.visible);
    TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.width);
    TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.height);
    action("vector.grid.toggle");
    TEST_ASSERT_TRUE(doc->grid.visible);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_grid_toggle"));
    const char *invalid[] = {"0", "-2", "nan", "inf", "10001", "24px", "", "1e90"};
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        pui_view_set_text(shell.view, "vector_grid_width", invalid[i]);
        action("vector.grid.width");
        TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.width);
        TEST_ASSERT_EQUAL_STRING("16", pui_view_text(shell.view, "vector_grid_width"));
    }
    pui_view_set_text(shell.view, "vector_grid_width", "24");
    action("vector.grid.width");
    pui_view_set_text(shell.view, "vector_grid_height", "32");
    action("vector.grid.height");
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "vector_grid_type", "hexagonal"));
    action("vector.grid.type");
    TEST_ASSERT_TRUE(pui_view_set_color(shell.view, "vector_grid_color", 0xAABBCC99));
    action("vector.grid.color");
    TEST_ASSERT_EQUAL_INT(0, doc->history.count);
    action("vector.add_document");
    TEST_ASSERT_FALSE(document()->grid.visible);
    TEST_ASSERT_EQUAL_FLOAT(16, document()->grid.width);
    activate("scripts");
    activate("vector");
    VectorEditor *editor = host.instance;
    doc = vector_documents_find(&editor->documents, 1);
    TEST_ASSERT_TRUE(doc->grid.visible);
    TEST_ASSERT_EQUAL_FLOAT(24, doc->grid.width);
    TEST_ASSERT_EQUAL_FLOAT(32, doc->grid.height);
    TEST_ASSERT_EQUAL_INT(VECTOR_GRID_HEXAGONAL, doc->grid.type);
    TEST_ASSERT_EQUAL_HEX32(0xAABBCC99, doc->grid.color);
    pui_pointer(&shell.input, 300, 108, true);
    pui_pointer(&shell.input, 300, 108, false);
    commit();
    commit();
    TEST_ASSERT_EQUAL_STRING("24", pui_view_text(shell.view, "vector_grid_width"));
    TEST_ASSERT_EQUAL_STRING("hexagonal", pui_view_value(shell.view, "vector_grid_type"));
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_grid_toggle"));
}

static Uint32 pixel(SDL_Surface *surface, int x, int y) {
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
    return ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | a;
}

static void test_grids_clip_follow_zoom_and_bound_dense_rendering(void) {
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect clip = {12, 12, 140, 140};
    SDL_SetRenderClipRect(renderer, &clip);
    VectorGrid grid = {.visible = true, .width = 16, .height = 16, .color = 0xFF0000FF};
    vector_grid_draw(&grid, renderer, (SDL_FRect){20, 20, 100, 100}, 2);
    SDL_Rect restored;
    SDL_GetRenderClipRect(renderer, &restored);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &restored, sizeof clip);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0xFF0000FF, pixel(surface, 52, 30));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 36, 30));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 52, 121));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 52, 19));
    SDL_DestroySurface(surface);
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    grid.type = VECTOR_GRID_HEXAGONAL;
    vector_grid_draw(&grid, renderer, (SDL_FRect){20, 20, 100, 100}, 1);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0xFF0000FF, pixel(surface, 20, 28));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 28, 28));
    SDL_DestroySurface(surface);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    grid.width = grid.height = 1;
    vector_grid_draw(&grid, renderer, (SDL_FRect){-1000000, -1000000, 2000000, 2000000}, .1f);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 20, 28));
    SDL_DestroySurface(surface);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_preferences_validate_isolate_and_survive_editor_unload);
    RUN_TEST(test_grids_clip_follow_zoom_and_bound_dense_rendering);
    return UNITY_END();
}

#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/effects/render.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/gradient/render.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/mask/render.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/object/group.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/preview/render.h"
#include "editors/vector/preview/tool.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/rendering/objects.h"
#include "editors/vector/repeat/render.h"
#include "editors/vector/repeat/repeat.h"
#include "editors/vector/text/text.h"
#include "editors/vector/ui/document_tree/groups.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_TRUE(editor_host_action(context, shell.view, name, widget));
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action, &host);
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("element snapping", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    VectorDocument *d = document();
    vector_history_destroy(&d->history);
    vector_document_init(d, d->id);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){0, 0, 600, 400}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 40, 30}));
    vector_document_select(d, 1);
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static VectorGradient ramp(void) {
    VectorGradient g = vector_gradient_default(0xff0000ff);
    g.stops[1].color = 0x0000ffff;
    return g;
}

static void set_gradient(VectorGradient g) {
    VectorShape shape = document()->rectangles[0];
    shape.gradient = g;
    TEST_ASSERT_TRUE(vector_shape_update(document(), &shape));
}

static void test_pure_types_alpha_and_stop_validation(void) {
    VectorGradient g = vector_gradient_default(0x000000ff);
    TEST_ASSERT_EQUAL_HEX32(0x808080ff, vector_gradient_sample(&g, 50, 50, 100, 100));
    g.x1 = .5f;
    g.kind = VECTOR_GRADIENT_RADIAL;
    TEST_ASSERT_EQUAL_HEX32(0x000000ff, vector_gradient_sample(&g, 50, 50, 100, 100));
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, vector_gradient_sample(&g, 50, 100, 100, 100));
    g.kind = VECTOR_GRADIENT_REFLECTED;
    TEST_ASSERT_EQUAL_HEX32(vector_gradient_sample(&g, 25, 50, 100, 100),
                            vector_gradient_sample(&g, 75, 50, 100, 100));
    g.kind = VECTOR_GRADIENT_CONICAL;
    TEST_ASSERT_EQUAL_HEX32(0x404040ff, vector_gradient_sample(&g, 50, 75, 100, 100));
    TEST_ASSERT_EQUAL_HEX32(0x808080ff, vector_gradient_sample(&g, 25, 50, 100, 100));
    TEST_ASSERT_EQUAL_HEX32(0xbfbfbfff, vector_gradient_sample(&g, 50, 25, 100, 100));
    g = ramp();
    g.stops[1].color = 0x0000ff00;
    TEST_ASSERT_EQUAL_HEX32(0xff000080, vector_gradient_sample(&g, 50, 50, 100, 100));
    TEST_ASSERT_EQUAL_INT(1, vector_gradient_add_stop(&g));
    TEST_ASSERT_EQUAL_FLOAT(.5f, g.stops[1].position);
    g.stops[1].position = 0;
    TEST_ASSERT_FALSE(vector_gradient_valid(&g));
    g.stops[1].position = .5f;
    TEST_ASSERT_TRUE(vector_gradient_remove_stop(&g, 0));
    TEST_ASSERT_EQUAL_FLOAT(0, g.stops[0].position);
    TEST_ASSERT_FALSE(vector_gradient_remove_stop(&g, 0));
    g.x2 = NAN;
    TEST_ASSERT_FALSE(vector_gradient_valid(&g));
}

static SDL_Surface *render_shapes(void) {
    VectorDocument *d = document();
    VectorEditor *editor = host.instance;
    vector_document_select(d, 0);
    d->viewport.zoom = 1;
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    vector_objects_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 600, 400},
                        editor->text_renderer, editor->image_renderer);
    return SDL_RenderReadPixels(renderer, NULL);
}

static uint32_t pixel(SDL_Surface *s, int x, int y) {
    Uint8 r, g, b, a;
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

static void test_render_gradient_rectangle_ellipse_and_holed_closed_path(void) {
    set_gradient(ramp());
    SDL_Surface *pixels = render_shapes();
    TEST_ASSERT_GREATER_THAN(200, pixel(pixels, 24, 40) >> 24);
    TEST_ASSERT_GREATER_THAN(200, (pixel(pixels, 55, 40) >> 8) & 255);
    SDL_DestroySurface(pixels);
    VectorShape shape;
    TEST_ASSERT_TRUE(vector_ellipse_shape((PUIRect){20, 30, 40, 30}, &shape));
    shape.id = 1;
    shape.parent_id = 1;
    shape.gradient = ramp();
    snprintf(shape.name, sizeof shape.name, "Ellipse");
    TEST_ASSERT_TRUE(vector_shape_update(document(), &shape));
    pixels = render_shapes();
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(pixels, 21, 31));
    TEST_ASSERT_LESS_THAN(200, pixel(pixels, 40, 45) >> 24);
    SDL_DestroySurface(pixels);
    shape = document()->rectangles[0];
    shape.curve = (VectorBezier){0};
    shape.curve_kind = 0;
    shape.path = (VectorPath){.count = 8,
                              .contours = 2,
                              .ends = {4, 8},
                              .points = {{0, 0},
                                         {1, 0},
                                         {1, 1},
                                         {0, 1},
                                         {.25f, .25f},
                                         {.25f, .75f},
                                         {.75f, .75f},
                                         {.75f, .25f}}};
    TEST_ASSERT_TRUE(vector_shape_update(document(), &shape));
    pixels = render_shapes();
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(pixels, 40, 45));
    TEST_ASSERT_GREATER_THAN(200, pixel(pixels, 24, 40) >> 24);
    SDL_DestroySurface(pixels);
}

static void test_partial_clip_and_rotation_keep_correct_texture_coordinates(void) {
    set_gradient(ramp());
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(document(), 1, 25));
    VectorDocument *d = document();
    VectorEditor *editor = host.instance;
    vector_document_select(d, 0);
    d->viewport.zoom = 1;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_Rect clip = {30, 35, 20, 15};
    SDL_SetRenderClipRect(renderer, &clip);
    vector_objects_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 600, 400},
                        editor->text_renderer, editor->image_renderer);
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_EQUAL(0xffffffff, pixel(pixels, 40, 42));
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(pixels, 25, 42));
    SDL_DestroySurface(pixels);
}

static void test_yaml_stop_edits_are_atomic_and_solid_fill_also_works_for_closed_curves(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "gradient_type", "radial"));
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.gradient.kind", "gradient_type"));
    commit();
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_EQUAL_UINT(VECTOR_GRADIENT_RADIAL, d->rectangles[0].gradient.kind);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.gradient.add", ""));
    commit();
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[0].gradient.count);
    TEST_ASSERT_EQUAL_STRING("50", pui_view_text(shell.view, "gradient_stop_position"));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "gradient_stop_position", "40"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.gradient.position", ""));
    commit();
    TEST_ASSERT_EQUAL_FLOAT(.4f, d->rectangles[0].gradient.stops[1].position);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    commit();
    TEST_ASSERT_EQUAL_STRING("50", pui_view_text(shell.view, "gradient_stop_position"));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "gradient_stop_position", "110"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.gradient.position", ""));
    commit();
    TEST_ASSERT_EQUAL_STRING("50", pui_view_text(shell.view, "gradient_stop_position"));
    VectorBezier closed = {.count = 4, .closed = 1};
    VectorPoint points[] = {{20, 30}, {60, 30}, {60, 60}, {20, 60}};
    for (int i = 0; i < 4; i++) {
        closed.nodes[i] = (VectorBezierNode){points[i], points[i], points[i]};
    }
    VectorShape shape = d->rectangles[0];
    TEST_ASSERT_TRUE(vector_curve_shape(&shape, &closed));
    TEST_ASSERT_TRUE(vector_shape_update(d, &shape));
    commit();
    TEST_ASSERT_TRUE(pui_view_set_color(shell.view, "rectangle_fill", 0x44aa55ff));
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.rectangle.fill", "rectangle_fill"));
    commit();
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].gradient.kind);
    SDL_Surface *pixels = render_shapes();
    TEST_ASSERT_EQUAL_HEX32(0x44aa55ff, pixel(pixels, 40, 45));
    SDL_DestroySurface(pixels);
}

static void pointer_event(Uint32 type, float x, float y) {
    SDL_Event e = {0};
    e.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = x;
        e.motion.y = y;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = x;
        e.button.y = y;
    }
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &e));
}

static void test_handle_drag_is_live_but_only_commits_on_release_and_escape_discards(void) {
    set_gradient(ramp());
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.tool_gradient", ""));
    commit();
    float x = d->viewport.bounds.x + d->viewport.x + 20 * d->viewport.zoom;
    float y = d->viewport.bounds.y + d->viewport.y + 45 * d->viewport.zoom;
    int cursor = d->history.cursor;
    pointer_event(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer_event(SDL_EVENT_MOUSE_MOTION, x + 10 * d->viewport.zoom, y);
    TEST_ASSERT_TRUE(d->gradient_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].gradient.x1);
    TEST_ASSERT_FLOAT_WITHIN(.001f, .25f, vector_gradient_display(d, &d->rectangles[0])->x1);
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    pointer_event(SDL_EVENT_MOUSE_BUTTON_UP, x + 10 * d->viewport.zoom, y);
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_FLOAT_WITHIN(.001f, .25f, d->rectangles[0].gradient.x1);
    pointer_event(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 10 * d->viewport.zoom, y);
    pointer_event(SDL_EVENT_MOUSE_MOTION, x + 15 * d->viewport.zoom, y);
    SDL_Event esc = {.type = SDL_EVENT_KEY_DOWN};
    esc.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &esc));
    TEST_ASSERT_FALSE(d->gradient_tool.dragging);
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].gradient.x1);
}

static void test_files_clipboard_repeats_and_editor_reload_retain_gradients(void) {
    VectorGradient g = ramp();
    TEST_ASSERT_EQUAL_INT(1, vector_gradient_add_stop(&g));
    set_gradient(g);
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_MEMORY(&g, &d->rectangles[1].gradient, sizeof g);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    VectorRepeat repeat = vector_repeat_default();
    repeat.columns = 2;
    repeat.rows = 1;
    repeat.column.value[VECTOR_REPEAT_FILL_HUE] = 30;
    VectorShape instance;
    TEST_ASSERT_TRUE(vector_repeat_instance(&d->rectangles[0], &repeat, 1, 0, &instance));
    TEST_ASSERT_NOT_EQUAL(g.stops[0].color, instance.gradient.stops[0].color);
    char file[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(file, sizeof file, "%sgradient-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(vector_document_file_save(d, file, error, sizeof error));
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, file, error, sizeof error), error);
    TEST_ASSERT_EQUAL_MEMORY(&g, &loaded->rectangles[0].gradient, sizeof g);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(file);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&g, &d->rectangles[0].gradient, sizeof g);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].gradient.kind);
}

static void test_shadow_mask_and_preview_use_gradient_alpha(void) {
    VectorGradient g = ramp();
    g.stops[0].color = 0xff000080;
    g.stops[1].color = 0x0000ff80;
    set_gradient(g);
    VectorDocument *d = document();
    VectorShape shape = d->rectangles[0];
    shape.shadow = (VectorShadow){.enabled = 1, .color = 0x000000ff, .offset_x = 100};
    TEST_ASSERT_TRUE(vector_shape_update(d, &shape));
    SDL_Surface *pixels = render_shapes();
    TEST_ASSERT_INT_WITHIN(2, 127, pixel(pixels, 130, 40) >> 24);
    SDL_DestroySurface(pixels);
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 20, 30}));
    vector_document_select(d, 2);
    vector_document_toggle(d, 1);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    pixels = render_shapes();
    uint32_t visible = pixel(pixels, 25, 40);
    TEST_ASSERT_INT_WITHIN(3, 127, (visible >> 16) & 255);
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(pixels, 55, 40));
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(pixels, 130, 40));
    SDL_DestroySurface(pixels);
    d->preview.board = 1;
    d->preview.region = (PUIRect){0, 0, 200, 140};
    VectorEditor *editor = host.instance;
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 208, 168}, editor->text_renderer,
                        editor->image_renderer, NULL);
    pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_INT_WITHIN(3, 121, (pixel(pixels, 29, 64) >> 16) & 255);
    TEST_ASSERT_EQUAL_HEX32(0xf2f2f2ff, pixel(pixels, 59, 64));
    SDL_DestroySurface(pixels);
}

static void test_legacy_version_five_and_malformed_stop_count(void) {
    VectorDocument *d = document();
    VectorShape shape = d->rectangles[0];
    shape.shadow = vector_shadow_default();
    TEST_ASSERT_TRUE(vector_shape_update(d, &shape));
    char path[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(path, sizeof path, "%sgradient-legacy-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    size_t length = 0;
    unsigned char *bytes = SDL_LoadFile(path, &length);
    TEST_ASSERT_NOT_NULL(bytes);
    TEST_ASSERT_TRUE(length > 24);
    bytes[8] = 5;
    TEST_ASSERT_TRUE(SDL_SaveFile(path, bytes, length - 52));
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_UINT(0, loaded->rectangles[0].gradient.kind);
    TEST_ASSERT_EQUAL_FLOAT(12, loaded->rectangles[0].shadow.blur);
    vector_history_destroy(&loaded->history);
    vector_document_init(loaded, 1);
    bytes[8] = 6;
    bytes[length - 32] = VECTOR_GRADIENT_STOPS + 1;
    TEST_ASSERT_TRUE(SDL_SaveFile(path, bytes, length));
    SDL_free(bytes);
    TEST_ASSERT_FALSE(vector_document_file_load(loaded, path, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, loaded->rectangle_count);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);
}

static void test_middle_stop_drag_is_single_undoable_property_edit(void) {
    VectorGradient g = ramp();
    TEST_ASSERT_EQUAL_INT(1, vector_gradient_add_stop(&g));
    set_gradient(g);
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.tool_gradient", ""));
    commit();
    float x = d->viewport.bounds.x + d->viewport.x + 40 * d->viewport.zoom;
    float y = d->viewport.bounds.y + d->viewport.y + 45 * d->viewport.zoom;
    int cursor = d->history.cursor;
    pointer_event(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer_event(SDL_EVENT_MOUSE_MOTION, x + 4 * d->viewport.zoom, y);
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    TEST_ASSERT_FLOAT_WITHIN(.001f, .6f,
                             vector_gradient_display(d, &d->rectangles[0])->stops[1].position);
    pointer_event(SDL_EVENT_MOUSE_BUTTON_UP, x + 4 * d->viewport.zoom, y);
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_FLOAT_WITHIN(.001f, .6f, d->rectangles[0].gradient.stops[1].position);
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].gradient.x1);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(.5f, d->rectangles[0].gradient.stops[1].position);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_middle_stop_drag_is_single_undoable_property_edit);
    RUN_TEST(test_shadow_mask_and_preview_use_gradient_alpha);
    RUN_TEST(test_legacy_version_five_and_malformed_stop_count);
    RUN_TEST(test_pure_types_alpha_and_stop_validation);
    RUN_TEST(test_render_gradient_rectangle_ellipse_and_holed_closed_path);
    RUN_TEST(test_partial_clip_and_rotation_keep_correct_texture_coordinates);
    RUN_TEST(test_yaml_stop_edits_are_atomic_and_solid_fill_also_works_for_closed_curves);
    RUN_TEST(test_handle_drag_is_live_but_only_commits_on_release_and_escape_discards);
    RUN_TEST(test_files_clipboard_repeats_and_editor_reload_retain_gradients);
    return UNITY_END();
}

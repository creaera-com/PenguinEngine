#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/ellipse/render.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/orientation.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/workspace/box_tool.h"
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
    commit();
}

void tearDown(void) {
    SDL_SetModState(0);
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void tool(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void send(SDL_Event *e) {
    if (!pui_view_event(shell.view, &shell.input, window, e)) {
        editor_host_event(&host, shell.view, e);
    }
    commit();
}

static void pointer(Uint32 type, float x, float y) {
    VectorViewport *v = &document()->viewport;
    float sx = v->bounds.x + v->x + x * v->zoom, sy = v->bounds.y + v->y + y * v->zoom;
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = sx;
        e.motion.y = sy;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = sx;
        e.button.y = sy;
    }
    send(&e);
}

static void key(SDL_Keycode code, SDL_Keymod modifiers) {
    SDL_SetModState(modifiers);
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = code;
    e.key.mod = modifiers;
    send(&e);
}

static void test_factory_ellipse_extents_corners_style_and_undo(void) {
    VectorDocument *d = document();
    int history = d->history.count;
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){50, 60, 200, 100}));
    VectorShape *s = &d->rectangles[0];
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_ELLIPSE, s->curve_kind);
    TEST_ASSERT_EQUAL_UINT(4, s->curve.count);
    TEST_ASSERT_TRUE(s->curve.closed);
    TEST_ASSERT_TRUE(vector_shape_contains(s, 150, 110));
    TEST_ASSERT_FALSE(vector_shape_contains(s, 51, 61));
    TEST_ASSERT_EQUAL_UINT(VECTOR_RECTANGLE_DEFAULT_FILL, s->fill);
    TEST_ASSERT_EQUAL_STRING("Ellipse 1", s->name);
    VectorBezier curve = vector_curve_parent(s);
    PUIRect bounds = vector_bezier_bounds(&curve);
    TEST_ASSERT_EQUAL_FLOAT(50, bounds.x);
    TEST_ASSERT_EQUAL_FLOAT(60, bounds.y);
    TEST_ASSERT_EQUAL_FLOAT(200, bounds.w);
    TEST_ASSERT_EQUAL_FLOAT(100, bounds.h);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    VectorShape saved = *s;
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[0], sizeof saved);
    TEST_ASSERT_FALSE(vector_ellipse_add(d, 1, (PUIRect){0, 0, 0, 20}));
    TEST_ASSERT_FALSE(vector_ellipse_add(d, 1, (PUIRect){0, 0, NAN, 20}));
}

static void test_native_preview_stays_active_and_switches_exclusively(void) {
    VectorDocument *d = document();
    int history = d->history.count;
    tool("vector.add_ellipse");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_ellipse"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_rect"));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 50, 60);
    pointer(SDL_EVENT_MOUSE_MOTION, 250, 160);
    TEST_ASSERT_TRUE(d->rectangle_tool.dragging);
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 250, 160);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_ELLIPSE, d->rectangles[0].curve_kind);
    TEST_ASSERT_EQUAL_STRING("Ellipse", pui_view_text(shell.view, "rectangle_kind"));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 300, 60);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 380, 160);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangle_tool.enabled && d->rectangle_tool.ellipse);
    tool("vector.add_rect");
    TEST_ASSERT_FALSE(d->rectangle_tool.ellipse);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_rect"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_ellipse"));
    tool("vector.add_ellipse");
    tool("vector.add_ellipse");
    TEST_ASSERT_FALSE(d->rectangle_tool.enabled);
}

static void test_shift_alt_zoom_grid_and_cancel(void) {
    VectorDocument *d = document();
    d->viewport.zoom = 2;
    d->grid.snap_enabled = true;
    commit();
    tool("vector.add_ellipse");
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 160, 160);
    key(SDLK_LSHIFT, SDL_KMOD_SHIFT | SDL_KMOD_ALT);
    pointer(SDL_EVENT_MOUSE_MOTION, 191, 175);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 191, 175);
    PUIRect r = d->rectangles[0].rect;
    TEST_ASSERT_EQUAL_FLOAT(r.w, r.h);
    TEST_ASSERT_EQUAL_FLOAT(160, r.x + r.w / 2);
    TEST_ASSERT_EQUAL_FLOAT(160, r.y + r.h / 2);
    key(SDLK_LSHIFT, 0);
    int history = d->history.count;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, 60);
    pointer(SDL_EVENT_MOUSE_MOTION, 100, 100);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    TEST_ASSERT_FALSE(d->rectangle_tool.enabled);
    tool("vector.add_ellipse");
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, 60);
    SDL_Event event = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    send(&event);
    TEST_ASSERT_FALSE(d->rectangle_tool.enabled);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
}

static void test_checkpoint_duplicate_stroke_and_editable_nodes(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){50, 60, 200, 100}));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xff8000ff, 4, VECTOR_STROKE_INSIDE}));
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_ELLIPSE, d->rectangles[1].curve_kind);
    VectorShape saved = d->rectangles[1];
    activate("scripts");
    activate("vector");
    commit();
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[1], sizeof saved);
    VectorBezier edited = vector_curve_parent(&d->rectangles[1]);
    edited.nodes[0].point.y -= 10;
    TEST_ASSERT_TRUE(vector_curve_update(d, d->rectangles[1].id, &edited));
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_FREE, d->rectangles[1].curve_kind);
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_ELLIPSE, d->rectangles[0].curve_kind);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[1], sizeof saved);
}

static void test_ellipse_border_placement_rotation_and_clip(void) {
    VectorShape shape;
    TEST_ASSERT_TRUE(vector_ellipse_shape((PUIRect){100, 100, 200, 100}, &shape));
    SDL_Rect clip = {50, 50, 500, 400}, after;
    SDL_SetRenderClipRect(renderer, &clip);
    for (int placement = VECTOR_STROKE_INSIDE; placement <= VECTOR_STROKE_OUTSIDE; placement++) {
        SDL_SetRenderDrawColor(renderer, 242, 242, 242, 255);
        SDL_RenderClear(renderer);
        shape.stroke = (VectorStroke){0xff0000ff, 4, (VectorStrokeAlignment)placement};
        vector_ellipse_draw(renderer, &shape, shape.rect, 0, (SDL_FRect){0, 0, 600, 400}, 1,
                            shape.fill, false);
        SDL_GetRenderClipRect(renderer, &after);
        TEST_ASSERT_EQUAL_MEMORY(&clip, &after, sizeof clip);
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        Uint8 r, g, b, a;
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(
            surface, 200, placement == VECTOR_STROKE_INSIDE ? 102 : 98, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(255, r);
        TEST_ASSERT_EQUAL_UINT8(0, g);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 200, 150, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(115, r);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 100, 100, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(242, r);
        SDL_DestroySurface(surface);
    }
    SDL_SetRenderDrawColor(renderer, 242, 242, 242, 255);
    SDL_RenderClear(renderer);
    shape.stroke.width = 0;
    vector_ellipse_draw(renderer, &shape, shape.rect, 90, (SDL_FRect){0, 0, 600, 400}, 1,
                        shape.fill, false);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 200, 60, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(115, r);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 120, 150, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(242, r);
    SDL_DestroySurface(surface);
    SDL_SetRenderClipRect(renderer, NULL);
}

static void test_segments_preserve_geometry_history_inspector_and_files(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){40, 30, 200, 100}));
    TEST_ASSERT_TRUE(vector_ellipse_set_segment(d, 1, (VectorEllipse){VECTOR_ELLIPSE_PIE, 0, 90}));
    VectorShape *shape = &d->rectangles[0];
    TEST_ASSERT_TRUE(shape->curve.closed);
    TEST_ASSERT_EQUAL_UINT(3, shape->curve.count);
    TEST_ASSERT_TRUE(vector_shape_contains(shape, 155, 90));
    TEST_ASSERT_FALSE(vector_shape_contains(shape, 90, 50));
    TEST_ASSERT_EQUAL_FLOAT(200, shape->rect.w);
    int history = d->history.count;
    TEST_ASSERT_FALSE(
        vector_ellipse_set_segment(d, 1, (VectorEllipse){VECTOR_ELLIPSE_ARC, 10, 10}));
    TEST_ASSERT_FALSE(
        vector_ellipse_set_segment(d, 1, (VectorEllipse){VECTOR_ELLIPSE_PIE, NAN, 90}));
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    TEST_ASSERT_TRUE(
        vector_ellipse_set_segment(d, 1, (VectorEllipse){VECTOR_ELLIPSE_CHORD, 0, 90}));
    TEST_ASSERT_EQUAL_UINT(2, shape->curve.count);
    TEST_ASSERT_FALSE(vector_shape_contains(shape, 150, 85));
    TEST_ASSERT_TRUE(vector_shape_contains(shape, 210, 105));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(VECTOR_ELLIPSE_PIE, shape->ellipse.mode);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(VECTOR_ELLIPSE_CHORD, shape->ellipse.mode);
    commit();
    TEST_ASSERT_EQUAL_STRING("chord", pui_view_value(shell.view, "ellipse_mode"));
    TEST_ASSERT_EQUAL_STRING("90", pui_view_text(shell.view, "ellipse_end"));
    pui_view_set_text(shell.view, "ellipse_end", "180");
    tool("vector.ellipse.end");
    TEST_ASSERT_EQUAL_FLOAT(180, shape->ellipse.end);
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    TEST_ASSERT_EQUAL_FLOAT(-180, shape->ellipse.end - shape->ellipse.start);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    char path[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(path, sizeof path, "%sellipse-segment-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_save(d, path, error, sizeof error), error);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 2);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_UINT(VECTOR_ELLIPSE_CHORD, loaded->rectangles[0].ellipse.mode);
    TEST_ASSERT_EQUAL_FLOAT(180, loaded->rectangles[0].ellipse.end);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_FLOAT(180, d->rectangles[0].ellipse.end);
    TEST_ASSERT_TRUE(vector_ellipse_set_segment(d, 1, (VectorEllipse){VECTOR_ELLIPSE_ARC, 0, -90}));
    TEST_ASSERT_FALSE(d->rectangles[0].curve.closed);
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].path.count);
    TEST_ASSERT_TRUE(d->rectangles[0].stroke.width > 0);
    TEST_ASSERT_TRUE(vector_rectangle_valid(d->rectangles[0]));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_segments_preserve_geometry_history_inspector_and_files);
    RUN_TEST(test_ellipse_border_placement_rotation_and_clip);
    RUN_TEST(test_factory_ellipse_extents_corners_style_and_undo);
    RUN_TEST(test_native_preview_stays_active_and_switches_exclusively);
    RUN_TEST(test_shift_alt_zoom_grid_and_cancel);
    RUN_TEST(test_checkpoint_duplicate_stroke_and_editable_nodes);
    return UNITY_END();
}

#include "editor_host/host.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/alignment.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/order.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void action_callback(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
}

static void frame(void) {
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action_callback, NULL);
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    frame();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void event(SDL_Event *e) {
    if (!pui_view_event(shell.view, &shell.input, window, e)) {
        editor_host_event(&host, shell.view, e);
    }
    commit();
}

static void click(int button, float x, float y) {
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = button;
    e.button.x = x;
    e.button.y = y;
    event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event(&e);
}

static void key(SDL_Keycode key, SDL_Keymod mod) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = key;
    e.key.mod = mod;
    event(&e);
}

static void radial_click(float cx, float cy, int index, int count) {
    float angle = -1.570796327f + index * 6.283185307f / count;
    click(SDL_BUTTON_LEFT, cx + cosf(angle) * 104, cy + sinf(angle) * 104);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetModState(0);
    window = SDL_CreateWindow("objects", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){10, 20, 420, 340}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){30, 40, 70, 50}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){170, 110, 90, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 30));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xff8000ff, 8, VECTOR_STROKE_OUTSIDE}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_group_delete_undo_restores_order_ids_properties_and_selection(void) {
    VectorDocument *d = document();
    int history = d->history.count;
    VectorRectangle originals[2];
    memcpy(originals, d->rectangles, sizeof originals);
    action("vector.objects.delete");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(originals, d->rectangles, sizeof originals);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    activate("scripts");
    activate("vector");
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(originals, document()->rectangles, sizeof originals);
}

static void test_copy_across_tabs_owns_values_and_preserves_clipboard_after_editor_switch(void) {
    VectorDocument *source = document();
    action("vector.objects.copy");
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(source, 1, 0x000000ff));
    TEST_ASSERT_TRUE(vector_objects_delete(source));
    action("vector.add_document");
    VectorDocument *target = document();
    TEST_ASSERT_EQUAL_INT(2, target->id);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(1, target->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, target->rectangle_count);
    TEST_ASSERT_EQUAL_HEX32(VECTOR_RECTANGLE_DEFAULT_FILL, target->rectangles[0].fill);
    TEST_ASSERT_EQUAL_FLOAT(30, target->rectangles[0].angle);
    TEST_ASSERT_EQUAL_FLOAT(8, target->rectangles[0].stroke.width);
    TEST_ASSERT_EQUAL_INT(2, target->selection.count);
    TEST_ASSERT_EQUAL_INT(1, target->history.count);
    activate("scripts");
    activate("vector");
    target = document();
    TEST_ASSERT_EQUAL_INT(2, target->id);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(0, target->artboard_count);
    TEST_ASSERT_EQUAL_INT(0, target->rectangle_count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_INT(2, target->rectangle_count);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(4, target->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, target->artboard_count);
    TEST_ASSERT_NOT_EQUAL(target->rectangles[0].id, target->rectangles[2].id);
}

static void test_artboard_children_cut_and_duplicate_are_atomic_and_clipboard_is_unchanged(void) {
    VectorDocument *d = document();
    vector_document_select(d, 0);
    d->active_artboard = 1;
    commit();
    action("vector.objects.copy");
    VectorClipboard original = ((VectorEditor *)host.instance)->object_actions.clipboard;
    int count = d->history.count;
    action("vector.objects.duplicate");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[2].parent_id);
    TEST_ASSERT_EQUAL_FLOAT(26, d->artboards[1].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(30, d->rectangles[2].rect.x);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    action("vector.objects.cut");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(3, d->artboards[1].id); // Undo never recycles IDs.
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_INT(2, document()->artboard_count);
}

static void test_locked_child_and_capacity_failure_do_not_delete_or_overwrite_clipboard(void) {
    VectorDocument *d = document();
    action("vector.objects.copy");
    VectorClipboard original = ((VectorEditor *)host.instance)->object_actions.clipboard;
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    vector_document_select(d, 0);
    d->active_artboard = 1;
    int count = d->history.count;
    action("vector.objects.cut");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, 0));
    int cursor = d->history.cursor;
    d->history.cursor = VECTOR_HISTORY_CAPACITY;
    TEST_ASSERT_FALSE(
        vector_objects_cut(d, &((VectorEditor *)host.instance)->object_actions.clipboard));
    d->history.cursor = cursor;
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    VectorClipboard invalid = original;
    invalid.rectangles[0].parent_id = 999;
    TEST_ASSERT_FALSE(vector_objects_paste(d, &invalid));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
}

static void test_radial_context_click_escape_and_shortcuts(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    click(SDL_BUTTON_LEFT, 630, 296); // Top sector: copy.
    TEST_ASSERT_EQUAL_INT(
        2, ((VectorEditor *)host.instance)->object_actions.clipboard.rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    key(SDLK_D, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    key(SDLK_DELETE, 0);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    key(SDLK_V, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    key(SDLK_X, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    key(SDLK_V, SDL_KMOD_GUI);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
}

static void test_multi_parent_duplicate_keeps_parents_and_offsets(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 40, 30}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_INT(5, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, d->rectangles[3].parent_id);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[4].parent_id);
    TEST_ASSERT_EQUAL_FLOAT(46, d->rectangles[3].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(26, d->rectangles[4].rect.x);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
}

static void
test_tree_context_selects_row_without_collapsing_and_text_shortcuts_stay_in_input(void) {
    VectorDocument *d = document();
    click(SDL_BUTTON_RIGHT, 65, 130); // Existing selected member keeps the group.
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_RIGHT, 65, 102); // Artboard context selects the artboard, not its children.
    TEST_ASSERT_EQUAL_INT(0, d->selection.count);
    TEST_ASSERT_EQUAL_INT(1, d->active_artboard);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_RIGHT, 65, 158);
    TEST_ASSERT_EQUAL_INT(1, d->selection.count);
    TEST_ASSERT_EQUAL_INT(1, d->active_rectangle);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_LEFT, 1160, 138); // Rectangle name input.
    key(SDLK_A, SDL_KMOD_CTRL);
    key(SDLK_C, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(0, ((VectorEditor *)host.instance)->object_actions.clipboard.board_count);
    key(SDLK_BACKSPACE, 0);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_STRING("", pui_view_text(shell.view, "rectangle_name"));
    key(SDLK_V, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_STRING("Rechteck 1", pui_view_text(shell.view, "rectangle_name"));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
}

static void test_radial_disabled_paste_outside_click_and_focus_loss_are_safe(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    radial_click(630, 400, 2, 7); // Paste is unavailable in an empty clipboard.
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    click(SDL_BUTTON_LEFT, 280, 70); // Dismiss over add-document: no click-through.
    TEST_ASSERT_EQUAL_INT(1, ((VectorEditor *)host.instance)->documents.count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    event(&lost);
    click(SDL_BUTTON_LEFT, 630, 296);
    TEST_ASSERT_EQUAL_INT(0, ((VectorEditor *)host.instance)->object_actions.clipboard.board_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

static void assert_order(VectorDocument *d, const unsigned *ids, int count) {
    TEST_ASSERT_EQUAL_INT(count, d->rectangle_count);
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL_UINT(ids[i], d->rectangles[i].id);
    }
}

static void test_order_stable_groups_stay_in_parent_slots_and_use_one_history_entry(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 200}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 10, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 10, 20, 20}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 4);
    int count = d->history.count;
    TEST_ASSERT_TRUE(vector_objects_order(d, VECTOR_ORDER_FORWARD));
    assert_order(d, (unsigned[]){2, 1, 3, 5, 4}, 5);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[2].parent_id);
    TEST_ASSERT_TRUE(vector_objects_order(d, VECTOR_ORDER_BACKWARD));
    assert_order(d, (unsigned[]){1, 2, 3, 4, 5}, 5);
    TEST_ASSERT_TRUE(vector_objects_order(d, VECTOR_ORDER_FRONT));
    assert_order(d, (unsigned[]){2, 5, 3, 1, 4}, 5);
    TEST_ASSERT_FALSE(vector_objects_can_order(d, VECTOR_ORDER_FORWARD));
    TEST_ASSERT_FALSE(vector_objects_order(d, VECTOR_ORDER_FRONT));
    TEST_ASSERT_EQUAL_INT(count + 3, d->history.count);
    TEST_ASSERT_TRUE(vector_objects_order(d, VECTOR_ORDER_BACK));
    assert_order(d, (unsigned[]){1, 4, 3, 2, 5}, 5);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    assert_order(d, (unsigned[]){2, 5, 3, 1, 4}, 5);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    assert_order(d, (unsigned[]){1, 4, 3, 2, 5}, 5);
}

static void test_order_boundaries_locks_and_multiple_parents(void) {
    VectorDocument *d = document();
    TEST_ASSERT_FALSE(vector_objects_can_order(d, VECTOR_ORDER_FRONT)); // All siblings selected.
    vector_document_select(d, 1);
    TEST_ASSERT_FALSE(vector_objects_can_order(d, VECTOR_ORDER_BACK));
    TEST_ASSERT_TRUE(vector_objects_can_order(d, VECTOR_ORDER_FRONT));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    int count = d->history.count;
    TEST_ASSERT_FALSE(vector_objects_order(d, VECTOR_ORDER_FRONT));
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 1, 0));
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 200}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){50, 10, 20, 20}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_objects_order(d, VECTOR_ORDER_FRONT));
    assert_order(d, (unsigned[]){2, 1, 4, 3}, 4);
    TEST_ASSERT_EQUAL_INT(1, d->rectangles[1].parent_id);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[3].parent_id);
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    count = d->history.count;
    TEST_ASSERT_FALSE(vector_objects_order(d, VECTOR_ORDER_BACK)); // Atomic failure across parents.
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

static void test_root_order_and_checkpoint_redo_preserve_children_and_selection(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 200}));
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){800, 20, 200, 200}));
    VectorRectangle children[2];
    memcpy(children, d->rectangles, sizeof children);
    vector_document_select(d, 0);
    d->active_artboard = 1;
    commit();
    action("vector.objects.order_front");
    TEST_ASSERT_EQUAL_UINT(1, d->artboards[2].id);
    TEST_ASSERT_EQUAL_MEMORY(children, d->rectangles, sizeof children);
    TEST_ASSERT_EQUAL_UINT(1, d->active_artboard);
    action("editor.undo");
    TEST_ASSERT_EQUAL_UINT(1, d->artboards[0].id);
    activate("scripts");
    activate("vector");
    d = document();
    action("editor.redo");
    TEST_ASSERT_EQUAL_UINT(1, d->artboards[2].id);
    TEST_ASSERT_EQUAL_MEMORY(children, d->rectangles, sizeof children);
    action("editor.undo");
    TEST_ASSERT_EQUAL_UINT(1, d->artboards[0].id);
}

static Uint8 scene_red(VectorDocument *d, float x, float y) {
    VectorViewport *v = &d->viewport;
    SDL_Rect pixel = {(int)(v->bounds.x + v->x + x * v->zoom),
                      (int)(v->bounds.y + v->y + y * v->zoom), 1, 1};
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, &pixel);
    Uint8 r, g, b, a;
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 0, 0, &r, &g, &b, &a));
    SDL_DestroySurface(surface);
    return r;
}

static void test_layer_submenu_back_navigation_tree_rendering_and_hit_order_agree(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 0));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Red", (PUIRect){20, 20, 140, 120}));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 2, "Blue", (PUIRect){20, 20, 140, 120}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 2, 0x0000ffff));
    vector_document_select(d, 1);
    commit();
    TEST_ASSERT_EQUAL_UINT8(0, scene_red(d, 65, 75));
    TEST_ASSERT_EQUAL_STRING("vector_document_1.rectangle_2",
                             pui_view_tree_item_at(shell.view, "vector_document_tree", 65, 130));
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    radial_click(630, 400, 5, 7); // Root submenu: Ebenen.
    key(SDLK_ESCAPE, 0);          // Back to root, not closed.
    radial_click(630, 400, 5, 7);
    click(SDL_BUTTON_LEFT, 630, 400); // Center: back.
    radial_click(630, 400, 5, 7);
    TEST_ASSERT_EQUAL_INT(count, d->history.count); // Navigation is never a model command.
    click(SDL_BUTTON_LEFT, 630, 296);               // Four-sector submenu: Ganz nach oben.
    commit();
    assert_order(d, (unsigned[]){2, 1}, 2);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT8(255, scene_red(d, 65, 75));
    TEST_ASSERT_EQUAL_STRING("vector_document_1.rectangle_1",
                             pui_view_tree_item_at(shell.view, "vector_document_tree", 65, 130));
    vector_document_select(d, 0);
    d->active_artboard = 0;
    commit();
    VectorViewport *v = &d->viewport;
    click(SDL_BUTTON_LEFT, v->bounds.x + v->x + 65 * v->zoom, v->bounds.y + v->y + 75 * v->zoom);
    TEST_ASSERT_EQUAL_UINT(1, d->active_rectangle);
    action("editor.undo");
    TEST_ASSERT_EQUAL_UINT8(0, scene_red(d, 65, 75));
    activate("scripts");
    activate("vector");
    action("editor.redo");
    assert_order(document(), (unsigned[]){2, 1}, 2);
}

static PUIRect world_bounds(VectorDocument *d, unsigned id) {
    VectorRectangle *r = vector_rectangle_find(d, id);
    VectorArtboard *p = vector_artboard_find(d, r->parent_id);
    PUIRect bounds = vector_rotated_bounds(r->rect, r->angle);
    bounds.x += p->rect.x;
    bounds.y += p->rect.y;
    return bounds;
}

static void test_six_alignments_preserve_geometry_selection_and_history(void) {
    VectorDocument *d = document();
    const VectorAlignment modes[] = {VECTOR_ALIGN_LEFT, VECTOR_ALIGN_CENTER_X, VECTOR_ALIGN_RIGHT,
                                     VECTOR_ALIGN_TOP,  VECTOR_ALIGN_CENTER_Y, VECTOR_ALIGN_BOTTOM};
    PUIRect original[2] = {d->rectangles[0].rect, d->rectangles[1].rect};
    for (int i = 0; i < 6; i++) {
        int count = d->history.cursor;
        TEST_ASSERT_TRUE(vector_objects_can_align(d, modes[i]));
        TEST_ASSERT_TRUE(vector_objects_align(d, modes[i]));
        PUIRect a = world_bounds(d, 1), b = world_bounds(d, 2);
        switch (modes[i]) {
        case VECTOR_ALIGN_LEFT:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x, b.x);
            break;
        case VECTOR_ALIGN_CENTER_X:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x + a.w / 2, b.x + b.w / 2);
            break;
        case VECTOR_ALIGN_RIGHT:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x + a.w, b.x + b.w);
            break;
        case VECTOR_ALIGN_TOP:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.y, b.y);
            break;
        case VECTOR_ALIGN_CENTER_Y:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.y + a.h / 2, b.y + b.h / 2);
            break;
        case VECTOR_ALIGN_BOTTOM:
            TEST_ASSERT_FLOAT_WITHIN(.0001f, a.y + a.h, b.y + b.h);
            break;
        default:
            TEST_FAIL_MESSAGE("Unexpected alignment");
            break;
        }
        TEST_ASSERT_EQUAL_FLOAT(30, d->rectangles[0].angle);
        TEST_ASSERT_EQUAL_FLOAT(original[0].w, d->rectangles[0].rect.w);
        TEST_ASSERT_EQUAL_FLOAT(original[1].h, d->rectangles[1].rect.h);
        TEST_ASSERT_EQUAL_FLOAT(8, d->rectangles[0].stroke.width);
        TEST_ASSERT_EQUAL_INT(2, d->selection.count);
        TEST_ASSERT_EQUAL_INT(count + 1, d->history.cursor);
        TEST_ASSERT_FALSE(vector_objects_can_align(d, modes[i]));
        TEST_ASSERT_FALSE(vector_objects_align(d, modes[i]));
        TEST_ASSERT_EQUAL_INT(count + 1, d->history.cursor);
        TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
        TEST_ASSERT_EQUAL_MEMORY(&original[0], &d->rectangles[0].rect, sizeof original[0]);
        TEST_ASSERT_EQUAL_MEMORY(&original[1], &d->rectangles[1].rect, sizeof original[1]);
    }
}

static void test_distribution_equal_edge_gaps_handles_varied_sizes_and_spatial_order(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 0));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "First", (PUIRect){20, 20, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 2, "Last", (PUIRect){220, 220, 40, 40}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){70, 70, 60, 60}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    vector_document_toggle(d, 3);
    int count = d->history.count;
    TEST_ASSERT_TRUE(vector_objects_align(d, VECTOR_DISTRIBUTE_X));
    PUIRect a = world_bounds(d, 1), b = world_bounds(d, 3), c = world_bounds(d, 2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, b.x - a.x - a.w, c.x - b.x - b.w);
    TEST_ASSERT_EQUAL_FLOAT(20, d->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(220, d->rectangles[1].rect.x);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_FALSE(vector_objects_can_align(d, VECTOR_DISTRIBUTE_X));
    TEST_ASSERT_TRUE(vector_objects_align(d, VECTOR_DISTRIBUTE_Y));
    a = world_bounds(d, 1);
    b = world_bounds(d, 3);
    c = world_bounds(d, 2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, b.y - a.y - a.h, c.y - b.y - b.h);
    TEST_ASSERT_EQUAL_FLOAT(20, d->rectangles[0].rect.y);
    TEST_ASSERT_EQUAL_FLOAT(220, d->rectangles[1].rect.y);
    TEST_ASSERT_EQUAL_INT(count + 2, d->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(70, d->rectangles[2].rect.y);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(70, d->rectangles[2].rect.x);
}

static void test_alignment_world_coordinates_locks_and_minimum_selection(void) {
    VectorDocument *d = document();
    TEST_ASSERT_FALSE(vector_objects_can_align(d, VECTOR_DISTRIBUTE_X));
    vector_document_select(d, 1);
    TEST_ASSERT_FALSE(vector_objects_can_align(d, VECTOR_ALIGN_LEFT));
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 200}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 20, 20}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_objects_align(d, VECTOR_ALIGN_RIGHT));
    PUIRect a = world_bounds(d, 1), b = world_bounds(d, 3);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x + a.w, b.x + b.w);
    TEST_ASSERT_EQUAL_INT(1, d->rectangles[0].parent_id);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[2].parent_id);
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    int count = d->history.count;
    PUIRect original = d->rectangles[0].rect;
    TEST_ASSERT_FALSE(vector_objects_align(d, VECTOR_ALIGN_LEFT));
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0].rect, sizeof original);
}

static void test_alignment_radial_action_and_checkpoint_redo(void) {
    VectorDocument *d = document();
    PUIRect original[2] = {d->rectangles[0].rect, d->rectangles[1].rect};
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    radial_click(630, 400, 6, 7); // Ausrichten.
    radial_click(630, 400, 0, 8); // Links.
    commit();
    PUIRect a = world_bounds(d, 1), b = world_bounds(d, 2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x, b.x);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(&original[1], &d->rectangles[1].rect, sizeof original[1]);
    activate("scripts");
    activate("vector");
    d = document();
    action("editor.redo");
    a = world_bounds(d, 1);
    b = world_bounds(d, 2);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x, b.x);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_six_alignments_preserve_geometry_selection_and_history);
    RUN_TEST(test_distribution_equal_edge_gaps_handles_varied_sizes_and_spatial_order);
    RUN_TEST(test_alignment_world_coordinates_locks_and_minimum_selection);
    RUN_TEST(test_alignment_radial_action_and_checkpoint_redo);
    RUN_TEST(test_order_stable_groups_stay_in_parent_slots_and_use_one_history_entry);
    RUN_TEST(test_order_boundaries_locks_and_multiple_parents);
    RUN_TEST(test_root_order_and_checkpoint_redo_preserve_children_and_selection);
    RUN_TEST(test_layer_submenu_back_navigation_tree_rendering_and_hit_order_agree);
    RUN_TEST(test_tree_context_selects_row_without_collapsing_and_text_shortcuts_stay_in_input);
    RUN_TEST(test_radial_disabled_paste_outside_click_and_focus_loss_are_safe);
    RUN_TEST(test_group_delete_undo_restores_order_ids_properties_and_selection);
    RUN_TEST(test_copy_across_tabs_owns_values_and_preserves_clipboard_after_editor_switch);
    RUN_TEST(test_artboard_children_cut_and_duplicate_are_atomic_and_clipboard_is_unchanged);
    RUN_TEST(test_locked_child_and_capacity_failure_do_not_delete_or_overwrite_clipboard);
    RUN_TEST(test_radial_context_click_escape_and_shortcuts);
    RUN_TEST(test_multi_parent_duplicate_keeps_parents_and_offsets);
    return UNITY_END();
}

#include "app/editor_internal.h"

static void pointer(Uint32 type, float x, float y) {
    SDL_Event event = {0};
    event.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    editor_event(&event);
    editor_draw();
}

static void gesture(float x, float y, float endx, float endy) {
    VectorEditor *v = app.vector;
    float sx = v->canvas.x + (x - v->view.x) * v->view.zoom,
          sy = v->canvas.y + (y - v->view.y) * v->view.zoom;
    float ex = v->canvas.x + (endx - v->view.x) * v->view.zoom,
          ey = v->canvas.y + (endy - v->view.y) * v->view.zoom;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    pointer(SDL_EVENT_MOUSE_MOTION, ex, ey);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, ex, ey);
}

static bool interaction_smoke(void) {
    VectorEditor *v = app.vector;
    vector_new();
    editor_draw();
    vector_action("vector.rectangle");
    gesture(32, 40, 132, 100);
    bool ok = v->document.shapes[0].used && v->document.shapes[0].x == 32 &&
              v->document.shapes[0].w == 100 && v->history.position == 1;
    vector_action("vector.select");
    gesture(80, 70, 96, 82);
    ok = ok && v->document.shapes[0].x == 48 && v->document.shapes[0].y == 52 &&
         v->history.position == 2;
    SDL_Event key = {0};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.key = SDLK_Z;
    key.key.mod = SDL_KMOD_CTRL;
    editor_event(&key);
    ok = ok && v->document.shapes[0].x == 32;
    key.key.key = SDLK_Y;
    editor_event(&key);
    ok = ok && v->document.shapes[0].x == 48;
    editor_draw();
    float fx = app.width - RIGHT + PANEL_GAP + 9 + 170, fy = TOP + TABS + 12 + 26 + 35 + 12;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, fx, fy);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, fx, fy);
    ok = ok && app.field == FIELD_V_X;
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "96";
    editor_event(&text);
    key.key.key = SDLK_RETURN;
    key.key.mod = 0;
    editor_event(&key);
    ok = ok && v->document.shapes[0].x == 96 && app.field == FIELD_NONE;
    vector_undo(false);
    editor_draw();
    vector_action("vector.convert");
    vector_action("vector.nodes");
    editor_draw();
    gesture(48, 52, 56, 56);
    ok = ok && v->document.shapes[0].nodes[0].p.x == 56 && v->document.shapes[0].nodes[0].p.y == 56;
    v->view.node = 1;
    vector_action("vector.split");
    ok = ok && v->document.shapes[0].count == 6;
    SDL_Keymod mod = SDL_GetModState();
    SDL_SetModState(SDL_KMOD_ALT);
    gesture(300, 240, 320, 250);
    SDL_SetModState(mod);
    ok = ok && v->document.boards[0].x == 20 && v->document.boards[0].y == 10 &&
         v->document.shapes[0].nodes[0].p.x == 56;
    vector_action("vector.artboard");
    gesture(590, 20, 630, 140);
    ok = ok && v->document.boards[1].used && v->document.boards[1].w == 40;
    vector_action("vector.select");
    v->view.selected = 1;
    v->view.board = 0;
    app.selected = v->document.shapes[0].id;
    editor_draw();
    gesture(100, 90, 605, 80);
    ok = ok && v->document.shapes[0].board == 1 &&
         v->document.shapes[0].nodes[0].p.x + v->document.boards[1].x == 581;
    char path[sizeof app.input];
    snprintf(path, sizeof path, "%svector-smoke-%llx.svg", SDL_GetBasePath(),
             (unsigned long long)SDL_GetPerformanceCounter());
    editor_field_start(FIELD_V_SAVE, path);
    editor_field_apply();
    ok = ok && app.field == FIELD_NONE && !ve_dirty(&v->history) && vector_open(path) &&
         v->document.boards[1].used && v->document.shapes[0].count == 6;
    remove(path);
    ve_move(&v->document.shapes[0], 1, 0);
    ve_history_commit(&v->history, &v->document);
    editor_request(REQUEST_VECTOR_NEW, NULL);
    ok = ok && app.request == REQUEST_VECTOR_NEW;
    editor_request_finish();
    ok = ok && !v->document.shapes[0].used;
    ve_add_shape(&v->document, 0, VE_RECT, 10, 10, 40, 40);
    ve_history_commit(&v->history, &v->document);
    editor_request(REQUEST_VECTOR_NEW, NULL);
    app.deferred_save = true;
    editor_process_deferred_ui();
    ok = ok && app.field == FIELD_V_SAVE && app.request == REQUEST_NONE &&
         v->after_save == REQUEST_VECTOR_NEW;
    snprintf(app.input, sizeof app.input, "%s", path);
    editor_field_apply();
    ok = ok && app.field == FIELD_NONE && !v->document.shapes[0].used;
    remove(path);
    if (!ok) {
        fprintf(stderr, "Vector drawing / inline / node / artboard / SVG smoke failed\n");
    }
    return ok;
}

static WidgetState *tool_state(const char *name) {
    for (int i = 0; i < app.widget_count; i++) {
        if (app.widgets[i].id == ui_id(name)) {
            return &app.widgets[i];
        }
    }
    return NULL;
}

static bool tools_smoke(void) {
    app.right_tabs.selected = ui_id("vector_tools_tab");
    editor_draw();
    WidgetState *group = tool_state("vector_transform_tools");
    WidgetState *panel = tool_state("vector_tool_panel");
    if (!group || !panel || !group->open) {
        return false;
    }
    float x = app.width - RIGHT + PANEL_GAP + 40, y = TOP + TABS + 18;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, app.width - 164, y + 39);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, app.width - 164, y + 39);
    bool ok =
        app.vector->document.shapes[2].nodes[0].p.x == 202 && app.vector->history.position == 1;
    vector_undo(false);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
    editor_draw();
    ok = ok && !group->open && app.vector->history.position == 0;
    int height = app.height;
    SDL_SetWindowMinimumSize(app.window, 1100, 300);
    SDL_SetWindowSize(app.window, app.width, 360);
    editor_draw();
    pointer(SDL_EVENT_MOUSE_MOTION, x, 220);
    float inspector_offset = app.inspector_view.offset;
    SDL_Event wheel = {0};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.y = -3;
    editor_event(&wheel);
    editor_draw();
    ok = ok && panel->scroll > 0 && app.inspector_view.offset == inspector_offset;
    float offset = panel->scroll;
    ok = editor_reload_layout() && ok;
    editor_draw();
    panel = tool_state("vector_tool_panel");
    group = tool_state("vector_transform_tools");
    ok = ok && panel && group && panel->scroll == offset && !group->open;
    SDL_SetWindowSize(app.window, app.width, height);
    SDL_SetWindowMinimumSize(app.window, 1100, 740);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Vector Tools tab / actions / collapse / scrolling smoke failed\n");
    }
    return ok;
}

static bool tree_smoke(void) {
    VectorEditor *v = app.vector;
    app.left_tabs.selected = UI_SCENE_TAB;
    app.scene_view.offset = 0;
    editor_draw();
    float x = PANEL_GAP + 10, y = TOP + TABS - PANEL_GAP + 10;
    float content = app.scene_view.content;
    int revision = v->history.position;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 30, y + 3 * 26 + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 30, y + 3 * 26 + 12);
    editor_draw();
    bool ok = !v->view.expanded_paths[0] && app.scene_view.content == content - 5 * 26;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 30, y + 3 * 26 + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 30, y + 3 * 26 + 12);
    editor_draw();
    ok = ok && v->view.expanded_paths[0] == v->document.shapes[2].id;
    v->view.node = -1;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 80, y + 5 * 26 + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 80, y + 5 * 26 + 12);
    editor_draw();
    ok = ok && v->view.node == 1 && v->view.tool == VT_NODE && v->view.selected == 4 &&
         v->history.position == revision;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 80, y + 8 * 26 + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 80, y + 8 * 26 + 12);
    ok = ok && v->view.node == 4 && v->history.position == revision;
    /* Expansion belongs to stable IDs even when stacking order changes slots. */
    uint32_t expanded = v->view.expanded_paths[0];
    vector_action("vector.raise");
    ok = ok && v->view.expanded_paths[0] == expanded && v->document.shapes[3].id == expanded;
    vector_undo(false);
    editor_draw();
    /* Closing the artboard hides all its descendants, not the next artboard. */
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 10, y + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 10, y + 12);
    editor_draw();
    ok = ok && !v->view.expanded[0] && app.scene_view.content == 3 * 26;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 10, y + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 10, y + 12);
    editor_draw();
    ok = ok && v->view.expanded[0] && app.scene_view.content == content;
    if (!ok) {
        fprintf(stderr, "Vector tree collapse / segment selection smoke failed\n");
    }
    return ok;
}

bool vector_smoke(void) {
    editor_select_workspace(UI_VECTOR_VIEW);
    if (!app.vector || app.map || app.script) {
        return false;
    }
    bool interaction = interaction_smoke();
    vector_demo();
    interaction = tools_smoke() && interaction;
    interaction = tree_smoke() && interaction;
    VectorEditor *v = app.vector;
    int shape = 2;
    char original[64];
    snprintf(original, sizeof original, "%s", v->document.shapes[shape].name);
    snprintf(v->document.shapes[shape].name, 64, "Edited vector");
    if (!ve_history_commit(&v->history, &v->document)) {
        return false;
    }
    vector_undo(false);
    bool ok = interaction && !strcmp(v->document.shapes[shape].name, original);
    vector_undo(true);
    ok = ok && !strcmp(v->document.shapes[shape].name, "Edited vector");
    ok = ok && editor_reload_layout();
    editor_select_workspace(UI_MAP_VIEW);
    ok = ok && !app.vector && app.map && !app.script;
    editor_select_workspace(UI_VECTOR_VIEW);
    ok = ok && app.vector && !app.map && !app.script &&
         !strcmp(app.vector->document.shapes[shape].name, "Edited vector") &&
         app.vector->history.position == 1 &&
         app.vector->view.expanded_paths[0] == app.vector->document.shapes[shape].id &&
         app.right_tabs.selected == ui_id("vector_tools_tab") &&
         tool_state("vector_transform_tools") && !tool_state("vector_transform_tools")->open;
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && !app.vector && app.script && !app.map;
    editor_select_workspace(UI_MAP_VIEW);
    if (!ok) {
        fprintf(stderr, "Vector lifecycle smoke failed\n");
    }
    return ok;
}

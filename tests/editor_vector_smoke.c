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

bool vector_smoke(void) {
    editor_select_workspace(UI_VECTOR_VIEW);
    if (!app.vector || app.map || app.script) {
        return false;
    }
    bool interaction = interaction_smoke();
    vector_demo();
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
         app.vector->history.position == 1;
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && !app.vector && app.script && !app.map;
    editor_select_workspace(UI_MAP_VIEW);
    if (!ok) {
        fprintf(stderr, "Vector lifecycle smoke failed\n");
    }
    return ok;
}

#include "application/internal.h"
#include "editors/graphics/vector/internal.h"

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
    VectorEditor *v = app.workspace.vector;
    float sx = v->canvas.x + (x - v->view.x) * v->view.zoom,
          sy = v->canvas.y + (y - v->view.y) * v->view.zoom;
    float ex = v->canvas.x + (endx - v->view.x) * v->view.zoom,
          ey = v->canvas.y + (endy - v->view.y) * v->view.zoom;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    pointer(SDL_EVENT_MOUSE_MOTION, ex, ey);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, ex, ey);
}

static bool interaction_smoke(void) {
    VectorEditor *v = app.workspace.vector;
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
    float fx = app.platform.width - RIGHT + PANEL_GAP + 9 + 170,
          fy = TOP + TABS + 12 + 26 + 35 + 12;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, fx, fy);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, fx, fy);
    ok = ok && app.shell.field == FIELD_V_X;
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "96";
    editor_event(&text);
    key.key.key = SDLK_RETURN;
    key.key.mod = 0;
    editor_event(&key);
    ok = ok && v->document.shapes[0].x == 96 && app.shell.field == FIELD_NONE;
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
    app.shell.selected = v->document.shapes[0].id;
    editor_draw();
    gesture(100, 90, 605, 80);
    ok = ok && v->document.shapes[0].board == 1 &&
         v->document.shapes[0].nodes[0].p.x + v->document.boards[1].x == 581;
    char path[sizeof app.shell.input];
    snprintf(path, sizeof path, "%svector-smoke-%llx.svg", SDL_GetBasePath(),
             (unsigned long long)SDL_GetPerformanceCounter());
    editor_field_start(FIELD_V_SAVE, path);
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NONE && !ve_dirty(&v->history) && vector_open(path) &&
         v->document.boards[1].used && v->document.shapes[0].count == 6;
    remove(path);
    ve_move(&v->document.shapes[0], 1, 0);
    ve_history_commit(&v->history, &v->document);
    editor_request(REQUEST_VECTOR_NEW, NULL);
    ok = ok && app.shell.request == REQUEST_VECTOR_NEW;
    editor_request_finish();
    ok = ok && !v->document.shapes[0].used;
    ve_add_shape(&v->document, 0, VE_RECT, 10, 10, 40, 40);
    ve_history_commit(&v->history, &v->document);
    editor_request(REQUEST_VECTOR_NEW, NULL);
    app.shell.deferred_save = true;
    editor_process_deferred_ui();
    ok = ok && app.shell.field == FIELD_V_SAVE && app.shell.request == REQUEST_NONE &&
         v->after_save == REQUEST_VECTOR_NEW;
    snprintf(app.shell.input, sizeof app.shell.input, "%s", path);
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NONE && !v->document.shapes[0].used;
    remove(path);
    if (!ok) {
        fprintf(stderr, "Vector drawing / inline / node / artboard / SVG smoke failed\n");
    }
    return ok;
}

static WidgetState *tool_state(const char *name) {
    for (int i = 0; i < app.shell.widget_count; i++) {
        if (app.shell.widgets[i].id == ui_id(name)) {
            return &app.shell.widgets[i];
        }
    }
    return NULL;
}

static bool zoom_smoke(void) {
    VectorEditor *v = app.workspace.vector;
    VeView before = v->view;
    int revision = v->history.position, count = v->history.count;
    editor_draw();
    float x = v->canvas.x + v->canvas.w * .3f, y = v->canvas.y + v->canvas.h * .4f;
    float world_x = v->view.x + (x - v->canvas.x) / v->view.zoom;
    float world_y = v->view.y + (y - v->canvas.y) / v->view.zoom;
    pointer(SDL_EVENT_MOUSE_MOTION, x, y);
    SDL_Event wheel = {0};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.y = 3;
    editor_event(&wheel);
    bool ok = v->view.zoom > before.zoom &&
              fabsf(world_x - v->view.x - (x - v->canvas.x) / v->view.zoom) < .001f &&
              fabsf(world_y - v->view.y - (y - v->canvas.y) / v->view.zoom) < .001f;
    editor_draw();
    float center_x = v->view.x + v->canvas.w * .5f / v->view.zoom;
    float center_y = v->view.y + v->canvas.h * .5f / v->view.zoom;
    float zoom = v->view.zoom;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, v->canvas.x + 438, TOP + 21);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, v->canvas.x + 438, TOP + 21);
    ok = ok && v->view.zoom > zoom &&
         fabsf(center_x - v->view.x - v->canvas.w * .5f / v->view.zoom) < .001f &&
         fabsf(center_y - v->view.y - v->canvas.h * .5f / v->view.zoom) < .001f;
    vector_action("vector.zoom_out");
    ok = ok && fabsf(v->view.zoom - zoom) < .001f;
    vector_action("vector.zoom_reset");
    ok = ok && v->view.zoom == 1;
    vector_zoom(10000, x, y);
    ok = ok && v->view.zoom == 32;
    vector_zoom(0, x, y);
    ok = ok && v->view.zoom == .02f;
    for (int b = 0; b < 2; b++) {
        v->view.board = b;
        vector_action("vector.fit");
        VeBoard *board = &v->document.boards[b];
        ok = ok && (board->x - v->view.x) * v->view.zoom >= 30 &&
             (board->y - v->view.y) * v->view.zoom >= 30 && board->w * v->view.zoom < v->canvas.w &&
             board->h * v->view.zoom < v->canvas.h;
    }
    ok = ok && v->history.position == revision && v->history.count == count;
    v->view = before;
    v->render_dirty = true;
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Vector wheel / toolbar zoom / bounds / Fit smoke failed\n");
    }
    return ok;
}

static bool tools_smoke(void) {
    app.shell.right_tabs.selected = ui_id("vector_tools_tab");
    editor_draw();
    WidgetState *grid_group = tool_state("vector_grid_tools");
    if (grid_group) {
        grid_group->open = 0;
        editor_draw();
    }
    WidgetState *preview_group = tool_state("vector_preview_tools");
    if (preview_group) {
        preview_group->open = 0;
        editor_draw();
    }
    WidgetState *group = tool_state("vector_transform_tools");
    WidgetState *panel = tool_state("vector_tool_panel");
    if (!group || !panel || !group->open) {
        return false;
    }
    float x = app.platform.width - RIGHT + PANEL_GAP + 40, y = TOP + TABS + 18 + 82;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, app.platform.width - 164, y + 39);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, app.platform.width - 164, y + 39);
    bool ok = app.workspace.vector->document.shapes[2].nodes[0].p.x == 202 &&
              app.workspace.vector->history.position == 1;
    vector_undo(false);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
    editor_draw();
    ok = ok && !group->open && app.workspace.vector->history.position == 0;
    int height = app.platform.height;
    SDL_SetWindowMinimumSize(app.platform.window, 1100, 300);
    SDL_SetWindowSize(app.platform.window, app.platform.width, 360);
    editor_draw();
    pointer(SDL_EVENT_MOUSE_MOTION, x, 220);
    float inspector_offset = app.shell.inspector_view.offset;
    SDL_Event wheel = {0};
    wheel.type = SDL_EVENT_MOUSE_WHEEL;
    wheel.wheel.y = -3;
    editor_event(&wheel);
    editor_draw();
    ok = ok && panel->scroll > 0 && app.shell.inspector_view.offset == inspector_offset;
    float offset = panel->scroll;
    ok = editor_reload_layout() && ok;
    editor_draw();
    panel = tool_state("vector_tool_panel");
    group = tool_state("vector_transform_tools");
    ok = ok && panel && group && panel->scroll == offset && !group->open;
    SDL_SetWindowSize(app.platform.window, app.platform.width, height);
    SDL_SetWindowMinimumSize(app.platform.window, 1100, 740);
    editor_draw();
    if (grid_group) {
        grid_group->open = 1;
    }
    if (preview_group) {
        preview_group->open = 1;
        editor_draw();
    }
    if (!ok) {
        fprintf(stderr, "Vector Tools tab / actions / collapse / scrolling smoke failed\n");
    }
    return ok;
}

static bool snap_smoke(void) {
    VectorEditor *v = app.workspace.vector;
    vector_new();
    editor_draw();
    vector_action("vector.rectangle");
    gesture(32, 40, 132, 100);
    vector_action("vector.select");
    bool ok = !v->view.snap_enabled && v->view.snap_tolerance == 5;
    vector_action("vector.toggle_snap");
    gesture(80, 70, 95, 91); /* Desired origin 47,61 -> 48,64. */
    ok = ok && v->document.shapes[0].x == 48 && v->document.shapes[0].y == 64;
    vector_undo(false);
    ok = ok && v->document.shapes[0].x == 32 && v->document.shapes[0].y == 40;
    vector_undo(true);
    /* Leaving the capture radius must immediately release the snap. */
    gesture(90, 80, 96, 86);
    ok = ok && fabsf(v->document.shapes[0].x - 54) < .001f &&
         fabsf(v->document.shapes[0].y - 70) < .001f;
    /* Multiple motion events use the original pointer offset, avoiding sticky snapping. */
    float sx = v->canvas.x + (90 - v->view.x) * v->view.zoom;
    float sy = v->canvas.y + (90 - v->view.y) * v->view.zoom;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    pointer(SDL_EVENT_MOUSE_MOTION, sx + 8 * v->view.zoom, sy + 8 * v->view.zoom);
    ok = ok && v->document.shapes[0].x == 64 && v->document.shapes[0].y == 80;
    pointer(SDL_EVENT_MOUSE_MOTION, sx, sy);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx, sy);
    ok = ok && v->document.shapes[0].x == 54 && v->document.shapes[0].y == 70;
    vector_action("vector.toggle_snap");
    gesture(90, 90, 92, 92);
    ok = ok && fabsf(v->document.shapes[0].x - 56) < .001f;
    v->view.selected = 0;
    vector_action("vector.pen");
    vector_action("vector.toggle_snap");
    gesture(193, 161, 193, 161);
    int path = vector_first();
    ok = ok && path >= 0 && v->document.shapes[path].nodes[0].p.x == 192 &&
         v->document.shapes[path].nodes[0].p.y == 160;
    gesture(214, 182, 214, 182); /* Outside the circular tolerance. */
    ok = ok && fabsf(v->document.shapes[path].nodes[1].p.x - 214) < .001f;
    /* A group follows the clicked object's origin and preserves relative offsets. */
    vector_new();
    int a = ve_add_shape(&v->document, 0, VE_RECT, 32, 32, 40, 40);
    int b = ve_add_shape(&v->document, 0, VE_RECT, 96, 32, 40, 40);
    v->view.selected = (UINT64_C(1) << a) | (UINT64_C(1) << b);
    vector_commit();
    vector_action("vector.select");
    gesture(110, 45, 125, 60);
    ok = ok && v->document.shapes[a].x == 48 && v->document.shapes[b].x == 112 &&
         v->document.shapes[a].y == 48 && v->document.shapes[b].y == 48;
    vector_undo(false);
    ok = ok && v->document.shapes[a].x == 32 && v->document.shapes[b].x == 96;
    /* The destination artboard supplies the grid origin before reparenting. */
    ve_add_board(&v->document, 403, 0, 200, 200);
    v->view.selected = UINT64_C(1) << a;
    vector_commit();
    gesture(45, 45, 462, 60);
    ok = ok && v->document.shapes[a].board == 1 && v->document.shapes[a].x == 48 &&
         v->document.shapes[a].y == 48;
    editor_field_start(FIELD_V_SNAP_TOLERANCE, "-1");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_V_SNAP_TOLERANCE && v->view.snap_tolerance == 5;
    editor_field_stop();
    editor_field_start(FIELD_V_SNAP_TOLERANCE, "8");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NONE && v->view.snap_tolerance == 8;
    v->view.snap_tolerance = 5;
    vector_action("vector.toggle_snap");
    if (!ok) {
        fprintf(stderr, "Vector grid snapping / pen / tolerance smoke failed\n");
    }
    return ok;
}

static bool grid_smoke(void) {
    VectorEditor *v = app.workspace.vector;
    app.shell.right_tabs.selected = ui_id("vector_tools_tab");
    editor_draw();
    int revision = v->history.position;
    bool ok = v->view.grid_width == 16 && v->view.grid_height == 16 &&
              v->view.grid_color == 0x80808060 && !v->view.grid_isometric;
    bool visible = v->view.grid_visible;
    /* Toggle through the actual last toolbar button. */
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, v->canvas.x + 582, TOP + 21);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, v->canvas.x + 582, TOP + 21);
    ok = ok && v->view.grid_visible != visible;
    vector_action("vector.toggle_grid");
    float x = app.platform.width - RIGHT + PANEL_GAP + 179, y = TOP + TABS + 99 + 281;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
    ok = ok && app.shell.field == FIELD_V_GRID_WIDTH;
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "32";
    editor_event(&text);
    SDL_Event enter = {0};
    enter.type = SDL_EVENT_KEY_DOWN;
    enter.key.key = SDLK_RETURN;
    editor_event(&enter);
    ok = ok && app.shell.field == FIELD_NONE && v->view.grid_width == 32;
    editor_field_start(FIELD_V_GRID_HEIGHT, "0");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_V_GRID_HEIGHT && v->view.grid_height == 16;
    editor_field_stop();
    editor_field_start(FIELD_V_GRID_COLOR, "broken");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_V_GRID_COLOR && v->view.grid_color == 0x80808060;
    editor_field_stop();
    editor_field_start(FIELD_V_GRID_COLOR, "55667788");
    editor_field_apply();
    vector_action("vector.grid_isometric");
    editor_draw();
    ok = ok && v->view.grid_isometric && v->view.grid_visible == visible &&
         v->view.grid_color == 0x55667788 && v->history.position == revision;
    if (editor_screenshot_path) {
        char path[1600];
        int length = snprintf(path, sizeof path, "%s.grid-isometric.bmp", editor_screenshot_path);
        if (length > 0 && length < (int)sizeof path) {
            const char *original = editor_screenshot_path;
            editor_screenshot_path = path;
            editor_draw();
            editor_screenshot_path = original;
        }
    }
    if (!ok) {
        fprintf(stderr, "Vector grid toolbar / settings / validation smoke failed\n");
    }
    return ok;
}

static bool image_buttons_smoke(void) {
    PeUiImages *cache = &app.shell.editor_images;
    SDL_Texture *image = pe_ui_images_get(cache, app.platform.renderer, app.workspace.active_layout,
                                          "../icons/grid.png");
    float width = 0, height = 0;
    bool ok = image && SDL_GetTextureSize(image, &width, &height) && width == 32 && height == 32;
    int count = cache->count;
    ok = ok &&
         image == pe_ui_images_get(cache, app.platform.renderer, app.workspace.active_layout,
                                   "../icons/grid.png") &&
         cache->count == count;
    const Uint32 states[] = {UI_SURFACE, UI_HOVER, UI_ACCENT, UI_PANEL, UI_ACCENT, UI_ACCENT};
    for (int i = 0; image && i < 6; i++) {
        SDL_SetTextureAlphaMod(image, 200);
        ui_draw_image_button(app.platform.renderer, (SDL_FRect){20, 100, 32, 25}, image, "", 4,
                             i == 2, i != 3, i == 1, i == 4, i == 5);
        Uint8 alpha = 0;
        SDL_GetTextureAlphaMod(image, &alpha);
        ok = ok && alpha == 200;
        SDL_Rect pixel_area = {i == 5 ? 20 : 21, i == 5 ? 100 : 101, 1, 1};
        SDL_Surface *raw = SDL_RenderReadPixels(app.platform.renderer, &pixel_area);
        SDL_Surface *rgba = raw ? SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32) : NULL;
        if (rgba) {
            const unsigned char *pixel = rgba->pixels;
            ok = ok && pixel[0] == (states[i] >> 24) && pixel[1] == ((states[i] >> 16) & 255) &&
                 pixel[2] == ((states[i] >> 8) & 255);
        } else {
            ok = false;
        }
        SDL_DestroySurface(rgba);
        SDL_DestroySurface(raw);
    }
    if (image) {
        SDL_SetTextureAlphaMod(image, 255);
    }
    ok = ok && !pe_ui_images_get(cache, app.platform.renderer, app.workspace.active_layout,
                                 "../icons/missing-smoke.png");
    count = cache->count;
    ok = ok &&
         !pe_ui_images_get(cache, app.platform.renderer, app.workspace.active_layout,
                           "../icons/missing-smoke.png") &&
         cache->count == count;
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Image button texture / state margin / cache smoke failed\n");
    }
    return ok;
}

static bool preview_smoke(void) {
    VectorEditor *v = app.workspace.vector;
    app.shell.right_tabs.selected = ui_id("vector_tools_tab");
    editor_draw();
    int revision = v->history.position;
    vector_action("vector.preview_select");
    gesture(120, 140, 40, 50);
    VeRegion r = v->view.preview;
    bool ok = v->view.preview_enabled && v->view.preview_board == 0 && !v->preview_drag &&
              r.x == 40 && r.y == 50 && r.w == 80 && r.h == 90 && v->preview_width == 80 &&
              v->preview_height == 90 && v->preview_cache_valid && v->history.position == revision;
    float sx = v->canvas.x + (32 - v->view.x) * v->view.zoom;
    float sy = v->canvas.y + (32 - v->view.y) * v->view.zoom;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    SDL_Event key = {0};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    editor_draw();
    ok = ok && !v->preview_drag && v->view.preview.x == r.x && v->view.preview.w == r.w;
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx, sy);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    SDL_Event focus = {0};
    focus.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    editor_event(&focus);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx, sy);
    ok = ok && !v->preview_drag && v->view.preview.w == r.w;
    gesture(300, 220, 900, 900);
    ok = ok && v->view.preview.x == 300 && v->view.preview.y == 220 &&
         v->view.preview.x + v->view.preview.w == 448 &&
         v->view.preview.y + v->view.preview.h == 384;
    /* The second board contains only the difference contour, not the first board's objects. */
    gesture(500, 20, 650, 180);
    ok = ok && v->view.preview_board == 1 && v->preview_cached_board == 1;
    if (v->preview_pixels && v->preview_width > 28 && v->preview_height > 130) {
        const unsigned char *pixel = v->preview_pixels + (130 * v->preview_width + 28) * 4;
        ok = ok && pixel[0] == 194 && pixel[1] == 142 && pixel[2] == 92 && pixel[3] == 255;
    } else {
        ok = false;
    }
    vector_action("vector.preview_clear");
    ok = ok && !v->view.preview_enabled && v->history.position == revision;
    vector_action("vector.preview_select");
    gesture(220, 152, 380, 312);
    vector_action("vector.nodes");
    editor_draw();
    ok = ok && v->view.preview_board == 0 && v->view.preview.w == 160 && v->preview_cache_valid &&
         v->preview_revision == v->history.ids[revision];
    /* Camera movement changes only the crop of an oversized selection, never its 1:1 scale. */
    v->view.preview = (VeRegion){0, 0, 448, 384};
    v->view.x = -48;
    v->view.y = -44;
    editor_draw();
    VeRegion crop = v->preview_cached_source;
    v->view.x = -300;
    v->view.y = -200;
    editor_draw();
    ok = ok && v->preview_cached_source.x < crop.x && v->preview_cached_source.w == crop.w &&
         v->preview_cached_source.h == crop.h;
    v->view.x = -48;
    v->view.y = -44;
    v->view.preview = (VeRegion){220, 152, 160, 160};
    editor_draw();
    v->document.boards[0].background = 0;
    ve_history_commit(&v->history, &v->document);
    editor_draw();
    size_t last = ((size_t)v->preview_width * v->preview_height - 1) * 4 + 3;
    ok = ok && v->preview_pixels && v->preview_pixels[last] == 0;
    vector_undo(false);
    editor_draw();
    ok = ok && v->preview_pixels && v->preview_pixels[last] == 255 &&
         v->history.position == revision;
    if (!ok) {
        fprintf(stderr, "Vector preview selection / cancel / crop / cache smoke failed\n");
    }
    return ok;
}

static bool tree_smoke(void) {
    VectorEditor *v = app.workspace.vector;
    app.shell.left_tabs.selected = UI_SCENE_TAB;
    app.shell.scene_view.offset = 0;
    editor_draw();
    float x = PANEL_GAP + 10, y = TOP + TABS - PANEL_GAP + 10;
    float content = app.shell.scene_view.content;
    int revision = v->history.position;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 30, y + 3 * 26 + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 30, y + 3 * 26 + 12);
    editor_draw();
    bool ok = !v->view.expanded_paths[0] && app.shell.scene_view.content == content - 5 * 26;
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
    ok = ok && !v->view.expanded[0] && app.shell.scene_view.content == 3 * 26;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 10, y + 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 10, y + 12);
    editor_draw();
    ok = ok && v->view.expanded[0] && app.shell.scene_view.content == content;
    if (!ok) {
        fprintf(stderr, "Vector tree collapse / segment selection smoke failed\n");
    }
    return ok;
}

bool vector_smoke(void) {
    editor_select_workspace(UI_VECTOR_VIEW);
    if (!app.workspace.vector || app.workspace.map || app.workspace.script) {
        return false;
    }
    bool interaction = interaction_smoke();
    interaction = snap_smoke() && interaction;
    vector_demo();
    interaction = zoom_smoke() && interaction;
    interaction = grid_smoke() && interaction;
    interaction = image_buttons_smoke() && interaction;
    interaction = preview_smoke() && interaction;
    interaction = tools_smoke() && interaction;
    interaction = tree_smoke() && interaction;
    VectorEditor *v = app.workspace.vector;
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
    v->view.snap_enabled = true;
    v->view.snap_tolerance = 7;
    ok = ok && editor_reload_layout();
    editor_select_workspace(UI_MAP_VIEW);
    ok = ok && !app.workspace.vector && app.workspace.map && !app.workspace.script;
    editor_select_workspace(UI_VECTOR_VIEW);
    ok =
        ok && app.workspace.vector && !app.workspace.map && !app.workspace.script &&
        !strcmp(app.workspace.vector->document.shapes[shape].name, "Edited vector") &&
        app.workspace.vector->history.position == 1 && !app.workspace.vector->preview_texture &&
        !app.workspace.vector->preview_pixels && !app.shell.editor_images.entries &&
        app.workspace.vector->view.preview_enabled && app.workspace.vector->view.preview.w == 160 &&
        app.workspace.vector->view.grid_width == 32 && app.workspace.vector->view.grid_isometric &&
        app.workspace.vector->view.snap_enabled && app.workspace.vector->view.snap_tolerance == 7 &&
        app.workspace.vector->view.grid_color == 0x55667788 &&
        app.workspace.vector->view.expanded_paths[0] ==
            app.workspace.vector->document.shapes[shape].id &&
        app.shell.right_tabs.selected == ui_id("vector_tools_tab") &&
        tool_state("vector_transform_tools") && !tool_state("vector_transform_tools")->open;
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && !app.workspace.vector && app.workspace.script && !app.workspace.map;
    editor_select_workspace(UI_MAP_VIEW);
    if (!ok) {
        fprintf(stderr, "Vector lifecycle smoke failed\n");
    }
    return ok;
}

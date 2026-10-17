#include "editors/game/map/internal.h"

#include "editors/game/map/adapter.h"

static PeDocumentStatus status(void *context) {
    MapPayload *map = context;
    return (PeDocumentStatus){.loaded = true,
                              .dirty = pe_history_dirty(&map->history),
                              .exists = true,
                              .pending = map->history.pending != NULL,
                              .can_undo = map->history.cursor > 0,
                              .can_redo = map->history.cursor < map->history.count};
}

static bool save(void *context) {
    (void)context;
    return editor_save_scene();
}

static void undo(void *context, bool redo) {
    MapPayload *map = context;
    if (redo) {
        pe_redo(&map->history, &map->scene);
    } else {
        pe_undo(&map->history, &map->scene);
    }
    editor_selection_sync();
}

static void describe(void *context, char *buffer, size_t size) {
    MapPayload *map = context;
    snprintf(buffer, size, "Zoom %.0f%% | Tile %d | Undo %d / %d",
             pe_map_active_instance()->view.camera.zoom * 100, pe_map_active_instance()->view.tile,
             map->history.cursor, map->history.count);
}

static PeDocument document(void) {
    static const PeDocumentOps ops = {
        .status = status, .save = save, .undo = undo, .describe = describe};
    return (PeDocument){pe_map_active_instance(), &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.instance = calloc(1, sizeof(MapPayload));
    if (!pe_map_active_instance()) {
        snprintf(error, size, "Out of memory loading Map editor");
    }
    return pe_map_active_instance() != NULL;
}

static bool initialize(char *error, size_t size) {
    pe_map_active_instance()->view = (PeMapViewState){.camera = {-16, -16, 1.4f},
                                                      .tile = 1,
                                                      .tool = BRUSH,
                                                      .grid = true,
                                                      .layers_open = true,
                                                      .palette_open = true};
    pe_history_init(&pe_map_active_instance()->history);
    return pe_scene_load(&pe_map_active_instance()->scene, app.project.scene_path, error, size);
}

static void destroy(void) {
    if (pe_map_active_instance()) {
        pe_assets_free(&pe_map_active_instance()->assets);
        pe_history_free(&pe_map_active_instance()->history);
        free(pe_map_active_instance());
        app.workspace.instance = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return me_checkpoint(pe_map_active_instance(), io);
}

static bool action(const char *name) {
    editor_map_action(name);
    return !strncmp(name, "map.", 4) || !strncmp(name, "scene.", 6);
}

static bool action_enabled(const char *name) {
    if (!strcmp(name, "scene.toggle_play")) {
        return !editor_edit_pending();
    }
    if (!strncmp(name, "scene.", 6)) {
        bool enabled = !pe_map_active_instance()->view.playing && !editor_edit_pending();
        if (strcmp(name, "scene.add_root")) {
            enabled = enabled && pe_entity_valid(&pe_map_active_instance()->scene,
                                                 pe_entity_find(&pe_map_active_instance()->scene,
                                                                app.shell.selected));
        }
        if (enabled && !strcmp(name, "scene.add_child")) {
            const PeObject *object =
                pe_entity_get(&pe_map_active_instance()->scene,
                              pe_entity_find(&pe_map_active_instance()->scene, app.shell.selected));
            enabled = (object->mask & PE_TRANSFORM) != 0;
        }
        return enabled;
    }
    if (!strncmp(name, "map.", 4)) {
        return !pe_map_active_instance()->view.playing && !editor_edit_pending();
    }
    return strncmp(name, "vector.", 7) && strncmp(name, "script.", 7);
}

static bool action_active(const char *name) {
    if (!strcmp(name, "scene.toggle_play")) {
        return pe_map_active_instance()->view.playing;
    }
    const char *tools[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                           "map.select_fill", "map.select_entity"};
    for (int i = 0; i < 5; i++) {
        if (!strcmp(name, tools[i])) {
            return pe_map_active_instance()->view.tool == i;
        }
    }
    return false;
}

static bool boolean_binding(const char *name) {
    if (!strcmp(name, "map.grid_visible")) {
        return pe_map_active_instance()->view.grid;
    }
    if (!strcmp(name, "map.collision_visible")) {
        return pe_map_active_instance()->view.collision;
    }
    return false;
}

static bool playing(void) {
    return pe_map_active_instance()->view.playing;
}

static void cancel(void) {
    if (editor_edit_pending()) {
        pe_history_cancel(&pe_map_active_instance()->history, &pe_map_active_instance()->scene);
        pe_map_active_instance()->view.painting = pe_map_active_instance()->view.dragging = false;
    }
    pe_map_active_instance()->view.panning = false;
}

static bool overlay_open(void) {
    return pe_map_active_instance()->component_add_open;
}

static void begin_frame(void) {
    pe_map_active_instance()->component_anchor_seen = false;
}

static UiScroll *panel_scroll(UiId tab) {
    if (tab == UI_MAP_TAB) {
        return &pe_map_active_instance()->view.map_view;
    }
    if (tab == UI_TILES_TAB) {
        return &pe_map_active_instance()->view.tiles_view;
    }
    return NULL;
}

static void startup(const char *hint) {
    if (!strcmp(hint, "tools")) {
        app.shell.left_tabs.selected = UI_MAP_TAB;
        app.shell.right_tabs.selected = UI_TILES_TAB;
    } else if (!strcmp(hint, "scene-tree")) {
        app.shell.left_tabs.selected = UI_SCENE_TAB;
        for (int i = 0; i < PE_ENTITIES; i++) {
            if (pe_map_active_instance()->scene.objects[i].alive) {
                app.shell.selected = pe_map_active_instance()->scene.objects[i].stable_id;
                editor_inspect_entity();
                pe_map_active_instance()->view.tool = SELECT;
                break;
            }
        }
    }
}

static void draw_node(const char *type, const cJSON *node, SDL_FRect area) {
    if (!strcmp(type, "entity_tree")) {
        const cJSON *tools = pe_ui_get(node, "tools");
        if (tools) {
            editor_draw_yaml_tool_list(tools, area, true);
            float height = editor_yaml_tool_rows(tools, area.w) * 32.f + 6;
            app.shell.panel_area.y += height;
            app.shell.panel_area.h = fmaxf(0, app.shell.panel_area.h - height);
            app.shell.panel_top = app.shell.panel_area.y;
        }
        editor_draw_scene_tree();
    } else if (!strcmp(type, "map_canvas")) {
        editor_draw_map_content();
    } else if (!strcmp(type, "map_layers")) {
        editor_draw_map_panel();
    } else if (!strcmp(type, "tile_palette")) {
        editor_draw_tiles_panel();
    } else if (!strcmp(type, "inspector")) {
        editor_draw_inspector();
    } else if (!strcmp(type, "scene_info")) {
        editor_draw_info();
    }
}

static void request(int command, const char *path) {
    if (command == REQUEST_OPEN) {
        editor_load_scene(path);
    } else if (command == REQUEST_IMPORT) {
        editor_import_png(path);
    }
}

const PeEditorBackend pe_map_backend = {.overlay_event = editor_component_popup_event,
                                        .pointer = editor_map_pointer,
                                        .key = editor_map_key,
                                        .cancel = cancel,
                                        .zoom = editor_map_zoom,
                                        .drop = editor_import_png,
                                        .panel_scroll = panel_scroll,
                                        .overlay_open = overlay_open,
                                        .begin_frame = begin_frame,
                                        .draw_overlay = editor_draw_component_popup,
                                        .request = request,
                                        .draw_node = draw_node,
                                        .canvas_type = "map_canvas",
                                        .startup = startup,
                                        .update = editor_map_update,
                                        .create = create,
                                        .initialize = initialize,
                                        .destroy = destroy,
                                        .checkpoint = checkpoint,
                                        .action = action,
                                        .action_enabled = action_enabled,
                                        .action_active = action_active,
                                        .apply_field = editor_map_apply_field,
                                        .ready = editor_assets_sync,
                                        .boolean_binding = boolean_binding,
                                        .playing = playing,
                                        .document = document};

const EditorView pe_map_view = {.id = UI_MAP_VIEW,
                                .label = "Map",
                                .icon = ICON_GRID,
                                .action_count = 9,
                                .draw_tools = editor_draw_map_tools,
                                .draw_content = editor_draw_map_content,
                                .module = "map",
                                .open_request = REQUEST_OPEN,
                                .new_request = 0,
                                .import_request = REQUEST_IMPORT,
                                .backend = &pe_map_backend};

MapPayload *pe_map_active_instance(void) {
    return app.workspace.backend == &pe_map_backend ? app.workspace.instance : NULL;
}

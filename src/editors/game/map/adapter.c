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
             app.workspace.map->view.camera.zoom * 100, app.workspace.map->view.tile,
             map->history.cursor, map->history.count);
}

static PeDocument document(void) {
    static const PeDocumentOps ops = {
        .status = status, .save = save, .undo = undo, .describe = describe};
    return (PeDocument){app.workspace.map, &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.map = calloc(1, sizeof *app.workspace.map);
    if (!app.workspace.map) {
        snprintf(error, size, "Out of memory loading Map editor");
    }
    return app.workspace.map != NULL;
}

static bool initialize(char *error, size_t size) {
    app.workspace.map->view = (PeMapViewState){.camera = {-16, -16, 1.4f},
                                               .tile = 1,
                                               .tool = BRUSH,
                                               .grid = true,
                                               .layers_open = true,
                                               .palette_open = true};
    pe_history_init(&app.workspace.map->history);
    return pe_scene_load(&app.workspace.map->scene, app.project.scene_path, error, size);
}

static void destroy(void) {
    if (app.workspace.map) {
        pe_assets_free(&app.workspace.map->assets);
        pe_history_free(&app.workspace.map->history);
        free(app.workspace.map);
        app.workspace.map = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return me_checkpoint(app.workspace.map, io);
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
        bool enabled = !app.workspace.map->view.playing && !editor_edit_pending();
        if (strcmp(name, "scene.add_root")) {
            enabled = enabled && pe_entity_valid(
                                     &app.workspace.map->scene,
                                     pe_entity_find(&app.workspace.map->scene, app.shell.selected));
        }
        if (enabled && !strcmp(name, "scene.add_child")) {
            const PeObject *object =
                pe_entity_get(&app.workspace.map->scene,
                              pe_entity_find(&app.workspace.map->scene, app.shell.selected));
            enabled = (object->mask & PE_TRANSFORM) != 0;
        }
        return enabled;
    }
    if (!strncmp(name, "map.", 4)) {
        return !app.workspace.map->view.playing && !editor_edit_pending();
    }
    return strncmp(name, "vector.", 7) && strncmp(name, "script.", 7);
}

static bool action_active(const char *name) {
    if (!strcmp(name, "scene.toggle_play")) {
        return app.workspace.map->view.playing;
    }
    const char *tools[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                           "map.select_fill", "map.select_entity"};
    for (int i = 0; i < 5; i++) {
        if (!strcmp(name, tools[i])) {
            return app.workspace.map->view.tool == i;
        }
    }
    return false;
}

static bool boolean_binding(const char *name) {
    if (!strcmp(name, "map.grid_visible")) {
        return app.workspace.map->view.grid;
    }
    if (!strcmp(name, "map.collision_visible")) {
        return app.workspace.map->view.collision;
    }
    return false;
}

static bool playing(void) {
    return app.workspace.map->view.playing;
}

const PeEditorBackend pe_map_backend = {.create = create,
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

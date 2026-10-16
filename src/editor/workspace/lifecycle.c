#include "app/editor_internal.h"

typedef struct {
    float left, right;
    PeCamera camera;
    float scroll[6];
    int layer, tile, tool, palette;
    uint32_t selected, flags;
    int counts[2];
    UiId tabs[2][UI_MAX_TABS], selected_tabs[2];
    char scene_path[1024], status[256];
    int widget_count;
    WidgetState widgets[64];
    int expanded_count;
    char expanded[64][PE_PATH];
} UiSnapshot;

static void snapshot_ui(UiSnapshot *s, bool load);
static bool valid_snapshot(const UiSnapshot *s);
static void release_editor(void);
static void default_editor_ui(bool scripts);

static void snapshot_ui(UiSnapshot *s, bool load) {
    if (!load) {
        memset(s, 0, sizeof *s);
        s->expanded_count = app.expanded_count;
        memcpy(s->expanded, app.expanded, sizeof s->expanded);
        s->widget_count = app.widget_count;
        memcpy(s->widgets, app.widgets, sizeof s->widgets);
        s->left = app.left_width;
        s->right = app.right_width;
        s->camera = app.camera;
        s->scroll[0] = app.tree_view.offset;
        s->scroll[1] = app.scene_view.offset;
        s->scroll[2] = app.inspector_view.offset;
        s->scroll[3] = app.info_view.offset;
        s->scroll[4] = app.map_view.offset;
        s->scroll[5] = app.tiles_view.offset;
        s->layer = app.layer;
        s->tile = app.tile;
        s->tool = app.tool;
        s->palette = app.palette_scroll;
        s->selected = app.selected;
        s->flags =
            app.grid | (app.collision << 1) | (app.layers_open << 2) | (app.palette_open << 4);
        UiTabs *tabs[] = {&app.left_tabs, &app.right_tabs};
        for (int k = 0; k < 2; k++) {
            s->counts[k] = tabs[k]->count;
            s->selected_tabs[k] = tabs[k]->selected;
            for (int i = 0; i < tabs[k]->count; i++) {
                s->tabs[k][i] = tabs[k]->items[i].id;
            }
        }
        snprintf(s->scene_path, sizeof s->scene_path, "%s", app.scene_path);
        snprintf(s->status, sizeof s->status, "%s", app.status);
    } else {
        app.expanded_count = s->expanded_count;
        memcpy(app.expanded, s->expanded, sizeof app.expanded);
        app.widget_count = s->widget_count;
        memcpy(app.widgets, s->widgets, sizeof app.widgets);
        app.left_width = s->left;
        app.right_width = s->right;
        app.camera = s->camera;
        app.tree_view = (UiScroll){s->scroll[0], 0, 0};
        app.scene_view = (UiScroll){s->scroll[1], 0, 0};
        app.inspector_view = (UiScroll){s->scroll[2], 0, 0};
        app.info_view = (UiScroll){s->scroll[3], 0, 0};
        app.map_view = (UiScroll){s->scroll[4], 0, 0};
        app.tiles_view = (UiScroll){s->scroll[5], 0, 0};
        app.layer = s->layer;
        app.tile = s->tile;
        app.tool = s->tool;
        app.palette_scroll = s->palette;
        app.selected = s->selected;
        app.grid = (s->flags & 1) != 0;
        app.collision = (s->flags & 2) != 0;
        app.layers_open = (s->flags & 4) != 0;
        app.palette_open = (s->flags & 16) != 0;
        UiTabs *tabs[] = {&app.left_tabs, &app.right_tabs};
        for (int k = 0; k < 2; k++) {
            UiTab items[UI_MAX_TABS];
            for (int i = 0; i < s->counts[k]; i++) {
                items[i] = (UiTab){s->tabs[k][i], true, 0};
            }
            ui_tabs_init(tabs[k], items, s->counts[k], s->selected_tabs[k]);
        }
        snprintf(app.scene_path, sizeof app.scene_path, "%s", s->scene_path);
        snprintf(app.status, sizeof app.status, "%s", s->status);
    }
}

static bool valid_snapshot(const UiSnapshot *s) {
    if (s->expanded_count < 0 || s->expanded_count > 64) {
        return false;
    }
    for (int i = 0; i < s->expanded_count; i++) {
        if (!memchr(s->expanded[i], 0, PE_PATH)) {
            return false;
        }
    }
    if (s->widget_count < 0 || s->widget_count > 64) {
        return false;
    }
    for (int i = 0; i < s->widget_count; i++) {
        if (s->widgets[i].count < 0 || s->widgets[i].count > UI_MAX_TABS ||
            !isfinite(s->widgets[i].scroll) || s->widgets[i].scroll < 0 ||
            !isfinite(s->widgets[i].extent) || s->widgets[i].extent < 0 || s->widgets[i].kind < 0 ||
            s->widgets[i].kind > 2) {
            return false;
        }
    }
    if (!isfinite(s->left) || !isfinite(s->right) || s->left < 64 || s->right < 64 ||
        !isfinite(s->camera.x) || !isfinite(s->camera.y) || !isfinite(s->camera.zoom) ||
        s->camera.zoom <= 0 || s->layer < 0 || s->layer >= PE_LAYERS || s->tool < BRUSH ||
        s->tool > SELECT || !memchr(s->scene_path, 0, sizeof s->scene_path) ||
        !memchr(s->status, 0, sizeof s->status)) {
        return false;
    }
    for (int k = 0; k < 6; k++) {
        if (!isfinite(s->scroll[k]) || s->scroll[k] < 0) {
            return false;
        }
    }
    for (int k = 0; k < 2; k++) {
        UiTabs t;
        UiTab items[UI_MAX_TABS];
        if (s->counts[k] < 1 || s->counts[k] > UI_MAX_TABS) {
            return false;
        }
        for (int i = 0; i < s->counts[k]; i++) {
            items[i] = (UiTab){s->tabs[k][i], true, 0};
        }
        if (!ui_tabs_init(&t, items, s->counts[k], s->selected_tabs[k])) {
            return false;
        }
    }

    return true;
}

bool editor_checkpoint(PeSession *io, void *opaque) {
    UiId id = *(UiId *)opaque;
    uint32_t abi[4] = {sizeof(PeScene), sizeof(UiSnapshot), se_checkpoint_header_size(),
                       sizeof(MapPayload)};
    uint32_t expected[4];
    memcpy(expected, abi, sizeof abi);
    if (!pe_session_block(io, abi, sizeof abi) || memcmp(abi, expected, sizeof abi)) {
        return false;
    }

    UiSnapshot ui;
    if (!io->reading) {
        snapshot_ui(&ui, false);
    }
    if (!pe_session_block(io, &ui, sizeof ui) || !valid_snapshot(&ui)) {
        return false;
    }

    bool ok = id == UI_MAP_VIEW      ? me_checkpoint(app.map, io)
              : id == UI_VECTOR_VIEW ? vector_checkpoint(app.vector, io)
                                     : se_checkpoint(app.script, app.root, app.window, io);
    if (!ok) {
        return false;
    }
    if (io->reading) {
        snapshot_ui(&ui, true);
    }

    return true;
}

static void release_editor(void) {
    if (app.vector) {
        vector_free(app.vector);
        free(app.vector);
        app.vector = NULL;
    }
    if (app.script) {
        se_focus(app.script, app.window, false);
        se_free(app.script);
        free(app.script);
        app.script = NULL;
    }
    if (app.map) {
        pe_assets_free(&app.map->assets);
        pe_history_free(&app.map->history);
        free(app.map);
        app.map = NULL;
    }
    ui_cancel(&app.ui);
    ui_focus(&app.ui, 0);
    pe_ui_layout_free(app.active_layout);
    app.active_layout = NULL;
    app.widget_count = 0;
    memset(app.widgets, 0, sizeof app.widgets);
    app.loaded_view = 0;
    app.painting = app.dragging = app.panning = false;
    memset(app.tree, 0, sizeof app.tree);
    app.tree_count = 0;
}

static void default_editor_ui(bool scripts) {
    const cJSON *columns = pe_ui_get(pe_ui_get(app.shell_layout->root, "layout"), "columns");
    app.left_width = (float)pe_ui_get(pe_ui_get(columns, "left"), "initial")->valuedouble;
    app.right_width = (float)pe_ui_get(pe_ui_get(columns, "right"), "initial")->valuedouble;
    app.camera = (PeCamera){-16, -16, 1.4f};
    app.layer = 0;
    app.tile = 1;
    app.tool = BRUSH;
    app.selected = 0;
    app.expanded_count = 3;
    snprintf(app.expanded[0], PE_PATH, "assets");
    snprintf(app.expanded[1], PE_PATH, "scenes");
    snprintf(app.expanded[2], PE_PATH, "scripts");
    app.grid = true;
    app.collision = false;
    app.layers_open = app.palette_open = true;
    app.tree_view = app.scene_view = app.inspector_view = app.info_view = app.map_view =
        app.tiles_view = (UiScroll){0};
    UiTab left[] = {{UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}, {UI_MAP_TAB, true, 0}};
    UiTab right[] = {{UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}, {UI_TILES_TAB, true, 0}};
    ui_tabs_init(&app.left_tabs, left, scripts ? 1 : 3, UI_EXPLORER_TAB);
    ui_tabs_init(&app.right_tabs, right, scripts ? 2 : 3, UI_INSPECTOR_TAB);
}

bool editor_activate(UiId id, char *error, size_t size) {
    if (!app.active_layout) {
        app.active_layout = editor_load_editor_layout(id, error, size);
    }
    if (!app.active_layout) {
        return false;
    }

    int index = editor_session_index(id);
    if (index == 0) {
        app.map = calloc(1, sizeof *app.map);
    } else if (index == 2) {
        app.vector = calloc(1, sizeof *app.vector);
    } else {
        app.script = calloc(1, sizeof *app.script);
    }
    if ((index == 0 && !app.map) || (index == 1 && !app.script) || (index == 2 && !app.vector)) {
        snprintf(error, size, "Out of memory loading editor");
        return false;
    }

    bool ok;
    if (app.session_exists[index]) {
        ok = pe_session_load(app.session_paths[index], id, editor_checkpoint, &id, error, size);
    } else {
        default_editor_ui(index == 1);
        if (index == 0) {
            pe_history_init(&app.map->history);
            ok = pe_scene_load(&app.map->scene, app.scene_path, error, size);
        } else if (index == 2) {
            ok = vector_init(app.vector);
            app.left_tabs.selected = UI_SCENE_TAB;
            app.selected = 10000000;
        } else {
            ok = se_init(app.script, app.root);
            if (!ok) {
                snprintf(error, size, "%s", app.script->status);
            }
        }
    }
    if (!ok) {
        release_editor();
        return false;
    }
    app.loaded_view = id;
    app.needs_layout = true;
    editor_tree_refresh();
    if (index == 0) {
        editor_assets_sync();
    } else if (index == 1) {
        se_check(app.script);
    }

    return true;
}

bool editor_switch(UiId id) {
    if (id == app.loaded_view) {
        return true;
    }

    char error[256];
    PeUiLayout *candidate = editor_load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        editor_message(error);
        return false;
    }

    UiId previous = app.loaded_view;
    if (previous) {
        int index = editor_session_index(previous);
        if (!pe_session_save(app.session_paths[index], previous, editor_checkpoint, &previous,
                             error, sizeof error)) {
            pe_ui_layout_free(candidate);
            editor_message(error);
            return false;
        }
        app.session_dirty[index] = index == 0   ? editor_map_dirty()
                                   : index == 1 ? editor_script_dirty()
                                                : editor_vector_dirty();
        app.session_exists[index] = true;
        release_editor();
    }
    app.active_layout = candidate;
    if (editor_activate(id, error, sizeof error)) {
        return true;
    }

    char original[256];
    snprintf(original, sizeof original, "%s", error);
    if (previous) {
        editor_activate(previous, error, sizeof error);
    }
    editor_message(original);
    return false;
}

bool editor_reload_layout(void) {
    if (editor_edit_pending() || app.playing || app.request != REQUEST_NONE ||
        app.field != FIELD_NONE) {
        return false;
    }

    char error[256];
    PeUiLayout *next = pe_ui_layout_load(app.ui_path, "shell", error, sizeof error);
    if (!next) {
        editor_message(error);
        return false;
    }

    PeUiLayout *old_shell = app.shell_layout;
    app.shell_layout = next;
    UiId id = app.loaded_view;
    PeUiLayout *candidate = editor_load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        app.shell_layout = old_shell;
        pe_ui_layout_free(next);
        editor_message(error);
        return false;
    }

    int index = editor_session_index(id);
    if (!pe_session_save(app.session_paths[index], id, editor_checkpoint, &id, error,
                         sizeof error)) {
        app.shell_layout = old_shell;
        pe_ui_layout_free(next);
        pe_ui_layout_free(candidate);
        editor_message(error);
        return false;
    }
    app.session_exists[index] = true;
    app.session_dirty[index] = index == 0   ? editor_map_dirty()
                               : index == 1 ? editor_script_dirty()
                                            : editor_vector_dirty();
    PeUiLayout *old_layout = app.active_layout;
    app.active_layout = NULL;
    release_editor();
    app.active_layout = candidate;
    if (editor_activate(id, error, sizeof error)) {
        pe_ui_layout_free(old_layout);
        pe_ui_layout_free(old_shell);
        editor_apply_shell_layout();
        editor_window_init();
        editor_message("UI reloaded (F6)");
        return true;
    }

    char original[256];
    snprintf(original, sizeof original, "%s", error);
    app.shell_layout = old_shell;
    pe_ui_layout_free(next);
    app.active_layout = old_layout;
    editor_activate(id, error, sizeof error);
    editor_message(original);
    return false;
}

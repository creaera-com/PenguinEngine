#include "editor/host.h"

typedef struct {
    float left, right;
    float scroll[4];
    uint32_t selected;
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
static void default_editor_ui(bool scripts);

static void snapshot_ui(UiSnapshot *s, bool load) {
    if (!load) {
        memset(s, 0, sizeof *s);
        s->expanded_count = app.shell.expanded_count;
        memcpy(s->expanded, app.shell.expanded, sizeof s->expanded);
        s->widget_count = app.shell.widget_count;
        memcpy(s->widgets, app.shell.widgets, sizeof s->widgets);
        s->left = app.shell.left_width;
        s->right = app.shell.right_width;
        s->scroll[0] = app.shell.tree_view.offset;
        s->scroll[1] = app.shell.scene_view.offset;
        s->scroll[2] = app.shell.inspector_view.offset;
        s->scroll[3] = app.shell.info_view.offset;
        s->selected = app.shell.selected;
        UiTabs *tabs[] = {&app.shell.left_tabs, &app.shell.right_tabs};
        for (int k = 0; k < 2; k++) {
            s->counts[k] = tabs[k]->count;
            s->selected_tabs[k] = tabs[k]->selected;
            for (int i = 0; i < tabs[k]->count; i++) {
                s->tabs[k][i] = tabs[k]->items[i].id;
            }
        }
        snprintf(s->scene_path, sizeof s->scene_path, "%s", app.project.scene_path);
        snprintf(s->status, sizeof s->status, "%s", app.shell.status);
    } else {
        app.shell.expanded_count = s->expanded_count;
        memcpy(app.shell.expanded, s->expanded, sizeof app.shell.expanded);
        app.shell.widget_count = s->widget_count;
        memcpy(app.shell.widgets, s->widgets, sizeof app.shell.widgets);
        app.shell.left_width = s->left;
        app.shell.right_width = s->right;
        app.shell.tree_view = (UiScroll){s->scroll[0], 0, 0};
        app.shell.scene_view = (UiScroll){s->scroll[1], 0, 0};
        app.shell.inspector_view = (UiScroll){s->scroll[2], 0, 0};
        app.shell.info_view = (UiScroll){s->scroll[3], 0, 0};
        app.shell.selected = s->selected;
        UiTabs *tabs[] = {&app.shell.left_tabs, &app.shell.right_tabs};
        for (int k = 0; k < 2; k++) {
            UiTab items[UI_MAX_TABS];
            for (int i = 0; i < s->counts[k]; i++) {
                items[i] = (UiTab){s->tabs[k][i], true, 0};
            }
            ui_tabs_init(tabs[k], items, s->counts[k], s->selected_tabs[k]);
        }
        snprintf(app.project.scene_path, sizeof app.project.scene_path, "%s", s->scene_path);
        snprintf(app.shell.status, sizeof app.shell.status, "%s", s->status);
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
        !memchr(s->scene_path, 0, sizeof s->scene_path) ||
        !memchr(s->status, 0, sizeof s->status)) {
        return false;
    }
    for (int k = 0; k < 4; k++) {
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
    if (!app.workspace.backend || app.workspace.backend != editor_view(id)->backend) {
        return false;
    }
    uint32_t abi[2] = {1, sizeof(UiSnapshot)};
    uint32_t expected[2];
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

    bool ok = app.workspace.backend && app.workspace.backend->checkpoint(io);
    if (!ok) {
        return false;
    }
    if (io->reading) {
        snapshot_ui(&ui, true);
    }

    return true;
}

void editor_release_active(void) {
    pe_ui_images_free(&app.shell.editor_images);
    if (app.workspace.backend) {
        app.workspace.backend->destroy();
        app.workspace.backend = NULL;
    }
    ui_cancel(&app.shell.ui);
    ui_focus(&app.shell.ui, 0);
    pe_ui_layout_free(app.workspace.active_layout);
    app.workspace.active_layout = NULL;
    app.shell.widget_count = 0;
    memset(app.shell.widgets, 0, sizeof app.shell.widgets);
    app.workspace.loaded_view = 0;
    memset(app.shell.tree, 0, sizeof app.shell.tree);
    app.shell.tree_count = 0;
}

static void default_editor_ui(bool scripts) {
    const cJSON *columns = pe_ui_get(pe_ui_get(app.shell.shell_layout->root, "layout"), "columns");
    app.shell.left_width = (float)pe_ui_get(pe_ui_get(columns, "left"), "initial")->valuedouble;
    app.shell.right_width = (float)pe_ui_get(pe_ui_get(columns, "right"), "initial")->valuedouble;
    app.shell.selected = 0;
    app.shell.expanded_count = 3;
    snprintf(app.shell.expanded[0], PE_PATH, "assets");
    snprintf(app.shell.expanded[1], PE_PATH, "scenes");
    snprintf(app.shell.expanded[2], PE_PATH, "scripts");
    app.shell.tree_view = app.shell.scene_view = app.shell.inspector_view = app.shell.info_view =
        (UiScroll){0};
    UiTab left[] = {{UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}, {UI_MAP_TAB, true, 0}};
    UiTab right[] = {{UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}, {UI_TILES_TAB, true, 0}};
    ui_tabs_init(&app.shell.left_tabs, left, scripts ? 1 : 3, UI_EXPLORER_TAB);
    ui_tabs_init(&app.shell.right_tabs, right, scripts ? 2 : 3, UI_INSPECTOR_TAB);
}

bool editor_activate(UiId id, char *error, size_t size) {
    if (!app.workspace.active_layout) {
        app.workspace.active_layout = editor_load_editor_layout(id, error, size);
    }
    if (!app.workspace.active_layout) {
        return false;
    }

    int index = editor_session_index(id);
    const PeEditorBackend *backend = editor_view(id)->backend;
    if (!backend->create(error, size)) {
        backend->destroy();
        pe_ui_layout_free(app.workspace.active_layout);
        app.workspace.active_layout = NULL;
        return false;
    }
    app.workspace.backend = backend;

    bool ok;
    if (app.workspace.session_exists[index]) {
        ok = pe_session_load(app.workspace.session_paths[index], id, editor_checkpoint, &id, error,
                             size);
    } else {
        default_editor_ui(index == 1);
        ok = backend->initialize(error, size);
    }

    if (!ok) {
        editor_release_active();
        return false;
    }
    app.workspace.loaded_view = id;
    app.shell.needs_layout = true;
    editor_tree_refresh();
    if (backend->ready) {
        backend->ready();
    }

    return true;
}

bool editor_switch(UiId id) {
    if (id == app.workspace.loaded_view) {
        return true;
    }

    char error[256];
    PeUiLayout *candidate = editor_load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        editor_message(error);
        return false;
    }

    UiId previous = app.workspace.loaded_view;
    if (previous) {
        int index = editor_session_index(previous);
        if (!pe_session_save(app.workspace.session_paths[index], previous, editor_checkpoint,
                             &previous, error, sizeof error)) {
            pe_ui_layout_free(candidate);
            editor_message(error);
            return false;
        }
        app.workspace.session_dirty[index] = index == 0   ? editor_map_dirty()
                                             : index == 1 ? editor_script_dirty()
                                                          : editor_vector_dirty();
        app.workspace.session_exists[index] = true;
        editor_release_active();
    }
    app.workspace.active_layout = candidate;
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
    if (editor_edit_pending() || editor_playing() || app.shell.request != REQUEST_NONE ||
        app.shell.field != FIELD_NONE) {
        return false;
    }

    char error[256];
    PeUiLayout *next = pe_ui_layout_load(app.shell.ui_path, "shell", error, sizeof error);
    if (!next) {
        editor_message(error);
        return false;
    }

    PeUiLayout *old_shell = app.shell.shell_layout;
    app.shell.shell_layout = next;
    UiId id = app.workspace.loaded_view;
    PeUiLayout *candidate = editor_load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        app.shell.shell_layout = old_shell;
        pe_ui_layout_free(next);
        editor_message(error);
        return false;
    }

    int index = editor_session_index(id);
    if (!pe_session_save(app.workspace.session_paths[index], id, editor_checkpoint, &id, error,
                         sizeof error)) {
        app.shell.shell_layout = old_shell;
        pe_ui_layout_free(next);
        pe_ui_layout_free(candidate);
        editor_message(error);
        return false;
    }
    app.workspace.session_exists[index] = true;
    app.workspace.session_dirty[index] = index == 0   ? editor_map_dirty()
                                         : index == 1 ? editor_script_dirty()
                                                      : editor_vector_dirty();
    PeUiLayout *old_layout = app.workspace.active_layout;
    app.workspace.active_layout = NULL;
    editor_release_active();
    app.workspace.active_layout = candidate;
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
    app.shell.shell_layout = old_shell;
    pe_ui_layout_free(next);
    app.workspace.active_layout = old_layout;
    editor_activate(id, error, sizeof error);
    editor_message(original);
    return false;
}

#include "editors/graphics/vector/internal.h"

#include "editors/graphics/vector/adapter.h"

static PeDocumentStatus status(void *context) {
    VectorEditor *vector = context;
    return (PeDocumentStatus){.loaded = true,
                              .dirty = ve_dirty(&vector->history),
                              .exists = vector->view.exists,
                              .pending = vector->dragging || vector->preview_drag,
                              .can_undo = vector->history.position > 0,
                              .can_redo = vector->history.position + 1 < vector->history.count};
}

static bool save(void *context) {
    (void)context;
    return vector_save();
}

static void undo(void *context, bool redo) {
    (void)context;
    vector_undo(redo);
}

static void describe(void *context, char *buffer, size_t size) {
    VectorEditor *vector = context;
    snprintf(buffer, size, "SVG | Zoom %.0f%% | px | Undo %d / %d", vector->view.zoom * 100,
             vector->history.position, vector->history.count - 1);
}

static PeDocument document(void) {
    static const PeDocumentOps ops = {
        .status = status, .save = save, .undo = undo, .describe = describe};
    return (PeDocument){pe_vector_active_instance(), &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.instance = calloc(1, sizeof(VectorEditor));
    if (!pe_vector_active_instance()) {
        snprintf(error, size, "Out of memory loading Vector editor");
    }
    return pe_vector_active_instance() != NULL;
}

static bool initialize(char *error, size_t size) {
    bool ok = vector_init(pe_vector_active_instance());
    if (!ok) {
        snprintf(error, size, "Cannot initialize Vector document");
    }
    app.shell.left_tabs.selected = UI_SCENE_TAB;
    app.shell.selected = 10000000;
    return ok;
}

static void destroy(void) {
    if (pe_vector_active_instance()) {
        vector_free(pe_vector_active_instance());
        free(pe_vector_active_instance());
        app.workspace.instance = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return vector_checkpoint(pe_vector_active_instance(), io);
}

static bool action(const char *name) {
    return vector_action(name);
}

static bool action_enabled(const char *name) {
    if (!strncmp(name, "vector.", 7)) {
        return !editor_edit_pending();
    }
    return strncmp(name, "scene.", 6) && strncmp(name, "map.", 4) && strncmp(name, "script.", 7);
}

static bool action_active(const char *action) {
    if (pe_vector_active_instance()) {
        if (!strcmp(action, "vector.toggle_snap")) {
            return pe_vector_active_instance()->view.snap_enabled;
        }
        if (!strcmp(action, "vector.toggle_grid")) {
            return pe_vector_active_instance()->view.grid_visible;
        }
        if (!strcmp(action, "vector.grid_standard") || !strcmp(action, "vector.grid_isometric")) {
            return pe_vector_active_instance()->view.grid_isometric ==
                   !strcmp(action, "vector.grid_isometric");
        }
        if (!strcmp(action, "vector.preview_select")) {
            return pe_vector_active_instance()->view.tool == VT_PREVIEW;
        }
        const char *tools[] = {"vector.select", "vector.rectangle", "vector.ellipse",
                               "vector.pen",    "vector.nodes",     "vector.artboard"};
        for (int i = 0; i < 6; i++) {
            if (!strcmp(action, tools[i])) {
                return pe_vector_active_instance()->view.tool == i;
            }
        }
    }
    return false;
}

static void apply_field(void) {
    vector_apply_field(app.shell.field);
}

static bool event(SDL_Event *input, bool modal) {
    (void)modal;
    return vector_event(input);
}

static void field_stopped(void) {
    if (app.shell.field == FIELD_V_SAVE) {
        pe_vector_active_instance()->after_save = REQUEST_NONE;
    }
}

static void startup(const char *hint) {
    if (!strcmp(hint, "demo")) {
        vector_demo();
    }
}

static void draw_node(const char *type, const cJSON *node, SDL_FRect area) {
    (void)node;
    if (!strcmp(type, "vector_canvas")) {
        vector_draw_canvas();
    } else if (!strcmp(type, "vector_tree")) {
        vector_draw_tree();
    } else if (!strcmp(type, "vector_inspector")) {
        vector_draw_inspector();
    } else if (!strcmp(type, "vector_grid")) {
        vector_draw_grid_settings(area);
    } else if (!strcmp(type, "vector_preview")) {
        vector_draw_preview(area);
    }
}

static void request(int command, const char *path) {
    if (command == REQUEST_VECTOR_OPEN) {
        vector_open(path);
    } else if (command == REQUEST_VECTOR_NEW) {
        vector_new();
    }
}

const PeEditorBackend pe_vector_backend = {.event = event,
                                           .request = request,
                                           .draw_node = draw_node,
                                           .canvas_type = "vector_canvas",
                                           .field_stopped = field_stopped,
                                           .startup = startup,
                                           .create = create,
                                           .initialize = initialize,
                                           .destroy = destroy,
                                           .checkpoint = checkpoint,
                                           .action = action,
                                           .action_enabled = action_enabled,
                                           .action_active = action_active,
                                           .apply_field = apply_field,
                                           .document = document};

const EditorView pe_vector_view = {.id = UI_VECTOR_VIEW,
                                   .label = "Vector Art",
                                   .icon = ICON_EDIT,
                                   .action_count = 1,
                                   .draw_tools = vector_draw_tools,
                                   .draw_content = vector_draw_canvas,
                                   .module = "vector",
                                   .open_request = REQUEST_VECTOR_OPEN,
                                   .new_request = REQUEST_VECTOR_NEW,
                                   .import_request = 0,
                                   .backend = &pe_vector_backend};

VectorEditor *pe_vector_active_instance(void) {
    return app.workspace.backend == &pe_vector_backend ? app.workspace.instance : NULL;
}

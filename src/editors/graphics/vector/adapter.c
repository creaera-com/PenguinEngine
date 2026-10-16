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
    return (PeDocument){app.workspace.vector, &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.vector = calloc(1, sizeof *app.workspace.vector);
    if (!app.workspace.vector) {
        snprintf(error, size, "Out of memory loading Vector editor");
    }
    return app.workspace.vector != NULL;
}

static bool initialize(char *error, size_t size) {
    bool ok = vector_init(app.workspace.vector);
    if (!ok) {
        snprintf(error, size, "Cannot initialize Vector document");
    }
    app.shell.left_tabs.selected = UI_SCENE_TAB;
    app.shell.selected = 10000000;
    return ok;
}

static void destroy(void) {
    if (app.workspace.vector) {
        vector_free(app.workspace.vector);
        free(app.workspace.vector);
        app.workspace.vector = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return vector_checkpoint(app.workspace.vector, io);
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
    if (app.workspace.vector) {
        if (!strcmp(action, "vector.toggle_snap")) {
            return app.workspace.vector->view.snap_enabled;
        }
        if (!strcmp(action, "vector.toggle_grid")) {
            return app.workspace.vector->view.grid_visible;
        }
        if (!strcmp(action, "vector.grid_standard") || !strcmp(action, "vector.grid_isometric")) {
            return app.workspace.vector->view.grid_isometric ==
                   !strcmp(action, "vector.grid_isometric");
        }
        if (!strcmp(action, "vector.preview_select")) {
            return app.workspace.vector->view.tool == VT_PREVIEW;
        }
        const char *tools[] = {"vector.select", "vector.rectangle", "vector.ellipse",
                               "vector.pen",    "vector.nodes",     "vector.artboard"};
        for (int i = 0; i < 6; i++) {
            if (!strcmp(action, tools[i])) {
                return app.workspace.vector->view.tool == i;
            }
        }
    }
    return false;
}

static void apply_field(void) {
    vector_apply_field(app.shell.field);
}

const PeEditorBackend pe_vector_backend = {.create = create,
                                           .initialize = initialize,
                                           .destroy = destroy,
                                           .checkpoint = checkpoint,
                                           .action = action,
                                           .action_enabled = action_enabled,
                                           .action_active = action_active,
                                           .apply_field = apply_field,
                                           .document = document};

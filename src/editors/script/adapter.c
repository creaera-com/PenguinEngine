#include "editors/script/internal.h"

#include "editors/script/adapter.h"

static PeDocumentStatus status(void *context) {
    PeScriptEditor *script = context;
    return (PeDocumentStatus){.loaded = true,
                              .dirty = se_dirty(script),
                              .exists = script->exists,
                              .can_undo = script->document.position > 0,
                              .can_redo = script->document.position < script->document.count};
}

static bool save(void *context) {
    PeScriptEditor *script = context;
    bool ok = se_save(script);
    editor_message(script->status);
    return ok;
}

static void undo(void *context, bool redo) {
    PeScriptEditor *script = context;
    bool changed = redo ? pe_text_redo(&script->document) : pe_text_undo(&script->document);
    if (changed) {
        se_changed(script, app.shell.viewport, false);
    }
}

static void describe(void *context, char *buffer, size_t size) {
    PeScriptEditor *script = context;
    int line, column;
    pe_text_location(&script->document, script->document.cursor, &line, &column);
    snprintf(buffer, size, "JS | Ln %d, Col %d | Undo %d / %d", line + 1, column + 1,
             script->document.position, script->document.count);
}

static PeDocument document(void) {
    static const PeDocumentOps ops = {
        .status = status, .save = save, .undo = undo, .describe = describe};
    return (PeDocument){app.workspace.script, &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.script = calloc(1, sizeof *app.workspace.script);
    if (!app.workspace.script) {
        snprintf(error, size, "Out of memory loading Script editor");
    }
    return app.workspace.script != NULL;
}

static bool initialize(char *error, size_t size) {
    bool ok = se_init(app.workspace.script, app.project.root);
    if (!ok) {
        snprintf(error, size, "%s", app.workspace.script->status);
    }
    return ok;
}

static void destroy(void) {
    if (app.workspace.script) {
        se_focus(app.workspace.script, app.platform.window, false);
        se_free(app.workspace.script);
        free(app.workspace.script);
        app.workspace.script = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return se_checkpoint(app.workspace.script, app.project.root, app.platform.window, io);
}

static bool action(const char *name) {
    return editor_script_action(name);
}

static bool action_enabled(const char *name) {
    return strncmp(name, "scene.", 6) && strncmp(name, "map.", 4) && strncmp(name, "vector.", 7);
}

static bool action_active(const char *name) {
    (void)name;
    return false;
}

static void ready(void) {
    se_check(app.workspace.script);
}

static const char *message(void) {
    return app.workspace.script->status;
}

const PeEditorBackend pe_script_backend = {.create = create,
                                           .initialize = initialize,
                                           .destroy = destroy,
                                           .checkpoint = checkpoint,
                                           .action = action,
                                           .action_enabled = action_enabled,
                                           .action_active = action_active,
                                           .apply_field = NULL,
                                           .ready = ready,
                                           .message = message,
                                           .document = document};

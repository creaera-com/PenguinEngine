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
    return (PeDocument){pe_script_active_instance(), &ops};
}

static bool create(char *error, size_t size) {
    app.workspace.instance = calloc(1, sizeof(PeScriptEditor));
    if (!pe_script_active_instance()) {
        snprintf(error, size, "Out of memory loading Script editor");
    }
    return pe_script_active_instance() != NULL;
}

static bool initialize(char *error, size_t size) {
    bool ok = se_init(pe_script_active_instance(), app.project.root);
    if (!ok) {
        snprintf(error, size, "%s", pe_script_active_instance()->status);
    }
    return ok;
}

static void destroy(void) {
    if (pe_script_active_instance()) {
        se_focus(pe_script_active_instance(), app.platform.window, false);
        se_free(pe_script_active_instance());
        free(pe_script_active_instance());
        app.workspace.instance = NULL;
    }
}

static bool checkpoint(PeSession *io) {
    return se_checkpoint(pe_script_active_instance(), app.project.root, app.platform.window, io);
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
    se_check(pe_script_active_instance());
}

static const char *message(void) {
    return pe_script_active_instance()->status;
}

static void blur(void) {
    se_focus(pe_script_active_instance(), app.platform.window, false);
}

static bool controls_hit(float x, float y) {
    return se_scroll_hit(pe_script_active_instance(), x, y, app.shell.viewport);
}

static bool event(SDL_Event *input, bool modal) {
    if (!app.shell.canvas_enabled || modal) {
        return false;
    }
    bool consumed =
        se_event(pe_script_active_instance(), app.platform.window, input, app.shell.viewport);
    if (consumed && input->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        pe_script_active_instance()->focused) {
        ui_focus(&app.shell.ui, UI_SCRIPT_DOCUMENT);
    }
    return consumed;
}

static UiId text_focus(void) {
    return pe_script_active_instance()->focused ? UI_SCRIPT_DOCUMENT : 0;
}

static void notify(const char *message) {
    snprintf(pe_script_active_instance()->status, sizeof pe_script_active_instance()->status, "%s",
             message);
}

static void draw_node(const char *type, const cJSON *node, SDL_FRect area) {
    (void)node;
    (void)area;
    if (!strcmp(type, "script_editor")) {
        editor_draw_script_content();
    } else if (!strcmp(type, "script_info")) {
        editor_draw_script_inspector();
    }
}

static void focus(void) {
    se_focus(pe_script_active_instance(), app.platform.window, !editor_playing());
    ui_focus(&app.shell.ui, UI_SCRIPT_DOCUMENT);
}

static void request(int command, const char *path) {
    if (command == REQUEST_SCRIPT_OPEN) {
        if (se_open(pe_script_active_instance(), path)) {
            focus();
        } else {
            editor_message(pe_script_active_instance()->status);
        }
    }
}

const PeEditorBackend pe_script_backend = {.event = event,
                                           .blur = blur,
                                           .controls_hit = controls_hit,
                                           .request = request,
                                           .focus = focus,
                                           .draw_node = draw_node,
                                           .canvas_type = "script_editor",
                                           .text_focus = text_focus,
                                           .notify = notify,
                                           .create = create,
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

const EditorView pe_script_view = {.id = UI_SCRIPT_VIEW,
                                   .label = "Scripts",
                                   .icon = ICON_FILE,
                                   .action_count = 1,
                                   .draw_tools = editor_draw_script_tools,
                                   .draw_content = editor_draw_script_content,
                                   .module = "scripts",
                                   .open_request = REQUEST_SCRIPT_OPEN,
                                   .new_request = 0,
                                   .import_request = 0,
                                   .backend = &pe_script_backend};

PeScriptEditor *pe_script_active_instance(void) {
    return app.workspace.backend == &pe_script_backend ? app.workspace.instance : NULL;
}

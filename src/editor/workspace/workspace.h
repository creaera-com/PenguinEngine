#ifndef PENGUIN_WORKSPACE_WORKSPACE_H
#define PENGUIN_WORKSPACE_WORKSPACE_H
#include "penguin/ui.h"
#include "workspace/session.h"

typedef enum {
    REQUEST_NONE,
    REQUEST_QUIT,
    REQUEST_OPEN,
    REQUEST_SCRIPT_OPEN,
    REQUEST_IMPORT
} Request;

bool editor_activate(UiId id, char *error, size_t size);
bool editor_checkpoint(PeSession *io, void *opaque);
void editor_process_deferred_ui(void);
bool editor_reload_layout(void);
void editor_request(Request type, const char *path);
void editor_request_finish(void);
bool editor_save(void);
void editor_select_workspace(UiId id);
bool editor_switch(UiId id);
void editor_tab_select(bool scripts);
void editor_undo_active(bool redo);
#endif

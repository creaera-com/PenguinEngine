/** @file @brief App-wide Save As dialogs and file opening via editor contracts. */
#ifndef PENGUENGINE_DOCUMENT_CONTROLLER_H
#define PENGUENGINE_DOCUMENT_CONTROLLER_H
#include "editor_host/host.h"
#include "project/controller.h"

typedef struct {
    Uint32 event;
    bool dialog_pending;
    const EditorDefinition *owner;
    unsigned document;
    char pending_open[4096], pending_save[4096], default_path[4096];
} DocumentController;

/** @brief Build the native Save As suggestion; unsaved documents start inside the project. */
bool document_save_default_path(char *path, size_t capacity, const EditorFileInfo *info,
                                const char *project_root);
/** @brief Register a dedicated event type for owned native-dialog results. */
void document_controller_init(DocumentController *controller);
/** @brief Free queued results; pending callbacks have independent, self-owned context. */
void document_controller_destroy(DocumentController *controller);
/** @brief Route Save and supported Explorer files; other actions remain untouched. */
bool document_controller_action(DocumentController *controller, EditorHost *host,
                                ProjectController *projects, SDL_Window *window, PUIView *view,
                                const char *action, const char *widget);
/** @brief Receive native dialog results on the main thread without touching editor data yet. */
bool document_controller_event(DocumentController *controller, PUIView *view,
                               const SDL_Event *event);
/** @brief Apply queued file I/O between frames and refresh Explorer only on successful saves. */
void document_controller_commit(DocumentController *controller, EditorHost *host,
                                ProjectController *projects, PUIView *view, PUIContext *input);
#endif

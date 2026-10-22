/** @file
 * @brief App-wide Save As dialogs and file opening via editor contracts. */
#ifndef PENGUENGINE_DOCUMENT_CONTROLLER_H
#define PENGUENGINE_DOCUMENT_CONTROLLER_H
#include "editor_host/host.h"
#include "project/controller.h"

typedef struct {
    /** @brief Native dialog backend, initialized to SDL; replaceable by deterministic tests. */
    bool(SDLCALL *confirm)(const SDL_MessageBoxData *data, int *choice);
    void(SDLCALL *save_dialog)(SDL_DialogFileCallback callback, void *context, SDL_Window *window,
                               const SDL_DialogFileFilter *filters, int count,
                               const char *location);
    Uint32 event;
    bool dialog_pending, importing;
    unsigned artboard;
    char import_root[4096], pending_import[4096];
    const EditorDefinition *owner;
    unsigned document;
    bool close_after_save;
    unsigned pending_close;
    char close_request[64];
    SDL_Window *close_window; ///< Borrowed application window, only used on the main thread.
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

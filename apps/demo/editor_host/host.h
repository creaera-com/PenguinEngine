/** @file
 * @brief Queued editor switching; all mutations are committed between frames. */
#ifndef PE_EDITOR_HOST_H
#define PE_EDITOR_HOST_H
#include "editor.h"
#include "monitor/history.h"
#include "monitor/log.h"

/** @brief Owns the single active editor; definitions contain no inactive runtime resources. */
typedef struct {
    EditorServices services;
    MonitorLog log;         ///< Opt-in project recording survives editor switches.
    ProcessMonitor monitor; ///< Session-wide observations survive unloading individual editors.
    const EditorDefinition *active, *pending;
    void *instance;
    struct EditorCheckpoint
        *checkpoints; ///< Opaque session snapshots; no inactive editor instances.
} EditorHost;

/** @brief Initialize an empty host, borrowing the services' resource root. */
void editor_host_init(EditorHost *host, EditorServices services);
/** @brief Queue an editor ID. Reloading the active ID is explicit via force=true. */
bool editor_host_request(EditorHost *host, const char *id, bool force);
/** @brief Build the candidate and replace shell slot editor_area. On failure keep old editor.
 * Call only between frames. Frees the previous instance after successful replacement.
 */
bool editor_host_commit(EditorHost *host, PUIView *shell, PUIContext *input, char *error,
                        size_t capacity);
/** @brief Forward an editor action through its adapter; false when unhandled. */
bool editor_host_action(EditorHost *host, PUIView *view, const char *name, const char *widget);
bool editor_host_event(EditorHost *host, PUIView *view, const SDL_Event *event);
/** @brief Update active project identity; changing it stops any running recording. */
void editor_host_project(EditorHost *host, const char *root);
/** @brief Record an app/UI action without field values or document contents. */
void editor_host_log_action(EditorHost *host, PUIView *view, const char *name, const char *widget);
/** @brief Release the active instance; unmount its UI before continuing to draw the shell. */
void editor_host_destroy(EditorHost *host);
#endif

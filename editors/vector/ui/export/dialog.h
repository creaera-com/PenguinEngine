/** @file @brief YAML export form, safe native folder picker and deferred raster jobs. */
#ifndef PE_VECTOR_UI_EXPORT_DIALOG_H
#define PE_VECTOR_UI_EXPORT_DIALOG_H
#include "editor_host/editor.h"
#include "editors/vector/export/render.h"
/** @brief Independent ref-counted mailbox outlives the editor if a native dialog returns late. */
typedef struct VectorExportMailbox VectorExportMailbox;

typedef struct {
    SDL_Renderer *renderer;
    PUIView *view; ///< Borrowed while mounted, for deferred status updates.
    VectorExportMailbox *mailbox;
    VectorDocument *request;
    VectorExportPlan plan;
    char directory[4096], stem[96];
    bool initialized, jpeg, transparent;
    int quality;
} VectorExportUI;

/** @brief Compose a module-owned YAML modal trigger into the toolbar; consumes base layout. */
PUILayout *vector_export_ui_layout(const EditorServices *services, PUILayout *base, char *error,
                                   size_t capacity);
/** @brief Release pending value snapshot and owner reference; late dialog callbacks remain safe. */
void vector_export_ui_destroy(VectorExportUI *ui);
/** @brief Initialize fields once and receive native folder results on the main thread. */
void vector_export_ui_sync(VectorExportUI *ui, const VectorDocument *document, PUIView *view);
/** @brief Snapshot validated committed artwork when Export is pressed. */
bool vector_export_ui_action(VectorExportUI *ui, const VectorDocument *document, PUIView *view,
                             const char *action);
/** @brief Execute a pending job when the editor receives its renderer, preserving renderer state.
 */
void vector_export_ui_render(VectorExportUI *ui, SDL_Renderer *renderer, VectorTextRenderer *text,
                             VectorImageRenderer *images);
#endif

/** @file @brief Private Vector editor lifecycle and panel assembly. */
#ifndef PE_VECTOR_INTERNAL_H
#define PE_VECTOR_INTERNAL_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
#include "editors/vector/image/render.h"
#include "editors/vector/rendering/artwork.h"
#include "editors/vector/text/render.h"
#include "editors/vector/ui/context_menu/object_actions.h"
#include "editors/vector/ui/document_tree/document_tree.h"
#include "editors/vector/ui/inspector/inspector.h"
#include "editors/vector/ui/tools/repeat.h"
#include "ui/export/dialog.h"
#include "workspace/documents.h"
#include "workspace/style.h"

typedef struct {
    PUILayout *layout;
    VectorWorkspaceStyle workspace_style;
    VectorArtworkRenderer artwork;
    PUIFonts *fonts;
    VectorTextRenderer *text_renderer;
    VectorImageRenderer *image_renderer;
    VectorDocuments documents;
    VectorDocumentTabs document_tabs;
    VectorDocumentTree document_tree;
    VectorInspector inspector;
    VectorRepeatUI repeat_ui;
    VectorExportUI export_ui;
    unsigned grid_document_id;
    unsigned active_document_id; ///< Resolved during UI commit for shared preview canvas routing.
    VectorObjectActions object_actions;
} VectorEditor;

void *vector_editor_create(const EditorServices *services, char *error, size_t capacity);
const PUILayout *vector_editor_layout(const void *instance);
void vector_editor_destroy(void *instance);
/** @brief Route UI actions; structural changes are deferred until commit_ui. */
bool vector_editor_action(void *instance, PUIView *view, const char *name, const char *widget);
/** @brief Apply document UI requests between frames. */
bool vector_editor_commit_ui(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity);
void vector_editor_canvas(void *instance, SDL_Renderer *renderer, PUIRect area, const char *widget);
bool vector_editor_event(void *instance, PUIView *view, const SDL_Event *event);
#endif

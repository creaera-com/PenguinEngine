/** @file @brief Vector editor lifecycle and routing to document, canvas and history modules. */
#include "artboard/tool.h"
#include "curve/tool.h"
#include "document/selection.h"
#include "editors/vector/text/tool.h"
#include "editors/vector/ui/inspector/inspector.h"
#include "editors/vector/ui/toolbar/toolbar.h"
#include "editors/vector/ui/tools/grid.h"
#include "gradient/tool.h"
#include "internal.h"
#include "preview/render.h"
#include "preview/tool.h"
#include "workspace/box_tool.h"
#include "workspace/canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *vector_editor_create(const EditorServices *services, char *error, size_t capacity) {
    VectorEditor *editor = calloc(1, sizeof *editor);
    if (!editor) {
        snprintf(error, capacity, "could not allocate Vector editor");
        return NULL;
    }
    if (!vector_workspace_style_load(&editor->workspace_style, services->resource_root, error,
                                     capacity)) {
        free(editor);
        return NULL;
    }
    editor->palette_ui = (VectorPaletteUI){.base = 0x5b9bd5ff, .spread = 30};
    editor->repeat_ui.property = VECTOR_REPEAT_ANGLE;
    PUILayout *root = editor_load_ui(services, "editors/vector/ui/editor.yaml", error, capacity);
    if (!root) {
        free(editor);
        return NULL;
    }
    PUILayout *inspector = vector_inspector_load(services, error, capacity);
    if (inspector) {
        editor->layout = pui_layout_compose(root, "inspector_content", inspector, error, capacity);
    }
    if (editor->layout) {
        PUILayout *repeat =
            editor_load_ui(services, "editors/vector/ui/tools/repeat.yaml", error, capacity);
        PUILayout *combined =
            repeat ? pui_layout_compose(editor->layout, "repeat_content", repeat, error, capacity)
                   : NULL;
        pui_layout_free(editor->layout);
        pui_layout_free(repeat);
        editor->layout = combined;
    }
    if (editor->layout) {
        PUILayout *palette =
            editor_load_ui(services, "editors/vector/ui/palette/palette.yaml", error, capacity);
        PUILayout *combined = palette ? pui_layout_compose(editor->layout, "palette_content",
                                                           palette, error, capacity)
                                      : NULL;
        pui_layout_free(editor->layout);
        pui_layout_free(palette);
        editor->layout = combined;
    }
    if (editor->layout) {
        editor->layout = vector_export_ui_layout(services, editor->layout, error, capacity);
    }
    pui_layout_free(inspector);
    pui_layout_free(root);
    vector_documents_init(&editor->documents);
    if (!editor->layout) {
        vector_inspector_destroy(&editor->inspector);
        vector_artwork_renderer_destroy(&editor->artwork);
        vector_image_renderer_destroy(editor->image_renderer);
        vector_text_renderer_destroy(editor->text_renderer);
        pui_fonts_destroy(editor->fonts);
        pui_layout_free(editor->layout);
        free(editor);
        return NULL;
    }
    editor->image_renderer = vector_image_renderer_create();
    editor->fonts = pui_fonts_create(services->resource_root);
    editor->text_renderer = editor->fonts ? vector_text_renderer_create(editor->fonts) : NULL;
    if (!editor->image_renderer || !editor->text_renderer ||
        !vector_inspector_init(&editor->inspector, services, editor->fonts, editor->text_renderer,
                               error, capacity)) {
        if (!editor->text_renderer) {
            snprintf(error, capacity,
                     "could not load bundled fonts; build the editor resource target");
        }
        vector_editor_destroy(editor);
        return NULL;
    }
    return editor;
}

const PUILayout *vector_editor_layout(const void *instance) {
    const VectorEditor *editor = instance;
    return editor->layout;
}

void vector_editor_destroy(void *instance) {
    VectorEditor *editor = instance;
    if (editor) {
        vector_export_ui_destroy(&editor->export_ui);
        vector_workspace_cancel_inactive(&editor->documents, NULL);
        vector_documents_destroy(&editor->documents);
        vector_inspector_destroy(&editor->inspector);
        vector_artwork_renderer_destroy(&editor->artwork);
        vector_image_renderer_destroy(editor->image_renderer);
        vector_text_renderer_destroy(editor->text_renderer);
        pui_fonts_destroy(editor->fonts);
        pui_layout_free(editor->layout);
        free(editor);
    }
}

bool vector_editor_action(void *instance, PUIView *view, const char *name, const char *widget) {
    VectorEditor *editor = instance;
    if (vector_palette_ui_action(&editor->palette_ui, view, name)) {
        return true;
    }
    vector_text_tool_end(vector_document_tabs_active(&editor->documents, view));
    if (!strncmp(name, "vector.objects.", 15) || !strcmp(name, "vector.tool_selection") ||
        !strcmp(name, "vector.add_rect") || !strcmp(name, "vector.add_ellipse") ||
        !strcmp(name, "vector.add_polygon") || !strcmp(name, "vector.add_text") ||
        !strcmp(name, "vector.add_artboard") || !strcmp(name, "vector.tool_pen") ||
        !strcmp(name, "vector.tool_nodes") || !strcmp(name, "editor.undo") ||
        !strcmp(name, "editor.redo")) {
        vector_inspector_cancel_color(&editor->inspector, &editor->documents, view);
    }
    if (vector_inspector_action(&editor->inspector, &editor->documents, view, name)) {
        return true;
    }
    if (!strcmp(name, "vector.add_document")) {
        vector_document_tabs_request(&editor->document_tabs, &editor->documents, view);
        return true;
    }
    VectorDocument *document = vector_document_tabs_active(&editor->documents, view);
    if (!strcmp(name, "vector.artboard.center")) {
        if (document) {
            vector_canvas_center(document);
        }
        return true;
    }
    if (vector_export_ui_action(&editor->export_ui, document, view, name)) {
        return true;
    }
    if (vector_repeat_ui_action(&editor->repeat_ui, document, view, name)) {
        return true;
    }
    if (vector_preview_action(document, view, name)) {
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_object_actions_action(&editor->object_actions, document, view, name)) {
        return true;
    }
    if (vector_toolbar_action(view, document, name)) {
        return true;
    }
    if (!strcmp(name, "vector.element.snap")) {
        if (document) {
            document->element_snap_enabled = !document->element_snap_enabled;
        }
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_grid_action(document, view, name)) {
        return true;
    }
    if (vector_gradient_tool_action(document, view, name)) {
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_curve_tool_action(document, view, name)) {
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_rectangle_tool_action(document, view, name)) {
        vector_toolbar_sync(view, document);
        return true;
    }
    if (!strcmp(name, "vector.tool_selection")) {
        if (document) {
            vector_artboard_tool_cancel(document);
        }
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_document_tree_action(document, view, name, widget)) {
        return true;
    }
    if (!strcmp(name, "vector.add_artboard") || !strcmp(name, "editor.undo") ||
        !strcmp(name, "editor.redo")) {
        if (!document) {
            vector_toolbar_sync(view, NULL);
            pui_view_set_text(view, "status", "Zuerst ein Dokument anlegen oder auswaehlen");
            return true;
        }
        if (!strcmp(name, "vector.add_artboard")) {
            if (document->artboard_tool.enabled) {
                vector_artboard_tool_cancel(document);
            } else {
                vector_artboard_tool_arm(document);
            }
            vector_toolbar_sync(view, document);
            pui_view_set_text(view, "status",
                              document->artboard_tool.enabled
                                  ? "Artboard aufziehen (linke Maustaste), Esc: abbrechen"
                                  : "Auswahlwerkzeug aktiv");
            return true;
        }
        vector_artboard_tool_cancel(document);
        vector_toolbar_sync(view, document);
        bool changed = !strcmp(name, "editor.undo")
                           ? vector_history_undo(&document->history, document)
                           : vector_history_redo(&document->history, document);
        if (changed) {
            vector_canvas_center(document);
        }
        pui_view_set_text(view, "status",
                          changed ? "Dokument aktualisiert" : "Keine Aenderung moeglich");
        return true;
    }
    return false;
}

void vector_editor_canvas(void *instance, SDL_Renderer *renderer, PUIRect area,
                          const char *widget) {
    VectorEditor *editor = instance;
    vector_export_ui_render(&editor->export_ui, renderer, editor->text_renderer,
                            editor->image_renderer);
    if (!strcmp(widget, "palette_wheel")) {
        vector_palette_ui_draw(&editor->palette_ui, renderer, area);
        return;
    }
    if (!strcmp(widget, "vector_preview_canvas")) {
        vector_preview_draw(
            vector_inspector_preview_document(
                &editor->inspector,
                vector_documents_find(&editor->documents, editor->active_document_id)),
            renderer, area, editor->text_renderer, editor->image_renderer, &editor->artwork);
        return;
    }
    for (int i = 0; i < editor->documents.count; i++) {
        VectorDocument *document = &editor->documents.items[i];
        char id[96];
        snprintf(id, sizeof id, "vector_document_%u.canvas", document->id);
        if (!strcmp(id, widget)) {
            editor->active_document_id = document->id;
            vector_workspace_cancel_inactive(&editor->documents, document);
            vector_canvas_draw(vector_inspector_preview_document(&editor->inspector, document),
                               renderer, area, editor->text_renderer, editor->image_renderer,
                               &editor->workspace_style, &editor->artwork);
            return;
        }
    }
}

bool vector_editor_event(void *instance, PUIView *view, const SDL_Event *event) {
    VectorEditor *editor = instance;
    if (vector_palette_ui_event(&editor->palette_ui, view, event)) {
        return true;
    }
    VectorDocument *document = vector_document_tabs_active(&editor->documents, view);
    vector_workspace_cancel_inactive(&editor->documents, document);
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
        (event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) &&
        (event->key.key == SDLK_Z || event->key.key == SDLK_Y)) {
        bool redo = event->key.key == SDLK_Y || (event->key.mod & SDL_KMOD_SHIFT);
        return vector_editor_action(instance, view, redo ? "editor.redo" : "editor.undo", "");
    }
    bool consumed = vector_preview_event(document, view, event) ||
                    vector_text_tool_event(document, editor->text_renderer, view, event) ||
                    vector_curve_tool_event(document, view, event) ||
                    vector_gradient_tool_event(document, view, event) ||
                    vector_object_actions_event(&editor->object_actions, document, view, event) ||
                    (document && vector_canvas_event(document, view, event));
    vector_toolbar_sync(view, document);
    return consumed;
}

bool vector_editor_commit_ui(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity) {
    VectorEditor *editor = instance;
    if (!vector_document_tabs_commit(&editor->document_tabs, &editor->documents, view, input, error,
                                     capacity)) {
        return false;
    }
    VectorDocument *active = vector_document_tabs_active(&editor->documents, view);
    editor->active_document_id = active ? active->id : 0;
    vector_export_ui_sync(&editor->export_ui, active, view);
    vector_workspace_cancel_inactive(&editor->documents, active);
    if (active) {
        vector_document_selection_sync(active);
    }
    if (!vector_inspector_sync(&editor->inspector, active, view, input, error, capacity)) {
        return false;
    }
    vector_toolbar_sync(view, active);
    vector_grid_sync(view, active, &editor->grid_document_id);
    vector_repeat_ui_sync(&editor->repeat_ui, active, view);
    if (!vector_document_tree_sync(&editor->document_tree, active, view, error, capacity)) {
        return false;
    }
    vector_object_actions_commit(&editor->object_actions, active, view);
    vector_document_tabs_sync(&editor->documents, view);
    vector_inspector_preview_sync(&editor->inspector, active, view);
    return true;
}

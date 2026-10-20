/** @file @brief Drag-to-create tool; intermediate previews never enter history. */
#include "tool.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/document/document.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/preview/tool.h"
#include "editors/vector/text/tool.h"
#include "editors/vector/workspace/box_tool.h"
#include "editors/vector/workspace/modifiers.h"
#include "editors/vector/workspace/selection.h"
#include <math.h>
#include <stdio.h>

void vector_artboard_tool_cancel(VectorDocument *document) {
    vector_preview_cancel(document);
    vector_text_tool_end(document);
    vector_curve_tool_cancel(document);
    vector_selection_cancel(document);
    vector_rectangle_tool_cancel(document);
    if (document->artboard_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    document->artboard_tool = (VectorArtboardTool){0};
}

void vector_artboard_tool_arm(VectorDocument *document) {
    vector_artboard_tool_cancel(document);
    document->viewport.panning = false;
    document->artboard_tool.enabled = true;
}

PUIRect vector_artboard_tool_rect(const VectorArtboardTool *tool) {
    return vector_draw_constrained(tool->start_x, tool->start_y, tool->end_x, tool->end_y,
                                   tool->keep_aspect, tool->from_center, NULL);
}

static void update_endpoint(VectorDocument *document, float x, float y) {
    const VectorViewport *viewport = &document->viewport;
    document->artboard_tool.end_x = roundf((x - viewport->bounds.x - viewport->x) / viewport->zoom);
    document->artboard_tool.end_y = roundf((y - viewport->bounds.y - viewport->y) / viewport->zoom);
}

bool vector_artboard_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorArtboardTool *tool = &document->artboard_tool;
    if (!tool->enabled) {
        return false;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        vector_artboard_tool_cancel(document);
        pui_view_set_text(view, "status", "Artboard-Werkzeug abgebrochen");
        return true;
    }
    bool modifier = vector_transform_modifiers(event, &tool->keep_aspect, &tool->from_center);
    if (tool->dragging && modifier) {
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        char id[96];
        snprintf(id, sizeof id, "vector_document_%u.canvas", document->id);
        if (!document->viewport.panning &&
            pui_view_hit(view, id, event->button.x, event->button.y)) {
            update_endpoint(document, event->button.x, event->button.y);
            tool->start_x = tool->end_x;
            tool->start_y = tool->end_y;
            tool->dragging = true;
            SDL_CaptureMouse(true);
            return true;
        }
    } else if (tool->dragging && event->type == SDL_EVENT_MOUSE_MOTION) {
        update_endpoint(document, event->motion.x, event->motion.y);
        return true;
    } else if (tool->dragging && event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
               event->button.button == SDL_BUTTON_LEFT) {
        update_endpoint(document, event->button.x, event->button.y);
        PUIRect rect = vector_artboard_tool_rect(tool);
        tool->dragging = false;
        SDL_CaptureMouse(false);
        if (rect.w < 1 || rect.h < 1) {
            pui_view_set_text(view, "status", "Artboard mit gedrueckter Maustaste aufziehen");
        } else if (vector_artboard_add(document, rect)) {
            tool->enabled = false;
            pui_view_set_text(view, "status", "Artboard angelegt");
        } else {
            pui_view_set_text(view, "status", "Artboard konnte nicht angelegt werden");
        }
        return true;
    }
    /* Keep the viewport stable while the mouse defines document-space geometry. */
    return tool->dragging &&
           (event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
}

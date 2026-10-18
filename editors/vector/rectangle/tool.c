/** @file @brief Artboard-local rectangle creation with clipped drag preview. */
#include "tool.h"
#include "editors/vector/artboard/tool.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void vector_rectangle_tool_cancel(VectorDocument *document) {
    document->viewport.panning = false;
    if (document->rectangle_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    document->rectangle_tool = (VectorRectangleTool){0};
}

bool vector_rectangle_tool_action(VectorDocument *document, PUIView *view, const char *action) {
    if (strcmp(action, "vector.add_rect")) {
        return false;
    }
    if (!document) {
        pui_view_set_text(view, "status", "Zuerst ein Dokument und Artboard anlegen");
        return true;
    }
    bool enabled = document->rectangle_tool.enabled;
    vector_artboard_tool_cancel(document);
    document->rectangle_tool.enabled = !enabled;
    document->viewport.panning = false;
    pui_view_set_text(view, "status",
                      enabled ? "Auswahlwerkzeug aktiv"
                              : "Rechteck in einem entsperrten Artboard aufziehen; Esc: abbrechen");
    return true;
}

PUIRect vector_rectangle_tool_rect(const VectorRectangleTool *tool) {
    return (PUIRect){fminf(tool->start_x, tool->end_x), fminf(tool->start_y, tool->end_y),
                     fabsf(tool->end_x - tool->start_x), fabsf(tool->end_y - tool->start_y)};
}

static void endpoint(VectorDocument *document, VectorArtboard *parent, float x, float y) {
    VectorViewport *v = &document->viewport;
    document->rectangle_tool.end_x = fmaxf(
        0, fminf(parent->rect.w, roundf((x - v->bounds.x - v->x) / v->zoom - parent->rect.x)));
    document->rectangle_tool.end_y = fmaxf(
        0, fminf(parent->rect.h, roundf((y - v->bounds.y - v->y) / v->zoom - parent->rect.y)));
}

bool vector_rectangle_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorRectangleTool *tool = &document->rectangle_tool;
    if (!tool->enabled) {
        return false;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        vector_rectangle_tool_cancel(document);
        pui_view_set_text(view, "status", "Rechteck-Werkzeug abgebrochen");
        return true;
    }
    VectorArtboard *parent = vector_artboard_find(document, tool->parent_id);
    if (tool->dragging && (!parent || parent->flags & VECTOR_OBJECT_STATE_MASK)) {
        vector_rectangle_tool_cancel(document);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT &&
        !tool->dragging) {
        char canvas[96];
        snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", document->id);
        if (document->viewport.panning ||
            !pui_view_hit(view, canvas, event->button.x, event->button.y)) {
            return false;
        }
        VectorViewport *v = &document->viewport;
        float x = (event->button.x - v->bounds.x - v->x) / v->zoom,
              y = (event->button.y - v->bounds.y - v->y) / v->zoom;
        // The topmost artboard wins; an overlapping locked board is not drawn through.
        for (int i = document->artboard_count - 1; i >= 0; i--) {
            parent = &document->artboards[i];
            if (parent->flags & VECTOR_OBJECT_HIDDEN) {
                continue;
            }
            PUIRect r = parent->rect;
            if (x < r.x || x >= r.x + r.w || y < r.y || y >= r.y + r.h) {
                continue;
            }
            if (parent->flags & VECTOR_OBJECT_LOCKED) {
                break;
            }
            tool->parent_id = parent->id;
            endpoint(document, parent, event->button.x, event->button.y);
            tool->start_x = tool->end_x;
            tool->start_y = tool->end_y;
            tool->dragging = true;
            SDL_CaptureMouse(true);
            return true;
        }
        pui_view_set_text(view, "status", "Ein sichtbares, entsperrtes Artboard verwenden");
        return true;
    }
    if (tool->dragging && event->type == SDL_EVENT_MOUSE_MOTION) {
        endpoint(document, parent, event->motion.x, event->motion.y);
        return true;
    }
    if (tool->dragging && event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
        event->button.button == SDL_BUTTON_LEFT) {
        endpoint(document, parent, event->button.x, event->button.y);
        PUIRect rect = vector_rectangle_tool_rect(tool);
        tool->dragging = false;
        SDL_CaptureMouse(false);
        bool added = vector_rectangle_add(document, parent->id, rect);
        if (added) {
            tool->enabled = false;
        }
        pui_view_set_text(view, "status",
                          added ? "Rechteck angelegt"
                                : "Rechteck konnte nicht angelegt werden (mindestens 1 x 1 px)");
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        return true;
    }
    return tool->dragging &&
           (event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
}

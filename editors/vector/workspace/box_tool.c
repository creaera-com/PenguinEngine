/** @file @brief Shared artboard-local box gesture for rectangle, ellipse and polygon factories. */
#include "box_tool.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/polygon/polygon.h"
#include "editors/vector/workspace/modifiers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void vector_rectangle_tool_cancel(VectorDocument *document) {
    document->fill_preview = (VectorFillPreview){0};
    document->viewport.panning = false;
    if (document->rectangle_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    document->rectangle_tool = (VectorRectangleTool){0};
}

bool vector_rectangle_tool_action(VectorDocument *document, PUIView *view, const char *action) {
    bool polygon = !strcmp(action, "vector.add_polygon");
    bool ellipse = !strcmp(action, "vector.add_ellipse");
    if (!polygon && !ellipse && strcmp(action, "vector.add_rect")) {
        return false;
    }
    if (!document) {
        pui_view_set_text(view, "status", "Zuerst ein Dokument und Artboard anlegen");
        return true;
    }
    bool enabled = document->rectangle_tool.enabled &&
                   document->rectangle_tool.ellipse == ellipse &&
                   document->rectangle_tool.polygon == polygon;
    vector_artboard_tool_cancel(document);
    document->rectangle_tool.enabled = !enabled;
    document->rectangle_tool.ellipse = ellipse;
    document->rectangle_tool.polygon = polygon;
    document->viewport.panning = false;
    pui_view_set_text(
        view, "status",
        enabled   ? "Auswahlwerkzeug aktiv"
        : polygon ? "Polygon aufziehen; Shift: gleiche Achsen; Alt: Mittelpunkt; Esc: abbrechen"
        : ellipse ? "Ellipse aufziehen; Shift: Kreis; Alt: Mittelpunkt; Esc: abbrechen"
                  : "Rechteck in einem entsperrten Artboard aufziehen; Esc: abbrechen");
    return true;
}

PUIRect vector_rectangle_tool_rect(const VectorRectangleTool *tool) {
    PUIRect bounds = {0, 0, tool->parent_width, tool->parent_height};
    return vector_draw_constrained(tool->start_x, tool->start_y, tool->end_x, tool->end_y,
                                   tool->keep_aspect, tool->from_center, &bounds);
}

static void endpoint(VectorDocument *document, VectorArtboard *parent, float x, float y) {
    VectorViewport *v = &document->viewport;
    float local_x = roundf((x - v->bounds.x - v->x) / v->zoom - parent->rect.x);
    float local_y = roundf((y - v->bounds.y - v->y) / v->zoom - parent->rect.y);
    vector_grid_snap_point(&document->grid, v->zoom, &local_x, &local_y);
    document->rectangle_tool.end_x = local_x;
    document->rectangle_tool.end_y = local_y;
    document->rectangle_tool.parent_width = parent->rect.w;
    document->rectangle_tool.parent_height = parent->rect.h;
}

bool vector_rectangle_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorRectangleTool *tool = &document->rectangle_tool;
    if (!tool->enabled) {
        return false;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        vector_rectangle_tool_cancel(document);
        pui_view_set_text(view, "status", "Zeichenwerkzeug abgebrochen");
        return true;
    }
    VectorArtboard *parent = vector_artboard_find(document, tool->parent_id);
    if (tool->dragging && (!parent || parent->flags & VECTOR_OBJECT_STATE_MASK)) {
        vector_rectangle_tool_cancel(document);
        return true;
    }
    bool modifier = vector_transform_modifiers(event, &tool->keep_aspect, &tool->from_center);
    if (tool->dragging && modifier) {
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
            tool->start_x = tool->end_x = fmaxf(0, fminf(parent->rect.w, tool->end_x));
            tool->start_y = tool->end_y = fmaxf(0, fminf(parent->rect.h, tool->end_y));
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
        bool added = tool->polygon   ? vector_polygon_add(document, parent->id, rect, 6)
                     : tool->ellipse ? vector_ellipse_add(document, parent->id, rect)
                                     : vector_rectangle_add(document, parent->id, rect);
        pui_view_set_text(view, "status",
                          added
                              ? (tool->polygon   ? "Polygon angelegt; weitere aufziehen oder Esc"
                                 : tool->ellipse ? "Ellipse angelegt; weitere aufziehen oder Esc"
                                                 : "Rechteck angelegt; weiteres aufziehen oder Esc")
                              : "Form konnte nicht angelegt werden (mindestens 1 x 1 px)");
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        return true;
    }
    return tool->dragging &&
           (event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
}

/** @file @brief Transient label drags become one history command on release. */
#include "selection.h"
#include <math.h>
#include <stdio.h>

void vector_selection_cancel(VectorDocument *document) {
    if (document->selection_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    document->selection_tool = (VectorSelectionTool){0};
}

PUIRect vector_artboard_display_rect(const VectorDocument *document, const VectorArtboard *board) {
    PUIRect rect = board->rect;
    const VectorSelectionTool *tool = &document->selection_tool;
    if (tool->dragging && tool->artboard_id == board->id) {
        rect.x += tool->offset_x;
        rect.y += tool->offset_y;
    }
    return rect;
}

PUIRect vector_artboard_label_rect(const VectorDocument *document, const VectorArtboard *board) {
    PUIRect rect = vector_artboard_display_rect(document, board);
    const VectorViewport *v = &document->viewport;
    return (PUIRect){v->bounds.x + v->x + rect.x * v->zoom,
                     v->bounds.y + v->y + rect.y * v->zoom - 30,
                     fmaxf(24, pui_text_width(board->name) * .875f), PUI_FONT_HEIGHT * .875f + 4};
}

static bool contains(PUIRect rect, float x, float y) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

static void update(VectorDocument *document, float x, float y) {
    VectorSelectionTool *tool = &document->selection_tool;
    tool->offset_x = roundf((x - tool->start_x) / document->viewport.zoom);
    tool->offset_y = roundf((y - tool->start_y) / document->viewport.zoom);
}

bool vector_selection_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorSelectionTool *tool = &document->selection_tool;
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        bool consumed = tool->dragging;
        vector_selection_cancel(document);
        return consumed;
    }
    if (document->artboard_tool.enabled) {
        return false;
    }
    if (tool->dragging) {
        if (event->type == SDL_EVENT_MOUSE_MOTION) {
            update(document, event->motion.x, event->motion.y);
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
            update(document, event->button.x, event->button.y);
            bool changed = false;
            for (int i = 0; i < document->artboard_count; i++) {
                VectorArtboard *board = &document->artboards[i];
                if (board->id == tool->artboard_id) {
                    changed =
                        vector_artboard_move(document, board->id, board->rect.x + tool->offset_x,
                                             board->rect.y + tool->offset_y);
                    break;
                }
            }
            vector_selection_cancel(document);
            pui_view_set_text(view, "status",
                              changed ? "Artboard positioniert"
                                      : "Artboard konnte nicht verschoben werden");
            return true;
        }
        return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
    }
    char canvas[96];
    snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", document->id);
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT ||
        document->viewport.panning ||
        !pui_view_hit(view, canvas, event->button.x, event->button.y)) {
        return false;
    }
    float x = event->button.x, y = event->button.y;
    // Labels sit above their boards and take priority over board surfaces.
    for (int i = document->artboard_count - 1; i >= 0; i--) {
        VectorArtboard *board = &document->artboards[i];
        if (contains(vector_artboard_label_rect(document, board), x, y)) {
            document->active_artboard = board->id;
            *tool = (VectorSelectionTool){
                .dragging = true, .artboard_id = board->id, .start_x = x, .start_y = y};
            SDL_CaptureMouse(true);
            return true;
        }
    }
    x = (x - document->viewport.bounds.x - document->viewport.x) / document->viewport.zoom;
    y = (y - document->viewport.bounds.y - document->viewport.y) / document->viewport.zoom;
    document->active_artboard = 0;
    for (int i = document->artboard_count - 1; i >= 0; i--) {
        if (contains(document->artboards[i].rect, x, y)) {
            document->active_artboard = document->artboards[i].id;
            break;
        }
    }
    return true;
}

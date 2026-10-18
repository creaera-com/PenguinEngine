/** @file @brief Live rectangle selection by visible-area intersection. */
#include "marquee.h"
#include "editors/vector/document/selection.h"
#include "selection.h"
#include <math.h>

void vector_marquee_begin(VectorDocument *document, float x, float y, bool additive,
                          unsigned board) {
    document->selection_tool =
        (VectorSelectionTool){.dragging = false,
                              .marquee = true,
                              .additive = additive,
                              .start_x = x,
                              .start_y = y,
                              .before = document->selection,
                              .previous_board = document->active_artboard,
                              .previous_rectangle = document->active_rectangle};
    if (!additive) {
        vector_document_select(document, 0);
        document->active_artboard = board;
    }
    SDL_CaptureMouse(true);
}

static void update(VectorDocument *document, float x, float y) {
    VectorSelectionTool *tool = &document->selection_tool;
    tool->offset_x = x - tool->start_x;
    tool->offset_y = y - tool->start_y;
    tool->marquee_moved |= hypotf(tool->offset_x, tool->offset_y) >= 3;
    tool->dragging = tool->marquee_moved;
    if (!tool->marquee_moved) {
        return;
    }
    if (tool->additive) {
        document->selection = tool->before;
        document->active_artboard = tool->previous_board;
        document->active_rectangle = tool->previous_rectangle;
    } else {
        vector_document_select(document, 0);
        document->active_artboard = 0;
    }
    const VectorViewport *v = &document->viewport;
    float left = (fminf(x, tool->start_x) - v->bounds.x - v->x) / v->zoom;
    float top = (fminf(y, tool->start_y) - v->bounds.y - v->y) / v->zoom;
    float right = left + fabsf(tool->offset_x) / v->zoom;
    float bottom = top + fabsf(tool->offset_y) / v->zoom;
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorRectangle *rectangle = &document->rectangles[i];
        if (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK) {
            continue;
        }
        VectorArtboard *board = vector_artboard_find(document, rectangle->parent_id);
        float rx = board->rect.x + fmaxf(0, rectangle->rect.x);
        float ry = board->rect.y + fmaxf(0, rectangle->rect.y);
        float rr = board->rect.x + fminf(board->rect.w, rectangle->rect.x + rectangle->rect.w);
        float rb = board->rect.y + fminf(board->rect.h, rectangle->rect.y + rectangle->rect.h);
        if (rr > rx && rb > ry && rx < right && ry < bottom && rr > left && rb > top &&
            !vector_document_is_selected(document, rectangle->id)) {
            vector_document_toggle(document, rectangle->id);
        }
    }
}

bool vector_marquee_event(VectorDocument *document, const SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        update(document, event->motion.x, event->motion.y);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        update(document, event->button.x, event->button.y);
        SDL_CaptureMouse(false);
        document->selection_tool.marquee = false;
        vector_selection_cancel(document);
        return true;
    }
    return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

void vector_marquee_draw(const VectorDocument *document, SDL_Renderer *renderer) {
    const VectorSelectionTool *tool = &document->selection_tool;
    if (!tool->marquee || !tool->marquee_moved) {
        return;
    }
    SDL_FRect rect = {fminf(tool->start_x, tool->start_x + tool->offset_x),
                      fminf(tool->start_y, tool->start_y + tool->offset_y), fabsf(tool->offset_x),
                      fabsf(tool->offset_y)};
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 194, 142, 92, 35);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
    SDL_RenderRect(renderer, &rect);
    SDL_SetRenderDrawBlendMode(renderer, blend);
}

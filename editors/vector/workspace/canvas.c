/** @file @brief Light-gray document workspace with independent pan and zoom. */
#include "canvas.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/curve/render.h"
#include "editors/vector/document/document.h"
#include "editors/vector/grid/render.h"
#include "editors/vector/rectangle/render.h"
#include "editors/vector/workspace/box_tool.h"
#include "editors/vector/workspace/selection.h"
#include "element_snap.h"
#include "marquee.h"
#include "ruler.h"
#include <stdio.h>
#include <stdlib.h>

PUILayout *vector_canvas_layout(char *error, size_t capacity) {
    PUILayout *layout = calloc(1, sizeof *layout);
    if (!layout) {
        snprintf(error, capacity, "could not allocate document canvas");
        return NULL;
    }
    layout->count = 1;
    layout->root = 0;
    layout->nodes[0] = (PUINode){.type = PUI_NODE_CANVAS, .child = -1, .next = -1, .enabled = true};
    snprintf(layout->nodes[0].name, sizeof layout->nodes[0].name, "canvas");
    layout->nodes[0].id = pui_id("canvas");
    return layout;
}

void vector_canvas_center(VectorDocument *document) {
    VectorViewport *viewport = &document->viewport;
    if (viewport->bounds.w <= 0 || viewport->bounds.h <= 0) {
        return;
    }
    for (int i = 0; i < document->artboard_count; i++) {
        VectorArtboard *board = &document->artboards[i];
        if (board->id == document->active_artboard) {
            viewport->x =
                viewport->bounds.w / 2 - (board->rect.x + board->rect.w / 2) * viewport->zoom;
            viewport->y =
                viewport->bounds.h / 2 - (board->rect.y + board->rect.h / 2) * viewport->zoom;
        }
    }
}

void vector_canvas_draw(VectorDocument *document, SDL_Renderer *renderer, PUIRect area) {
    VectorViewport *viewport = &document->viewport;
    viewport->bounds = area;
    float pointer_x, pointer_y;
    SDL_GetMouseState(&pointer_x, &pointer_y);
    bool pointer_on_canvas = SDL_GetMouseFocus() == SDL_GetRenderWindow(renderer) &&
                             pui_contains(area, pointer_x, pointer_y);
    SDL_SetRenderDrawColor(renderer, 104, 104, 104, 255);
    SDL_FRect background = {area.x, area.y, area.w, area.h};
    SDL_RenderFillRect(renderer, &background);
    for (int i = 0; i < document->artboard_count; i++) {
        VectorArtboard *board = &document->artboards[i];
        if (board->flags & VECTOR_OBJECT_HIDDEN) {
            continue;
        }
        PUIRect display = vector_artboard_display_rect(document, board);
        SDL_FRect rect = {area.x + viewport->x + display.x * viewport->zoom,
                          area.y + viewport->y + display.y * viewport->zoom,
                          display.w * viewport->zoom, display.h * viewport->zoom};
        PUIRect label = vector_artboard_label_rect(document, board);
        pui_text_scaled(renderer, label.x, label.y, board->name,
                        board->id == document->active_artboard ? 0xE2AF7EFF : 0xEEEEEEFF, .875f);
        vector_ruler_draw(renderer, rect, viewport->zoom);
        if (pointer_on_canvas) {
            vector_ruler_draw_pointer(renderer, rect, pointer_x, pointer_y);
        }
        SDL_SetRenderDrawColor(renderer, 242, 242, 242, 255);
        SDL_RenderFillRect(renderer, &rect);
        vector_rectangles_draw(document, board, renderer, rect);
        vector_grid_draw(&document->grid, renderer, rect, viewport->zoom);
        if (board->id == document->active_artboard && !document->active_rectangle) {
            SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
            SDL_RenderRect(renderer, &rect);
        }
    }
    vector_element_guides_draw(document, renderer);
    if (!document->curve_tool.pen && !document->curve_tool.nodes) {
        vector_selection_draw_handles(document, renderer);
    }
    vector_curve_tool_draw(document, renderer);
    vector_marquee_draw(document, renderer);
    if (document->artboard_tool.dragging) {
        PUIRect preview = vector_artboard_tool_rect(&document->artboard_tool);
        SDL_FRect rect = {area.x + viewport->x + preview.x * viewport->zoom,
                          area.y + viewport->y + preview.y * viewport->zoom,
                          preview.w * viewport->zoom, preview.h * viewport->zoom};
        SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
        SDL_RenderRect(renderer, &rect);
        char size[64];
        snprintf(size, sizeof size, "%.0f x %.0f px", preview.w, preview.h);
        pui_text_scaled(renderer, rect.x + 4, rect.y + 4, size, 0xFFFFFFFF, .875f);
    }
}

bool vector_canvas_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    if (vector_rectangle_tool_event(document, view, event) ||
        vector_artboard_tool_event(document, view, event)) {
        return true;
    }
    VectorViewport *viewport = &document->viewport;
    char id[96];
    snprintf(id, sizeof id, "vector_document_%u.canvas", document->id);
    if (vector_selection_event(document, view, event)) {
        return true;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        viewport->panning = false;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_MIDDLE &&
               pui_view_hit(view, id, event->button.x, event->button.y)) {
        viewport->panning = true;
        viewport->mouse_x = event->button.x;
        viewport->mouse_y = event->button.y;
        return true;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
               event->button.button == SDL_BUTTON_MIDDLE) {
        bool was_panning = viewport->panning;
        viewport->panning = false;
        return was_panning;
    } else if (event->type == SDL_EVENT_MOUSE_MOTION && viewport->panning) {
        viewport->x += event->motion.x - viewport->mouse_x;
        viewport->y += event->motion.y - viewport->mouse_y;
        viewport->mouse_x = event->motion.x;
        viewport->mouse_y = event->motion.y;
        return true;
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL &&
               pui_view_hit(view, id, event->wheel.mouse_x, event->wheel.mouse_y)) {
        float factor = event->wheel.y > 0 ? 1.1f : event->wheel.y < 0 ? 1 / 1.1f : 1;
        if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
            factor = 1 / factor;
        }
        float zoom = viewport->zoom * factor;
        zoom = zoom < .1f ? .1f : zoom > 4 ? 4 : zoom;
        float x = event->wheel.mouse_x - viewport->bounds.x;
        float y = event->wheel.mouse_y - viewport->bounds.y;
        viewport->x = x - (x - viewport->x) * zoom / viewport->zoom;
        viewport->y = y - (y - viewport->y) * zoom / viewport->zoom;
        viewport->zoom = zoom;
        return true;
    }
    return false;
}

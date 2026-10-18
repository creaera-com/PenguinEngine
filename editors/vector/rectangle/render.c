/** @file @brief Render rectangle children and creation previews inside their artboard. */
#include "render.h"
#include "editors/vector/workspace/selection.h"
#include "tool.h"
#include <math.h>

static void draw(SDL_Renderer *renderer, SDL_FRect board, float zoom, PUIRect local, bool selected,
                 bool preview) {
    SDL_FRect rect = {board.x + local.x * zoom, board.y + local.y * zoom, local.w * zoom,
                      local.h * zoom};
    SDL_SetRenderDrawColor(renderer, 115, 147, 175, preview ? 150 : 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, selected || preview ? 194 : 66, selected || preview ? 142 : 89,
                           selected || preview ? 92 : 109, 255);
    SDL_RenderRect(renderer, &rect);
}

void vector_rectangles_draw(VectorDocument *document, const VectorArtboard *parent,
                            SDL_Renderer *renderer, SDL_FRect board) {
    bool clipped = SDL_RenderClipEnabled(renderer);
    SDL_Rect outer, clip = {0};
    SDL_GetRenderClipRect(renderer, &outer);
    // Clamp in floating point before conversion so a distant board cannot overflow SDL_Rect.
    int width, height;
    SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
    float x = fmaxf(0, board.x), y = fmaxf(0, board.y), right = fminf(width, board.x + board.w),
          bottom = fminf(height, board.y + board.h);
    if (right <= x || bottom <= y) {
        return;
    }
    SDL_Rect requested = {(int)ceilf(x), (int)ceilf(y), (int)fmaxf(0, floorf(right) - ceilf(x)),
                          (int)fmaxf(0, floorf(bottom) - ceilf(y))};
    if (clipped) {
        if (!SDL_GetRectIntersection(&outer, &requested, &clip)) {
            return;
        }
    } else {
        clip = requested;
    }
    SDL_SetRenderClipRect(renderer, &clip);
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorRectangle *rectangle = &document->rectangles[i];
        if (rectangle->parent_id != parent->id || rectangle->flags & VECTOR_OBJECT_HIDDEN) {
            continue;
        }
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_display_rect(document, rectangle),
             document->active_rectangle == rectangle->id, false);
    }
    if (document->rectangle_tool.dragging && document->rectangle_tool.parent_id == parent->id) {
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_tool_rect(&document->rectangle_tool), true, true);
    }
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

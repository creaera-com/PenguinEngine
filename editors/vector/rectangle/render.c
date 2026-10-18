/** @file @brief Render rectangle children and creation previews inside their artboard. */
#include "render.h"
#include "editors/vector/workspace/selection.h"
#include "tool.h"
#include <math.h>

static void draw(SDL_Renderer *renderer, SDL_FRect board, float zoom, PUIRect local, bool selected,
                 bool preview, uint32_t fill) {
    SDL_FRect rect = {board.x + local.x * zoom, board.y + local.y * zoom, local.w * zoom,
                      local.h * zoom};
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    unsigned alpha = fill & 255u;
    if (preview) {
        alpha = alpha * 150 / 255;
    }
    SDL_SetRenderDrawColor(renderer, fill >> 24, (fill >> 16) & 255, (fill >> 8) & 255,
                           (Uint8)alpha);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, selected || preview ? 194 : 66, selected || preview ? 142 : 89,
                           selected || preview ? 92 : 109,
                           selected || preview ? 255 : (Uint8)alpha);
    SDL_RenderRect(renderer, &rect);
    SDL_SetRenderDrawBlendMode(renderer, blend);
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
        uint32_t fill =
            document->fill_preview.active && document->fill_preview.rectangle_id == rectangle->id
                ? document->fill_preview.color
                : rectangle->fill;
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_display_rect(document, rectangle),
             document->active_rectangle == rectangle->id, false, fill);
    }
    if (document->rectangle_tool.dragging && document->rectangle_tool.parent_id == parent->id) {
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_tool_rect(&document->rectangle_tool), true, true,
             VECTOR_RECTANGLE_DEFAULT_FILL);
    }
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

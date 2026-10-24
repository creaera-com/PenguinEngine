/** @file @brief Rectangular and pointy-top hexagonal drawing guides. */
#include "render.h"
#include <math.h>

void vector_grid_draw(const VectorGrid *grid, SDL_Renderer *renderer, SDL_FRect board, float zoom) {
    float w = grid->width * zoom, h = grid->height * zoom;
    if (!grid->visible || !isfinite(w) || !isfinite(h) || w < 4 || h < 4) {
        return;
    }
    SDL_Rect old_clip;
    bool clipped = SDL_RenderClipEnabled(renderer);
    SDL_GetRenderClipRect(renderer, &old_clip);
    SDL_Rect viewport;
    SDL_GetRenderViewport(renderer, &viewport);
    SDL_Rect bounds = {(int)floorf(board.x), (int)floorf(board.y),
                       (int)ceilf(board.x + board.w) - (int)floorf(board.x),
                       (int)ceilf(board.y + board.h) - (int)floorf(board.y)};
    SDL_Rect clip;
    if (!SDL_GetRectIntersection(&bounds, clipped ? &old_clip : &viewport, &clip)) {
        return;
    }
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    uint32_t color = grid->color;
    SDL_SetRenderDrawColor(renderer, color >> 24, color >> 16, color >> 8, color);
    if (grid->type == VECTOR_GRID_STANDARD) {
        float start_x = board.x + ceilf((clip.x - board.x) / w) * w;
        float start_y = board.y + ceilf((clip.y - board.y) / h) * h;
        for (float x = start_x; x < clip.x + clip.w; x += w) {
            SDL_RenderLine(renderer, x, clip.y, x, clip.y + clip.h);
        }
        for (float y = start_y; y < clip.y + clip.h; y += h) {
            SDL_RenderLine(renderer, clip.x, y, clip.x + clip.w, y);
        }
    } else if (grid->type == VECTOR_GRID_ISOMETRIC) {
        float slope = h / w;
        for (int direction = -1; direction <= 1; direction += 2) {
            float m = direction * slope;
            float left = clip.x - board.x, right = left + clip.w;
            float top = clip.y - board.y, bottom = top + clip.h;
            float low = fminf(top - m * left, top - m * right);
            float high = fmaxf(bottom - m * left, bottom - m * right);
            int first = (int)ceilf(low / h), last = (int)floorf(high / h);
            for (int line = first; line <= last; line++) {
                float offset = line * h;
                SDL_RenderLine(renderer, clip.x, board.y + m * left + offset, clip.x + clip.w,
                               board.y + m * right + offset);
            }
        }
    } else {
        /* Staggered rows; shared edges are emitted only once for uniform alpha. */
        int first = (int)floorf((clip.y - board.y) / (.75f * h)) - 1;
        int last = (int)ceilf((clip.y + clip.h - board.y) / (.75f * h));
        for (int row = first; row <= last; row++) {
            float shift = (row % 2 != 0) ? w / 2 : 0;
            int left = (int)floorf((clip.x - board.x - shift) / w) - 1;
            int right = (int)ceilf((clip.x + clip.w - board.x - shift) / w);
            float y = board.y + row * .75f * h;
            for (int col = left; col <= right; col++) {
                float x = board.x + shift + col * w;
                SDL_FPoint edges[] = {
                    {x, y + h * .75f}, {x, y + h * .25f}, {x + w / 2, y}, {x + w, y + h * .25f}};
                SDL_RenderLines(renderer, edges, 4);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, blend);
    SDL_SetRenderClipRect(renderer, clipped ? &old_clip : NULL);
}

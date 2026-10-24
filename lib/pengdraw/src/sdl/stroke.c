#include "pengdraw/pengdraw.h"
/** @file @brief A non-overlapping triangle ring supports alpha, rotation and thick solid strokes.
 */
#include <math.h>

void pdraw_stroke_draw(SDL_Renderer *renderer, PDrawRect rect, float angle, float zoom,
                       PDrawStroke stroke) {
    if (stroke.width <= 0 || !(stroke.color & 255u)) {
        return;
    }
    float width = stroke.width * zoom;
    float outset = stroke.alignment == PDRAW_STROKE_OUTSIDE  ? width
                   : stroke.alignment == PDRAW_STROKE_CENTER ? width / 2
                                                             : 0;
    float inset = width - outset;
    PDrawRect outer = {rect.x - outset, rect.y - outset, rect.w + 2 * outset, rect.h + 2 * outset};
    // Collapse the inner hole at the center when the stroke consumes the entire fill.
    float inner_width = fmaxf(0, rect.w - 2 * inset);
    float inner_height = fmaxf(0, rect.h - 2 * inset);
    PDrawRect inner = {rect.x + (rect.w - inner_width) / 2, rect.y + (rect.h - inner_height) / 2,
                       inner_width, inner_height};
    PDrawRect corners[8];
    pdraw_rotated_corners(outer, angle, corners);
    pdraw_rotated_corners(inner, angle, corners + 4);
    uint32_t color = stroke.color;
    SDL_FColor rgba = {(color >> 24) / 255.f, ((color >> 16) & 255) / 255.f,
                       ((color >> 8) & 255) / 255.f, (color & 255) / 255.f};
    SDL_Vertex vertices[8];
    int indices[24];
    for (int i = 0; i < 8; i++) {
        vertices[i] = (SDL_Vertex){.position = {corners[i].x, corners[i].y}, .color = rgba};
    }
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        int *side = indices + i * 6;
        side[0] = i;
        side[1] = next;
        side[2] = next + 4;
        side[3] = i;
        side[4] = next + 4;
        side[5] = i + 4;
    }
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(renderer, &previous);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, NULL, vertices, 8, indices, 24);
    SDL_SetRenderDrawBlendMode(renderer, previous);
}

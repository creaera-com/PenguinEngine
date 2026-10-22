#include "pengdraw/pengdraw.h"
/** @file @brief Tessellated fill and contour bands preserve holes, alpha and document-pixel width.
 */

typedef struct {
    SDL_Renderer *renderer;
    SDL_FRect board;
    float zoom;
    SDL_FColor color;
} Draw;

static SDL_FColor color(uint32_t value) {
    return (SDL_FColor){(value >> 24) / 255.f, ((value >> 16) & 255) / 255.f,
                        ((value >> 8) & 255) / 255.f, (value & 255) / 255.f};
}

static void triangle(void *context, const PDrawPoint points[3]) {
    Draw *draw = context;
    SDL_Vertex vertices[3];
    for (int i = 0; i < 3; i++) {
        vertices[i] = (SDL_Vertex){.position = {draw->board.x + points[i].x * draw->zoom,
                                                draw->board.y + points[i].y * draw->zoom},
                                   .color = draw->color};
    }
    SDL_RenderGeometry(draw->renderer, NULL, vertices, 3, NULL, 0);
}

void pdraw_path_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect, float angle,
                     SDL_FRect board, float zoom, uint32_t fill, uint32_t outline) {
    PDrawPath path = pdraw_shape_path(shape, rect, angle);
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(renderer, &previous);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Draw draw = {renderer, board, zoom, color(fill)};
    pdraw_path_triangles(&path, triangle, &draw);
    if (shape->stroke.width > 0 && (shape->stroke.color & 255)) {
        PDrawPath outline;
        int placement = shape->stroke.alignment == PDRAW_STROKE_INSIDE    ? -1
                        : shape->stroke.alignment == PDRAW_STROKE_OUTSIDE ? 1
                                                                          : 0;
        if (pdraw_shape_outline(shape, rect, angle, shape->stroke.width, placement, &outline)) {
            draw.color = color(shape->stroke.color);
            pdraw_path_triangles(&outline, triangle, &draw);
        }
    }
    if (outline) {
        SDL_SetRenderDrawColor(renderer, outline >> 24, outline >> 16, outline >> 8, outline);
        for (unsigned c = 0, first = 0; c < path.contours; first = path.ends[c++]) {
            SDL_FPoint points[PDRAW_PATH_POINTS + 1];
            unsigned count = path.ends[c] - first;
            for (unsigned i = 0; i <= count; i++) {
                PDrawPoint point = path.points[first + i % count];
                points[i] = (SDL_FPoint){board.x + point.x * zoom, board.y + point.y * zoom};
            }
            SDL_RenderLines(renderer, points, (int)count + 1);
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, previous);
}

#include "pengdraw/pengdraw.h"
#include <math.h>

static void line(SDL_Renderer *r, PDrawPoint a, PDrawPoint b, float width, uint32_t packed) {
    float length = hypotf(b.x - a.x, b.y - a.y);
    if (length <= 0 || width <= 0) {
        return;
    }
    float nx = -(b.y - a.y) * width / (2 * length), ny = (b.x - a.x) * width / (2 * length);
    SDL_FColor color = {(packed >> 24) / 255.f, ((packed >> 16) & 255) / 255.f,
                        ((packed >> 8) & 255) / 255.f, (packed & 255) / 255.f};
    SDL_Vertex v[4] = {{{a.x + nx, a.y + ny}, color, {0, 0}},
                       {{b.x + nx, b.y + ny}, color, {0, 0}},
                       {{b.x - nx, b.y - ny}, color, {0, 0}},
                       {{a.x - nx, a.y - ny}, color, {0, 0}}};
    const int indices[] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, NULL, v, 4, indices, 6);
}

void pdraw_curve_draw(SDL_Renderer *r, const PDrawShape *s, PDrawRect rect, float angle,
                      SDL_FRect board, float zoom, uint32_t fill, uint32_t outline) {
    if (s->curve.closed) {
        pdraw_path_draw(r, s, rect, angle, board, zoom, fill, outline);
        return;
    }
    PDrawPath path = pdraw_shape_path(s, rect, angle);
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(r, &previous);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (unsigned i = 1; i < path.count; i++) {
        PDrawPoint a = {board.x + path.points[i - 1].x * zoom,
                        board.y + path.points[i - 1].y * zoom};
        PDrawPoint b = {board.x + path.points[i].x * zoom, board.y + path.points[i].y * zoom};
        if (s->stroke.width > 0) {
            line(r, a, b, s->stroke.width * zoom, s->stroke.color);
        }
        if (outline) {
            line(r, a, b, 1, outline);
        }
    }
    SDL_SetRenderDrawBlendMode(r, previous);
}

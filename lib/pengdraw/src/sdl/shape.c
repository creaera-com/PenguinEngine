/** @file @brief Immediate shape painting from geometry values, independent of editor selection. */
#include "pengdraw/pengdraw.h"
#include <math.h>

void pdraw_shape_draw(SDL_Renderer *r, const PDrawShape *shape, PDrawRect rect, float angle,
                      SDL_FRect origin, float zoom, uint32_t fill) {
    if (!r || !shape || rect.w <= 0 || rect.h <= 0 || zoom <= 0 || !isfinite(rect.x) ||
        !isfinite(rect.y) || !isfinite(rect.w) || !isfinite(rect.h) || !isfinite(angle) ||
        !isfinite(zoom) || !isfinite(origin.x) || !isfinite(origin.y)) {
        return;
    }
    if (pdraw_gradient_draw(r, shape, rect, angle, origin, zoom)) {
        fill = 0;
    }
    if (shape->ellipse) {
        pdraw_ellipse_draw(r, shape, rect, angle, origin, zoom, fill, 0);
    } else if (shape->curve.count) {
        pdraw_curve_draw(r, shape, rect, angle, origin, zoom, fill, 0);
    } else if (shape->path.count || shape->corner_radius > 0) {
        pdraw_path_draw(r, shape, rect, angle, origin, zoom, fill, 0);
    } else {
        SDL_BlendMode blend;
        SDL_GetRenderDrawBlendMode(r, &blend);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_FColor color = {(fill >> 24) / 255.f, ((fill >> 16) & 255) / 255.f,
                            ((fill >> 8) & 255) / 255.f, (fill & 255) / 255.f};
        PDrawRect box = {origin.x + rect.x * zoom, origin.y + rect.y * zoom, rect.w * zoom,
                         rect.h * zoom};
        if (pdraw_angle_normalize(angle) == 0) {
            SDL_SetRenderDrawColor(r, fill >> 24, fill >> 16, fill >> 8, fill);
            SDL_RenderFillRect(r, &(SDL_FRect){box.x, box.y, box.w, box.h});
        } else {
            PDrawRect corners[4];
            pdraw_rotated_corners(box, angle, corners);
            SDL_Vertex vertices[4];
            for (int i = 0; i < 4; i++) {
                vertices[i] =
                    (SDL_Vertex){.position = {corners[i].x, corners[i].y}, .color = color};
            }
            const int indices[] = {0, 1, 2, 0, 2, 3};
            SDL_RenderGeometry(r, NULL, vertices, 4, indices, 6);
        }
        pdraw_stroke_draw(r, box, angle, zoom, shape->stroke);
        SDL_SetRenderDrawBlendMode(r, blend);
    }
}

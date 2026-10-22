#include "pengdraw/pengdraw.h"
/** @file @brief Ellipse contours bypass polygon-operation limits to retain smooth borders. */
#include <math.h>

static SDL_FColor color(uint32_t value) {
    return (SDL_FColor){(value >> 24) / 255.f, ((value >> 16) & 255) / 255.f,
                        ((value >> 8) & 255) / 255.f, (value & 255) / 255.f};
}

static SDL_FPoint point(float cx, float cy, float x, float y, float cosine, float sine) {
    return (SDL_FPoint){cx + cosine * x - sine * y, cy + sine * x + cosine * y};
}

void pdraw_ellipse_draw(SDL_Renderer *r, const PDrawShape *shape, PDrawRect rect, float angle,
                        SDL_FRect board, float zoom, uint32_t fill, uint32_t outline) {
    float a = rect.w * zoom / 2, b = rect.h * zoom / 2;
    if (a <= 0 || b <= 0) {
        return;
    }
    float cx = board.x + (rect.x + rect.w / 2) * zoom, cy = board.y + (rect.y + rect.h / 2) * zoom;
    float cosine = cosf(angle * .017453293f), sine = sinf(angle * .017453293f);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    /* Keep the chord error below roughly 0.15 raster pixels as zoom increases. */
    int segments = (int)fminf(1024, fmaxf(128, ceilf(3.141592654f * sqrtf(fmaxf(a, b) / .3f))));
    SDL_FPoint contour[1025];
    float width = shape->stroke.width * zoom;
    float inward = shape->stroke.alignment == PDRAW_STROKE_OUTSIDE  ? 0
                   : shape->stroke.alignment == PDRAW_STROKE_INSIDE ? width
                                                                    : width / 2;
    float outward = shape->stroke.alignment == PDRAW_STROKE_INSIDE    ? 0
                    : shape->stroke.alignment == PDRAW_STROKE_OUTSIDE ? width
                                                                      : width / 2;
    for (int i = 0; i < segments; i++) {
        float t = i * 6.283185307f / segments, u = (i + 1) * 6.283185307f / segments;
        float x[2] = {cosf(t) * a, cosf(u) * a}, y[2] = {sinf(t) * b, sinf(u) * b};
        SDL_FPoint p = point(cx, cy, x[0], y[0], cosine, sine),
                   q = point(cx, cy, x[1], y[1], cosine, sine);
        contour[i] = p;
        if (fill & 255u) {
            SDL_FColor tint = color(fill);
            SDL_Vertex vertices[3] = {
                {{cx, cy}, tint, {0, 0}}, {p, tint, {0, 0}}, {q, tint, {0, 0}}};
            SDL_RenderGeometry(r, NULL, vertices, 3, NULL, 0);
        }
        if (width > 0 && (shape->stroke.color & 255u)) {
            SDL_Vertex vertices[4];
            SDL_FColor tint = color(shape->stroke.color);
            for (int j = 0; j < 2; j++) {
                float nx = x[j] / (a * a), ny = y[j] / (b * b), length = hypotf(nx, ny);
                nx /= length;
                ny /= length;
                SDL_FPoint outer =
                    point(cx, cy, x[j] + nx * outward, y[j] + ny * outward, cosine, sine);
                SDL_FPoint inner = inward >= fminf(a, b) ? (SDL_FPoint){cx, cy}
                                                         : point(cx, cy, x[j] - nx * inward,
                                                                 y[j] - ny * inward, cosine, sine);
                vertices[j] = (SDL_Vertex){outer, tint, {0, 0}};
                vertices[3 - j] = (SDL_Vertex){inner, tint, {0, 0}};
            }
            const int indices[] = {0, 1, 2, 0, 2, 3};
            SDL_RenderGeometry(r, NULL, vertices, 4, indices, 6);
        }
    }
    if (outline) {
        contour[segments] = contour[0];
        SDL_SetRenderDrawColor(r, outline >> 24, outline >> 16, outline >> 8, outline);
        SDL_RenderLines(r, contour, segments + 1);
    }
    SDL_SetRenderDrawBlendMode(r, blend);
}

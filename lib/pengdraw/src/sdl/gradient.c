#include "pengdraw/pengdraw.h"
/** @file @brief Bound gradient rasterization to the visible contour and map it through triangles.
 */
#include <math.h>

typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_FRect board;
    PDrawRect crop;
    float zoom;
    bool ok;
} Paint;

static float edge_distance(SDL_FPoint p, PDrawRect crop, int edge) {
    return edge == 0   ? p.x - crop.x
           : edge == 1 ? crop.x + crop.w - p.x
           : edge == 2 ? p.y - crop.y
                       : crop.y + crop.h - p.y;
}

static void triangle(void *context, const PDrawPoint points[3]) {
    Paint *p = context;
    SDL_FPoint polygon[12], clipped[12];
    int count = 3;
    for (int i = 0; i < 3; i++) {
        polygon[i] =
            (SDL_FPoint){p->board.x + points[i].x * p->zoom, p->board.y + points[i].y * p->zoom};
    }
    /* Clipping geometry first keeps texture UVs in range for partially visible objects. */
    for (int edge = 0; edge < 4 && count; edge++) {
        int out = 0;
        for (int i = 0; i < count; i++) {
            SDL_FPoint a = polygon[i], b = polygon[(i + 1) % count];
            float da = edge_distance(a, p->crop, edge), db = edge_distance(b, p->crop, edge);
            if (da >= 0) {
                clipped[out++] = a;
            }
            if ((da >= 0) != (db >= 0)) {
                float t = da / (da - db);
                clipped[out++] = (SDL_FPoint){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
            }
        }
        count = out;
        for (int i = 0; i < count; i++) {
            polygon[i] = clipped[i];
        }
    }
    if (count < 3) {
        return;
    }
    SDL_Vertex vertices[12];
    int indices[30];
    for (int i = 0; i < count; i++) {
        float u = (polygon[i].x - p->crop.x) / p->crop.w;
        float v = (polygon[i].y - p->crop.y) / p->crop.h;
        vertices[i] = (SDL_Vertex){.position = polygon[i],
                                   .color = {1, 1, 1, 1},
                                   .tex_coord = {fmaxf(0, fminf(1, u)), fmaxf(0, fminf(1, v))}};
    }
    for (int i = 0; i < count - 2; i++) {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }
    p->ok &= SDL_RenderGeometry(p->renderer, p->texture, vertices, count, indices, (count - 2) * 3);
}

bool pdraw_gradient_draw(SDL_Renderer *r, const PDrawShape *s, PDrawRect rect, float angle,
                         SDL_FRect board, float zoom) {
    if (!s->gradient.kind || !pdraw_gradient_valid(&s->gradient) ||
        (s->curve.count && !s->curve.closed)) {
        return false;
    }
    PDrawPath path = pdraw_shape_path(s, rect, angle);
    if (!path.count) {
        return false;
    }
    PDrawRect bounds = pdraw_path_bounds(&path);
    float left = board.x + bounds.x * zoom, top = board.y + bounds.y * zoom;
    float right = left + bounds.w * zoom, bottom = top + bounds.h * zoom;
    int ow, oh;
    SDL_GetCurrentRenderOutputSize(r, &ow, &oh);
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    if (sx <= 0 || sy <= 0) {
        return false;
    }
    left = fmaxf(0, floorf(left));
    top = fmaxf(0, floorf(top));
    right = fminf(ow / sx, ceilf(right));
    bottom = fminf(oh / sy, ceilf(bottom));
    if (SDL_RenderClipEnabled(r)) {
        SDL_Rect clip;
        SDL_GetRenderClipRect(r, &clip);
        left = fmaxf(left, clip.x);
        top = fmaxf(top, clip.y);
        right = fminf(right, clip.x + clip.w);
        bottom = fminf(bottom, clip.y + clip.h);
    }
    if (right <= left || bottom <= top) {
        return true;
    }
    float scale = fminf(1, 2048.f / fmaxf((right - left) * sx, (bottom - top) * sy));
    int w = (int)ceilf((right - left) * sx * scale), h = (int)ceilf((bottom - top) * sy * scale);
    SDL_Surface *pixels = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!pixels) {
        return false;
    }
    float cosine = cosf(angle * .017453293f), sine = sinf(angle * .017453293f);
    for (int y = 0; y < h; y++) {
        unsigned char *line = (unsigned char *)pixels->pixels + y * pixels->pitch;
        for (int x = 0; x < w; x++) {
            float px =
                ((left + (x + .5f) * (right - left) / w) - board.x) / zoom - rect.x - rect.w / 2;
            float py =
                ((top + (y + .5f) * (bottom - top) / h) - board.y) / zoom - rect.y - rect.h / 2;
            float lx = cosine * px + sine * py + rect.w / 2;
            float ly = -sine * px + cosine * py + rect.h / 2;
            uint32_t color = pdraw_gradient_sample(&s->gradient, lx, ly, rect.w, rect.h);
            line[4 * x] = color >> 24;
            line[4 * x + 1] = color >> 16;
            line[4 * x + 2] = color >> 8;
            line[4 * x + 3] = color;
        }
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, pixels);
    SDL_DestroySurface(pixels);
    if (!texture) {
        return false;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    Paint paint = {r, texture, board, {left, top, right - left, bottom - top}, zoom, true};
    bool ok = pdraw_path_triangles(&path, triangle, &paint);
    SDL_DestroyTexture(texture);
    return ok && paint.ok;
}

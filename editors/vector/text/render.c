/** @file @brief Rasterize Unicode glyphs into a bounded, cached frame texture. */
#include "render.h"
#include "editors/vector/geometry/rotation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define FRAME_CACHE 8

typedef struct {
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    VectorShape shape;
    VectorTextLayout layout;
    unsigned age;
} Frame;

struct VectorTextRenderer {
    PUIFonts *fonts;
    Frame frames[FRAME_CACHE];
    unsigned age;
};

typedef struct {
    PUIFonts *fonts;
    int face;
} Metrics;

static float advance(void *context, unsigned code, unsigned next, float size) {
    Metrics *m = context;
    return pui_fonts_advance(m->fonts, m->face, code, next, size);
}

static int face(VectorTextRenderer *r, const VectorText *t) {
    return pui_fonts_style(r->fonts, pui_fonts_find(r->fonts, t->font, t->face), t->bold != 0,
                           t->italic != 0);
}

VectorTextRenderer *vector_text_renderer_create(PUIFonts *fonts) {
    VectorTextRenderer *r = calloc(1, sizeof *r);
    if (r) {
        r->fonts = fonts;
    }
    return r;
}

void vector_text_renderer_destroy(VectorTextRenderer *r) {
    if (!r) {
        return;
    }
    for (int i = 0; i < FRAME_CACHE; i++) {
        SDL_DestroyTexture(r->frames[i].texture);
    }
    free(r);
}

void vector_text_measure(VectorTextRenderer *r, const VectorShape *s, VectorTextLayout *out) {
    Metrics m = {r->fonts, face(r, &s->text)};
    vector_text_layout(&s->text, s->rect.w, s->rect.h, advance, &m, out);
}

static Frame *frame(VectorTextRenderer *r, SDL_Renderer *renderer, const VectorShape *s) {
    Frame *oldest = &r->frames[0];
    for (int i = 0; i < FRAME_CACHE; i++) {
        Frame *f = &r->frames[i];
        if (f->texture && f->renderer == renderer &&
            !memcmp(&s->text, &f->shape.text, sizeof s->text) && s->rect.w == f->shape.rect.w &&
            s->rect.h == f->shape.rect.h) {
            f->age = ++r->age;
            return f;
        }
        if (!f->texture || f->age < oldest->age) {
            oldest = f;
        }
    }
    Frame *f = oldest;
    SDL_DestroyTexture(f->texture);
    *f = (Frame){.renderer = renderer, .shape = *s, .age = ++r->age};
    vector_text_measure(r, s, &f->layout);
    float scale = fminf(2, 2048.f / fmaxf(s->rect.w, s->rect.h));
    int width = (int)ceilf(s->rect.w * scale), height = (int)ceilf(s->rect.h * scale);
    if (width < 1 || height < 1) {
        return f;
    }
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return f;
    }
    SDL_FillSurfaceRect(surface, NULL, 0);
    int index = face(r, &s->text);
    const PUIFontEntry *entry = pui_fonts_entry(r->fonts, index);
    bool faux_bold = s->text.bold && entry && !entry->bold,
         faux_italic = s->text.italic && entry && !entry->italic;
    float ascent = pui_fonts_ascent(r->fonts, index, s->text.size);
    for (int i = 0; i < f->layout.count; i++) {
        const VectorTextGlyph *g = &f->layout.glyphs[i];
        if (g->code == ' ' || g->code == '\t' || g->code == '\n') {
            continue;
        }
        int w, h, ox, oy;
        unsigned char *bitmap =
            pui_fonts_bitmap(r->fonts, index, g->code, s->text.size * scale, &w, &h, &ox, &oy);
        if (!bitmap) {
            continue;
        }
        float column_width =
            (s->rect.w - (s->text.columns - 1) * s->text.column_gap) / s->text.columns;
        int column = (int)floorf(g->x / (column_width + s->text.column_gap));
        column = column < 0                       ? 0
                 : column >= (int)s->text.columns ? (int)s->text.columns - 1
                                                  : column;
        float left = column * (column_width + s->text.column_gap) * scale;
        float right = left + column_width * scale;
        int x = (int)roundf(g->x * scale) + ox, y = (int)roundf((g->y + ascent) * scale) + oy;
        for (int row = 0; row < h; row++) {
            int slant = faux_italic ? (int)roundf((h - row) * .2f) : 0;
            for (int col = 0; col < w; col++) {
                unsigned char alpha = bitmap[row * w + col];
                for (int b = 0; b <= (faux_bold ? 1 : 0); b++) {
                    int px = x + col + slant + b, py = y + row;
                    if (px < 0 || py < 0 || px >= width || py >= height || px < left ||
                        px >= right) {
                        continue;
                    }
                    unsigned char *pixel =
                        (unsigned char *)surface->pixels + py * surface->pitch + px * 4;
                    pixel[0] = pixel[1] = pixel[2] = 255;
                    pixel[3] = (unsigned char)(alpha + pixel[3] * (255 - alpha) / 255);
                }
            }
        }
        pui_fonts_free_bitmap(bitmap);
    }
    f->texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (f->texture) {
        SDL_SetTextureBlendMode(f->texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(f->texture, SDL_SCALEMODE_LINEAR);
    }
    SDL_DestroySurface(surface);
    return f;
}

static SDL_FPoint screen(PUIRect rect, float angle, SDL_FRect board, float zoom, float x, float y) {
    x += rect.x;
    y += rect.y;
    vector_rotate_point(&x, &y, rect.x + rect.w / 2, rect.y + rect.h / 2, angle);
    return (SDL_FPoint){board.x + x * zoom, board.y + y * zoom};
}

void vector_text_draw(VectorTextRenderer *cache, SDL_Renderer *r, const VectorDocument *d,
                      const VectorShape *s, PUIRect rect, float angle, SDL_FRect board, float zoom,
                      uint32_t fill, bool selected) {
    VectorShape displayed = *s;
    displayed.rect = rect;
    Frame *f = frame(cache, r, &displayed);
    SDL_FRect dest = {board.x + rect.x * zoom, board.y + rect.y * zoom, rect.w * zoom,
                      rect.h * zoom};
    if (f->texture) {
        SDL_SetTextureColorMod(f->texture, fill >> 24, (fill >> 16) & 255, (fill >> 8) & 255);
        SDL_SetTextureAlphaMod(f->texture, fill & 255);
        SDL_RenderTextureRotated(r, f->texture, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
    }
    if (selected) {
        SDL_FPoint corners[5];
        corners[0] = screen(rect, angle, board, zoom, 0, 0);
        corners[1] = screen(rect, angle, board, zoom, rect.w, 0);
        corners[2] = screen(rect, angle, board, zoom, rect.w, rect.h);
        corners[3] = screen(rect, angle, board, zoom, 0, rect.h);
        corners[4] = corners[0];
        SDL_SetRenderDrawColor(r, f->layout.overflow ? 230 : 194, f->layout.overflow ? 70 : 142, 92,
                               255);
        SDL_RenderLines(r, corners, 5);
    }
    if (!s->id || d->text_tool.editing_id != s->id) {
        return;
    }
    size_t cursor = d->text_tool.edit.cursor, anchor = d->text_tool.edit.anchor;
    size_t low = cursor < anchor ? cursor : anchor, high = cursor > anchor ? cursor : anchor;
    float x = f->layout.end_x, y = f->layout.end_y;
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < f->layout.count; i++) {
        VectorTextGlyph *g = &f->layout.glyphs[i];
        if (g->byte >= cursor) {
            x = g->x;
            y = g->y;
            break;
        }
    }
    for (int i = 0; i < f->layout.count; i++) {
        VectorTextGlyph *g = &f->layout.glyphs[i];
        if (g->byte < low || g->byte >= high) {
            continue;
        }
        SDL_FPoint p[4] = {screen(rect, angle, board, zoom, g->x, g->y),
                           screen(rect, angle, board, zoom, g->x + g->advance, g->y),
                           screen(rect, angle, board, zoom, g->x + g->advance, g->y + s->text.size),
                           screen(rect, angle, board, zoom, g->x, g->y + s->text.size)};
        SDL_Vertex v[4];
        for (int j = 0; j < 4; j++) {
            v[j] = (SDL_Vertex){p[j], {.76f, .55f, .36f, .3f}, {0, 0}};
        }
        const int indices[] = {0, 1, 2, 0, 2, 3};
        SDL_RenderGeometry(r, NULL, v, 4, indices, 6);
    }
    if ((SDL_GetTicks() / 500) % 2 == 0) {
        SDL_FPoint a = screen(rect, angle, board, zoom, x, y),
                   b = screen(rect, angle, board, zoom, x, fminf(rect.h, y + s->text.size));
        SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
        SDL_RenderLine(r, a.x, a.y, b.x, b.y);
    }
    SDL_SetRenderDrawBlendMode(r, blend);
}

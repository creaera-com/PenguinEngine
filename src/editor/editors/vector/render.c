#include "app/editor_internal.h"
#include "nanosvg.h"
#include "nanosvgrast.h"

static VePoint point(float x, float y) {
    VectorEditor *v = app.vector;
    return (VePoint){v->canvas.x + (x - v->view.x) * v->view.zoom,
                     v->canvas.y + (y - v->view.y) * v->view.zoom};
}

static void line(VePoint a, VePoint b, uint32_t rgba) {
    editor_draw_color(rgba);
    SDL_RenderLine(app.renderer, a.x, a.y, b.x, b.y);
}

static void handle(VePoint p, float size, uint32_t rgba) {
    editor_box((SDL_FRect){p.x - size * .5f - 1, p.y - size * .5f - 1, size + 2, size + 2},
               UI_CANVAS);
    editor_box((SDL_FRect){p.x - size * .5f, p.y - size * .5f, size, size}, rgba);
}

static bool raster(void) {
    VectorEditor *v = app.vector;
    int w = (int)ceilf(v->canvas.w), h = (int)ceilf(v->canvas.h);
    if (w < 1 || h < 1 || w > 8192 || h > 8192) {
        return false;
    }
    if (w != v->width || h != v->height) {
        SDL_DestroyTexture(v->texture);
        v->texture = NULL;
        free(v->pixels);
        v->pixels = NULL;
        v->width = w;
        v->height = h;
        v->pixels = calloc((size_t)w * h, 4);
        v->texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!v->pixels || !v->texture) {
            return false;
        }
        SDL_SetTextureBlendMode(v->texture, SDL_BLENDMODE_BLEND);
        v->render_dirty = true;
    }
    if (!v->render_dirty && v->cached_x == v->view.x && v->cached_y == v->view.y &&
        v->cached_zoom == v->view.zoom) {
        return true;
    }
    if (!v->rasterizer) {
        v->rasterizer = nsvgCreateRasterizer();
    }
    float x, y;
    char *svg = ve_svg(&v->document, false, &x, &y);
    if (!svg || !v->rasterizer) {
        free(svg);
        return false;
    }
    NSVGimage *image = nsvgParse(svg, "px", 96);
    free(svg);
    if (!image) {
        return false;
    }
    memset(v->pixels, 0, (size_t)w * h * 4);
    nsvgRasterize(v->rasterizer, image, (x - v->view.x) * v->view.zoom,
                  (y - v->view.y) * v->view.zoom, v->view.zoom, v->pixels, w, h, w * 4);
    nsvgDelete(image);
    SDL_UpdateTexture(v->texture, NULL, v->pixels, w * 4);
    v->render_dirty = false;
    v->cached_x = v->view.x;
    v->cached_y = v->view.y;
    v->cached_zoom = v->view.zoom;
    return true;
}

static void rulers(SDL_FRect full) {
    VectorEditor *v = app.vector;
    VeBoard *b = &v->document.boards[v->view.board];
    editor_box((SDL_FRect){full.x, full.y, full.w, 22}, UI_SURFACE);
    editor_box((SDL_FRect){full.x, full.y, 22, full.h}, UI_SURFACE);
    float steps[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    float step = 5000;
    for (size_t i = 0; i < sizeof steps / sizeof *steps; i++) {
        if (steps[i] * v->view.zoom >= 10) {
            step = steps[i];
            break;
        }
    }
    float x0 = v->view.x - b->x, y0 = v->view.y - b->y;
    for (int axis = 0; axis < 2; axis++) {
        float begin = axis ? y0 : x0, extent = (axis ? v->canvas.h : v->canvas.w) / v->view.zoom;
        int start = (int)floorf(begin / step), end = (int)ceilf((begin + extent) / step);
        for (int i = start; i <= end && i - start < 2000; i++) {
            float value = i * step;
            VePoint p = point(axis ? b->x : b->x + value, axis ? b->y + value : b->y);
            bool major = i % 5 == 0;
            editor_draw_color(UI_MUTED);
            if (axis) {
                SDL_RenderLine(app.renderer, full.x + 22 - (major ? 10 : 5), p.y, full.x + 22, p.y);
            } else {
                SDL_RenderLine(app.renderer, p.x, full.y + 22 - (major ? 10 : 5), p.x, full.y + 22);
            }
            if (major) {
                char text[32];
                snprintf(text, sizeof text, "%.0f", value);
                if (axis) {
                    editor_text(full.x + 2, p.y + 7, text, UI_MUTED);
                } else {
                    editor_text(p.x + 3, full.y + 8, text, UI_MUTED);
                }
            }
        }
    }
    editor_box((SDL_FRect){full.x, full.y, 22, 22}, UI_PANEL);
    editor_text(full.x + 2, full.y + 9, "px", UI_ACCENT);
}

void vector_draw_canvas(void) {
    VectorEditor *v = app.vector;
    SDL_FRect full = app.viewport;
    editor_box(full, 0x383838ff);
    v->canvas = (SDL_FRect){full.x + 22, full.y + 22, fmaxf(0, full.w - 22), fmaxf(0, full.h - 22)};
    UiRect clip = {v->canvas.x, v->canvas.y, v->canvas.w, v->canvas.h};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    if (raster()) {
        SDL_RenderTexture(app.renderer, v->texture, NULL, &v->canvas);
    }
    for (int b = 0; b < VE_BOARDS; b++) {
        VeBoard *board = &v->document.boards[b];
        if (!board->used) {
            continue;
        }
        VePoint p = point(board->x, board->y);
        SDL_FRect rect = {p.x, p.y, board->w * v->view.zoom, board->h * v->view.zoom};
        editor_draw_color(v->view.board == b ? UI_ACCENT : UI_BORDER);
        SDL_RenderRect(app.renderer, &rect);
        editor_text(p.x, p.y - 10, board->name, v->view.board == b ? UI_ACCENT : UI_MUTED);
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        VeShape *s = &v->document.shapes[i];
        if (!s->used || !(v->view.selected & (UINT64_C(1) << i))) {
            continue;
        }
        VeBoard *b = &v->document.boards[s->board];
        VePoint p = point(s->x + b->x, s->y + b->y);
        SDL_FRect rect = {p.x - 2, p.y - 2, s->w * v->view.zoom + 4, s->h * v->view.zoom + 4};
        editor_draw_color(UI_ACCENT);
        SDL_RenderRect(app.renderer, &rect);
        if (v->view.tool != VT_NODE || s->kind != VE_PATH) {
            handle(p, 5, UI_ACCENT);
            handle((VePoint){p.x + s->w * v->view.zoom, p.y}, 5, UI_ACCENT);
            handle((VePoint){p.x, p.y + s->h * v->view.zoom}, 5, UI_ACCENT);
            handle((VePoint){p.x + s->w * v->view.zoom, p.y + s->h * v->view.zoom}, 5, UI_ACCENT);
            continue;
        }
        VePoint previous = {0};
        for (int n = 0; n < s->count; n++) {
            VeNode *node = &s->nodes[n];
            if (node->command == VE_CLOSE) {
                continue;
            }
            VePoint anchor = point(b->x + node->p.x, b->y + node->p.y);
            if (node->command == VE_CUBIC) {
                VePoint c1 = point(b->x + node->c1.x, b->y + node->c1.y),
                        c2 = point(b->x + node->c2.x, b->y + node->c2.y);
                line(previous, c1, UI_DISABLED);
                line(anchor, c2, UI_DISABLED);
                handle(c1, 4, UI_TEXT);
                handle(c2, 4, UI_TEXT);
            }
            handle(anchor, n == v->view.node ? 8 : 6, n == v->view.node ? UI_ACCENT : UI_TEXT);
            previous = anchor;
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    rulers(full);
}

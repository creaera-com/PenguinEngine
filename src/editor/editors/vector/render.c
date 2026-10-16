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

static void ruler_axis(const VeBoard *board, SDL_FRect strip, float step, int axis) {
    VectorEditor *v = app.vector;
    if (strip.x + strip.w < v->canvas.x || strip.y + strip.h < v->canvas.y ||
        strip.x > v->canvas.x + v->canvas.w || strip.y > v->canvas.y + v->canvas.h) {
        return;
    }
    editor_box(strip, UI_SURFACE);
    ui_clip_push(&app.ui, (UiRect){strip.x, strip.y, strip.w, strip.h});
    ui_apply_clip(app.renderer, &app.ui);
    float length = axis ? board->h : board->w;
    VePoint origin = point(board->x, board->y);
    float screen = axis ? origin.y : origin.x;
    float visible = axis ? v->canvas.y : v->canvas.x;
    float extent = axis ? v->canvas.h : v->canvas.w;
    float begin = fmaxf(0, (visible - screen) / v->view.zoom);
    float end = fminf(length, (visible + extent - screen) / v->view.zoom);
    int start = (int)floorf(begin / step), last = (int)ceilf(end / step);
    for (int i = start; i <= last && i - start < 2000; i++) {
        float value = i * step;
        if (value < 0 || value > length) {
            continue;
        }
        float position = screen + value * v->view.zoom;
        bool major = i % 5 == 0;
        editor_draw_color(UI_MUTED);
        if (axis) {
            SDL_RenderLine(app.renderer, strip.x + strip.w - (major ? 10 : 5), position,
                           strip.x + strip.w, position);
        } else {
            SDL_RenderLine(app.renderer, position, strip.y + strip.h - (major ? 10 : 5), position,
                           strip.y + strip.h);
        }
        if (major) {
            char text[32];
            snprintf(text, sizeof text, "%.0f", value);
            float label_width = ui_text_width(text);
            if (label_width + 4 <= strip.w || axis) {
                float x = axis ? strip.x + 2
                               : fmaxf(strip.x + 2,
                                       fminf(position + 3, strip.x + strip.w - label_width - 2));
                float y = axis ? fminf(position + 8, strip.y + strip.h - 12) : strip.y + 8;
                editor_text(x, y, text, UI_MUTED);
            }
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
}

static void rulers(const VeBoard *board, bool active) {
    VectorEditor *v = app.vector;
    VePoint p = point(board->x, board->y);
    char largest[32];
    snprintf(largest, sizeof largest, "%.0f", board->h);
    float width = fmaxf(30, ui_text_width(largest) + 8), height = 22;
    float steps[] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    float step = 5000;
    for (size_t i = 0; i < sizeof steps / sizeof *steps; i++) {
        if (steps[i] * v->view.zoom >= 10) {
            step = steps[i];
            break;
        }
    }
    ruler_axis(board, (SDL_FRect){p.x, p.y - height, board->w * v->view.zoom, height}, step, 0);
    ruler_axis(board, (SDL_FRect){p.x - width, p.y, width, board->h * v->view.zoom}, step, 1);
    editor_box((SDL_FRect){p.x - width, p.y - height, width, height}, UI_PANEL);
    editor_text(p.x - width + 3, p.y - height + 9, "px", active ? UI_ACCENT : UI_MUTED);
    editor_text(p.x, p.y - height - 10, board->name, active ? UI_ACCENT : UI_MUTED);
}

void vector_draw_canvas(void) {
    VectorEditor *v = app.vector;
    SDL_FRect full = app.viewport;
    editor_box(full, 0x383838ff);
    v->canvas = full;
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
    for (int b = 0; b < VE_BOARDS; b++) {
        if (v->document.boards[b].used) {
            rulers(&v->document.boards[b], v->view.board == b);
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
}

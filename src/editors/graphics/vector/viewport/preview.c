#include "editors/graphics/vector/internal.h"
#include "nanosvg.h"
#include "nanosvgrast.h"

bool vector_preview_selection(VeRegion *r) {
    VectorEditor *v = app.workspace.vector;
    if (!v->view.preview_enabled || v->view.preview_board < 0 ||
        v->view.preview_board >= VE_BOARDS) {
        return false;
    }
    VeBoard *b = &v->document.boards[v->view.preview_board];
    *r = v->view.preview;
    r->x = fminf(b->w, fmaxf(0, r->x));
    r->y = fminf(b->h, fmaxf(0, r->y));
    r->w = fminf(r->w, b->w - r->x);
    r->h = fminf(r->h, b->h - r->y);
    return b->used && r->w >= 1 && r->h >= 1;
}

void vector_draw_preview_selection(void) {
    VectorEditor *v = app.workspace.vector;
    VeRegion r;
    if (!vector_preview_selection(&r)) {
        return;
    }
    VeBoard *b = &v->document.boards[v->view.preview_board];
    SDL_FRect rect = {v->canvas.x + (b->x + r.x - v->view.x) * v->view.zoom,
                      v->canvas.y + (b->y + r.y - v->view.y) * v->view.zoom, r.w * v->view.zoom,
                      r.h * v->view.zoom};
    editor_draw_color(UI_ACCENT);
    SDL_RenderRect(app.platform.renderer, &rect);
    editor_text(rect.x + 3, rect.y + 10, "Preview", UI_ACCENT);
}

static bool same_region(VeRegion a, VeRegion b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static bool raster(VeRegion source) {
    VectorEditor *v = app.workspace.vector;
    int w = (int)source.w, h = (int)source.h;
    if (w < 1 || h < 1 || w > 4096 || h > 4096 || (size_t)w * h > 4 * 1024 * 1024) {
        return false;
    }
    if (w != v->preview_width || h != v->preview_height || !v->preview_texture ||
        !v->preview_pixels) {
        SDL_DestroyTexture(v->preview_texture);
        free(v->preview_pixels);
        v->preview_texture = NULL;
        v->preview_pixels = NULL;
        v->preview_width = w;
        v->preview_height = h;
        v->preview_cache_valid = false;
        v->preview_pixels = calloc((size_t)w * h, 4);
        v->preview_texture = SDL_CreateTexture(app.platform.renderer, SDL_PIXELFORMAT_RGBA32,
                                               SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!v->preview_pixels || !v->preview_texture) {
            return false;
        }
        SDL_SetTextureBlendMode(v->preview_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(v->preview_texture, SDL_SCALEMODE_NEAREST);
    }
    uint64_t revision = v->history.ids[v->history.position];
    if (v->preview_cache_valid && !v->dragging && !v->preview_was_dragging &&
        revision == v->preview_revision && v->preview_cached_board == v->view.preview_board &&
        same_region(source, v->preview_cached_source)) {
        return true;
    }
    VeDocument *isolated = malloc(sizeof *isolated);
    if (!isolated) {
        return false;
    }
    *isolated = v->document;
    int board = v->view.preview_board;
    for (int i = 0; i < VE_BOARDS; i++) {
        isolated->boards[i].used = i == board;
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        isolated->shapes[i].used = isolated->shapes[i].used && isolated->shapes[i].board == board;
    }
    float ox, oy;
    char *svg = ve_svg(isolated, false, &ox, &oy);
    free(isolated);
    if (!svg) {
        return false;
    }
    NSVGimage *image = nsvgParse(svg, "px", 96);
    free(svg);
    if (!v->rasterizer) {
        v->rasterizer = nsvgCreateRasterizer();
    }
    if (!image || !v->rasterizer) {
        nsvgDelete(image);
        return false;
    }
    VeBoard *b = &v->document.boards[board];
    memset(v->preview_pixels, 0, (size_t)w * h * 4);
    nsvgRasterize(v->rasterizer, image, ox - b->x - source.x, oy - b->y - source.y, 1,
                  v->preview_pixels, w, h, w * 4);
    nsvgDelete(image);
    if (!SDL_UpdateTexture(v->preview_texture, NULL, v->preview_pixels, w * 4)) {
        return false;
    }
    v->preview_revision = revision;
    v->preview_cached_board = board;
    v->preview_cached_source = source;
    v->preview_cache_valid = true;
    v->preview_was_dragging = v->dragging;
    return true;
}

void vector_draw_preview(SDL_FRect area) {
    VectorEditor *v = app.workspace.vector;
    editor_box(area, UI_CANVAS);
    VeRegion selected;
    if (!vector_preview_selection(&selected)) {
        editor_text(area.x + 8, area.y + 12, "Select an artboard region", UI_MUTED);
        return;
    }
    char caption[96];
    snprintf(caption, sizeof caption, "1:1 | %.0f x %.0f", selected.w, selected.h);
    editor_text(area.x + 6, area.y + 10, caption, UI_MUTED);
    VeBoard *b = &v->document.boards[v->view.preview_board];
    float cx = v->view.x + v->canvas.w * .5f / v->view.zoom - b->x;
    float cy = v->view.y + v->canvas.h * .5f / v->view.zoom - b->y;
    VeRegion viewport = {area.x + 6, area.y + 24, fmaxf(0, area.w - 12), fmaxf(0, area.h - 30)};
    VePreviewWindow window;
    if (!ve_preview_window(selected, viewport, cx, cy, &window)) {
        return;
    }
    VeRegion target = window.destination;
    ui_clip_push(&app.shell.ui, (UiRect){target.x, target.y, target.w, target.h});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    for (int y = 0; y < (int)target.h; y += 12) {
        for (int x = 0; x < (int)target.w; x += 12) {
            editor_box((SDL_FRect){target.x + x, target.y + y, 12, 12},
                       ((x / 12 + y / 12) & 1) ? UI_SURFACE : UI_PANEL);
        }
    }
    if (raster(window.source)) {
        SDL_FRect destination = {target.x, target.y, target.w, target.h};
        SDL_RenderTexture(app.platform.renderer, v->preview_texture, NULL, &destination);
    } else {
        editor_text(target.x + 3, target.y + 10, "Preview unavailable", UI_MUTED);
    }
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    editor_draw_color(UI_BORDER);
    SDL_RenderRect(app.platform.renderer,
                   &(SDL_FRect){target.x - 1, target.y - 1, target.w + 2, target.h + 2});
}

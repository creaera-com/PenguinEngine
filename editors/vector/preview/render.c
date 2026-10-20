/** @file @brief Reuse shape rendering at zoom one; only the small visible crop is drawn. */
#include "render.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/rectangle/render.h"
#include "editors/vector/workspace/selection.h"
#include "tool.h"
#include <math.h>
#include <stdlib.h>

void vector_preview_draw(const VectorDocument *d, SDL_Renderer *r, PUIRect area,
                         VectorTextRenderer *text, VectorImageRenderer *images) {
    SDL_SetRenderDrawColor(r, 38, 38, 38, 255);
    SDL_RenderFillRect(r, &(SDL_FRect){area.x, area.y, area.w, area.h});
    PUIRect region;
    const VectorArtboard *board = vector_preview_region(d, &region);
    if (!board) {
        pui_text_scaled(r, area.x + 6, area.y + 8, "Bereich im Artboard waehlen", 0xaaaaaaff, .75f);
        return;
    }
    char caption[80];
    snprintf(caption, sizeof caption, "1:1 | %.0f x %.0f px", region.w, region.h);
    pui_text_scaled(r, area.x + 6, area.y + 5, caption, 0xbbbbbbff, .75f);
    const VectorViewport *v = &d->viewport;
    PUIRect display = vector_artboard_display_rect(d, board);
    float cx = (v->bounds.w / 2 - v->x) / v->zoom - display.x;
    float cy = (v->bounds.h / 2 - v->y) / v->zoom - display.y;
    VectorPreviewWindow window;
    if (!vector_preview_window(region, (PUIRect){area.x + 4, area.y + 24, area.w - 8, area.h - 28},
                               cx, cy, &window)) {
        return;
    }
    VectorDocument *copy = malloc(sizeof *copy);
    if (!copy) {
        return;
    }
    *copy = *d; // Borrow history payloads for this read-only draw; never destroy them here.
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *shown = vector_curve_display_shape(d, &d->rectangles[i]);
        copy->rectangles[i] = *shown;
        copy->rectangles[i].gradient = *vector_gradient_display(d, shown);
        copy->rectangles[i].rect = vector_rectangle_display_rect(d, shown);
        copy->rectangles[i].angle = vector_rectangle_display_angle(d, shown);
    }
    copy->selection = (VectorRectangleSelection){0};
    copy->active_rectangle = 0;
    copy->gradient_tool = (VectorGradientTool){0};
    copy->selection_tool = (VectorSelectionTool){0};
    copy->rectangle_tool = (VectorRectangleTool){0};
    copy->curve_tool = (VectorCurveTool){0};
    copy->text_tool = (VectorTextTool){0};
    copy->viewport.zoom = 1;
    SDL_Rect previous, clip = {(int)window.destination.x, (int)window.destination.y,
                               (int)window.destination.w, (int)window.destination.h};
    bool clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderClipRect(r, &previous);
    if (clipped && !SDL_GetRectIntersection(&previous, &clip, &clip)) {
        free(copy);
        return;
    }
    SDL_SetRenderClipRect(r, &clip);
    SDL_SetRenderDrawColor(r, 242, 242, 242, 255);
    SDL_RenderFillRect(r, &(SDL_FRect){window.destination.x, window.destination.y,
                                       window.destination.w, window.destination.h});
    SDL_FRect target = {window.destination.x - window.source.x,
                        window.destination.y - window.source.y, display.w, display.h};
    vector_rectangles_draw(copy, board, r, target, text, images);
    SDL_SetRenderClipRect(r, clipped ? &previous : NULL);
    free(copy);
}

void vector_preview_overlay(const VectorDocument *d, SDL_Renderer *r) {
    PUIRect region;
    const VectorArtboard *b = vector_preview_region(d, &region);
    if (d->preview.dragging) {
        b = NULL;
        for (int i = 0; i < d->artboard_count; i++) {
            if (d->artboards[i].id == d->preview.draft_board) {
                b = &d->artboards[i];
            }
        }
        region = d->preview.draft;
    }
    if (!b) {
        return;
    }
    PUIRect display = vector_artboard_display_rect(d, b);
    const VectorViewport *v = &d->viewport;
    SDL_FRect rect = {v->bounds.x + v->x + (display.x + region.x) * v->zoom,
                      v->bounds.y + v->y + (display.y + region.y) * v->zoom, region.w * v->zoom,
                      region.h * v->zoom};
    pui_render_color(r, PUI_HIGHLIGHT);
    // Corner-independent dashes keep the selection distinct from transform handles.
    for (float x = floorf(fmaxf(0, v->bounds.x - rect.x) / 8) * 8;
         x < fminf(rect.w, v->bounds.x + v->bounds.w - rect.x); x += 8) {
        SDL_RenderLine(r, rect.x + x, rect.y, rect.x + fminf(x + 4, rect.w), rect.y);
        SDL_RenderLine(r, rect.x + x, rect.y + rect.h, rect.x + fminf(x + 4, rect.w),
                       rect.y + rect.h);
    }
    for (float y = floorf(fmaxf(0, v->bounds.y - rect.y) / 8) * 8;
         y < fminf(rect.h, v->bounds.y + v->bounds.h - rect.y); y += 8) {
        SDL_RenderLine(r, rect.x, rect.y + y, rect.x, rect.y + fminf(y + 4, rect.h));
        SDL_RenderLine(r, rect.x + rect.w, rect.y + y, rect.x + rect.w,
                       rect.y + fminf(y + 4, rect.h));
    }
}

/** @file @brief Render rectangle children and creation previews inside their artboard. */
#include "render.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/path/render.h"
#include "editors/vector/workspace/selection.h"
#include "stroke_render.h"
#include "tool.h"
#include <math.h>

static void draw(SDL_Renderer *renderer, SDL_FRect board, float zoom, PUIRect local, bool selected,
                 bool preview, uint32_t fill, float angle, VectorStroke stroke) {
    SDL_FRect rect = {board.x + local.x * zoom, board.y + local.y * zoom, local.w * zoom,
                      local.h * zoom};
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    unsigned alpha = fill & 255u;
    if (preview) {
        alpha = alpha * 150 / 255;
    }
    SDL_SetRenderDrawColor(renderer, fill >> 24, (fill >> 16) & 255, (fill >> 8) & 255,
                           (Uint8)alpha);
    if (vector_angle_normalize(angle) == 0) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        PUIRect corners[4];
        vector_rotated_corners((PUIRect){rect.x, rect.y, rect.w, rect.h}, angle, corners);
        SDL_Vertex vertices[4];
        SDL_FColor color = {(fill >> 24) / 255.f, ((fill >> 16) & 255) / 255.f,
                            ((fill >> 8) & 255) / 255.f, alpha / 255.f};
        for (int i = 0; i < 4; i++) {
            vertices[i] = (SDL_Vertex){.position = {corners[i].x, corners[i].y}, .color = color};
        }
        const int indices[6] = {0, 1, 2, 0, 2, 3};
        SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);
    }
    vector_stroke_draw(renderer, (PUIRect){rect.x, rect.y, rect.w, rect.h}, angle, zoom, stroke);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!selected && !preview) {
        SDL_SetRenderDrawBlendMode(renderer, blend);
        return;
    }
    SDL_SetRenderDrawColor(renderer, selected || preview ? 194 : 66, selected || preview ? 142 : 89,
                           selected || preview ? 92 : 109,
                           selected || preview ? 255 : (Uint8)alpha);
    if (vector_angle_normalize(angle) == 0) {
        SDL_RenderRect(renderer, &rect);
    } else {
        PUIRect corners[4];
        vector_rotated_corners((PUIRect){rect.x, rect.y, rect.w, rect.h}, angle, corners);
        SDL_FPoint points[5];
        for (int i = 0; i < 5; i++) {
            points[i] = (SDL_FPoint){corners[i % 4].x, corners[i % 4].y};
        }
        SDL_RenderLines(renderer, points, 5);
    }
    SDL_SetRenderDrawBlendMode(renderer, blend);
}

void vector_rectangles_draw(VectorDocument *document, const VectorArtboard *parent,
                            SDL_Renderer *renderer, SDL_FRect board) {
    bool clipped = SDL_RenderClipEnabled(renderer);
    SDL_Rect outer, clip = {0};
    SDL_GetRenderClipRect(renderer, &outer);
    // Clamp in floating point before conversion so a distant board cannot overflow SDL_Rect.
    int width, height;
    SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
    float x = fmaxf(0, board.x), y = fmaxf(0, board.y), right = fminf(width, board.x + board.w),
          bottom = fminf(height, board.y + board.h);
    if (right <= x || bottom <= y) {
        return;
    }
    SDL_Rect requested = {(int)ceilf(x), (int)ceilf(y), (int)fmaxf(0, floorf(right) - ceilf(x)),
                          (int)fmaxf(0, floorf(bottom) - ceilf(y))};
    if (clipped) {
        if (!SDL_GetRectIntersection(&outer, &requested, &clip)) {
            return;
        }
    } else {
        clip = requested;
    }
    SDL_SetRenderClipRect(renderer, &clip);
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorRectangle *rectangle = &document->rectangles[i];
        if (rectangle->parent_id != parent->id || rectangle->flags & VECTOR_OBJECT_HIDDEN) {
            continue;
        }
        uint32_t fill =
            document->fill_preview.active && document->fill_preview.rectangle_id == rectangle->id
                ? document->fill_preview.color
                : rectangle->fill;
        if (rectangle->path.count) {
            vector_path_draw(
                renderer, rectangle, vector_rectangle_display_rect(document, rectangle),
                vector_rectangle_display_angle(document, rectangle), board, document->viewport.zoom,
                fill, vector_document_is_selected(document, rectangle->id));
            continue;
        }
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_display_rect(document, rectangle),
             vector_document_is_selected(document, rectangle->id), false, fill,
             vector_rectangle_display_angle(document, rectangle), rectangle->stroke);
    }
    if (document->rectangle_tool.dragging && document->rectangle_tool.parent_id == parent->id) {
        draw(renderer, board, document->viewport.zoom,
             vector_rectangle_tool_rect(&document->rectangle_tool), true, true,
             VECTOR_RECTANGLE_DEFAULT_FILL, 0, (VectorStroke){0});
    }
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

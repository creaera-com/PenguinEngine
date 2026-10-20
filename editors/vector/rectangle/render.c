/** @file @brief Render rectangle children and creation previews inside their artboard. */
#include "render.h"
#include "editors/vector/curve/render.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/ellipse/render.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/mask/render.h"
#include "editors/vector/path/render.h"
#include "editors/vector/polygon/polygon.h"
#include "editors/vector/repeat/render.h"
#include "editors/vector/workspace/box_tool.h"
#include "editors/vector/workspace/selection.h"
#include "stroke_render.h"
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

typedef struct {
    VectorDocument *document;
    VectorShape *rectangle;
    SDL_Renderer *renderer;
    SDL_FRect board;
    VectorTextRenderer *text;
    VectorImageRenderer *images;
    bool instance;
} DrawShape;

static void draw_shape(void *data) {
    DrawShape *context = data;
    VectorDocument *document = context->document;
    VectorShape *rectangle = context->rectangle;
    SDL_Renderer *renderer = context->renderer;
    SDL_FRect board = context->board;
    VectorTextRenderer *text = context->text;
    uint32_t fill =
        document->fill_preview.active && document->fill_preview.rectangle_id == rectangle->id
            ? document->fill_preview.color
            : rectangle->fill;
    const VectorShape *shown =
        context->instance ? rectangle : vector_curve_display_shape(document, rectangle);
    PUIRect rect = context->instance ? shown->rect : vector_rectangle_display_rect(document, shown);
    float angle =
        context->instance ? shown->angle : vector_rectangle_display_angle(document, shown);
    if (*shown->image.source) {
        vector_image_draw(context->images, renderer, shown, rect, angle, board,
                          document->viewport.zoom);
        return;
    }
    if (shown->text.enabled) {
        if (shown->stroke.width > 0) {
            draw(renderer, board, document->viewport.zoom, rect, false, false, 0, angle,
                 shown->stroke);
        }
        vector_text_draw(text, renderer, document, shown, rect, angle, board,
                         document->viewport.zoom, fill,
                         vector_document_is_selected(document, shown->id));
        return;
    }
    if (shown->curve_kind == VECTOR_CURVE_ELLIPSE) {
        vector_ellipse_draw(renderer, shown, rect, angle, board, document->viewport.zoom, fill,
                            vector_document_is_selected(document, rectangle->id));
        return;
    }
    if (shown->curve.count) {
        vector_curve_draw(renderer, shown, rect, angle, board, document->viewport.zoom, fill,
                          vector_document_is_selected(document, rectangle->id));
        return;
    }
    if (rectangle->path.count) {
        vector_path_draw(renderer, rectangle, rect, angle, board, document->viewport.zoom, fill,
                         vector_document_is_selected(document, rectangle->id));
        return;
    }
    draw(renderer, board, document->viewport.zoom, rect,
         vector_document_is_selected(document, rectangle->id), false, fill, angle,
         rectangle->stroke);
}

static void draw_instance(void *data, VectorShape *shape) {
    DrawShape instance = *(DrawShape *)data;
    instance.rectangle = shape;
    instance.instance = true;
    draw_shape(&instance);
}

static void draw_repeated(void *data) {
    DrawShape *content = data;
    draw_shape(content);
    vector_repeat_render(content->document, content->rectangle, draw_instance, content);
}

void vector_rectangles_draw(VectorDocument *document, const VectorArtboard *parent,
                            SDL_Renderer *renderer, SDL_FRect board, VectorTextRenderer *text,
                            VectorImageRenderer *images) {
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
    VectorMaskRenderer masks = {.renderer = renderer};
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorRectangle *rectangle = &document->rectangles[i];
        if (rectangle->parent_id != parent->id || rectangle->flags & VECTOR_OBJECT_HIDDEN) {
            continue;
        }
        if (rectangle->clip_mask) {
            if (document->selection.count == 1 &&
                vector_document_is_selected(document, rectangle->id)) {
                const VectorShape *shown = vector_curve_display_shape(document, rectangle);
                VectorShape outline = *shown;
                outline.stroke.width = 0;
                vector_path_draw(renderer, &outline, vector_rectangle_display_rect(document, shown),
                                 vector_rectangle_display_angle(document, shown), board,
                                 document->viewport.zoom, 0, true);
            }
            continue;
        }
        DrawShape content = {document, rectangle, renderer, board, text, images, false};
        const VectorShape *mask = vector_mask_find(document, rectangle->group_id);
        if (mask) {
            const VectorShape *shown = vector_curve_display_shape(document, mask);
            vector_mask_render(&masks, shown, vector_rectangle_display_rect(document, shown),
                               vector_rectangle_display_angle(document, shown), board,
                               document->viewport.zoom, draw_repeated, &content);
        } else {
            draw_repeated(&content);
        }
    }
    vector_mask_renderer_destroy(&masks);
    if (document->rectangle_tool.dragging && document->rectangle_tool.parent_id == parent->id) {
        if (document->rectangle_tool.text) {
            draw(renderer, board, document->viewport.zoom,
                 vector_rectangle_tool_rect(&document->rectangle_tool), true, true, 0, 0,
                 (VectorStroke){0});
        } else if (document->rectangle_tool.polygon) {
            VectorShape polygon;
            if (vector_polygon_shape(vector_rectangle_tool_rect(&document->rectangle_tool), 6,
                                     &polygon)) {
                vector_path_draw(renderer, &polygon, polygon.rect, 0, board,
                                 document->viewport.zoom,
                                 (VECTOR_RECTANGLE_DEFAULT_FILL & 0xffffff00u) | 150u, true);
            }
        } else if (document->rectangle_tool.ellipse) {
            VectorShape ellipse;
            if (vector_ellipse_shape(vector_rectangle_tool_rect(&document->rectangle_tool),
                                     &ellipse)) {
                vector_ellipse_draw(renderer, &ellipse, ellipse.rect, 0, board,
                                    document->viewport.zoom,
                                    (VECTOR_RECTANGLE_DEFAULT_FILL & 0xffffff00u) | 150u, true);
            }
        } else {
            draw(renderer, board, document->viewport.zoom,
                 vector_rectangle_tool_rect(&document->rectangle_tool), true, true,
                 VECTOR_RECTANGLE_DEFAULT_FILL, 0, (VectorStroke){0});
        }
    }
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

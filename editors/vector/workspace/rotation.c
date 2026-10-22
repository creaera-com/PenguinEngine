/** @file @brief Stable rotation snapshots shared by individual and grouped rectangles. */
#include "rotation.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/instance/instance.h"
#include "editors/vector/rectangle/transform.h"
#include "modifiers.h"
#include "selection.h"
#include <math.h>
#include <stdio.h>

bool vector_rotation_hit(PUIRect bounds, float x, float y) {
    float radius = hypotf(x - bounds.x - bounds.w / 2, y - bounds.y - bounds.h / 2);
    return radius >= 5 && radius <= 11;
}

bool vector_rotation_begin(VectorDocument *document, PUIRect screen, float x, float y) {
    VectorSelectionTool tool = {.dragging = true,
                                .rotating = true,
                                .before = document->selection,
                                .artboard_id = document->active_artboard,
                                .rectangle_id = document->active_rectangle};
    if (!tool.before.count && tool.rectangle_id) {
        tool.before.ids[0] = tool.rectangle_id;
        tool.before.count = 1;
    }
    if (!tool.before.count) {
        return false;
    }
    const VectorViewport *v = &document->viewport;
    float cx = screen.x + screen.w / 2, cy = screen.y + screen.h / 2;
    tool.pivot_x = (cx - v->bounds.x - v->x) / v->zoom;
    tool.pivot_y = (cy - v->bounds.y - v->y) / v->zoom;
    tool.rotation_start = atan2f(y - cy, x - cx) * 57.295779513f;
    tool.keep_aspect = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    for (int i = 0; i < tool.before.count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, tool.before.ids[i]);
        VectorArtboard *parent =
            rectangle ? vector_artboard_find(document, rectangle->parent_id) : NULL;
        if (!parent ||
            vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK) {
            return false;
        }
        tool.group_originals[i] = vector_instance_rect(document, rectangle);
        tool.group_parents[i] = parent->rect;
        tool.group_angles[i] = rectangle->angle;
    }
    document->selection_tool = tool;
    SDL_CaptureMouse(true);
    return true;
}

static bool targets(const VectorDocument *document, PUIRect rects[64], float angles[64]) {
    const VectorSelectionTool *tool = &document->selection_tool;
    for (int i = 0; i < tool->before.count; i++) {
        PUIRect r = tool->group_originals[i], parent = tool->group_parents[i];
        float cx = parent.x + r.x + r.w / 2, cy = parent.y + r.y + r.h / 2;
        float geometry_delta = vector_angle_normalize(tool->rotation_delta);
        if (geometry_delta != 0) {
            vector_rotate_point(&cx, &cy, tool->pivot_x, tool->pivot_y, geometry_delta);
            r.x = cx - parent.x - r.w / 2;
            r.y = cy - parent.y - r.h / 2;
        }
        if (!isfinite(r.x) || !isfinite(r.y) || fabsf(r.x) > VECTOR_MAX_ARTBOARD_SIZE ||
            fabsf(r.y) > VECTOR_MAX_ARTBOARD_SIZE) {
            return false;
        }
        rects[i] = r;
        angles[i] = vector_angle_turn(tool->group_angles[i] + tool->rotation_delta);
    }
    return true;
}

PUIRect vector_rotation_display_rect(const VectorDocument *document,
                                     const VectorRectangle *rectangle) {
    PUIRect rects[64];
    float angles[64];
    bool valid = targets(document, rects, angles);
    for (int i = 0; i < document->selection_tool.before.count; i++) {
        if (document->selection_tool.before.ids[i] == rectangle->id) {
            return valid ? rects[i] : document->selection_tool.group_originals[i];
        }
    }
    return vector_instance_rect(document, rectangle);
}

float vector_rotation_display_angle(const VectorDocument *document,
                                    const VectorRectangle *rectangle) {
    const VectorSelectionTool *tool = &document->selection_tool;
    if (!tool->rotating) {
        return rectangle->angle;
    }
    PUIRect rects[64];
    float angles[64];
    bool valid = targets(document, rects, angles);
    for (int i = 0; i < tool->before.count; i++) {
        if (tool->before.ids[i] == rectangle->id) {
            return valid ? angles[i] : tool->group_angles[i];
        }
    }
    return rectangle->angle;
}

static void update(VectorDocument *document, float x, float y) {
    VectorSelectionTool *tool = &document->selection_tool;
    const VectorViewport *v = &document->viewport;
    float cx = v->bounds.x + v->x + tool->pivot_x * v->zoom;
    float cy = v->bounds.y + v->y + tool->pivot_y * v->zoom;
    if (hypotf(x - cx, y - cy) >= 4) {
        float current = atan2f(y - cy, x - cx) * 57.295779513f;
        tool->rotation_raw += vector_angle_normalize(current - tool->rotation_start);
        tool->rotation_start = current;
    }
    tool->rotation_delta =
        tool->keep_aspect ? roundf(tool->rotation_raw / 15) * 15 : tool->rotation_raw;
}

bool vector_rotation_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorSelectionTool *tool = &document->selection_tool;
    for (int i = 0; i < tool->before.count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, tool->before.ids[i]);
        VectorArtboard *parent =
            rectangle ? vector_artboard_find(document, rectangle->parent_id) : NULL;
        if (!parent ||
            vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK ||
            parent->rect.x != tool->group_parents[i].x ||
            parent->rect.y != tool->group_parents[i].y) {
            vector_selection_cancel(document);
            return true;
        }
    }
    bool modifier = vector_transform_modifiers(event, &tool->keep_aspect, &tool->from_center);
    if (modifier) {
        tool->rotation_delta =
            tool->keep_aspect ? roundf(tool->rotation_raw / 15) * 15 : tool->rotation_raw;
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        update(document, event->motion.x, event->motion.y);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        update(document, event->button.x, event->button.y);
        PUIRect rects[64];
        float angles[64];
        bool changed = targets(document, rects, angles) &&
                       vector_rectangles_set_transforms(document, tool->before.ids, rects, angles,
                                                        tool->before.count);
        vector_selection_cancel(document);
        pui_view_set_text(view, "status",
                          changed ? "Auswahl rotiert" : "Rotation konnte nicht uebernommen werden");
        return true;
    }
    return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

void vector_rotation_draw(const VectorDocument *document, SDL_Renderer *renderer, PUIRect bounds) {
    const VectorSelectionTool *tool = &document->selection_tool;
    float cx = bounds.x + bounds.w / 2, cy = bounds.y + bounds.h / 2;
    if (tool->rotating) {
        cx = document->viewport.bounds.x + document->viewport.x +
             tool->pivot_x * document->viewport.zoom;
        cy = document->viewport.bounds.y + document->viewport.y +
             tool->pivot_y * document->viewport.zoom;
    }
    for (int ring = 0; ring < 2; ring++) {
        float radius = ring ? 7 : 9;
        SDL_FPoint points[41];
        for (int i = 0; i <= 40; i++) {
            float a = i * .157079632679f;
            points[i] = (SDL_FPoint){cx + cosf(a) * radius, cy + sinf(a) * radius};
        }
        pui_render_color(renderer, ring ? 0xffffffff : PUI_HIGHLIGHT);
        SDL_RenderLines(renderer, points, 41);
    }
    if (tool->rotating) {
        char text[48];
        snprintf(text, sizeof text, "%.1f Grad", vector_angle_turn(tool->rotation_delta));
        pui_panel(renderer,
                  (SDL_FRect){cx + 12, cy - 22, pui_text_width(text) * .875f + 8,
                              PUI_FONT_HEIGHT * .875f + 8},
                  PUI_PANEL);
        pui_text_scaled(renderer, cx + 16, cy - 18, text, 0xFFFFFFFF, .875f);
    }
}

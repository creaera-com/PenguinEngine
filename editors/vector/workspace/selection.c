/** @file @brief One move/resize interaction for artboards and rectangle children. */
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/rectangle/transform.h"
#include "element_snap.h"
#include "group_transform.h"
#include "marquee.h"
#include "modifiers.h"
#include "oriented_resize.h"
#include "rotation.h"
#include "selection.h"
#include <math.h>
#include <stdio.h>

void vector_selection_cancel(VectorDocument *document) {
    if (document->selection_tool.dragging || document->selection_tool.marquee) {
        SDL_CaptureMouse(false);
    }
    if (document->selection_tool.marquee) {
        document->selection = document->selection_tool.before;
        document->active_artboard = document->selection_tool.previous_board;
        document->active_rectangle = document->selection_tool.previous_rectangle;
    }
    document->selection_tool = (VectorSelectionTool){0};
}

static PUIRect preview(PUIRect original, const VectorSelectionTool *tool) {
    if (tool->resize_edges) {
        return vector_resize_constrained(original, tool->resize_edges, tool->offset_x,
                                         tool->offset_y, tool->keep_aspect, tool->from_center, 1, 1,
                                         VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
    }
    original.x += tool->offset_x;
    original.y += tool->offset_y;
    return original;
}

static PUIRect movement_preview(const VectorDocument *document, const VectorRectangle *r) {
    const VectorSelectionTool *tool = &document->selection_tool;
    PUIRect rect = vector_grid_snap_rotated_rect(&document->grid, document->viewport.zoom, r->rect,
                                                 r->angle, tool->offset_x, tool->offset_y);
    VectorElementSnap snap = vector_workspace_element_snap(document);
    /* Object anchors take priority; the grid still controls unmatched axes. */
    if (snap.snapped_x) {
        rect.x = r->rect.x + tool->offset_x + snap.dx;
    }
    if (snap.snapped_y) {
        rect.y = r->rect.y + tool->offset_y + snap.dy;
    }
    return rect;
}

PUIRect vector_artboard_display_rect(const VectorDocument *document, const VectorArtboard *board) {
    const VectorSelectionTool *tool = &document->selection_tool;
    if (tool->dragging && !tool->rectangle_id && tool->artboard_id == board->id) {
        PUIRect rect = preview(board->rect, tool);
        VectorElementSnap snap = vector_workspace_element_snap(document);
        rect.x += snap.dx;
        rect.y += snap.dy;
        return rect;
    }
    return board->rect;
}

PUIRect vector_rectangle_display_rect(const VectorDocument *document,
                                      const VectorRectangle *rectangle) {
    const VectorSelectionTool *tool = &document->selection_tool;
    if (tool->rotating) {
        return vector_rotation_display_rect(document, rectangle);
    }
    if (!tool->dragging || tool->marquee || !tool->rectangle_id ||
        !vector_document_is_selected(document, rectangle->id)) {
        return rectangle->rect;
    }
    if (tool->before.count > 1 && tool->resize_edges) {
        return vector_group_display_rect(document, rectangle);
    }
    if (tool->before.count > 1) {
        const VectorRectangle *reference = NULL;
        for (int i = 0; i < document->rectangle_count; i++) {
            if (document->rectangles[i].id == tool->rectangle_id) {
                reference = &document->rectangles[i];
                break;
            }
        }
        if (!reference) {
            return rectangle->rect;
        }
        PUIRect snapped = movement_preview(document, reference);
        PUIRect rect = rectangle->rect;
        rect.x += snapped.x - reference->rect.x;
        rect.y += snapped.y - reference->rect.y;
        return rect;
    }
    if (tool->resize_edges && rectangle->angle != 0) {
        return vector_oriented_resize(document, rectangle);
    }
    if (tool->resize_edges) {
        return vector_grid_resize_constrained(
            &document->grid, document->viewport.zoom, rectangle->rect, tool->resize_edges,
            tool->offset_x, tool->offset_y, tool->keep_aspect, tool->from_center, 1, 1,
            VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
    }
    return movement_preview(document, rectangle);
}

float vector_rectangle_display_angle(const VectorDocument *document,
                                     const VectorRectangle *rectangle) {
    return vector_rotation_display_angle(document, rectangle);
}

static PUIRect screen_rect(const VectorDocument *document, PUIRect world) {
    const VectorViewport *v = &document->viewport;
    return (PUIRect){v->bounds.x + v->x + world.x * v->zoom, v->bounds.y + v->y + world.y * v->zoom,
                     world.w * v->zoom, world.h * v->zoom};
}

PUIRect vector_artboard_label_rect(const VectorDocument *document, const VectorArtboard *board) {
    PUIRect screen = screen_rect(document, vector_artboard_display_rect(document, board));
    return (PUIRect){screen.x, screen.y - 30, fmaxf(24, pui_text_width(board->name) * .875f),
                     PUI_FONT_HEIGHT * .875f + 4};
}

static bool editable(VectorDocument *document, unsigned board_id, unsigned rectangle_id) {
    VectorArtboard *board = vector_artboard_find(document, board_id);
    if (!board || (board->flags & VECTOR_OBJECT_STATE_MASK)) {
        return false;
    }
    if (rectangle_id) {
        VectorRectangle *rectangle = vector_rectangle_find(document, rectangle_id);
        return rectangle && rectangle->parent_id == board_id &&
               !(rectangle->flags & VECTOR_OBJECT_STATE_MASK);
    }
    return true;
}

bool vector_selection_bounds(VectorDocument *document, PUIRect *bounds) {
    if (!editable(document, document->active_artboard, document->active_rectangle)) {
        return false;
    }
    VectorArtboard *board = vector_artboard_find(document, document->active_artboard);
    PUIRect world = vector_artboard_display_rect(document, board);
    if (document->active_rectangle) {
        VectorRectangle *rectangle = vector_rectangle_find(document, document->active_rectangle);
        PUIRect local = vector_rectangle_display_rect(document, rectangle);
        world = (PUIRect){world.x + local.x, world.y + local.y, local.w, local.h};
        world = vector_rotated_bounds(world, vector_rectangle_display_angle(document, rectangle));
    }
    if (document->selection.count > 1) {
        bool first = true;
        for (int i = 0; i < document->rectangle_count; i++) {
            VectorRectangle *rectangle = &document->rectangles[i];
            if (!vector_document_is_selected(document, rectangle->id) ||
                !editable(document, rectangle->parent_id, rectangle->id)) {
                continue;
            }
            VectorArtboard *parent = vector_artboard_find(document, rectangle->parent_id);
            PUIRect r = vector_rectangle_display_rect(document, rectangle);
            r.x += parent->rect.x;
            r.y += parent->rect.y;
            r = vector_rotated_bounds(r, vector_rectangle_display_angle(document, rectangle));
            if (first) {
                world = r;
                first = false;
            } else {
                float right = fmaxf(world.x + world.w, r.x + r.w),
                      bottom = fmaxf(world.y + world.h, r.y + r.h);
                world.x = fminf(world.x, r.x);
                world.y = fminf(world.y, r.y);
                world.w = right - world.x;
                world.h = bottom - world.y;
            }
        }
    }
    *bounds = screen_rect(document, world);
    return true;
}

PUIRect vector_selection_handle_rect(VectorDocument *document, int index, float size) {
    PUIRect bounds;
    if (!vector_selection_bounds(document, &bounds)) {
        return (PUIRect){0};
    }
    float angle = 0;
    if (document->active_rectangle && document->selection.count <= 1) {
        VectorRectangle *rectangle = vector_rectangle_find(document, document->active_rectangle);
        VectorArtboard *parent = vector_artboard_find(document, rectangle->parent_id);
        bounds = vector_rectangle_display_rect(document, rectangle);
        bounds.x += parent->rect.x;
        bounds.y += parent->rect.y;
        bounds = screen_rect(document, bounds);
        angle = vector_rectangle_display_angle(document, rectangle);
    }
    PUIRect handle = vector_resize_handle_rect(bounds, index, size);
    float x = handle.x + size / 2, y = handle.y + size / 2;
    vector_rotate_point(&x, &y, bounds.x + bounds.w / 2, bounds.y + bounds.h / 2, angle);
    handle.x = x - size / 2;
    handle.y = y - size / 2;
    return handle;
}

static unsigned handle_hit(VectorDocument *document, float x, float y) {
    const int order[8] = {0, 2, 4, 6, 1, 3, 5, 7};
    for (int i = 0; i < 8; i++) {
        PUIRect r = vector_selection_handle_rect(document, order[i], 10);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            return vector_resize_handle_edges(order[i]);
        }
    }
    return 0;
}

void vector_selection_draw_handles(VectorDocument *document, SDL_Renderer *renderer) {
    if (document->artboard_tool.enabled || document->rectangle_tool.enabled) {
        return;
    }
    PUIRect screen;
    if (!vector_selection_bounds(document, &screen)) {
        return;
    }
    if (document->active_rectangle) {
        // A selected child's bounds remain visible when it extends beyond artboard clipping.
        SDL_FRect outline = {screen.x, screen.y, screen.w, screen.h};
        SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
        if (document->selection.count <= 1) {
            VectorRectangle *rectangle =
                vector_rectangle_find(document, document->active_rectangle);
            VectorArtboard *parent = vector_artboard_find(document, rectangle->parent_id);
            PUIRect rect = vector_rectangle_display_rect(document, rectangle);
            rect.x += parent->rect.x;
            rect.y += parent->rect.y;
            rect = screen_rect(document, rect);
            PUIRect corners[4];
            vector_rotated_corners(rect, vector_rectangle_display_angle(document, rectangle),
                                   corners);
            SDL_FPoint points[5];
            for (int i = 0; i < 5; i++) {
                points[i] = (SDL_FPoint){corners[i % 4].x, corners[i % 4].y};
            }
            SDL_RenderLines(renderer, points, 5);
        } else {
            SDL_RenderRect(renderer, &outline);
        }
    }
    if (document->selection_tool.rotating) {
        vector_rotation_draw(document, renderer, screen);
        return;
    }
    for (int i = 0; i < 8; i++) {
        PUIRect handle = vector_selection_handle_rect(document, i, 7);
        SDL_FRect rect = {handle.x, handle.y, handle.w, handle.h};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
        SDL_RenderRect(renderer, &rect);
    }
    if (document->active_rectangle) {
        vector_rotation_draw(document, renderer, screen);
    }
}

static bool contains(PUIRect rect, float x, float y) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

static void begin(VectorDocument *document, unsigned board, unsigned rectangle, unsigned edges,
                  float x, float y) {
    document->active_artboard = board;
    document->active_rectangle = rectangle;
    document->selection_tool = (VectorSelectionTool){.dragging = true,
                                                     .artboard_id = board,
                                                     .rectangle_id = rectangle,
                                                     .resize_edges = edges,
                                                     .start_x = x,
                                                     .start_y = y,
                                                     .before = document->selection};
    SDL_Keymod mods = SDL_GetModState();
    document->selection_tool.keep_aspect = (mods & SDL_KMOD_SHIFT) != 0;
    document->selection_tool.from_center = (mods & SDL_KMOD_ALT) != 0;
    if (edges && document->selection.count > 1 && !vector_group_capture(document)) {
        vector_selection_cancel(document);
        return;
    }
    SDL_CaptureMouse(true);
}

static void update(VectorDocument *document, float x, float y) {
    VectorSelectionTool *tool = &document->selection_tool;
    float dx = (x - tool->start_x) / document->viewport.zoom;
    float dy = (y - tool->start_y) / document->viewport.zoom;
    /* Keep subpixel movement until grid testing: rounding here changes the
     * screen-space capture distance for fractional object coordinates. */
    bool rectangle_move = tool->rectangle_id && !tool->resize_edges;
    tool->offset_x = rectangle_move ? dx : roundf(dx);
    tool->offset_y = rectangle_move ? dy : roundf(dy);
}

static bool finish(VectorDocument *document) {
    VectorSelectionTool *tool = &document->selection_tool;
    if (tool->rectangle_id && tool->before.count > 1 && tool->resize_edges) {
        return vector_group_commit(document);
    }
    if (tool->rectangle_id && tool->before.count > 1) {
        VectorRectangle *reference = vector_rectangle_find(document, tool->rectangle_id);
        if (!reference) {
            return false;
        }
        PUIRect moved = vector_rectangle_display_rect(document, reference);
        return vector_rectangles_translate(document, tool->before.ids, tool->before.count,
                                           moved.x - reference->rect.x,
                                           moved.y - reference->rect.y);
    }
    if (tool->rectangle_id) {
        VectorRectangle *rectangle = vector_rectangle_find(document, tool->rectangle_id);
        return rectangle &&
               vector_rectangle_update(document, rectangle->id, rectangle->name,
                                       vector_rectangle_display_rect(document, rectangle));
    }
    VectorArtboard *board = vector_artboard_find(document, tool->artboard_id);
    return board && vector_artboard_set_rect(document, board->id,
                                             vector_artboard_display_rect(document, board));
}

static bool gesture_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    VectorSelectionTool *tool = &document->selection_tool;
    if (tool->rotating) {
        return vector_rotation_event(document, view, event);
    }
    if (tool->marquee) {
        return vector_marquee_event(document, event);
    }
    for (int i = 0; tool->before.count > 1 && i < tool->before.count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, tool->before.ids[i]);
        if (!rectangle || !editable(document, rectangle->parent_id, rectangle->id)) {
            vector_selection_cancel(document);
            return true;
        }
    }
    if (!editable(document, tool->artboard_id, tool->rectangle_id)) {
        vector_selection_cancel(document);
        return true;
    }
    bool modifier = vector_transform_modifiers(event, &tool->keep_aspect, &tool->from_center);
    if (modifier && tool->resize_edges) {
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        update(document, event->motion.x, event->motion.y);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        update(document, event->button.x, event->button.y);
        bool changed = finish(document);
        vector_selection_cancel(document);
        pui_view_set_text(view, "status",
                          changed ? "Objekt aktualisiert" : "Objekt konnte nicht geaendert werden");
        return true;
    }
    return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

bool vector_selection_event(VectorDocument *document, PUIView *view, const SDL_Event *event) {
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        bool consumed = document->selection_tool.dragging || document->selection_tool.marquee;
        vector_selection_cancel(document);
        return consumed;
    }
    if (document->artboard_tool.enabled || document->rectangle_tool.enabled) {
        return false;
    }
    if (document->selection_tool.marquee && event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event->button.button == SDL_BUTTON_LEFT) {
        document->selection_tool.marquee = false;
        vector_selection_cancel(document);
    }
    if (document->selection_tool.dragging || document->selection_tool.marquee) {
        return gesture_event(document, view, event);
    }
    char canvas[96];
    snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", document->id);
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT ||
        document->viewport.panning ||
        !pui_view_hit(view, canvas, event->button.x, event->button.y)) {
        return false;
    }
    float x = event->button.x, y = event->button.y;
    PUIRect screen;
    bool additive = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
    if (vector_selection_bounds(document, &screen)) {
        unsigned edges = handle_hit(document, x, y);
        if (edges) {
            begin(document, document->active_artboard, document->active_rectangle, edges, x, y);
            return true;
        }
        if (document->active_rectangle && vector_rotation_hit(screen, x, y)) {
            vector_rotation_begin(document, screen, x, y);
            return true;
        }
    }
    // Artboards retain label-only movement; their surface only selects.
    for (int i = document->artboard_count - 1; i >= 0; i--) {
        VectorArtboard *board = &document->artboards[i];
        if (!(board->flags & VECTOR_OBJECT_STATE_MASK) &&
            contains(vector_artboard_label_rect(document, board), x, y)) {
            vector_document_select(document, 0);
            begin(document, board->id, 0, 0, x, y);
            return true;
        }
    }
    float world_x =
        (x - document->viewport.bounds.x - document->viewport.x) / document->viewport.zoom;
    float world_y =
        (y - document->viewport.bounds.y - document->viewport.y) / document->viewport.zoom;
    // An already selected child can be brought back even after moving outside its parent.
    if (document->active_rectangle &&
        editable(document, document->active_artboard, document->active_rectangle)) {
        VectorRectangle *rectangle = vector_rectangle_find(document, document->active_rectangle);
        VectorArtboard *board = vector_artboard_find(document, rectangle->parent_id);
        if (!contains(board->rect, world_x, world_y) &&
            vector_shape_contains(rectangle, world_x - board->rect.x, world_y - board->rect.y)) {
            if (additive) {
                vector_document_toggle(document, rectangle->id);
            } else {
                begin(document, board->id, rectangle->id, 0, x, y);
            }
            return true;
        }
    }
    /* A mask selected explicitly in the tree remains directly movable on the canvas. */
    VectorShape *mask = vector_rectangle_find(document, document->active_rectangle);
    if (mask && mask->clip_mask && document->selection.count == 1 && !additive &&
        editable(document, mask->parent_id, mask->id)) {
        VectorArtboard *board = vector_artboard_find(document, mask->parent_id);
        VectorPath path = vector_shape_path(mask, mask->rect, mask->angle);
        if (contains(board->rect, world_x, world_y) &&
            vector_path_contains(&path, world_x - board->rect.x, world_y - board->rect.y)) {
            begin(document, board->id, mask->id, 0, x, y);
            return true;
        }
    }
    unsigned empty_board = 0;
    for (int i = document->artboard_count - 1; i >= 0; i--) {
        VectorArtboard *board = &document->artboards[i];
        if ((board->flags & VECTOR_OBJECT_STATE_MASK) || !contains(board->rect, world_x, world_y)) {
            continue;
        }
        empty_board = board->id;
        for (int j = document->rectangle_count - 1; j >= 0; j--) {
            VectorRectangle *rectangle = &document->rectangles[j];
            if (rectangle->parent_id != board->id ||
                (rectangle->flags & VECTOR_OBJECT_STATE_MASK) ||
                !vector_mask_contains(document, rectangle, world_x - board->rect.x,
                                      world_y - board->rect.y)) {
                continue;
            }
            if (additive) {
                vector_document_toggle(document, rectangle->id);
            } else {
                if (!vector_document_is_selected(document, rectangle->id)) {
                    vector_document_select(document, rectangle->id);
                }
                begin(document, board->id, rectangle->id, 0, x, y);
            }
            return true;
        }
        break;
    }
    vector_marquee_begin(document, x, y, additive, empty_board);
    return true;
}

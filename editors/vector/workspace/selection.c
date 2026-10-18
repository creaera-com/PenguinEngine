/** @file @brief One move/resize interaction for artboards and rectangle children. */
#include "selection.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/rectangle/transform.h"
#include "group_transform.h"
#include "marquee.h"
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
        return vector_resize_rect(original, tool->resize_edges, tool->offset_x, tool->offset_y,
                                  VECTOR_MAX_ARTBOARD_SIZE);
    }
    original.x += tool->offset_x;
    original.y += tool->offset_y;
    return original;
}

PUIRect vector_artboard_display_rect(const VectorDocument *document, const VectorArtboard *board) {
    const VectorSelectionTool *tool = &document->selection_tool;
    return tool->dragging && !tool->rectangle_id && tool->artboard_id == board->id
               ? preview(board->rect, tool)
               : board->rect;
}

PUIRect vector_rectangle_display_rect(const VectorDocument *document,
                                      const VectorRectangle *rectangle) {
    const VectorSelectionTool *tool = &document->selection_tool;
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
        PUIRect snapped =
            vector_grid_snap_rect(&document->grid, document->viewport.zoom, reference->rect, 0,
                                  tool->offset_x, tool->offset_y, VECTOR_MAX_ARTBOARD_SIZE);
        PUIRect rect = rectangle->rect;
        rect.x += snapped.x - reference->rect.x;
        rect.y += snapped.y - reference->rect.y;
        return rect;
    }
    return vector_grid_snap_rect(&document->grid, document->viewport.zoom, rectangle->rect,
                                 tool->resize_edges, tool->offset_x, tool->offset_y,
                                 VECTOR_MAX_ARTBOARD_SIZE);
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
        SDL_RenderRect(renderer, &outline);
    }
    for (int i = 0; i < 8; i++) {
        PUIRect handle = vector_resize_handle_rect(screen, i, 7);
        SDL_FRect rect = {handle.x, handle.y, handle.w, handle.h};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 194, 142, 92, 255);
        SDL_RenderRect(renderer, &rect);
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
    if (edges && document->selection.count > 1 && !vector_group_capture(document)) {
        vector_selection_cancel(document);
        return;
    }
    SDL_CaptureMouse(true);
}

static void update(VectorDocument *document, float x, float y) {
    VectorSelectionTool *tool = &document->selection_tool;
    tool->offset_x = roundf((x - tool->start_x) / document->viewport.zoom);
    tool->offset_y = roundf((y - tool->start_y) / document->viewport.zoom);
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
    if (!additive && vector_selection_bounds(document, &screen)) {
        unsigned edges = vector_resize_hit(screen, x, y, 10);
        if (edges) {
            begin(document, document->active_artboard, document->active_rectangle, edges, x, y);
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
            contains(rectangle->rect, world_x - board->rect.x, world_y - board->rect.y)) {
            if (additive) {
                vector_document_toggle(document, rectangle->id);
            } else {
                begin(document, board->id, rectangle->id, 0, x, y);
            }
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
                !contains(rectangle->rect, world_x - board->rect.x, world_y - board->rect.y)) {
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

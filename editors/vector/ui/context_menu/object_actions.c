/** @file @brief Native interaction stays outside clipboard, hierarchy and history logic. */
#include "object_actions.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/object/alignment.h"
#include "editors/vector/object/group.h"
#include "editors/vector/object/order.h"
#include "editors/vector/ui/document_tree/document_tree.h"
#include "orientation_actions.h"
#include "path_actions.h"
#include <string.h>

/** @brief Bind YAML IDs and semantic actions to the domain without expanding the routing chain. */
static const struct {
    const char *action, *widget;
    VectorAlignment alignment;
} alignments[] = {{"vector.objects.align_left", "vector_align_left", VECTOR_ALIGN_LEFT},
                  {"vector.objects.align_center_x", "vector_align_center_x", VECTOR_ALIGN_CENTER_X},
                  {"vector.objects.align_right", "vector_align_right", VECTOR_ALIGN_RIGHT},
                  {"vector.objects.distribute_x", "vector_distribute_x", VECTOR_DISTRIBUTE_X},
                  {"vector.objects.align_top", "vector_align_top", VECTOR_ALIGN_TOP},
                  {"vector.objects.align_center_y", "vector_align_center_y", VECTOR_ALIGN_CENTER_Y},
                  {"vector.objects.align_bottom", "vector_align_bottom", VECTOR_ALIGN_BOTTOM},
                  {"vector.objects.distribute_y", "vector_distribute_y", VECTOR_DISTRIBUTE_Y}};

bool vector_object_actions_action(VectorObjectActions *actions, VectorDocument *document,
                                  PUIView *view, const char *name) {
    if (strncmp(name, "vector.objects.", 15)) {
        return false;
    }
    if (!document) {
        return true;
    }
    if (vector_orientation_actions_action(document, view, name)) {
        return true;
    }
    if (vector_path_actions_action(document, view, name)) {
        return true;
    }
    vector_artboard_tool_cancel(document);
    for (size_t i = 0; i < sizeof alignments / sizeof *alignments; i++) {
        if (!strcmp(name, alignments[i].action)) {
            bool changed = vector_objects_align(document, alignments[i].alignment);
            pui_view_set_text(view, "status",
                              changed
                                  ? "Auswahl ausgerichtet"
                                  : "Keine Aenderung: Auswahl, Sperren oder Positionen pruefen");
            return true;
        }
    }
    bool changed;
    if (!strcmp(name, "vector.objects.mask_create")) {
        changed = vector_mask_create(document);
    } else if (!strcmp(name, "vector.objects.mask_release")) {
        changed = vector_mask_release(document);
    } else if (!strcmp(name, "vector.objects.group")) {
        changed = vector_objects_group(document);
    } else if (!strcmp(name, "vector.objects.ungroup")) {
        changed = vector_objects_ungroup(document);
    } else if (!strcmp(name, "vector.objects.copy")) {
        changed = vector_objects_copy(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.cut")) {
        changed = vector_objects_cut(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.paste")) {
        changed = vector_objects_paste(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.duplicate")) {
        changed = vector_objects_duplicate(document);
    } else if (!strcmp(name, "vector.objects.delete")) {
        changed = vector_objects_delete(document);
    } else if (!strcmp(name, "vector.objects.order_front")) {
        changed = vector_objects_order(document, VECTOR_ORDER_FRONT);
    } else if (!strcmp(name, "vector.objects.order_back")) {
        changed = vector_objects_order(document, VECTOR_ORDER_BACK);
    } else if (!strcmp(name, "vector.objects.order_forward")) {
        changed = vector_objects_order(document, VECTOR_ORDER_FORWARD);
    } else if (!strcmp(name, "vector.objects.order_backward")) {
        changed = vector_objects_order(document, VECTOR_ORDER_BACKWARD);
    } else {
        return false;
    }
    pui_view_set_text(
        view, "status",
        changed ? "Objektaktion ausgefuehrt"
                : "Keine Aktion: Auswahl, Zwischenablage, Sperre oder Kapazitaet pruefen");
    return true;
}

static void select_at(VectorDocument *d, float x, float y) {
    VectorViewport *v = &d->viewport;
    float wx = (x - v->bounds.x - v->x) / v->zoom;
    float wy = (y - v->bounds.y - v->y) / v->zoom;
    for (int i = d->rectangle_count - 1; i >= 0; i--) {
        VectorRectangle *r = &d->rectangles[i];
        VectorArtboard *b = vector_artboard_find(d, r->parent_id);
        if (!b || vector_rectangle_effective_flags(d, r) & VECTOR_OBJECT_HIDDEN ||
            !pui_contains(b->rect, wx, wy)) {
            continue;
        }
        if (vector_mask_contains(d, r, wx - b->rect.x, wy - b->rect.y)) {
            if (!vector_document_is_selected(d, r->id)) {
                vector_document_select(d, r->id);
            }
            return;
        }
    }
    // A context menu in empty space keeps the selection, including a multi-selection.
    if (d->active_rectangle || d->active_artboard) {
        return;
    }
    for (int i = d->artboard_count - 1; i >= 0; i--) {
        if (!(d->artboards[i].flags & VECTOR_OBJECT_HIDDEN) &&
            pui_contains(d->artboards[i].rect, wx, wy)) {
            d->active_artboard = d->artboards[i].id;
            return;
        }
    }
}

bool vector_object_actions_event(VectorObjectActions *actions, VectorDocument *document,
                                 PUIView *view, const SDL_Event *event) {
    if (!document) {
        return false;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_RIGHT) {
        char canvas[96];
        snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", document->id);
        float x = event->button.x, y = event->button.y;
        bool on_canvas = pui_view_hit(view, canvas, x, y);
        if (!on_canvas && !pui_view_hit(view, "vector_document_tree", x, y)) {
            return false;
        }
        vector_artboard_tool_cancel(document);
        if (on_canvas) {
            select_at(document, x, y);
        } else {
            vector_document_tree_context(document, view, x, y);
        }
        actions->pending = true;
        actions->menu_document = document->id;
        actions->x = x;
        actions->y = y;
        return true;
    }
    if (event->type != SDL_EVENT_KEY_DOWN || event->key.repeat) {
        return false;
    }
    bool primary = (event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    const char *name = NULL;
    if (primary && !(event->key.mod & SDL_KMOD_ALT)) {
        switch (event->key.key) {
        case SDLK_C:
            name = "vector.objects.copy";
            break;
        case SDLK_X:
            name = "vector.objects.cut";
            break;
        case SDLK_V:
            name = "vector.objects.paste";
            break;
        case SDLK_D:
            name = "vector.objects.duplicate";
            break;
        default:
            break;
        }
    } else if (!primary && !(event->key.mod & (SDL_KMOD_ALT | SDL_KMOD_SHIFT)) &&
               (event->key.key == SDLK_DELETE || event->key.key == SDLK_BACKSPACE)) {
        name = "vector.objects.delete";
    }
    return name && vector_object_actions_action(actions, document, view, name);
}

void vector_object_actions_commit(VectorObjectActions *actions, VectorDocument *document,
                                  PUIView *view) {
    if (actions->menu_document != (document ? document->id : 0)) {
        pui_view_close_radial(view);
        actions->pending = false;
    }
    if (actions->pending) {
        VectorClipboard selection;
        bool copy = vector_objects_copy(document, &selection);
        bool remove = vector_objects_can_delete(document);
        pui_view_set_radial_enabled(view, "vector_objects_copy", copy);
        pui_view_set_radial_enabled(view, "vector_objects_cut", remove);
        pui_view_set_radial_enabled(view, "vector_objects_delete", remove);
        pui_view_set_radial_enabled(view, "vector_objects_duplicate",
                                    vector_objects_can_duplicate(document));
        pui_view_set_radial_enabled(view, "vector_objects_paste",
                                    vector_objects_can_paste(document, &actions->clipboard));
        bool group = vector_objects_can_group(document),
             ungroup = vector_objects_can_ungroup(document);
        pui_view_set_radial_enabled(view, "vector_objects_group", group);
        pui_view_set_radial_enabled(view, "vector_objects_ungroup", ungroup);
        bool mask_create = vector_mask_can_create(document);
        bool mask_release = vector_mask_can_release(document);
        pui_view_set_radial_enabled(view, "vector_mask_create", mask_create);
        pui_view_set_radial_enabled(view, "vector_mask_release", mask_release);
        bool front = vector_objects_can_order(document, VECTOR_ORDER_FRONT);
        bool back = vector_objects_can_order(document, VECTOR_ORDER_BACK);
        bool forward = vector_objects_can_order(document, VECTOR_ORDER_FORWARD);
        bool backward = vector_objects_can_order(document, VECTOR_ORDER_BACKWARD);
        pui_view_set_radial_enabled(view, "vector_layers_menu",
                                    front || back || forward || backward || group || ungroup ||
                                        mask_create || mask_release);
        pui_view_set_radial_enabled(view, "vector_order_front", front);
        pui_view_set_radial_enabled(view, "vector_order_back", back);
        pui_view_set_radial_enabled(view, "vector_order_forward", forward);
        pui_view_set_radial_enabled(view, "vector_order_backward", backward);
        bool align = false;
        for (size_t i = 0; i < sizeof alignments / sizeof *alignments; i++) {
            bool enabled = vector_objects_can_align(document, alignments[i].alignment);
            pui_view_set_radial_enabled(view, alignments[i].widget, enabled);
            align |= enabled;
        }
        pui_view_set_radial_enabled(view, "vector_alignment_menu", align);
        vector_orientation_actions_sync(document, view, align);
        vector_path_actions_sync(document, view);
        pui_view_open_radial(view, "vector_object_menu", actions->x, actions->y);
        actions->pending = false;
    }
}

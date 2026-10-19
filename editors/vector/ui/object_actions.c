/** @file @brief Native interaction stays outside clipboard, hierarchy and history logic. */
#include "object_actions.h"
#include "document_tree.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/geometry/rotation.h"
#include <string.h>

bool vector_object_actions_action(VectorObjectActions *actions, VectorDocument *document,
                                  PUIView *view, const char *name) {
    if (strncmp(name, "vector.objects.", 15)) {
        return false;
    }
    if (!document) {
        return true;
    }
    vector_artboard_tool_cancel(document);
    bool changed;
    if (!strcmp(name, "vector.objects.copy")) {
        changed = vector_objects_copy(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.cut")) {
        changed = vector_objects_cut(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.paste")) {
        changed = vector_objects_paste(document, &actions->clipboard);
    } else if (!strcmp(name, "vector.objects.duplicate")) {
        changed = vector_objects_duplicate(document);
    } else if (!strcmp(name, "vector.objects.delete")) {
        changed = vector_objects_delete(document);
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
        if (vector_rotated_contains(r->rect, r->angle, wx - b->rect.x, wy - b->rect.y)) {
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
        pui_view_open_radial(view, "vector_object_menu", actions->x, actions->y);
        actions->pending = false;
    }
}

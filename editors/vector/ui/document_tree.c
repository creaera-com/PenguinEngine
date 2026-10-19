/** @file @brief Artboards and rectangle children become depth-first tree-table rows. */
#include "document_tree.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/document/selection.h"
#include "group_tree.h"
#include "toolbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(VECTOR_MAX_ARTBOARDS + 2 * VECTOR_MAX_RECTANGLES <= PUI_LAYOUT_NODES,
               "document rows must fit the runtime tree");

static bool unchanged(const VectorDocumentTree *tree, const VectorDocument *document) {
    if (!tree->initialized || tree->document_id != (document ? document->id : 0) ||
        tree->count != (document ? document->artboard_count : 0) ||
        tree->rectangle_count != (document ? document->rectangle_count : 0)) {
        return false;
    }
    if (document && (tree->primary != document->active_rectangle ||
                     memcmp(&tree->selection, &document->selection, sizeof tree->selection))) {
        return false;
    }
    for (int i = 0; i < tree->count; i++) {
        if (tree->artboard_ids[i] != document->artboards[i].id ||
            strcmp(tree->names[i], document->artboards[i].name) ||
            tree->flags[i] != document->artboards[i].flags) {
            return false;
        }
    }
    return !document || !memcmp(tree->rectangles, document->rectangles,
                                document->rectangle_count * sizeof *document->rectangles);
}

static void sync_selection(const VectorDocument *document, PUIView *view) {
    char id[64];
    if (document && document->active_rectangle) {
        const VectorRectangle *primary = NULL;
        for (int i = 0; i < document->rectangle_count; i++) {
            if (document->rectangles[i].id == document->active_rectangle) {
                primary = &document->rectangles[i];
            }
        }
        snprintf(id, sizeof id, "vector_document_%u.%s_%u", document->id,
                 primary && primary->group_id ? "group" : "rectangle",
                 primary && primary->group_id ? primary->group_id : document->active_rectangle);
    } else if (document && document->active_artboard) {
        snprintf(id, sizeof id, "vector_document_%u.artboard_%u", document->id,
                 document->active_artboard);
    } else {
        pui_view_select_tree_item(view, "vector_document_tree", NULL);
        return;
    }
    pui_view_select_tree_item(view, "vector_document_tree", id);
}

static void fill(PUITreeItem *item, unsigned document, unsigned id, const char *name,
                 const char *kind, unsigned flags, int depth) {
    snprintf(item->id, sizeof item->id, "vector_document_%u.%s_%u", document, kind, id);
    snprintf(item->text, sizeof item->text, "%s", name);
    snprintf(item->icon, sizeof item->icon, "tabler:%s", kind);
    snprintf(item->action, sizeof item->action, "vector.%s.select", kind);
    item->depth = depth;
    item->enabled = item->open = true;
    item->cell_count = 2;
    snprintf(item->cells[0].icon, sizeof item->cells[0].icon, "%s",
             flags & VECTOR_OBJECT_HIDDEN ? "tabler:eye-off" : "tabler:eye");
    snprintf(item->cells[0].action, sizeof item->cells[0].action, "vector.%s.visibility", kind);
    item->cells[0].checked = !(flags & VECTOR_OBJECT_HIDDEN);
    item->cells[0].enabled = true;
    snprintf(item->cells[1].icon, sizeof item->cells[1].icon, "%s",
             flags & VECTOR_OBJECT_LOCKED ? "tabler:lock" : "tabler:lock-open");
    snprintf(item->cells[1].action, sizeof item->cells[1].action, "vector.%s.lock", kind);
    item->cells[1].checked = (flags & VECTOR_OBJECT_LOCKED) != 0;
    item->cells[1].enabled = true;
}

bool vector_document_tree_sync(VectorDocumentTree *tree, const VectorDocument *document,
                               PUIView *view, char *error, size_t capacity) {
    if (unchanged(tree, document)) {
        sync_selection(document, view);
        return true;
    }
    int count = document ? document->artboard_count + document->rectangle_count +
                               vector_group_tree_count(document)
                         : 0;
    PUITreeItem *items = count ? calloc(count, sizeof *items) : NULL;
    if (count && !items) {
        snprintf(error, capacity, "could not allocate document tree rows");
        return false;
    }
    int row = 0;
    if (document) {
        for (int i = document->artboard_count - 1; i >= 0; i--) {
            const VectorArtboard *board = &document->artboards[i];
            fill(&items[row++], document->id, board->id, board->name, "artboard", board->flags, 0);
            unsigned emitted[VECTOR_MAX_RECTANGLES] = {0};
            int group_count = 0;
            for (int j = document->rectangle_count - 1; j >= 0; j--) {
                const VectorRectangle *rectangle = &document->rectangles[j];
                if (rectangle->parent_id != board->id) {
                    continue;
                }
                unsigned group = rectangle->group_id;
                bool seen = false;
                for (int g = 0; g < group_count; g++) {
                    seen |= emitted[g] == group;
                }
                if (group && seen) {
                    continue;
                }
                if (group) {
                    emitted[group_count++] = group;
                    char name[64];
                    snprintf(name, sizeof name, "Gruppe %u", group);
                    fill(&items[row], document->id, group, name, "group", 0, 1);
                    snprintf(items[row].icon, sizeof items[row].icon, "tabler:folder");
                    items[row].cell_count = 0;
                    items[row++].selected = vector_document_is_selected(document, rectangle->id);
                }
                for (int k = group ? document->rectangle_count - 1 : j; k >= 0; k--) {
                    const VectorRectangle *child = &document->rectangles[k];
                    if (group ? child->group_id != group : k != j) {
                        continue;
                    }
                    items[row].selected = vector_document_is_selected(document, child->id);
                    fill(&items[row++], document->id, child->id, child->name, "rectangle",
                         child->flags, group ? 2 : 1);
                    if (child->path.count || child->curve.count) {
                        snprintf(items[row - 1].icon, sizeof items[row - 1].icon,
                                 "tabler:vector-bezier-2");
                    }
                    if (!group) {
                        break;
                    }
                }
            }
        }
    }
    bool updated = pui_view_set_tree_items(view, "vector_document_tree", items, row);
    free(items);
    if (!updated) {
        snprintf(error, capacity, "could not update vector_document_tree");
        return false;
    }
    sync_selection(document, view);
    tree->initialized = true;
    tree->document_id = document ? document->id : 0;
    tree->count = document ? document->artboard_count : 0;
    tree->rectangle_count = document ? document->rectangle_count : 0;
    for (int i = 0; i < tree->count; i++) {
        tree->flags[i] = document->artboards[i].flags;
        tree->artboard_ids[i] = document->artboards[i].id;
        snprintf(tree->names[i], sizeof tree->names[i], "%s", document->artboards[i].name);
    }
    tree->selection = document ? document->selection : (VectorRectangleSelection){0};
    tree->primary = document ? document->active_rectangle : 0;
    if (document) {
        memcpy(tree->rectangles, document->rectangles,
               document->rectangle_count * sizeof *document->rectangles);
    }
    return true;
}

void vector_document_tree_context(VectorDocument *document, PUIView *view, float x, float y) {
    const char *row = pui_view_tree_item_at(view, "vector_document_tree", x, y);
    if (!row) {
        return;
    }
    char id[64];
    for (int i = 0; i < document->rectangle_count; i++) {
        unsigned group = document->rectangles[i].group_id;
        snprintf(id, sizeof id, "vector_document_%u.group_%u", document->id, group);
        if (group && !strcmp(row, id)) {
            if (!vector_document_is_selected(document, document->rectangles[i].id)) {
                vector_document_select(document, document->rectangles[i].id);
            }
            return;
        }
    }
    for (int i = 0; i < document->rectangle_count; i++) {
        unsigned object = document->rectangles[i].id;
        snprintf(id, sizeof id, "vector_document_%u.rectangle_%u", document->id, object);
        if (!strcmp(row, id)) {
            if (!vector_document_is_selected(document, object)) {
                vector_document_select(document, object);
            }
            return;
        }
    }
    for (int i = 0; i < document->artboard_count; i++) {
        unsigned object = document->artboards[i].id;
        snprintf(id, sizeof id, "vector_document_%u.artboard_%u", document->id, object);
        if (!strcmp(row, id)) {
            vector_document_select(document, 0);
            document->active_artboard = object;
            return;
        }
    }
}

bool vector_document_tree_action(VectorDocument *document, PUIView *view, const char *action,
                                 const char *widget) {
    if (vector_group_tree_action(document, action, widget)) {
        return true;
    }
    bool rectangle = !strncmp(action, "vector.rectangle.", 17);
    const char *kind = rectangle ? "rectangle" : "artboard";
    char expected[64];
    snprintf(expected, sizeof expected, "vector.%s.visibility", kind);
    bool visibility = !strcmp(action, expected);
    snprintf(expected, sizeof expected, "vector.%s.lock", kind);
    bool lock = !strcmp(action, expected);
    snprintf(expected, sizeof expected, "vector.%s.select", kind);
    if (!visibility && !lock && strcmp(action, expected)) {
        return false;
    }
    if (!document || !widget) {
        return true;
    }
    if (!visibility && !lock &&
        (document->artboard_tool.enabled || document->rectangle_tool.enabled)) {
        return true;
    }
    int count = rectangle ? document->rectangle_count : document->artboard_count;
    for (int i = 0; i < count; i++) {
        unsigned object = rectangle ? document->rectangles[i].id : document->artboards[i].id;
        char id[64];
        snprintf(id, sizeof id, "vector_document_%u.%s_%u", document->id, kind, object);
        if (strcmp(id, widget)) {
            continue;
        }
        vector_artboard_tool_cancel(document);
        if (visibility || lock) {
            unsigned flags =
                rectangle ? document->rectangles[i].flags : document->artboards[i].flags;
            flags ^= visibility ? VECTOR_OBJECT_HIDDEN : VECTOR_OBJECT_LOCKED;
            bool changed = rectangle ? vector_rectangle_set_state(document, object, flags)
                                     : vector_artboard_set_state(document, object, flags);
            pui_view_set_text(view, "status",
                              changed ? "Objektstatus aktualisiert"
                                      : "Objektstatus konnte nicht geaendert werden");
        } else {
            document->active_artboard = rectangle ? document->rectangles[i].parent_id : object;
            if (rectangle && (SDL_GetModState() & SDL_KMOD_SHIFT)) {
                vector_document_toggle(document, object);
            } else {
                vector_document_select(document, rectangle ? object : 0);
            }
        }
        break;
    }
    vector_toolbar_sync(view, document);
    return true;
}

/** @file @brief Artboards become root rows; future groups/elements may supply deeper rows. */
#include "document_tree.h"
#include "editors/vector/artboard/selection.h"
#include "editors/vector/artboard/tool.h"
#include "toolbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool unchanged(const VectorDocumentTree *tree, const VectorDocument *document) {
    if (!tree->initialized || tree->document_id != (document ? document->id : 0) ||
        tree->count != (document ? document->artboard_count : 0)) {
        return false;
    }
    for (int i = 0; i < tree->count; i++) {
        if (tree->artboard_ids[i] != document->artboards[i].id ||
            strcmp(tree->names[i], document->artboards[i].name) ||
            tree->flags[i] != document->artboards[i].flags) {
            return false;
        }
    }
    return true;
}

static void sync_selection(const VectorDocument *document, PUIView *view) {
    char id[64];
    if (document && document->active_artboard) {
        snprintf(id, sizeof id, "vector_document_%u.artboard_%u", document->id,
                 document->active_artboard);
        pui_view_select_tree_item(view, "vector_document_tree", id);
    } else {
        pui_view_select_tree_item(view, "vector_document_tree", NULL);
    }
}

bool vector_document_tree_sync(VectorDocumentTree *tree, const VectorDocument *document,
                               PUIView *view, char *error, size_t capacity) {
    if (unchanged(tree, document)) {
        sync_selection(document, view);
        return true;
    }
    int count = document ? document->artboard_count : 0;
    PUITreeItem *items = count ? calloc(count, sizeof *items) : NULL;
    if (count && !items) {
        snprintf(error, capacity, "could not allocate document tree rows");
        return false;
    }
    for (int i = 0; i < count; i++) {
        const VectorArtboard *artboard = &document->artboards[i];
        PUITreeItem *item = &items[i];
        snprintf(item->id, sizeof item->id, "vector_document_%u.artboard_%u", document->id,
                 artboard->id);
        snprintf(item->text, sizeof item->text, "%s", artboard->name);
        snprintf(item->icon, sizeof item->icon, "tabler:artboard");
        snprintf(item->action, sizeof item->action, "vector.artboard.select");
        item->cell_count = 2;
        snprintf(item->cells[0].icon, sizeof item->cells[0].icon, "%s",
                 artboard->flags & VECTOR_OBJECT_HIDDEN ? "tabler:eye-off" : "tabler:eye");
        snprintf(item->cells[0].action, sizeof item->cells[0].action, "vector.artboard.visibility");
        item->cells[0].checked = !(artboard->flags & VECTOR_OBJECT_HIDDEN);
        item->cells[0].enabled = true;
        snprintf(item->cells[1].icon, sizeof item->cells[1].icon, "%s",
                 artboard->flags & VECTOR_OBJECT_LOCKED ? "tabler:lock" : "tabler:lock-open");
        snprintf(item->cells[1].action, sizeof item->cells[1].action, "vector.artboard.lock");
        item->cells[1].checked = (artboard->flags & VECTOR_OBJECT_LOCKED) != 0;
        item->cells[1].enabled = true;
        item->depth = 0;
        item->enabled = item->open = true;
    }
    bool updated = pui_view_set_tree_items(view, "vector_document_tree", items, count);
    free(items);
    if (!updated) {
        snprintf(error, capacity, "could not update vector_document_tree");
        return false;
    }
    sync_selection(document, view);
    tree->initialized = true;
    tree->document_id = document ? document->id : 0;
    tree->count = count;
    for (int i = 0; i < count; i++) {
        tree->flags[i] = document->artboards[i].flags;
        tree->artboard_ids[i] = document->artboards[i].id;
        snprintf(tree->names[i], sizeof tree->names[i], "%s", document->artboards[i].name);
    }
    return true;
}

bool vector_document_tree_action(VectorDocument *document, PUIView *view, const char *action,
                                 const char *widget) {
    bool visibility = !strcmp(action, "vector.artboard.visibility"),
         lock = !strcmp(action, "vector.artboard.lock");
    if (!visibility && !lock && strcmp(action, "vector.artboard.select")) {
        return false;
    }
    if (document && widget && (visibility || lock || !document->artboard_tool.enabled)) {
        for (int i = 0; i < document->artboard_count; i++) {
            char id[64];
            snprintf(id, sizeof id, "vector_document_%u.artboard_%u", document->id,
                     document->artboards[i].id);
            if (!strcmp(id, widget)) {
                if (visibility || lock) {
                    vector_selection_cancel(document);
                    bool changed = vector_artboard_set_state(
                        document, document->artboards[i].id,
                        document->artboards[i].flags ^
                            (visibility ? VECTOR_OBJECT_HIDDEN : VECTOR_OBJECT_LOCKED));
                    pui_view_set_text(view, "status",
                                      changed ? "Objektstatus aktualisiert"
                                              : "Objektstatus konnte nicht geaendert werden");
                } else {
                    vector_artboard_tool_cancel(document);
                    document->active_artboard = document->artboards[i].id;
                }
                break;
            }
        }
    }
    vector_toolbar_sync(view, document);
    return true;
}

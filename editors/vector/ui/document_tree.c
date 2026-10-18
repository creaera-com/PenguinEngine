/** @file @brief Artboards become root rows; future groups/elements may supply deeper rows. */
#include "document_tree.h"
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
            strcmp(tree->names[i], document->artboards[i].name)) {
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
        tree->artboard_ids[i] = document->artboards[i].id;
        snprintf(tree->names[i], sizeof tree->names[i], "%s", document->artboards[i].name);
    }
    return true;
}

bool vector_document_tree_action(VectorDocument *document, PUIView *view, const char *action,
                                 const char *widget) {
    if (strcmp(action, "vector.artboard.select")) {
        return false;
    }
    if (document && widget) {
        for (int i = 0; i < document->artboard_count; i++) {
            char id[64];
            snprintf(id, sizeof id, "vector_document_%u.artboard_%u", document->id,
                     document->artboards[i].id);
            if (!strcmp(id, widget)) {
                vector_artboard_tool_cancel(document);
                document->active_artboard = document->artboards[i].id;
                break;
            }
        }
    }
    vector_toolbar_sync(view, document);
    return true;
}

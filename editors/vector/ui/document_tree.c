/** @file @brief Artboards become root rows; future groups/elements may supply deeper rows. */
#include "document_tree.h"
#include <stdio.h>
#include <stdlib.h>

static bool unchanged(const VectorDocumentTree *tree, const VectorDocument *document) {
    if (!tree->initialized || tree->document_id != (document ? document->id : 0) ||
        tree->count != (document ? document->artboard_count : 0)) {
        return false;
    }
    for (int i = 0; i < tree->count; i++) {
        if (tree->artboard_ids[i] != document->artboards[i].id) {
            return false;
        }
    }
    return true;
}

bool vector_document_tree_sync(VectorDocumentTree *tree, const VectorDocument *document,
                               PUIView *view, char *error, size_t capacity) {
    if (unchanged(tree, document)) {
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
        snprintf(item->text, sizeof item->text, "Artboard %u", artboard->id);
        snprintf(item->icon, sizeof item->icon, "tabler:artboard");
        item->depth = 0;
        item->enabled = item->open = true;
    }
    bool updated = pui_view_set_tree_items(view, "vector_document_tree", items, count);
    free(items);
    if (!updated) {
        snprintf(error, capacity, "could not update vector_document_tree");
        return false;
    }
    tree->initialized = true;
    tree->document_id = document ? document->id : 0;
    tree->count = count;
    for (int i = 0; i < count; i++) {
        tree->artboard_ids[i] = document->artboards[i].id;
    }
    return true;
}

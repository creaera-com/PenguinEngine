/** @file @brief Group row identity is UI data; group membership remains in the object model. */
#include "groups.h"
#include "editors/vector/document/selection.h"
#include <stdio.h>
#include <string.h>

unsigned vector_group_tree_member(const VectorDocument *document, unsigned group) {
    for (int i = 0; group && i < document->rectangle_count; i++) {
        if (document->rectangles[i].group_id == group && !document->rectangles[i].clip_mask) {
            return document->rectangles[i].id;
        }
    }
    return 0;
}

int vector_group_tree_count(const VectorDocument *document) {
    int count = 0;
    for (int i = 0; i < document->rectangle_count; i++) {
        unsigned group = document->rectangles[i].group_id;
        if (group && vector_group_tree_member(document, group) == document->rectangles[i].id) {
            count++;
        }
    }
    return count;
}

bool vector_group_tree_action(VectorDocument *document, const char *action, const char *widget) {
    if (strcmp(action, "vector.group.select")) {
        return false;
    }
    if (!document || !widget || document->artboard_tool.enabled ||
        document->rectangle_tool.enabled) {
        return true;
    }
    for (int i = 0; i < document->rectangle_count; i++) {
        unsigned group = document->rectangles[i].group_id;
        char id[64];
        snprintf(id, sizeof id, "vector_document_%u.group_%u", document->id, group);
        if (group && !strcmp(id, widget)) {
            if (SDL_GetModState() & SDL_KMOD_SHIFT) {
                vector_document_toggle(document, vector_group_tree_member(document, group));
            } else {
                vector_document_select(document, vector_group_tree_member(document, group));
            }
            break;
        }
    }
    return true;
}

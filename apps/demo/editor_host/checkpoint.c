/** @file @brief Transactional temporary checkpoint storage independent of editor domains. */
#include "checkpoint.h"
#include "platform/temp_file.h"
#include <stdlib.h>

static bool fail(char *error, size_t capacity, const char *message) {
    if (error && capacity) {
        snprintf(error, capacity, "%s", message);
    }
    return false;
}

bool editor_checkpoint_capture(const EditorDefinition *definition, const void *instance,
                               const PUIView *view, EditorCheckpoint **result, char *error,
                               size_t capacity) {
    *result = NULL;
    if (!definition->checkpoint) {
        return true;
    }
    if (!definition->restore) {
        return fail(error, capacity, "editor checkpoint requires a restore callback");
    }
    EditorCheckpoint *staged = calloc(1, sizeof *staged);
    if (!staged) {
        return fail(error, capacity, "could not allocate editor checkpoint");
    }
    staged->definition = definition;
    staged->file = editor_temporary_file();
    if (!staged->file) {
        editor_checkpoint_destroy(staged);
        return fail(error, capacity, "could not create temporary editor checkpoint");
    }
    if (!definition->checkpoint(instance, view, staged->file, error, capacity)) {
        editor_checkpoint_destroy(staged);
        return false;
    }
    if (fflush(staged->file) || fseek(staged->file, 0, SEEK_SET)) {
        editor_checkpoint_destroy(staged);
        return fail(error, capacity, "could not flush editor checkpoint");
    }
    *result = staged;
    return true;
}

FILE *editor_checkpoint_find(EditorCheckpoint *list, const EditorDefinition *definition) {
    for (; list; list = list->next) {
        if (list->definition == definition) {
            return list->file;
        }
    }
    return NULL;
}

bool editor_checkpoint_restore(const EditorDefinition *definition, void *instance, FILE *file,
                               char *error, size_t capacity) {
    if (!file) {
        return true;
    }
    clearerr(file);
    if (!definition->restore || fseek(file, 0, SEEK_SET)) {
        return fail(error, capacity, "could not read editor checkpoint");
    }
    return definition->restore(instance, file, error, capacity);
}

void editor_checkpoint_publish(EditorCheckpoint **list, EditorCheckpoint *staged) {
    if (!staged) {
        return;
    }
    EditorCheckpoint **link = list;
    while (*link && (*link)->definition != staged->definition) {
        link = &(*link)->next;
    }
    if (*link) {
        EditorCheckpoint *old = *link;
        *link = old->next;
        old->next = NULL;
        editor_checkpoint_destroy(old);
    }
    staged->next = *list;
    *list = staged;
}

void editor_checkpoint_destroy(EditorCheckpoint *list) {
    while (list) {
        EditorCheckpoint *next = list->next;
        if (list->file) {
            fclose(list->file);
        }
        free(list);
        list = next;
    }
}

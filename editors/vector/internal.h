/** @file @brief Private Vector editor lifecycle and panel assembly. */
#ifndef PE_VECTOR_INTERNAL_H
#define PE_VECTOR_INTERNAL_H
#include "document.h"
#include "editor_host/editor.h"

typedef struct {
    PUILayout *layout;
    VectorDocuments documents;
} VectorEditor;

void *vector_editor_create(const EditorServices *services, char *error, size_t capacity);
const PUILayout *vector_editor_layout(const void *instance);
void vector_editor_destroy(void *instance);
/** @brief Route UI actions; structural changes are deferred until commit_ui. */
bool vector_editor_action(void *instance, PUIView *view, const char *name, const char *widget);
/** @brief Apply document UI requests between frames. */
bool vector_editor_commit_ui(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity);
/** @brief Load the inspector's separate UI description; caller owns the result. */
PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity);
void vector_editor_canvas(void *instance, SDL_Renderer *renderer, PUIRect area, const char *widget);
bool vector_editor_event(void *instance, PUIView *view, const SDL_Event *event);
#endif

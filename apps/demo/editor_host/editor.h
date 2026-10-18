/** @file @brief Sole contract between editor modules and the editor host. */
#ifndef PE_EDITOR_H
#define PE_EDITOR_H
#include "pengui/pengui.h"
#include <stdio.h>

/** @brief Borrowed services; resource_root must outlive editor instances. */
typedef struct {
    const char *resource_root;
} EditorServices;

/** @brief A registered module; only create() allocates its runtime state. */
typedef struct {
    const char *id;
    const char *title;
    void *(*create)(const EditorServices *services, char *error, size_t capacity);
    const PUILayout *(*layout)(const void *editor);
    void (*destroy)(void *editor);
    bool (*action)(void *editor, PUIView *view, const char *name, const char *widget);
    /** @brief Optional: apply queued inspector/UI changes between frames. */
    bool (*commit_ui)(void *editor, PUIView *view, PUIContext *input, char *error, size_t capacity);
    /** @brief Optional custom canvas rendering, clipped by the UI view. */
    PUICanvasRenderer canvas;
    /** @brief Optional editor input after the UI has processed an event. */
    bool (*event)(void *editor, PUIView *view, const SDL_Event *event);
    /** @brief Optional paired callbacks: snapshot to a session file and restore a fresh candidate.
     * No pointers, SDL resources or transient gestures may be stored. Restore runs before mount.
     */
    bool (*checkpoint)(const void *editor, const PUIView *view, FILE *file, char *error,
                       size_t capacity);
    bool (*restore)(void *editor, FILE *file, char *error, size_t capacity);
} EditorDefinition;

/** @brief Load a module-owned UI file relative to the configured resource root. */
PUILayout *editor_load_ui(const EditorServices *services, const char *relative, char *error,
                          size_t capacity);
#endif

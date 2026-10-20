/** @file @brief Sole contract between editor modules and the editor host. */
#ifndef PE_EDITOR_H
#define PE_EDITOR_H
#include "fonts.h"
#include "images.h"
#include "pengui/pengui.h"
#include <stdio.h>

/** @brief Borrowed services; resource_root must outlive editor instances. */
typedef struct {
    const char *resource_root;
} EditorServices;

/** @brief Selected document identity for app-level Save/Save As routing. */
typedef struct {
    unsigned document;
    unsigned artboard; ///< Optional placement target for imports.
    char filename[4096], title[64];
    const char *extension;
} EditorFileInfo;

/** @brief A registered module; only create() allocates its runtime state. */
typedef struct {
    const char *id;
    const char *title;
    /** @brief Optional file integration; only the registry knows which editor owns each extension.
     */
    bool (*accepts_file)(const char *path);
    bool (*file_info)(void *editor, const PUIView *view, EditorFileInfo *info);
    bool (*open_file)(void *editor, PUIView *view, PUIContext *input, const char *path, char *error,
                      size_t capacity);
    bool (*save_file)(void *editor, PUIView *view, unsigned document, const char *path, char *error,
                      size_t capacity);
    /** @brief Optional image placement in a captured document/artboard using a project copy. */
    bool (*import_image)(void *editor, PUIView *view, unsigned document, unsigned artboard,
                         const char *source, const char *project, char *error, size_t capacity);
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

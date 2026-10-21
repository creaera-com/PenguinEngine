/** @file @brief Document tabs and file identity; implements the common host file contract. */
#ifndef PENGUENGINE_VECTOR_WORKSPACE_FILES_H
#define PENGUENGINE_VECTOR_WORKSPACE_FILES_H
#include "editor_host/editor.h"
/** @brief Recognize the editor-owned .pengvec extension. */
bool vector_workspace_accepts_file(const char *path);
/** @brief Copy active-tab identity for deferred app Save/Save As operations. */
bool vector_workspace_file_info(void *editor, const PUIView *view, EditorFileInfo *info);
/** @brief Resolve a document tab for app-level unsaved-change confirmation. */
bool vector_workspace_close_info(void *editor, const PUIView *view, const char *widget,
                                 EditorFileInfo *info);
/** @brief Remove a confirmed document and its owned history after successful tab removal. */
bool vector_workspace_close_file(void *editor, PUIView *view, PUIContext *input, unsigned document,
                                 char *error, size_t capacity);
/** @brief Validate a candidate and insert its canvas/tab only on successful loading. */
bool vector_workspace_open_file(void *editor, PUIView *view, PUIContext *input, const char *path,
                                char *error, size_t capacity);
/** @brief Save the captured document ID and update its tab caption on success. */
bool vector_workspace_save_file(void *editor, PUIView *view, unsigned document, const char *path,
                                char *error, size_t capacity);
#endif

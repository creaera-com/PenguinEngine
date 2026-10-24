/** @file
 * @brief Versioned, session-local persistence of Vector documents and command data. */
#ifndef PE_VECTOR_CHECKPOINT_H
#define PE_VECTOR_CHECKPOINT_H
#include "editor_host/editor.h"
/** @brief Write model, complete histories, viewports and document-tab order/selection. */
bool vector_editor_checkpoint(const void *instance, const PUIView *view, FILE *file, char *error,
                              size_t capacity);
/** @brief Restore into a fresh instance and rebuild its layout before mounting. */
bool vector_editor_restore(void *instance, FILE *file, char *error, size_t capacity);
#endif

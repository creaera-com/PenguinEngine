/** @file @brief Undoable shape value changes, independent of tools and UI. */
#ifndef PE_VECTOR_SHAPE_COMMANDS_H
#define PE_VECTOR_SHAPE_COMMANDS_H
#include "editors/vector/document/document.h"
/** @brief Add a validated shape with ID zero; assigns an ID only on success. */
bool vector_shape_add(VectorDocument *document, const VectorShape *shape);
/** @brief Replace one shape with matching ID/parent; honors locks and validation. */
bool vector_shape_update(VectorDocument *document, const VectorShape *shape);
/** @brief Coalesce typing after the supplied history cursor, never unrelated commands. */
bool vector_shape_update_typing(VectorDocument *document, const VectorShape *shape, int start);
/** @brief Private native command codec, tag 8. */
bool vector_shape_command_write(const VectorCommand *command, FILE *file);
bool vector_shape_command_read(VectorCommand *command, FILE *file);
#endif

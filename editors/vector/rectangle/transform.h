/** @file @brief Atomic, undoable translation of several rectangle children. */
#ifndef PE_VECTOR_RECTANGLE_TRANSFORM_H
#define PE_VECTOR_RECTANGLE_TRANSFORM_H
#include "rectangle.h"
/** @brief Translate stable IDs together; validation failure leaves every rectangle unchanged. */
bool vector_rectangles_translate(struct VectorDocument *document, const unsigned *ids, int count,
                                 float dx, float dy);
/** @brief Native checkpoint codec for the group translation command. */
bool vector_rectangles_transform_write(const VectorCommand *command, FILE *file);
bool vector_rectangles_transform_read(VectorCommand *command, FILE *file);
#endif

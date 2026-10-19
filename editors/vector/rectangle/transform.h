/** @file @brief Atomic, undoable transforms of several rectangle children. */
#ifndef PE_VECTOR_RECTANGLE_TRANSFORM_H
#define PE_VECTOR_RECTANGLE_TRANSFORM_H
#include "rectangle.h"
/** @brief Translate stable IDs together; validation failure leaves every rectangle unchanged. */
bool vector_rectangles_translate(struct VectorDocument *document, const unsigned *ids, int count,
                                 float dx, float dy);
/** @brief Apply complete target geometries atomically as one undoable command. */
bool vector_rectangles_set_rects(struct VectorDocument *document, const unsigned *ids,
                                 const PUIRect *rects, int count);
/** @brief Apply geometries and normalized angles together as one atomic command. */
bool vector_rectangles_set_transforms(struct VectorDocument *document, const unsigned *ids,
                                      const PUIRect *rects, const float *angles, int count);
/** @brief Native checkpoint codec for the group transform command. */
bool vector_rectangles_transform_write(const VectorCommand *command, FILE *file);
bool vector_rectangles_transform_read(VectorCommand *command, FILE *file);
#endif

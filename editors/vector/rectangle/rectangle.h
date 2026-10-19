/** @file @brief Rectangle children with artboard-local coordinates and undoable edits. */
#ifndef PE_VECTOR_RECTANGLE_H
#define PE_VECTOR_RECTANGLE_H
#include "editors/vector/artboard/artboard.h"
#define VECTOR_MAX_RECTANGLES 64
#define VECTOR_RECTANGLE_DEFAULT_FILL 0x7393afffu

#include "editors/vector/object/shape.h"
/** @brief Compatibility name for the existing shape collection and rectangle property APIs. */
typedef VectorShape VectorRectangle;
struct VectorDocument;
/** @brief Validate a value before accepting clipboard or checkpoint data. */
bool vector_rectangle_valid(VectorRectangle rectangle);
/** @brief Find a rectangle by stable model ID. */
VectorRectangle *vector_rectangle_find(struct VectorDocument *document, unsigned id);
/** @brief Effective flags combine child and parent visibility/lock state. */
unsigned vector_rectangle_effective_flags(struct VectorDocument *document,
                                          const VectorRectangle *rectangle);
/** @brief Create a rectangle inside its visible, editable parent through history. */
bool vector_rectangle_add(struct VectorDocument *document, unsigned parent_id, PUIRect rect);
/** @brief Update properties atomically; inherited locks block property changes. */
bool vector_rectangle_update(struct VectorDocument *document, unsigned id, const char *name,
                             PUIRect rect);
/** @brief Undoable angle input; finite values wrap by signed full turns and honor inherited locks.
 */
bool vector_rectangle_set_angle(struct VectorDocument *document, unsigned id, float degrees);
/** @brief Undoable fill color change; inherited locks prevent editing. */
bool vector_rectangle_set_fill(struct VectorDocument *document, unsigned id, uint32_t rgba);
/** @brief Atomically change stroke color, width and placement through history; honors locks. */
bool vector_rectangle_set_stroke(struct VectorDocument *document, unsigned id, VectorStroke stroke);
/** @brief Change this child's own visibility/lock flags, independently of inherited state. */
bool vector_rectangle_set_state(struct VectorDocument *document, unsigned id, unsigned flags);
/** @brief Native session codecs write values, never pointers. */
bool vector_rectangle_command_write(const VectorCommand *command, FILE *file);
bool vector_rectangle_command_read(VectorCommand *command, FILE *file, unsigned kind);
#endif

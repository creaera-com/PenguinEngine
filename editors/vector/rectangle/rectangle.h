/** @file @brief Rectangle children with artboard-local coordinates and undoable edits. */
#ifndef PE_VECTOR_RECTANGLE_H
#define PE_VECTOR_RECTANGLE_H
#include "editors/vector/artboard/artboard.h"
#define VECTOR_MAX_RECTANGLES 64
#define VECTOR_RECTANGLE_DEFAULT_FILL 0x7393afffu

/** @brief Rectangle geometry is relative to parent_id, never to the viewport. */
typedef struct {
    unsigned id, parent_id, flags;
    uint32_t fill; ///< Packed 0xRRGGBBAA; alpha is actual rendering opacity.
    char name[64];
    PUIRect rect;
    float angle; ///< Clockwise degrees around the rectangle center, normalized to [-180,180).
} VectorRectangle;
struct VectorDocument;
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
/** @brief Undoable fill color change; inherited locks prevent editing. */
bool vector_rectangle_set_fill(struct VectorDocument *document, unsigned id, uint32_t rgba);
/** @brief Change this child's own visibility/lock flags, independently of inherited state. */
bool vector_rectangle_set_state(struct VectorDocument *document, unsigned id, unsigned flags);
/** @brief Native session codecs write values, never pointers. */
bool vector_rectangle_command_write(const VectorCommand *command, FILE *file);
bool vector_rectangle_command_read(VectorCommand *command, FILE *file, unsigned kind);
#endif

/** @file @brief History-backed Bezier creation and node editing, independent of UI. */
#ifndef PE_VECTOR_CURVE_H
#define PE_VECTOR_CURVE_H
#include "editors/vector/document/document.h"
/** @brief Build a normalized shape from a parent-local curve, retaining supplied style/identity. */
bool vector_curve_shape(VectorShape *shape, const VectorBezier *parent_curve);
/** @brief Map normalized anchors/handles to parent pixels, including rotation. */
VectorBezier vector_curve_parent(const VectorShape *shape);
/** @brief Add an open/closed curve with a fresh ID through one history command. */
bool vector_curve_add(VectorDocument *document, unsigned parent, const VectorBezier *curve);
/** @brief Commit node changes to one existing curve; preserves other selected/grouped shapes. */
bool vector_curve_update(VectorDocument *document, unsigned id, const VectorBezier *curve);
/** @brief Native value codec for curve command kind 8. */
bool vector_curve_command_write(const VectorCommand *command, FILE *file);
bool vector_curve_command_read(VectorCommand *command, FILE *file);
#endif

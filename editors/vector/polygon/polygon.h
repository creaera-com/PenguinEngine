/** @file
 * @brief Parametric regular polygons retain their corner count until path conversion. */
#ifndef PE_VECTOR_POLYGON_H
#define PE_VECTOR_POLYGON_H
#include "editors/vector/document/document.h"
#define VECTOR_POLYGON_MAX_SIDES VECTOR_BEZIER_NODES
/** @brief Build a normalized straight-segment polygon, with 3..max corners. */
bool vector_polygon_shape(PUIRect rect, unsigned sides, VectorShape *out);
/** @brief Create a polygon through one undoable command. */
bool vector_polygon_add(VectorDocument *document, unsigned parent, PUIRect rect, unsigned sides);
/** @brief Change corner count without changing identity, style, transform or other selections. */
bool vector_polygon_set_sides(VectorDocument *document, unsigned id, unsigned sides);
#endif

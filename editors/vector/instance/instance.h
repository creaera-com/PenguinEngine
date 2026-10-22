/** @file @brief Linked shape instances with independent placement and scale. */
#ifndef VECTOR_INSTANCE_H
#define VECTOR_INSTANCE_H
#include "editors/vector/document/document.h"
/** @brief Resolve current source geometry/style into caller-owned storage; never mutate the model.
 * Missing sources retain the instance's saved fallback appearance. Cycles resolve to fallback. */
void vector_instance_resolve(const VectorDocument *document, const VectorShape *shape,
                             VectorShape *out);
/** @brief Resolve inherited dimensions while retaining the instance's local placement. */
PUIRect vector_instance_rect(const VectorDocument *document, const VectorShape *shape);
/** @brief Convert displayed dimensions back to stored instance scale coordinates. */
PUIRect vector_instance_store_rect(const VectorDocument *document, const VectorShape *shape,
                                   PUIRect rect);
/** @brief Preflight/create linked instances of selected shapes/groups as one history command. */
bool vector_instances_can_create(VectorDocument *document);
bool vector_instances_create(VectorDocument *document);
/** @brief Bake selected instances' current appearance into independent objects, with undo. */
bool vector_instances_detach(VectorDocument *document);
/** @brief Linked appearances may only change through their source or explicit detaching. */
bool vector_instance_edit_valid(const VectorShape *before, const VectorShape *after);
/** @brief Validate reference graph without requiring external/missing sources to exist. */
bool vector_instances_valid(const VectorShape *shapes, int count);
#endif

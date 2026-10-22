/** @file
 * @brief Non-destructive clipping groups and geometry-aware picking, without UI state. */
#ifndef PE_VECTOR_MASK_H
#define PE_VECTOR_MASK_H
#include "editors/vector/document/document.h"

/** @brief Find the special closed mask child of a group, or NULL for ordinary groups. */
const VectorShape *vector_mask_find(const VectorDocument *document, unsigned group);
/** @brief Check complete, editable same-artboard selection; its top independent path is the mask.
 */
bool vector_mask_can_create(VectorDocument *document);
/** @brief Create one clipping group in one reversible structural command. */
bool vector_mask_create(VectorDocument *document);
/** @brief Check whether the selection contains a complete editable clipping group. */
bool vector_mask_can_release(VectorDocument *document);
/** @brief Restore the mask's visible style while retaining an ordinary group, atomically. */
bool vector_mask_release(VectorDocument *document);
/** @brief Pick visible content only; a mask child itself is selected explicitly through the tree.
 */
bool vector_mask_contains(const VectorDocument *document, const VectorShape *shape, float x,
                          float y);
/** @brief Box intersection of clipped filled content, excluding mask children. */
bool vector_mask_intersects(const VectorDocument *document, const VectorShape *shape, PUIRect box);
#endif

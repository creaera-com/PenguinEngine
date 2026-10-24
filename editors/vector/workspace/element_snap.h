/** @file
 * @brief Document-to-geometry binding and transient rendering of object snapping. */
#ifndef PE_VECTOR_WORKSPACE_ELEMENT_SNAP_H
#define PE_VECTOR_WORKSPACE_ELEMENT_SNAP_H
#include "editors/vector/document/document.h"
#include "editors/vector/geometry/element_snap.h"
/** @brief Resolve movement for a single object, artboard or selected group in world coordinates.
 * Hidden objects and the moving selection are excluded. Locked objects remain references. */
VectorElementSnap vector_workspace_element_snap(const VectorDocument *document);
/** @brief Paint dashed screen-space guides during movement; respects the canvas clip. */
void vector_element_guides_draw(const VectorDocument *document, SDL_Renderer *renderer);
#endif

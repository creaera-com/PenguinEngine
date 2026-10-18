/** @file @brief Group-resize gesture snapshots and live rectangle previews. */
#ifndef PE_VECTOR_GROUP_TRANSFORM_H
#define PE_VECTOR_GROUP_TRANSFORM_H
#include "editors/vector/document/document.h"
/** @brief Capture original geometry and world-space union for the active group gesture. */
bool vector_group_capture(VectorDocument *document);
/** @brief Resolve one member's parent-local preview; invalid candidates keep original geometry. */
PUIRect vector_group_display_rect(const VectorDocument *document, const VectorRectangle *rectangle);
/** @brief Commit all preview geometries atomically as one history step. */
bool vector_group_commit(VectorDocument *document);
#endif

/** @file
 * @brief Persistent single-level groups of shapes within an artboard. */
#ifndef PE_VECTOR_OBJECT_GROUP_H
#define PE_VECTOR_OBJECT_GROUP_H
#include "editors/vector/document/document.h"
/** @brief Require at least two independent selected objects/groups in the same unlocked artboard.
 */
bool vector_objects_can_group(VectorDocument *document);
/** @brief Require selected grouped shapes and an editable, complete selection. */
bool vector_objects_can_ungroup(VectorDocument *document);
/** @brief Group selection atomically; existing groups are combined, geometry/style/IDs preserved.
 * Members become a contiguous layer block at the frontmost selected object's position. */
bool vector_objects_group(VectorDocument *document);
/** @brief Remove group membership in one undoable command, preserving geometry and layer order. */
bool vector_objects_ungroup(VectorDocument *document);
#endif

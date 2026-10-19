/** @file @brief Persistent object groups as optional intermediate document-tree rows. */
#ifndef PE_VECTOR_UI_GROUP_TREE_H
#define PE_VECTOR_UI_GROUP_TREE_H
#include "editors/vector/document/document.h"
/** @brief Return one group's first member ID; zero when missing. */
unsigned vector_group_tree_member(const VectorDocument *document, unsigned group);
/** @brief Number of distinct nonempty groups used to allocate tree rows. */
int vector_group_tree_count(const VectorDocument *document);
/** @brief Select/toggle a group row through the existing shape-selection model. */
bool vector_group_tree_action(VectorDocument *document, const char *action, const char *widget);
#endif

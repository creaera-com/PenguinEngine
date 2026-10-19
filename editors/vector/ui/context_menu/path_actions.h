/** @file @brief YAML-to-domain bindings for path conversion and boolean area operations. */
#ifndef PE_VECTOR_UI_PATH_ACTIONS_H
#define PE_VECTOR_UI_PATH_ACTIONS_H
#include "editors/vector/document/document.h"
/** @brief Recognize semantic path actions and report result without structural UI edits. */
bool vector_path_actions_action(VectorDocument *document, PUIView *view, const char *name);
/** @brief Update eligibility only; no tessellation on every frame or menu hover. */
void vector_path_actions_sync(VectorDocument *document, PUIView *view);
#endif

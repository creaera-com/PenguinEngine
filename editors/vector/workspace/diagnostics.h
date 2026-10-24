/** @file
 * @brief Adapter-facing read-only document and gesture diagnostics. */
#ifndef VECTOR_WORKSPACE_DIAGNOSTICS_H
#define VECTOR_WORKSPACE_DIAGNOSTICS_H
#include "editor_host/editor.h"
/** @brief Count actual objects/virtual instances and name the current interaction. */
EditorDiagnostics vector_workspace_diagnostics(const void *instance, const PUIView *view);
#endif

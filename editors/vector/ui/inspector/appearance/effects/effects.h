/** @file
 * @brief YAML effect panel and field-to-history translation for individual objects. */
#ifndef PE_VECTOR_UI_INSPECTOR_EFFECTS_H
#define PE_VECTOR_UI_INSPECTOR_EFFECTS_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
/** @brief Compose reusable effect controls into an inspector layout; consumes base. */
PUILayout *vector_effects_layout(const EditorServices *services, PUILayout *base, char *error,
                                 size_t capacity);
/** @brief Commit one effect property through the generic shape command; honor inherited locks. */
bool vector_effects_action(VectorDocument *document, unsigned id, PUIView *view,
                           const char *action);
/** @brief Refresh fields after selection/history changes without disrupting unrelated input. */
void vector_effects_sync(PUIView *view, const VectorShadow *value, const VectorShadow *previous,
                         bool reset);
#endif

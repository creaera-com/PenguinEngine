/** @file
 * @brief Inspector bindings for closed-area fill gradients and editable color stops. */
#ifndef PE_VECTOR_UI_GRADIENT_H
#define PE_VECTOR_UI_GRADIENT_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"

/** @brief Stop selection is inspector state, not part of the artwork or undo history. */
typedef struct {
    unsigned stop;
    bool mounted, dirty;
} VectorGradientInspector;

/** @brief Translate one field action into a validated, atomic shape change. */
bool vector_gradient_inspector_action(VectorGradientInspector *ui, VectorDocument *document,
                                      unsigned shape, PUIView *view, const char *action);
/** @brief Refresh committed gradient fields without resetting unrelated focused inputs. */
void vector_gradient_inspector_sync(VectorGradientInspector *ui, PUIView *view,
                                    const VectorGradient *gradient, const VectorGradient *previous,
                                    bool switched);
#endif

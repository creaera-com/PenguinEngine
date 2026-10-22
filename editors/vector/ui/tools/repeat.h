/** @file
 * @brief YAML bindings for repetition drafts; only Apply changes document history. */
#ifndef PE_VECTOR_UI_REPEAT_H
#define PE_VECTOR_UI_REPEAT_H
#include "editors/vector/document/document.h"

typedef struct {
    VectorRepeat draft, presented;
    unsigned document, source, property;
    bool linked, initialized;
} VectorRepeatUI;

/** @brief Translate controls into a draft or one domain command. */
bool vector_repeat_ui_action(VectorRepeatUI *ui, VectorDocument *document, PUIView *view,
                             const char *action);
/** @brief Refresh when source/settings change; leave focused draft fields untouched otherwise. */
void vector_repeat_ui_sync(VectorRepeatUI *ui, VectorDocument *document, PUIView *view);
#endif

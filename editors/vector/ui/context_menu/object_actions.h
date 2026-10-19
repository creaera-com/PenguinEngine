/** @file @brief Context-menu and shortcut bindings for document object operations. */
#ifndef PE_VECTOR_OBJECT_ACTIONS_H
#define PE_VECTOR_OBJECT_ACTIONS_H
#include "editors/vector/object/clipboard.h"
#include <SDL3/SDL.h>

/** @brief Clipboard owns values across document tabs; menu opening waits for UI composition. */
typedef struct {
    VectorClipboard clipboard;
    bool pending;
    float x, y;
    unsigned menu_document;
} VectorObjectActions;

/** @brief Translate semantic menu actions into domain commands. */
bool vector_object_actions_action(VectorObjectActions *actions, VectorDocument *document,
                                  PUIView *view, const char *name);
/** @brief Open context menu on canvas/tree right-click or process unconsumed shortcuts. */
bool vector_object_actions_event(VectorObjectActions *actions, VectorDocument *document,
                                 PUIView *view, const SDL_Event *event);
/** @brief Open after slot changes; close any stale menu when its document changes. */
void vector_object_actions_commit(VectorObjectActions *actions, VectorDocument *document,
                                  PUIView *view);
#endif

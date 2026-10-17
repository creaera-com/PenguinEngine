/** @file
 * @brief Besitz- und Zustandsbereich workspace; keine globale Instanz. */
#ifndef PENGUIN_PEWORKSPACESTATE_H
#define PENGUIN_PEWORKSPACESTATE_H
#include "editor/layout/ui_layout.h"
#include "editor/module.h"
#include "penguin/ui.h"
#include <stdbool.h>

/** @brief Zustand für workspace; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    void *instance; ///< Undurchsichtiger Payload, ausschließlich vom aktiven Backend interpretiert.
    const PeEditorBackend *backend; ///< Besitzer des aktiven Payloads, auch beim Initialisieren.
    UiId loaded_view;               ///< ID der tatsächlich geladenen Editorinstanz.
    char session_paths[UI_MAX_TABS]
                      [1200]; ///< Checkpointpfade je registriertem Editor; maximal UI_MAX_TABS.
    bool session_exists[UI_MAX_TABS]; ///< Ob für den jeweiligen Editor ein Checkpoint vorliegt.
    bool session_dirty[UI_MAX_TABS];  ///< Änderungszustand des jeweils entladenen Editors.
    PeUiLayout *active_layout;        ///< Besitzendes YAML-Modell des geladenen Editors.
    UiId pending_view; ///< Nach dem Frame zu aktivierende Editor-ID; 0 bedeutet keine.
} PeWorkspaceState;
#endif

/** @file
 * @brief Besitz- und Zustandsbereich workspace; keine globale Instanz. */
#ifndef PENGUIN_PEWORKSPACESTATE_H
#define PENGUIN_PEWORKSPACESTATE_H
#include "editor/layout/ui_layout.h"
#include "editor/module.h"
#include "penguin/ui.h"
#include <stdbool.h>

/** @brief Opaque, ausschließlich vom Map-Editor interpretierter Payload. */
typedef struct MapPayload MapPayload;
/** @brief Opaque, ausschließlich vom Vector-Editor interpretierter Payload. */
typedef struct VectorEditor VectorEditor;
/** @brief Opaque, ausschließlich vom Script-Editor interpretierter Payload. */
typedef struct PeScriptEditor PeScriptEditor;

/** @brief Zustand für workspace; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    VectorEditor *vector;   ///< Besitzende Vectorinstanz; NULL bei entladenem Arbeitsbereich.
    MapPayload *map;        ///< Besitzende Mapinstanz oder NULL bei entladenem Map-Editor.
    PeScriptEditor *script; ///< Besitzende Scriptinstanz oder NULL bei entladenem Script-Editor.
    const PeEditorBackend *backend; ///< Besitzer des aktiven Payloads, auch beim Initialisieren.
    UiId loaded_view;               ///< ID der tatsächlich geladenen Editorinstanz.
    char session_paths[3][1200];    ///< Temporäre Checkpointpfade für Map, Scripts und Vector.
    bool session_exists[3];         ///< Ob für den jeweiligen Editor ein Checkpoint vorliegt.
    bool session_dirty[3];          ///< Änderungszustand des jeweils entladenen Editors.
    PeUiLayout *active_layout;      ///< Besitzendes YAML-Modell des geladenen Editors.
    UiId pending_view;              ///< Nach dem Frame zu aktivierende Editor-ID; 0 bedeutet keine.
} PeWorkspaceState;
#endif

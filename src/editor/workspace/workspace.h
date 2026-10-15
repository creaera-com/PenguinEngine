/**
 * @file
 * @brief Private Modulverträge für src/editor/workspace.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_WORKSPACE_WORKSPACE_H
#define PENGUIN_WORKSPACE_WORKSPACE_H
#include "penguin/ui.h"
#include "workspace/session.h"

/** @brief Aufgeschobene Datei-/Beendenanfrage mit möglicher Speichern-Rückfrage. */
typedef enum {
    REQUEST_NONE,
    REQUEST_QUIT,
    REQUEST_OPEN,
    REQUEST_SCRIPT_OPEN,
    REQUEST_IMPORT
} Request;

/**
 * @brief Erzeugt den Ziel-Editor und stellt seinen Checkpoint wieder her oder lädt Standarddaten.
 */
bool editor_activate(UiId id, char *error, size_t size);

/**
 * @brief Serialisiert UI-Snapshot und das Dokument des über opaque bestimmten Editors.
 */
bool editor_checkpoint(PeSession *io, void *opaque);

/**
 * @brief Verarbeitet nach dem Rendern aufgeschobenes Speichern, Öffnen und Editorwechsel.
 */
void editor_process_deferred_ui(void);

/**
 * @brief Lädt Shell und aktives Layout atomar neu; ungültige Beschreibungen bleiben inaktiv.
 */
bool editor_reload_layout(void);

/**
 * @brief Startet Öffnen, Import oder Beenden mit erforderlicher Rückfrage bei Änderungen.
 */
void editor_request(Request type, const char *path);

/**
 * @brief Führt eine bestätigte Anfrage aus und schließt die Rückfrage.
 */
void editor_request_finish(void);

/**
 * @brief Speichert das Dokument des aktiven Editors.
 */
bool editor_save(void);

/**
 * @brief Koordiniert Editorwechsel und Fokus; während des Renderns wird der Wechsel aufgeschoben.
 */
void editor_select_workspace(UiId id);

/**
 * @brief Sichert und entlädt den aktiven Editor und aktiviert den Zielbereich mit Rückfall bei
 * Fehlern.
 */
bool editor_switch(UiId id);

/**
 * @brief Wählt den Script- oder Map-Arbeitsbereich.
 */
void editor_tab_select(bool scripts);

/**
 * @brief Führt Undo oder Redo im aktiven Dokument aus und synchronisiert dessen Ansicht.
 */
void editor_undo_active(bool redo);
/** @} */
#endif

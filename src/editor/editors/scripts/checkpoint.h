/**
 * @file
 * @brief Private Modulverträge für src/editor/editors/scripts.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_SCRIPT_CHECKPOINT_H
#define PENGUIN_SCRIPT_CHECKPOINT_H
#include "editors/scripts/script_editor.h"
#include "workspace/session.h"

/**
 * @brief Liefert die native Größe des Script-Checkpointheaders zur ABI-Prüfung.
 */
size_t se_checkpoint_header_size(void);

/**
 * @brief Serialisiert Scriptdokument, Undo/Redo und Scrollzustand ohne Anwendungssingleton.
 *
 * Lesen in eine nullinitialisierte Ansicht. project_root wird kopiert, window bleibt geliehen und
 * darf NULL sein. Nach Fehler den Textverlauf freigeben.
 *
 * @param editor Scriptansicht; beim Lesen leer und nullinitialisiert.
 * @param project_root In die Scriptansicht zu kopierender Projektwurzelpfad.
 * @param window Geliehenes SDL-Fenster; darf NULL sein.
 * @param io Nur während des Codecs gültiger Lese-/Schreibstream.
 */
bool se_checkpoint(PeScriptEditor *editor, const char *project_root, SDL_Window *window,
                   PeSession *io);
/** @} */
#endif

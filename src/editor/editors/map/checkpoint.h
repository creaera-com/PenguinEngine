/**
 * @file
 * @brief Private Modulverträge für src/editor/editors/map.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_MAP_CHECKPOINT_H
#define PENGUIN_MAP_CHECKPOINT_H
#include "editors/map/map_editor.h"
#include "workspace/session.h"

/**
 * @brief Serialisiert Map-Szene und vollständigen Undo-/Redo-Verlauf ohne Anwendungssingleton.
 *
 * Lesen in einen leeren, nullinitialisierten Payload. Auch nach Lesefehlern den möglicherweise
 * teilweise aufgebauten Verlauf freigeben.
 *
 * @param map Map-Payload; beim Lesen leer und nullinitialisiert.
 * @param io Nur während des Codecs gültiger Lese-/Schreibstream.
 */
bool me_checkpoint(MapPayload *map, PeSession *io);
/** @} */
#endif

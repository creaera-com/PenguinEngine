/** @file
 * @brief Private Zusammenarbeit der Vector-UI-Module; keine Engine-Abhängigkeit für das Dokument.
 */
#ifndef PENGUIN_VECTOR_INTERNAL_H
#define PENGUIN_VECTOR_INTERNAL_H
#include "editor/host.h"
#include "editors/graphics/vector/editor.h"
/** @brief Bereinigt Auswahl und markiert Raster-/Layoutcache als verändert. */
void vector_sync(void);
/** @brief Übernimmt eine gültige Dokumentänderung als einen Undo-Schritt oder stellt sie zurück. */
void vector_commit(void);
/** @brief Liefert den ersten ausgewählten Objekt-Slot, sonst -1. */
int vector_first(void);
/** @brief Anzahl ausgewählter Objekt-Slots. */
int vector_selected_count(void);
/** @brief Löst relative SVG-Pfade gegen das aktuelle Projekt auf. */
bool vector_resolve_path(const char *path, char *out, size_t size);
#endif

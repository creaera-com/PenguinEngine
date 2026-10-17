/** @file
 * @brief Integrationsadapter des map-Editors; ausschließlich am Composition Root
 * registrieren. */
#ifndef PENGUIN_MAP_ADAPTER_H
#define PENGUIN_MAP_ADAPTER_H
#include "editor/module.h"
/** @brief Statischer Vertrag für Payload-Lebenszyklus und Dokumentbefehle. */
extern const PeEditorBackend pe_map_backend;
/** @brief Öffentliche Registrierung ohne Zugriff auf interne Dokumenttypen. */
extern const EditorView pe_map_view;
#endif

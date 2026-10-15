/**
 * @file
 * @brief Private Modulverträge für tests.
 * @ingroup editor_api
 */
#ifndef PENGUIN_EDITOR_SMOKE_H
#define PENGUIN_EDITOR_SMOKE_H
#include <stdbool.h>

/**
 * @brief Prüft Map-Bearbeitung über dieselben Handler wie echte Eingaben.
 */
bool editor_verify_editor(void);
/**
 * @brief Prüft Entladen, Zustandswiederherstellung, Fehlerbehandlung und YAML-Reload.
 */
bool editor_verify_lifecycle(void);
/**
 * @brief Prüft Script-Eingaben, Textzustand und Undo/Redo.
 */
bool editor_verify_script_editor(void);
/**
 * @brief Prüft Panelgeometrie und Interaktionen der YAML-Oberfläche.
 */
bool editor_verify_ui_layout(void);
/**
 * @brief Prüft Editor-Navigation und kontextabhängige Werkzeuge.
 */
bool editor_verify_workspace_tools(void);

#endif

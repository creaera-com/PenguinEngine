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

/** @brief Prüft YAML-Splitter, Konsolenfokus und gemeinsame Sitzung über Editorwechsel. */
bool editor_verify_console(void);
/** @brief Prüft Rahmen-Hit-Test, Fensterbuttons und Größenwiederherstellung. */
bool editor_verify_window(void);
/** @brief Prüft Scene-Baum, Erstellen, Verschieben, Löschen, Verlauf und Editorwechsel. */
bool editor_verify_scene_graph(void);
/** @brief Prüft Komponentenmenü, Karten, Einstellungen, Verlauf und Editorwechsel. */
bool editor_verify_components(void);
#endif

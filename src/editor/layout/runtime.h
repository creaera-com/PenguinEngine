/**
 * @file
 * @brief Private Modulverträge für src/editor/layout.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_LAYOUT_RUNTIME_H
#define PENGUIN_LAYOUT_RUNTIME_H
#include "layout/ui_layout.h"
#include "penguin/ui_sdl.h"

/**
 * @brief Übernimmt Theme, Spaltenvorgaben und Editorliste aus dem Shellmodell.
 */
void editor_apply_shell_layout(void);

/**
 * @brief Zeichnet globale YAML-Toolbaraktionen vor der Editor-Navigation.
 */
void editor_draw_global_tools(void);

/**
 * @brief Zeichnet einen validierten YAML-Knoten rekursiv im zugewiesenen Bereich.
 */
void editor_draw_yaml_node(const cJSON *n, SDL_FRect area, int column);

/**
 * @brief Zeichnet die Werkzeugaktionen des aktiven Editorlayouts.
 */
void editor_draw_yaml_toolbar(SDL_FRect area);
/**
 * @brief Zeichnet eine validierte YAML-Buttonliste in einem lokalen Werkzeugbereich.
 * @param list Geliehene Liste aus dem aktuellen Layout; NULL ist eine leere Liste.
 * @param area Bereich in Fensterpixeln; berücksichtigt den aktiven Clip.
 * @param right_aligned Richtet jede umgebrochene Zeile rechts am Bereich aus.
 */
void editor_draw_yaml_tool_list(const cJSON *list, SDL_FRect area, bool right_aligned);
/**
 * @brief Berechnet die Zeilenzahl einer lokalen YAML-Werkzeugliste.
 * @param list Geliehene Buttonliste; NULL ist eine leere Liste.
 * @param width Verfügbare Breite in Fensterpixeln.
 */
int editor_yaml_tool_rows(const cJSON *list, float width);

/**
 * @brief Liefert einen geliehenen Shell-Registryeintrag zur Editor-ID.
 */
const cJSON *editor_layout_entry(UiId id);

/**
 * @brief Lädt und validiert das zur Editor-ID gehörende Layout relativ zur Shell.
 */
PeUiLayout *editor_load_editor_layout(UiId id, char *error, size_t size);

/**
 * @brief Verteilt globale Dokumentaktionen sowie Map- und Scriptaktionen.
 */
void editor_yaml_action(const char *name);

/**
 * @brief Ermittelt den Auswahlzustand einer YAML-Aktion.
 */
bool editor_yaml_action_active(const char *action);

/**
 * @brief Ermittelt, ob eine YAML-Aktion im aktuellen Kontext bedienbar ist.
 */
bool editor_yaml_action_enabled(const char *action);

/**
 * @brief Wertet ein boolesches Literal oder ein registriertes Binding aus.
 */
bool editor_yaml_boolean(const cJSON *value, bool fallback);

/**
 * @brief Ordnet einem registrierten Iconnamen sein eingebautes Symbol zu.
 */
UiIcon editor_yaml_icon(const char *name);

/**
 * @brief Liefert Literal-/Bindingtext; dynamische Texte werden in den Aufruferpuffer geschrieben.
 *
 * @return Einen geliehenen Literal-/Statuszeiger oder buffer; nicht freigeben und vor dem nächsten
 * Modellwechsel verbrauchen.
 */
const char *editor_yaml_text(const cJSON *value, char *buffer, size_t size);

/** @brief Berechnet die benötigte Zeilenzahl der kontextbezogenen Werkzeugleiste. */
int editor_yaml_toolbar_rows(float width);
/** @brief Prüft, ob ein YAML-Splitter die Zeigerinteraktion eingefangen hat. */
bool editor_layout_captured(void);

/** @brief Prüft, ob ein YAML-Tool-Panel die Mausradposition selbst verwaltet. */
bool editor_layout_scrolls_at(float x, float y);

/** @} */
#endif

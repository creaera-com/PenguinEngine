/**
 * @file
 * @brief Begrenztes Laden und Validieren der deklarativen YAML-Oberfläche.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_UI_LAYOUT_H
#define PENGUIN_UI_LAYOUT_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>

/** @brief Maximale Anzahl YAML-Parserereignisse einschließlich Schlüsseln. */
#define PE_UI_MAX_NODES 2048
/** @brief Maximale YAML-Verschachtelungstiefe. */
#define PE_UI_MAX_DEPTH 32
/** @brief Maximale Größe einer YAML-Datei in Bytes. */
#define PE_UI_MAX_FILE (256 * 1024)

/** @brief Geliehener Modellknoten und seine einbasierte YAML-Quellposition. */
typedef struct {
    const cJSON *value; ///< Geliehener JSON-Modellknoten.
    size_t line;        ///< Zeilenposition; Parserdiagnosen sind einbasiert.
    size_t column;      ///< Spaltenposition; Parserdiagnosen sind einbasiert.
} PeUiMark;

/** @brief Besitzender YAML-Modellbaum einschließlich Quellpositionen für Fehlermeldungen. */
typedef struct {
    cJSON *root;     ///< Besitzender cJSON-Modellbaum; über pe_ui_layout_free freigeben.
    char path[1024]; ///< Nullterminierter Datei- oder Projektpfad.
    PeUiMark *marks; ///< Besitzende Zuordnung der JSON-Knoten zu YAML-Quellpositionen.
    int mark_count;  ///< Anzahl gültiger Quellpositionen.
} PeUiLayout;

/**
 * @brief Lädt YAML, prüft Grenzen und Schema und erzeugt ein besitzendes Layoutmodell.
 *
 * @return Ein neues Modell oder NULL mit Dateipfad und Quellposition im Fehlerpuffer. Mit
 * pe_ui_layout_free() freigeben.
 *
 * @param path Nullterminierter Dateipfad.
 * @param kind Erwartete Layoutart: shell oder editor.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
PeUiLayout *pe_ui_layout_load(const char *path, const char *kind, char *error, size_t size);

/**
 * @brief Gibt Layoutbaum, Quellpositionen und Modell frei; NULL ist erlaubt.
 *
 * @param layout Besitzendes Layoutmodell oder NULL.
 */
void pe_ui_layout_free(PeUiLayout *layout);

/**
 * @brief Liefert einen geliehenen JSON-Mitgliedzeiger oder NULL.
 *
 * Der Zeiger bleibt nur gültig, solange der zugehörige Modellbaum besteht.
 *
 * @param object Geliehener Modellknoten bzw. zu bewegendes Objekt.
 * @param key Exakter, nullterminierter Mitgliedname.
 */
const cJSON *pe_ui_get(const cJSON *object, const char *key);

/**
 * @brief Liest ein Stringmitglied oder liefert den unveränderten Fallback-Zeiger.
 *
 * @param object Geliehener Modellknoten bzw. zu bewegendes Objekt.
 * @param key Exakter, nullterminierter Mitgliedname.
 * @param fallback Geliehener Ersatztext, falls kein String vorliegt.
 */
const char *pe_ui_string(const cJSON *object, const char *key, const char *fallback);

/**
 * @brief Liest einen skalaren Stringwert oder liefert den unveränderten Fallback-Zeiger.
 *
 * @param value Ein-/Ausgabe des veränderbaren Werts bzw. zu prüfender Modellknoten.
 * @param fallback Geliehener Ersatztext, falls kein String vorliegt.
 */
const char *pe_ui_scalar(const cJSON *value, const char *fallback);

/**
 * @brief Prüft einen Aktionsnamen gegen die interne Registry.
 *
 * @param name Nullterminierter Registryname.
 */
bool pe_ui_action_known(const char *name);

/**
 * @brief Prüft einen Bindingnamen gegen die interne Registry.
 *
 * @param name Nullterminierter Registryname.
 */
bool pe_ui_binding_known(const char *name);
/** @} */
#endif

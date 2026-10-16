/**
 * @file
 * @brief Fachlich unabhängige JSON-Datei- und Validierungshelfer für Szenen und Projekte.
 * @ingroup shared_api
 */

/** @addtogroup shared_api
 * @{
 */
#ifndef PENGUIN_JSON_IO_H
#define PENGUIN_JSON_IO_H
#include "cJSON.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Schreibt einen formatierten Fehler, sofern ein Zielpuffer angegeben ist, und liefert
 * false.
 *
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 * @param format printf-kompatibler Formattext.
 * @param ... Zusatzargumente passend zum Formattext.
 */
bool pe_json_fail(char *error, size_t size, const char *format, ...);

/**
 * @brief Liest eine JSON-Datei mit maximal 4 MiB und verlangt vollständiges, nullbytefreies JSON.
 *
 * @return Ein neuer cJSON-Baum (mit cJSON_Delete() freigeben) oder NULL mit Fehlermeldung.
 *
 * @param path Nullterminierter Dateipfad.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
cJSON *pe_json_read(const char *path, char *error, size_t size);

/**
 * @brief Prüft eine endliche JSON-Ganzzahl gegen inklusive Grenzen und schreibt sie bei Erfolg.
 *
 * @param json Geliehener zu prüfender JSON-Wert.
 * @param minimum Inklusive Untergrenze.
 * @param maximum Inklusive Obergrenze bzw. maximale Stringlänge in Bytes.
 * @param value Ein-/Ausgabe des veränderbaren Werts bzw. zu prüfender Modellknoten.
 */
bool pe_json_integer(const cJSON *json, int minimum, int maximum, int *value);

/**
 * @brief Prüft einen relativen Pfad ohne absolute Präfixe, Rückstriche oder Punktsegmente.
 *
 * @param path Nullterminierter Dateipfad.
 * @param maximum_length Maximale Pfadkapazität einschließlich Nullabschluss.
 */
bool pe_json_relative_path(const char *path, size_t maximum_length);
/** @} */
#endif

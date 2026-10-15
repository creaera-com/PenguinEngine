/**
 * @file
 * @brief Temporäre native Sitzungsdateien; kein Austausch- oder Projektformat.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_SESSION_H
#define PENGUIN_SESSION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** @brief Nur während eines Codecrückrufs gültiger, begrenzter Checkpoint-Stream. */
typedef struct {
    FILE *file; ///< Geliehener Stream; nicht selbst schließen oder außerhalb des Codecs verwenden.
    bool reading;       ///< true beim Laden, false beim Speichern.
    bool ok;            ///< Kumulativer Stream-Erfolgszustand; false bleibt ein Fehler.
    uint32_t hash;      ///< Laufende FNV-Prüfsumme der Nutzdaten.
    uint32_t bytes;     ///< Bisher verarbeitete Nutzdatenbytes.
    uint32_t remaining; ///< Beim Lesen noch verfügbare Nutzdatenbytes.
} PeSession;

/** @brief Synchroner Lese-/Schreibrückruf; false bricht die Sitzungsoperation ab. */
typedef bool (*PeSessionCodec)(PeSession *, void *);
/* Private, versioned native-build cache, not a portable project format. */
/**
 * @brief Schreibt einen begrenzten Checkpoint über eine temporäre Datei.
 *
 * Nur gleicher Build/Plattform, höchstens 32 MiB; kein portables Projektformat und keine
 * Crash-Recovery. Fehler erhalten den alten Checkpoint.
 *
 * @param path Nullterminierter Dateipfad.
 * @param kind Erwartete bzw. zu speichernde Editor-/Dokumentkennung.
 * @param codec Synchroner Lese-/Schreibrückruf; false bricht ab.
 * @param data Codeckontext bzw. Ein-/Ausgabepuffer des Datenblocks.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_session_save(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size);

/**
 * @brief Prüft Typ, Länge und Prüfsumme vollständig vor dem Aufruf des Lesecodecs.
 *
 * Checksum-/Framingfehler rufen den Codec nicht auf. Ein Codecfehler kann das Zieldokument
 * teilweise verändern; der Aufrufer verwirft es dann.
 *
 * @param path Nullterminierter Dateipfad.
 * @param kind Erwartete bzw. zu speichernde Editor-/Dokumentkennung.
 * @param codec Synchroner Lese-/Schreibrückruf; false bricht ab.
 * @param data Codeckontext bzw. Ein-/Ausgabepuffer des Datenblocks.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_session_load(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size);

/**
 * @brief Liest oder schreibt einen begrenzten Block im laufenden Checkpoint-Stream.
 *
 * @param session Nur während des Codecs gültiger Sitzungsstream.
 * @param data Codeckontext bzw. Ein-/Ausgabepuffer des Datenblocks.
 * @param size Anzahl zu übertragender Bytes.
 */
bool pe_session_block(PeSession *session, void *data, size_t size);

/**
 * @brief Serialisiert einen begrenzten String mit Längenpräfix und Nullabschluss; keine
 * UTF-8-Prüfung.
 *
 * Beim Lesen wird *text mit malloc belegt; der Aufrufer gibt den Speicher frei. Vor dem Lesen *text
 * auf NULL setzen.
 *
 * @param session Nur während des Codecs gültiger Sitzungsstream.
 * @param text Beim Schreiben Quellstring; beim Lesen Ausgabe eines neuen malloc-Puffers.
 * @param maximum Inklusive Obergrenze bzw. maximale Stringlänge in Bytes.
 */
bool pe_session_string(PeSession *session, char **text, size_t maximum);
/** @} */
#endif

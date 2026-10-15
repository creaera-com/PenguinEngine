/**
 * @file
 * @brief UTF-8-Dokumente und JavaScript-Sprachwerkzeuge ohne Gameplay-Ausführung.
 * @ingroup engine_api
 */

/** @addtogroup engine_api
 * @{
 */
#ifndef PENGUIN_SCRIPT_H
#define PENGUIN_SCRIPT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Maximale UTF-8-Dokumentlänge in Bytes ohne Nullabschluss. */
#define PE_TEXT_MAX 65536
/** @brief Maximale Anzahl gespeicherter Text-Undo-Schritte. */
#define PE_TEXT_HISTORY 64
/** @brief Maximale Anzahl gleichzeitig angebotener Vervollständigungen. */
#define PE_COMPLETIONS 12
/** @brief Kapazität eines Vorschlagstexts in Bytes einschließlich Nullabschluss. */
#define PE_COMPLETION_MAX 64

/** @brief Besitzende Textkopien und Cursor-/Revisionszustände eines Undo-Schritts. */
typedef struct {
    char *before;             ///< Besitzender Dokument-Snapshot vor der Änderung.
    char *after;              ///< Besitzender Dokument-Snapshot nach der Änderung.
    size_t before_cursor;     ///< Cursor-Byteoffset vor der Änderung.
    size_t before_anchor;     ///< Auswahlanker-Byteoffset vor der Änderung.
    size_t after_cursor;      ///< Cursor-Byteoffset nach der Änderung.
    uint64_t before_revision; ///< Revision vor der Änderung.
    uint64_t after_revision;  ///< Revision nach der Änderung.
} PeTextEdit;

/** @brief UTF-8-Textpuffer; Cursor, Auswahl und Länge sind Byteoffsets, keine Zeichenzahlen. */
typedef struct {
    char text[PE_TEXT_MAX + 1];        ///< Besitzender UTF-8-Puffer einschließlich Nullabschluss.
    size_t length;                     ///< Textlänge in Bytes ohne Nullabschluss.
    size_t cursor;                     ///< Cursor als UTF-8-Byteoffset innerhalb des Textpuffers.
    size_t anchor;                     ///< Auswahlanker als UTF-8-Byteoffset.
    PeTextEdit edits[PE_TEXT_HISTORY]; ///< Besitzende Text-Undo-Schritte.
    int count;                         ///< Anzahl gültiger Einträge.
    int position;                      ///< Position im Text-Undo-Verlauf.
    uint64_t revision;                 ///< Aktuelle Dokumentrevision.
    uint64_t next_revision;            ///< Nächste eindeutige Änderungsrevision.
    uint64_t saved_revision;           ///< Zuletzt als gespeichert markierte Revision.
} PeText;

/** @brief Lexikalische Highlightingklasse für ein einzelnes Quelltextbyte. */
typedef enum { JS_PLAIN, JS_KEYWORD, JS_STRING, JS_COMMENT, JS_NUMBER, JS_BUILTIN } PeJsToken;

/** @brief Parserergebnis mit Fehlermeldung und einbasierter Fehlerposition; 0 bedeutet unbekannt.
 */
typedef struct {
    bool valid;        ///< Ob die Syntaxprüfung erfolgreich war.
    int line;          ///< Zeilenposition; Parserdiagnosen sind einbasiert.
    int column;        ///< Spaltenposition; Parserdiagnosen sind einbasiert.
    char message[256]; ///< Nullterminierter Diagnose- oder Statustext.
} PeJsDiagnostic;

/** @brief Begrenzte Vorschlagsliste mit Ersetzungsbereich und zugehöriger Dokumentrevision. */
typedef struct {
    char values[PE_COMPLETIONS]
               [PE_COMPLETION_MAX]; ///< Nullterminierte Vervollständigungsvorschläge.
    int count;                      ///< Anzahl gültiger Einträge.
    size_t start;                   ///< Inklusiver Byteanfang des Ersetzungsbereichs.
    size_t end;                     ///< Exklusives Byteende des Ersetzungsbereichs.
    uint64_t revision;              ///< Aktuelle Dokumentrevision.
} PeCompletions;

/* Initialize once; free before reinitializing. Text must be UTF-8 without embedded NUL. */
/**
 * @brief Initialisiert ein UTF-8-Dokument ohne eingebettete Nullbytes.
 *
 * Einmal initialisieren und vor erneutem Initialisieren pe_text_free() aufrufen. Maximal
 * PE_TEXT_MAX Bytes.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param source Nullterminierter UTF-8-Quelltext.
 */
bool pe_text_init(PeText *document, const char *source);

/**
 * @brief Gibt die dynamischen Undo-Snapshots eines Textdokuments frei.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
void pe_text_free(PeText *document);

/**
 * @brief Ersetzt einen UTF-8-Bytebereich atomar und erzeugt einen Undo-Schritt.
 *
 * start/end sind Byteoffsets auf UTF-8-Grenzen, end ist exklusiv. bytes darf nicht in den
 * Dokumentpuffer zeigen.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param start Inklusiver Anfang als UTF-8-Byteoffset.
 * @param end Exklusives Ende als UTF-8-Byteoffset.
 * @param bytes Einzufügende UTF-8-Bytes ohne eingebettete Nullbytes.
 * @param length Anzahl einzufügender Bytes.
 */
bool pe_text_replace(PeText *document, size_t start, size_t end, const char *bytes, size_t length);

/**
 * @brief Ersetzt die Auswahl durch UTF-8-Text oder fügt ihn am Cursor ein.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param text Nullterminierter UTF-8-Text.
 */
bool pe_text_insert(PeText *document, const char *text);

/**
 * @brief Löscht die Auswahl oder das UTF-8-Zeichen vor dem Cursor.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
bool pe_text_backspace(PeText *document);

/**
 * @brief Löscht die Auswahl oder das UTF-8-Zeichen am Cursor.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
bool pe_text_delete(PeText *document);

/**
 * @brief Stellt den vorherigen Text einschließlich Cursor und Auswahl wieder her.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
bool pe_text_undo(PeText *document);

/**
 * @brief Wiederholt den nächsten verfügbaren Text-Undo-Schritt.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
bool pe_text_redo(PeText *document);

/**
 * @brief Prüft, ob Textrevision und gespeicherte Revision voneinander abweichen.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
bool pe_text_dirty(const PeText *document);

/**
 * @brief Markiert die aktuelle Textrevision als gespeichert.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
void pe_text_saved(PeText *document);

/**
 * @brief Ermittelt die vorherige UTF-8-Zeichengrenze als Byteoffset.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param offset Byteoffset im UTF-8-Puffer.
 */
size_t pe_text_previous(const PeText *document, size_t offset);

/**
 * @brief Ermittelt die nächste UTF-8-Zeichengrenze als Byteoffset.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param offset Byteoffset im UTF-8-Puffer.
 */
size_t pe_text_next(const PeText *document, size_t offset);

/**
 * @brief Ermittelt den Byteoffset des Zeilenanfangs.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param offset Byteoffset im UTF-8-Puffer.
 */
size_t pe_text_line_start(const PeText *document, size_t offset);

/**
 * @brief Ermittelt den Byteoffset des Zeilenendes vor dem Zeilenumbruch.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param offset Byteoffset im UTF-8-Puffer.
 */
size_t pe_text_line_end(const PeText *document, size_t offset);

/**
 * @brief Berechnet nullbasierte Zeile und Zeichenspalte aus einem Byteoffset.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param offset Byteoffset im UTF-8-Puffer.
 * @param line Ausgabe der nullbasierten Textzeile.
 * @param column Ausgabe der nullbasierten Zeichenspalte.
 */
void pe_text_location(const PeText *document, size_t offset, int *line, int *column);

/**
 * @brief Berechnet den Byteoffset einer nullbasierten Zeilen-/Zeichenspaltenposition.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param line Nullbasierte Textzeile bzw. Ausgabe ihrer Position.
 * @param column Nullbasierte Zeichenspalte bzw. Ausgabe ihrer Position.
 */
size_t pe_text_offset(const PeText *document, int line, int column);

/**
 * @brief Lädt eine UTF-8-Datei als Dokument und setzt dessen Änderungsverlauf zurück.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param path Nullterminierter Dateipfad.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_text_load(PeText *document, const char *path, char *error, size_t size);

/**
 * @brief Speichert Text über eine temporäre Datei und aktualisiert die Speicherrevision.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param path Nullterminierter Dateipfad.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_text_save(PeText *document, const char *path, char *error, size_t size);
/* Real parser check: compile only. Never executes scripts or resolves imported modules. */
/**
 * @brief Prüft JavaScript mit dem QuickJS-Parser ausschließlich durch Kompilieren.
 *
 * Führt keinen Code aus und löst keine Imports auf. filename dient nur der Parserdiagnose.
 *
 * @param source Nullterminierter UTF-8-Quelltext.
 * @param filename Diagnosename der Quelldatei; NULL verwendet untitled.js.
 */
PeJsDiagnostic pe_js_check(const char *source, const char *filename);
/* One token kind per source byte. Template strings are highlighted as a single string. */
/**
 * @brief Schreibt lexikalische Tokenklassen für die einzelnen Quelltextbytes.
 *
 * capacity bezeichnet Tokenplätze. Template-Strings erhalten in dieser ersten Version eine
 * gemeinsame Stringklasse.
 *
 * @param source Nullterminierter UTF-8-Quelltext.
 * @param tokens Aufruferpuffer für eine Tokenklasse pro Quelltextbyte.
 * @param capacity Anzahl verfügbarer Tokenplätze.
 */
void pe_js_highlight(const char *source, PeJsToken *tokens, size_t capacity);

/**
 * @brief Ermittelt Vorschläge aus Schlüsselwörtern, Standardobjekten und Dokumentbezeichnern.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 */
PeCompletions pe_js_complete(const PeText *document);

/**
 * @brief Übernimmt einen Vorschlag, falls Dokumentrevision und Ersetzungsbereich noch passen.
 *
 * @param document Initialisiertes UTF-8-Dokument; beim Initialisieren uninitialisiert bzw. zuvor
 * freigegeben.
 * @param completions Vorschlagsliste aus der passenden Dokumentrevision.
 * @param index Nullbasierter Index des gewünschten Eintrags.
 */
bool pe_js_accept(PeText *document, const PeCompletions *completions, int index);
/** @} */
#endif

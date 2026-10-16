/** @file
 * @brief SDL-unabhängiger Vertrag für gemeinsame Dokumentbefehle. */
#ifndef PENGUIN_EDITOR_DOCUMENT_H
#define PENGUIN_EDITOR_DOCUMENT_H
#include <stdbool.h>
#include <stddef.h>

/** @brief Momentaufnahme; Abfragen verändern weder Dokument noch Verlauf. */
typedef struct {
    bool loaded;   ///< Ein Dokument ist gebunden.
    bool dirty;    ///< Ungespeicherte Änderungen vorhanden.
    bool exists;   ///< Das Dokument hat bereits eine gespeicherte Datei.
    bool pending;  ///< Eine noch nicht abgeschlossene Bearbeitung verhindert Befehle.
    bool can_undo; ///< Mindestens ein rückgängig machbarer Schritt vorhanden.
    bool can_redo; ///< Mindestens ein wiederherstellbarer Schritt vorhanden.
} PeDocumentStatus;

/** @brief Implementiert der konkrete Editor; sämtliche Zeiger sind nur geliehen. */
typedef struct {
    PeDocumentStatus (*status)(void *context); ///< Liest den fachlichen Dokumentzustand.
    bool (*save)(void *context);               ///< Speichert; false bei Fehler oder Abbruch.
    void (*undo)(void *context, bool redo);    ///< Führt Undo oder Redo aus.
    void (*describe)(void *context, char *buffer, size_t size); ///< Schreibt Statusdetails.
} PeDocumentOps;

/** @brief Geliehene Bindung; nach Editorwechsel oder Reload ungültig, nie zwischenspeichern. */
typedef struct {
    void *context;            ///< Vom Editor besessener Implementierungskontext.
    const PeDocumentOps *ops; ///< Statische Operationstabelle des Editors oder NULL.
} PeDocument;

/** @brief Liest den Status; eine leere Bindung liefert ausschließlich false. */
PeDocumentStatus pe_document_status(PeDocument document);
/** @brief Speichert nur ein geladenes Dokument ohne offene Bearbeitung. */
bool pe_document_save(PeDocument document);
/** @brief Führt nur einen laut Status verfügbaren Undo-/Redo-Schritt aus. */
void pe_document_undo(PeDocument document, bool redo);
/** @brief Schreibt Details; NULL/0 sind erlaubt, Ergebnis ist stets nullterminiert. */
void pe_document_describe(PeDocument document, char *buffer, size_t size);
#endif

/** @file @brief Owned object copies and atomic structural edits, independent of UI and SDL. */
#ifndef PE_VECTOR_CLIPBOARD_H
#define PE_VECTOR_CLIPBOARD_H
#include "editors/vector/document/selection.h"

/** @brief Value-only clipboard. Parent artboards provide coordinates for rectangle-only copies. */
typedef struct {
    VectorArtboard boards[VECTOR_MAX_ARTBOARDS];
    VectorRectangle rectangles[VECTOR_MAX_RECTANGLES];
    int board_count, rectangle_count;
    unsigned whole_board; ///< Zero: rectangles; one: artboard with descendants.
} VectorClipboard;

/** @brief Validate clipboard values and parent references; an empty clipboard is valid. */
bool vector_clipboard_valid(const VectorClipboard *clipboard);
/** @brief Install a validated file snapshot as a single, protected initial history state. */
bool vector_objects_initialize(VectorDocument *document, const VectorClipboard *objects);
/** @brief Deep-copy selected rectangles or the selected artboard and all its children. */
bool vector_objects_copy(VectorDocument *document, VectorClipboard *clipboard);
/** @brief Read-only preflight for destructive selection commands. */
bool vector_objects_can_delete(VectorDocument *document);
/** @brief Read-only preflight for paste, including locks, geometry and capacity. */
bool vector_objects_can_paste(VectorDocument *document, const VectorClipboard *clipboard);
/** @brief Read-only preflight for duplicate without changing clipboard. */
bool vector_objects_can_duplicate(VectorDocument *document);
/** @brief Delete selection atomically; locked descendants also protect their artboard. */
bool vector_objects_delete(VectorDocument *document);
/** @brief Cut only replaces clipboard after deletion succeeds. */
bool vector_objects_cut(VectorDocument *document, VectorClipboard *clipboard);
/** @brief Paste with fresh IDs, selecting new objects; create source parents if no target exists.
 */
bool vector_objects_paste(VectorDocument *document, const VectorClipboard *clipboard);
/** @brief Duplicate selection in its original parents with a 16 px offset, preserving clipboard. */
bool vector_objects_duplicate(VectorDocument *document);
/** @brief Replace selected shapes as one command. ID zero allocates a fresh ID; existing IDs
 * must belong to the selection. Input is borrowed. Zero results removes an empty boolean result. */
bool vector_objects_replace_selection(VectorDocument *document, const VectorShape *shapes,
                                      int count);
/** @brief Structural replacement as one contiguous layer block at the last selected slot.
 * Uses the same atomic snapshot/validation as replace_selection; order outside selection is stable.
 */
bool vector_objects_replace_selection_block(VectorDocument *document, const VectorShape *shapes,
                                            int count);
/** @brief Private native codecs for structural command type 6. */
bool vector_objects_command_write(const VectorCommand *command, FILE *file);
bool vector_objects_command_read(VectorCommand *command, FILE *file);
#endif

/** @file
 * @brief Read-only diagnostic values exposed by editor adapters; no model ownership. */
#ifndef PE_EDITOR_DIAGNOSTICS_H
#define PE_EDITOR_DIAGNOSTICS_H
#include <stdbool.h>

/** @brief Optional context for performance logs, excluding text, file contents and model pointers.
 */
typedef struct {
    unsigned document, objects, instances, selected;
    int history_cursor;
    bool interacting;
    char operation[48];
} EditorDiagnostics;
#endif

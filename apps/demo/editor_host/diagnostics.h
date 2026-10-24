/** @file
 * @brief Read-only diagnostic values exposed by editor adapters; no model ownership. */
#ifndef PE_EDITOR_DIAGNOSTICS_H
#define PE_EDITOR_DIAGNOSTICS_H
#include <stdbool.h>
#include <stdint.h>
#define EDITOR_DIAGNOSTIC_SELECTION_CAPACITY 64

/** @brief Optional context for performance logs, excluding text, file contents and model pointers.
 */
typedef struct {
    unsigned document, objects, instances, selected;
    unsigned active_artboard, active_object;
    unsigned selected_ids[EDITOR_DIAGNOSTIC_SELECTION_CAPACITY]; ///< First selected object IDs.
    double viewport_x, viewport_y, zoom; ///< Screen-space pan offset and zoom factor.
    bool viewport_available;
    bool artwork_cache_available; ///< These counters belong to the active editor instance.
    uint64_t cache_tile_bytes, cache_scratch_bytes, cache_hits, cache_misses;
    uint64_t cache_repaints, cache_evictions;
    unsigned cache_tiles;
    int history_cursor;
    bool interacting;
    char operation[48];
} EditorDiagnostics;
#endif

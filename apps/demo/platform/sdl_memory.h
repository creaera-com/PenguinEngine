/** @file
 * @brief Process-lifetime accounting of allocations routed through SDL's allocator callbacks. */
#ifndef PENGUENGINE_SDL_MEMORY_H
#define PENGUENGINE_SDL_MEMORY_H
#include <stdbool.h>
#include <stdint.h>

/** @brief Requested live sizes, not resident memory; excludes direct libc/driver allocations. */
typedef struct {
    bool available, complete;
    uint64_t malloc_calls, calloc_calls, realloc_calls, free_calls, failures, metadata_failures;
    uint64_t live_blocks, live_bytes, peak_bytes, allocated_bytes, freed_bytes;
    uint64_t unknown_frees, unknown_reallocs, metadata_bytes;
} AppSDLMemory;

/** @brief Install once at application startup, before SDL_Init and before spawning threads.
 * Keeps the previous allocator and pointer alignment; safely forwards older untracked pointers.
 * Never uninstall or reset while allocations/SDL threads remain alive. */
bool app_sdl_memory_install(void);
/** @brief Thread-safe value snapshot; unavailable if hooks are absent or have been replaced. */
AppSDLMemory app_sdl_memory_read(void);
#endif

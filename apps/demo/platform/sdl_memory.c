/** @file @brief Allocation-safe side table: allocator callbacks never allocate while holding a
 * lock. */
#include "sdl_memory.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#define BUCKETS 4096u

typedef struct Allocation {
    uintptr_t address; /* Encoded address; never dereferenced or used as an owning pointer. */
    size_t size;
    struct Allocation *next;
} Allocation;

static Allocation *allocations[BUCKETS];
static SDL_SpinLock guard;
static AppSDLMemory totals;
static SDL_malloc_func previous_malloc;
static SDL_calloc_func previous_calloc;
static SDL_realloc_func previous_realloc;
static SDL_free_func previous_free;
static bool installed;

static void lock(void) {
    /* Only bounded bookkeeping is protected. No allocator or OS call while locked. */
    while (!SDL_TryLockSpinlock(&guard)) {
    }
}

static unsigned bucket(const void *pointer) {
    return (unsigned)(((uintptr_t)pointer >> 4) % BUCKETS);
}

/* Caller holds guard. Detaching does not yet change byte counts: realloc may fail. */
static Allocation *detach(const void *pointer) {
    Allocation **link = &allocations[bucket(pointer)];
    uintptr_t address = ~(uintptr_t)pointer;
    while (*link && (*link)->address != address) {
        link = &(*link)->next;
    }
    Allocation *record = *link;
    if (record) {
        *link = record->next;
    }
    return record;
}

static void attach(Allocation *record, const void *pointer, size_t size) {
    unsigned index = bucket(pointer);
    record->address = ~(uintptr_t)pointer;
    record->size = size;
    record->next = allocations[index];
    allocations[index] = record;
}

static void add_bytes(size_t size) {
    totals.live_bytes += size;
    totals.allocated_bytes += size;
    if (totals.live_bytes > totals.peak_bytes) {
        totals.peak_bytes = totals.live_bytes;
    }
}

static void register_pointer(void *pointer, size_t size) {
    if (!pointer) {
        lock();
        totals.failures++;
        SDL_UnlockSpinlock(&guard);
        return;
    }
    Allocation *record = previous_malloc(sizeof *record);
    lock();
    if (record) {
        attach(record, pointer, size);
        totals.live_blocks++;
        totals.metadata_bytes += sizeof *record;
        add_bytes(size);
    } else {
        totals.complete = false;
        totals.metadata_failures++;
    }
    SDL_UnlockSpinlock(&guard);
}

static void *SDLCALL tracked_malloc(size_t size) {
    size = size ? size : 1;
    lock();
    totals.malloc_calls++;
    SDL_UnlockSpinlock(&guard);
    void *pointer = previous_malloc(size);
    register_pointer(pointer, size);
    return pointer;
}

static void *SDLCALL tracked_calloc(size_t count, size_t size) {
    if (!count || !size) {
        count = size = 1;
    }
    lock();
    totals.calloc_calls++;
    SDL_UnlockSpinlock(&guard);
    if (size > SIZE_MAX / count) {
        lock();
        totals.failures++;
        SDL_UnlockSpinlock(&guard);
        return NULL;
    }
    void *pointer = previous_calloc(count, size);
    register_pointer(pointer, count * size);
    return pointer;
}

static void SDLCALL tracked_free(void *pointer) {
    if (!pointer) {
        return;
    }
    lock();
    totals.free_calls++;
    Allocation *record = detach(pointer);
    if (record) {
        totals.live_bytes -= record->size;
        totals.freed_bytes += record->size;
        totals.live_blocks--;
        totals.metadata_bytes -= sizeof *record;
    } else {
        totals.unknown_frees++;
    }
    SDL_UnlockSpinlock(&guard);
    previous_free(pointer);
    previous_free(record);
}

static void *SDLCALL tracked_realloc(void *pointer, size_t size) {
    size = size ? size : 1; /* SDL_realloc also normalizes zero to one. */
    lock();
    totals.realloc_calls++;
    Allocation *record = pointer ? detach(pointer) : NULL;
    if (pointer && !record) {
        totals.unknown_reallocs++;
    }
    SDL_UnlockSpinlock(&guard);
    /* Detach before realloc: another thread may immediately reuse the old address. */
    void *result = previous_realloc(pointer, size);
    if (record) {
        lock();
        if (result) {
            totals.live_bytes -= record->size;
            totals.freed_bytes += record->size;
            attach(record, result, size);
            add_bytes(size);
        } else {
            attach(record, pointer, record->size);
            totals.failures++;
        }
        SDL_UnlockSpinlock(&guard);
    } else {
        register_pointer(result, size);
    }
    return result;
}

bool app_sdl_memory_install(void) {
    if (installed) {
        return app_sdl_memory_read().available;
    }
    SDL_GetMemoryFunctions(&previous_malloc, &previous_calloc, &previous_realloc, &previous_free);
    if (!SDL_SetMemoryFunctions(tracked_malloc, tracked_calloc, tracked_realloc, tracked_free)) {
        return false;
    }
    totals.complete = true;
    installed = true;
    return true;
}

AppSDLMemory app_sdl_memory_read(void) {
    SDL_malloc_func current;
    SDL_calloc_func current_calloc;
    SDL_realloc_func current_realloc;
    SDL_free_func current_free;
    SDL_GetMemoryFunctions(&current, &current_calloc, &current_realloc, &current_free);
    lock();
    AppSDLMemory result = totals;
    result.available = installed && current == tracked_malloc && current_calloc == tracked_calloc &&
                       current_realloc == tracked_realloc && current_free == tracked_free;
    SDL_UnlockSpinlock(&guard);
    return result;
}

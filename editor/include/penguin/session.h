#ifndef PENGUIN_SESSION_H
#define PENGUIN_SESSION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef struct {
    FILE *file;
    bool reading, ok;
    uint32_t hash, bytes, remaining;
} PeSession;
typedef bool (*PeSessionCodec)(PeSession *, void *);
/* Private, versioned native-build cache, not a portable project format. */
bool pe_session_save(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size);
bool pe_session_load(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size);
bool pe_session_block(PeSession *session, void *data, size_t size);
bool pe_session_string(PeSession *session, char **text, size_t maximum);
#endif

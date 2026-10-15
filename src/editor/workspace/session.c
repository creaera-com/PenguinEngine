#include "workspace/session.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define SESSION_LIMIT (32u * 1024u * 1024u)

typedef struct {
    uint32_t magic, version, endian, kind, bytes, hash;
} Header;

static uint32_t digest(uint32_t hash, const unsigned char *data, size_t size) {
    while (size--) {
        hash = (hash ^ *data++) * 16777619u;
    }

    return hash;
}

bool pe_session_block(PeSession *s, void *data, size_t n) {
    if (!s->ok || n > SESSION_LIMIT || s->bytes > SESSION_LIMIT - n ||
        (s->reading && n > s->remaining)) {
        return s->ok = false;
    }
    if (s->reading) {
        s->ok = fread(data, 1, n, s->file) == n;
        s->remaining -= (uint32_t)n;
    } else {
        s->ok = fwrite(data, 1, n, s->file) == n;
    }
    if (s->ok) {
        s->hash = digest(s->hash, data, n);
        s->bytes += (uint32_t)n;
    }

    return s->ok;
}

bool pe_session_string(PeSession *s, char **text, size_t max) {
    uint32_t n = s->reading ? 0 : (uint32_t)strlen(*text);
    if (!pe_session_block(s, &n, sizeof n) || n > max) {
        return s->ok = false;
    }
    if (s->reading) {
        *text = calloc((size_t)n + 1, 1);
        if (!*text) {
            return s->ok = false;
        }
    }
    if (!pe_session_block(s, *text, n) || memchr(*text, 0, n)) {
        return s->ok = false;
    }

    return true;
}

static bool fail(char *error, size_t size, const char *path) {
    if (error && size) {
        snprintf(error, size, "Session read/write failed: %s", path);
    }

    return false;
}

bool pe_session_save(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size) {
    char temp[1400];
    if (snprintf(temp, sizeof temp, "%s.tmp", path) >= (int)sizeof temp) {
        return fail(error, size, path);
    }
    FILE *f = fopen(temp, "wb");
    if (!f) {
        return fail(error, size, path);
    }
    Header h = {0x50455353u, 1, 0x01020304u, kind, 0, 0};
    bool ok = fwrite(&h, sizeof h, 1, f) == 1;
    PeSession stream = {f, false, ok, 2166136261u, 0, 0};
    ok = ok && codec(&stream, data) && stream.ok;
    h.bytes = stream.bytes;
    h.hash = stream.hash;
    ok = ok && !fseek(f, 0, SEEK_SET) && fwrite(&h, sizeof h, 1, f) == 1 && !fflush(f);
    if (fclose(f)) {
        ok = false;
    }
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temp, path) == 0;
#endif
    }
    if (!ok) {
        remove(temp);
        return fail(error, size, path);
    }

    return true;
}

bool pe_session_load(const char *path, uint32_t kind, PeSessionCodec codec, void *data, char *error,
                     size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return fail(error, size, path);
    }
    Header h;
    bool ok = fread(&h, sizeof h, 1, f) == 1 && h.magic == 0x50455353u && h.version == 1 &&
              h.endian == 0x01020304u && h.kind == kind && h.bytes <= SESSION_LIMIT;
    uint32_t hash = 2166136261u, remaining = ok ? h.bytes : 0;
    unsigned char buffer[4096];
    while (ok && remaining) {
        size_t n = remaining < sizeof buffer ? remaining : sizeof buffer;
        ok = fread(buffer, 1, n, f) == n;
        if (ok) {
            hash = digest(hash, buffer, n);
        }
        remaining -= (uint32_t)n;
    }
    ok = ok && hash == h.hash && fgetc(f) == EOF && !ferror(f);
    if (ok) {
        ok = fseek(f, (long)sizeof h, SEEK_SET) == 0;
        PeSession stream = {f, true, ok, 2166136261u, 0, h.bytes};
        ok = ok && codec(&stream, data) && stream.ok && !stream.remaining;
    }
    fclose(f);
    return ok || fail(error, size, path);
}

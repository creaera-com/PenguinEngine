/** @file @brief Portable directory discovery and a bounded, lazily loaded font-face cache. */
#include "editor_host/fonts.h"
#include "stb_truetype.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FACE_CACHE 8

typedef struct {
    int index;
    unsigned char *data;
    stbtt_fontinfo info;
} LoadedFace;

struct PUIFonts {
    PUIFontEntry *entries;
    int capacity;
    int count, next;
    char bundled[1024];
    LoadedFace cache[FACE_CACHE];
};

static unsigned char *read_font(const char *path, size_t *length) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info) || info.size < 64 || info.size > 32u * 1024 * 1024) {
        return NULL;
    }
    void *bytes = SDL_LoadFile(path, length);
    if (*length < 64 || *length > 32u * 1024 * 1024) {
        SDL_free(bytes);
        return NULL;
    }
    return bytes;
}

static void name(stbtt_fontinfo *info, int id, char *out, size_t capacity) {
    int bytes = 0;
    const char *s = stbtt_GetFontNameString(info, &bytes, 3, 1, 0x409, id);
    size_t used = 0;
    if (s) {
        for (int i = 0; i + 1 < bytes && used + 4 < capacity; i += 2) {
            unsigned c = (unsigned char)s[i] * 256u + (unsigned char)s[i + 1];
            if (!c || (c >= 0xd800 && c <= 0xdfff)) {
                continue;
            }
            if (c < 128) {
                out[used++] = (char)c;
            } else if (c < 2048) {
                out[used++] = (char)(0xc0 | (c >> 6));
                out[used++] = (char)(0x80 | (c & 63));
            } else {
                out[used++] = (char)(0xe0 | (c >> 12));
                out[used++] = (char)(0x80 | ((c >> 6) & 63));
                out[used++] = (char)(0x80 | (c & 63));
            }
        }
    } else {
        s = stbtt_GetFontNameString(info, &bytes, 1, 0, 0, id);
        if (s) {
            for (int i = 0; i < bytes && used + 1 < capacity; i++) {
                if ((unsigned char)s[i] >= 32 && (unsigned char)s[i] < 127) {
                    out[used++] = s[i];
                }
            }
        }
    }
    out[used] = 0;
}

static void add(PUIFonts *fonts, const char *path, bool bundled) {
    if (strlen(path) >= sizeof fonts->entries[0].path) {
        return;
    }
    size_t length = 0;
    unsigned char *bytes = read_font(path, &length);
    if (!bytes) {
        return;
    }
    int count = stbtt_GetNumberOfFonts(bytes);
    for (int i = 0; i < count && i < 256; i++) {
        stbtt_fontinfo info;
        int offset = stbtt_GetFontOffsetForIndex(bytes, i);
        if (offset < 0 || (size_t)offset >= length || !stbtt_InitFont(&info, bytes, offset)) {
            continue;
        }
        PUIFontEntry e = {.index = (unsigned)i};
        snprintf(e.path, sizeof e.path, "%s", bundled ? "" : path);
        name(&info, 1, e.family, sizeof e.family);
        name(&info, 2, e.style, sizeof e.style);
        if (!*e.family) {
            snprintf(e.family, sizeof e.family, "Unbenannte Schrift");
        }
        if (!*e.style) {
            snprintf(e.style, sizeof e.style, "Regular");
        }
        e.bold =
            SDL_strcasestr(e.style, "bold") != NULL || SDL_strcasestr(e.style, "black") != NULL;
        e.italic =
            SDL_strcasestr(e.style, "italic") != NULL || SDL_strcasestr(e.style, "oblique") != NULL;
        snprintf(e.label, sizeof e.label, "%.120s / %.120s", e.family, e.style);
        bool duplicate = false;
        for (int n = 0; n < fonts->count; n++) {
            duplicate |=
                !strcmp(e.path, fonts->entries[n].path) && e.index == fonts->entries[n].index;
        }
        if (!duplicate) {
            if (fonts->count == fonts->capacity) {
                int capacity = fonts->capacity + 128;
                PUIFontEntry *entries = realloc(fonts->entries, (size_t)capacity * sizeof *entries);
                if (!entries) {
                    break;
                }
                fonts->entries = entries;
                fonts->capacity = capacity;
            }
            fonts->entries[fonts->count++] = e;
        }
    }
    SDL_free(bytes);
}

typedef struct {
    PUIFonts *fonts;
    int depth;
} Walk;

static SDL_EnumerationResult visit(void *context, const char *dir, const char *file) {
    Walk *walk = context;
    char path[2048];
    int n = snprintf(path, sizeof path, "%s%s", dir, file);
    if (n < 0 || n >= (int)sizeof path || file[0] == '.') {
        return SDL_ENUM_CONTINUE;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) {
        return SDL_ENUM_CONTINUE;
    }
    if (info.type == SDL_PATHTYPE_DIRECTORY && walk->depth < 8) {
        Walk child = {walk->fonts, walk->depth + 1};
        SDL_EnumerateDirectory(path, visit, &child);
    } else if (info.type == SDL_PATHTYPE_FILE) {
        const char *ext = strrchr(file, '.');
        if (ext && (!SDL_strcasecmp(ext, ".ttf") || !SDL_strcasecmp(ext, ".otf") ||
                    !SDL_strcasecmp(ext, ".ttc"))) {
            add(walk->fonts, path, false);
        }
    }
    return SDL_ENUM_CONTINUE;
}

PUIFonts *pui_fonts_create(const char *root) {
    PUIFonts *fonts = calloc(1, sizeof *fonts);
    if (!fonts) {
        return NULL;
    }
    snprintf(fonts->bundled, sizeof fonts->bundled, "%s/fonts/JetBrainsMono-Regular.ttf", root);
    add(fonts, fonts->bundled, true);
    if (!fonts->count) {
        free(fonts);
        return NULL;
    }
    const char *dirs[] = {"/usr/share/fonts", "/usr/local/share/fonts", "/System/Library/Fonts",
                          "/Library/Fonts"};
    Walk walk = {fonts, 0};
    for (size_t i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
        SDL_EnumerateDirectory(dirs[i], visit, &walk);
    }
    const char *home = SDL_getenv("HOME");
    char path[2048];
    if (home) {
        const char *suffixes[] = {"/.fonts", "/.local/share/fonts", "/Library/Fonts"};
        for (int i = 0; i < 3; i++) {
            snprintf(path, sizeof path, "%s%s", home, suffixes[i]);
            SDL_EnumerateDirectory(path, visit, &walk);
        }
    }
    const char *windows = SDL_getenv("WINDIR"), *local = SDL_getenv("LOCALAPPDATA");
    if (windows) {
        snprintf(path, sizeof path, "%s/Fonts", windows);
        SDL_EnumerateDirectory(path, visit, &walk);
    }
    if (local) {
        snprintf(path, sizeof path, "%s/Microsoft/Windows/Fonts", local);
        SDL_EnumerateDirectory(path, visit, &walk);
    }
    return fonts;
}

void pui_fonts_destroy(PUIFonts *fonts) {
    if (!fonts) {
        return;
    }
    for (int i = 0; i < FACE_CACHE; i++) {
        SDL_free(fonts->cache[i].data);
    }
    free(fonts->entries);
    free(fonts);
}

int pui_fonts_count(const PUIFonts *fonts) {
    return fonts ? fonts->count : 0;
}

const PUIFontEntry *pui_fonts_entry(const PUIFonts *fonts, int index) {
    return fonts && index >= 0 && index < fonts->count ? &fonts->entries[index] : NULL;
}

int pui_fonts_find(const PUIFonts *fonts, const char *path, unsigned face) {
    for (int i = 0; i < fonts->count; i++) {
        if (!strcmp(path, fonts->entries[i].path) && face == fonts->entries[i].index) {
            return i;
        }
    }
    return 0;
}

int pui_fonts_style(const PUIFonts *fonts, int index, bool bold, bool italic) {
    const PUIFontEntry *source = pui_fonts_entry(fonts, index);
    int best = index, score = source ? (source->bold != bold) + (source->italic != italic) : 3;
    if (!source) {
        return 0;
    }
    for (int i = 0; i < fonts->count; i++) {
        const PUIFontEntry *e = &fonts->entries[i];
        int s = (e->bold != bold) + (e->italic != italic);
        if (!strcmp(e->family, source->family) && s < score) {
            score = s;
            best = i;
        }
    }
    return best;
}

static stbtt_fontinfo *load(PUIFonts *fonts, int index) {
    for (int i = 0; i < FACE_CACHE; i++) {
        if (fonts->cache[i].data && fonts->cache[i].index == index) {
            return &fonts->cache[i].info;
        }
    }
    LoadedFace *f = &fonts->cache[fonts->next++ % FACE_CACHE];
    SDL_free(f->data);
    *f = (LoadedFace){.index = index};
    const PUIFontEntry *e = pui_fonts_entry(fonts, index);
    size_t length = 0;
    if (!e) {
        return NULL;
    }
    f->data = read_font(*e->path ? e->path : fonts->bundled, &length);
    if (!f->data) {
        return NULL;
    }
    int offset = stbtt_GetFontOffsetForIndex(f->data, e->index);
    if (offset < 0 || !stbtt_InitFont(&f->info, f->data, offset)) {
        SDL_free(f->data);
        f->data = NULL;
        return NULL;
    }
    return &f->info;
}

float pui_fonts_advance(PUIFonts *fonts, int index, unsigned code, unsigned next, float size) {
    stbtt_fontinfo *f = load(fonts, index);
    if (!f) {
        return size * .6f;
    }
    int advance, bearing;
    stbtt_GetCodepointHMetrics(f, (int)code, &advance, &bearing);
    return (advance + (next ? stbtt_GetCodepointKernAdvance(f, (int)code, (int)next) : 0)) *
           stbtt_ScaleForPixelHeight(f, size);
}

float pui_fonts_ascent(PUIFonts *fonts, int index, float size) {
    stbtt_fontinfo *f = load(fonts, index);
    if (!f) {
        return size;
    }
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(f, &ascent, &descent, &gap);
    return ascent * stbtt_ScaleForPixelHeight(f, size);
}

unsigned char *pui_fonts_bitmap(PUIFonts *fonts, int index, unsigned code, float size, int *w,
                                int *h, int *x, int *y) {
    stbtt_fontinfo *f = load(fonts, index);
    if (!f) {
        return NULL;
    }
    float scale = stbtt_ScaleForPixelHeight(f, size);
    return stbtt_GetCodepointBitmap(f, scale, scale, (int)code, w, h, x, y);
}

void pui_fonts_free_bitmap(unsigned char *bitmap) {
    stbtt_FreeBitmap(bitmap, NULL);
}

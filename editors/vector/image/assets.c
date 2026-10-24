/** @file @brief Imported originals belong to the project; document references stay relative. */
#include "assets.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static const char *basename(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    return b && (!a || b > a) ? b + 1 : a ? a + 1 : path;
}

bool vector_image_project_root(const char *document, char *root, size_t capacity) {
    if (!document || strlen(document) >= capacity) {
        return false;
    }
    snprintf(root, capacity, "%s", document);
    char *end = root + (basename(root) - root);
    if (end == root) {
        return false;
    }
    *end = 0;
    char candidate[4096], current[4096];
    snprintf(current, sizeof current, "%s", root);
    while (*current) {
        SDL_PathInfo info;
        if (snprintf(candidate, sizeof candidate, "%s/project.pengui.yaml", current) <
                (int)sizeof candidate &&
            SDL_GetPathInfo(candidate, &info) && info.type == SDL_PATHTYPE_FILE) {
            snprintf(root, capacity, "%s", current);
            return true;
        }
        size_t n = strlen(current);
        while (n && (current[n - 1] == '/' || current[n - 1] == '\\')) {
            current[--n] = 0;
        }
        const char *name = basename(current);
        if (name == current) {
            break;
        }
        current[name - current] = 0;
    }
    return true;
}

bool vector_image_relative_valid(const char *p) {
    return !strncmp(p, "assets/images/", 14) && p[14] && !strchr(p + 14, '/') &&
           !strchr(p + 14, '\\') && !strchr(p + 14, ':') && strcmp(p + 14, ".") &&
           strcmp(p + 14, "..");
}

static bool copy_file(const char *source, const char *target, bool reuse) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(source, &info) || info.type != SDL_PATHTYPE_FILE ||
        info.size > 32 * 1024 * 1024) {
        return false;
    }
    size_t size;
    void *data = SDL_LoadFile(source, &size);
    if (!data) {
        return false;
    }
    bool ok = false;
    if (SDL_GetPathInfo(target, &info)) {
        size_t other_size;
        void *other = reuse ? SDL_LoadFile(target, &other_size) : NULL;
        ok = other && size == other_size && !memcmp(data, other, size);
        SDL_free(other);
    } else {
        SDL_IOStream *file = SDL_IOFromFile(target, "wbx");
        if (file) {
            ok = SDL_WriteIO(file, data, size) == size;
            ok = SDL_CloseIO(file) && ok;
            if (!ok) {
                SDL_RemovePath(target);
            }
        }
    }
    SDL_free(data);
    return ok;
}

static bool directory(const char *root) {
    char folder[4096];
    SDL_PathInfo info;
    return root && *root && SDL_GetPathInfo(root, &info) && info.type == SDL_PATHTYPE_DIRECTORY &&
           snprintf(folder, sizeof folder, "%s/assets/images", root) < (int)sizeof folder &&
           SDL_CreateDirectory(folder);
}

bool vector_image_copy(const char *source, const char *root, char *target, size_t capacity) {
    if (!directory(root)) {
        return false;
    }
    const char *ext = strrchr(basename(source), '.');
    if (!ext || (SDL_strcasecmp(ext, ".png") && SDL_strcasecmp(ext, ".jpg") &&
                 SDL_strcasecmp(ext, ".jpeg"))) {
        return false;
    }
    // Use a safe unique basename; user names remain in the scene tree, not filesystem syntax.
    for (unsigned i = 0; i < 64; i++) {
        if (snprintf(target, capacity, "%s/assets/images/image-%llu-%u%s", root,
                     (unsigned long long)SDL_GetPerformanceCounter(), i, ext) >= (int)capacity) {
            return false;
        }
        if (copy_file(source, target, false)) {
            return true;
        }
    }
    return false;
}

bool vector_image_export(const VectorImage *image, const char *root, char *relative,
                         size_t capacity) {
    char target[4096];
    return directory(root) &&
           snprintf(relative, capacity, "assets/images/%s", basename(image->source)) <
               (int)capacity &&
           vector_image_relative_valid(relative) &&
           snprintf(target, sizeof target, "%s/%s", root, relative) < (int)sizeof target &&
           copy_file(image->source, target, true);
}

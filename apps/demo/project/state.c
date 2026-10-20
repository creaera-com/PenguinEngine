/** @file @brief Native user state uses penguengine, never the temporary checkpoint directory. */
#include "state.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

bool app_project_state_path(char *path, size_t capacity) {
    char directory[PROJECT_PATH_CAPACITY];
    int n;
#ifdef __linux__
    const char *xdg = SDL_getenv("XDG_STATE_HOME"), *home = SDL_getenv("HOME");
    if (xdg && *xdg == '/') {
        n = snprintf(directory, sizeof directory, "%s/penguengine", xdg);
    } else if (home && *home == '/') {
        n = snprintf(directory, sizeof directory, "%s/.local/state/penguengine", home);
    } else {
        return false;
    }
#else
    char *pref = SDL_GetPrefPath(NULL, "penguengine");
    if (!pref) {
        return false;
    }
    n = snprintf(directory, sizeof directory, "%s", pref);
    SDL_free(pref);
#endif
    if (n < 0 || n >= (int)sizeof directory || !SDL_CreateDirectory(directory)) {
        return false;
    }
    n = snprintf(path, capacity, "%s/last-project.txt", directory);
    return n >= 0 && (size_t)n < capacity;
}

bool app_project_state_save(const char *state, const char *project) {
    if (!state || !*state || !project || !*project || strlen(project) >= PROJECT_PATH_CAPACITY) {
        return false;
    }
    char staged[PROJECT_PATH_CAPACITY];
    if (snprintf(staged, sizeof staged, "%s.tmp-%llu", state,
                 (unsigned long long)SDL_GetPerformanceCounter()) >= (int)sizeof staged) {
        return false;
    }
    SDL_IOStream *file = SDL_IOFromFile(staged, "wb");
    if (!file) {
        return false;
    }
    size_t length = strlen(project);
    bool ok = SDL_WriteIO(file, project, length) == length;
    ok = SDL_CloseIO(file) && ok;
    if (ok) {
        ok = SDL_RenamePath(staged, state);
    }
    if (!ok) {
        SDL_RemovePath(staged);
    }
    return ok;
}

bool app_project_state_read(const char *state, char *project, size_t capacity) {
    SDL_PathInfo info;
    size_t length = 0;
    if (!state || !SDL_GetPathInfo(state, &info) || info.size == 0 || info.size >= capacity) {
        return false;
    }
    char *data = SDL_LoadFile(state, &length);
    if (!data || length >= capacity || memchr(data, 0, length)) {
        SDL_free(data);
        return false;
    }
    memcpy(project, data, length);
    project[length] = 0;
    SDL_free(data);
    return true;
}

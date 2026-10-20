/** @file @brief SDL 3.2 Windows file modes ignore x, so use native CREATE_NEW there. */
#include "files.h"
#include <SDL3/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <limits.h>
#include <windows.h>
#endif

bool editor_file_create_exclusive(const char *path, const void *data, size_t length) {
#ifdef _WIN32
    if (length > MAXDWORD) {
        return false;
    }
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (!count) {
        return false;
    }
    wchar_t *wide = SDL_malloc((size_t)count * sizeof *wide);
    if (!wide) {
        return false;
    }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count)) {
        SDL_free(wide);
        return false;
    }
    HANDLE file =
        CreateFileW(wide, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    SDL_free(wide);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool ok = WriteFile(file, data, (DWORD)length, &written, NULL) && written == length;
    ok = CloseHandle(file) && ok;
#else
    SDL_IOStream *file = SDL_IOFromFile(path, "wbx");
    if (!file) {
        return false;
    }
    bool ok = SDL_WriteIO(file, data, length) == length;
    ok = SDL_CloseIO(file) && ok;
#endif
    if (!ok) {
        SDL_RemovePath(path);
    }
    return ok;
}

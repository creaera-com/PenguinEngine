/** @file @brief Temporary-file portability; no editor-domain dependencies. */
#include "temp_file.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>

FILE *editor_temporary_file(void) {
    /* MSVC tmpfile() may use the drive root, which need not be writable. */
    wchar_t directory[MAX_PATH + 1], path[MAX_PATH + 1];
    DWORD length = GetTempPathW(MAX_PATH + 1, directory);
    if (!length || length > MAX_PATH || !GetTempFileNameW(directory, L"pui", 0, path)) {
        return NULL;
    }
    HANDLE handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DeleteFileW(path);
        return NULL;
    }
    int descriptor = _open_osfhandle((intptr_t)handle, _O_BINARY | _O_RDWR);
    if (descriptor == -1) {
        CloseHandle(handle);
        return NULL;
    }
    FILE *file = _fdopen(descriptor, "w+b");
    if (!file) {
        _close(descriptor);
    }
    return file;
}
#else
FILE *editor_temporary_file(void) {
    return tmpfile();
}
#endif

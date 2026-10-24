/** @file @brief Project directory creation is separate from the Explorer widget binding. */
#include "directories.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

bool project_directory_create(const char *parent, const char *name, char *path, size_t capacity,
                              char *error, size_t error_capacity) {
    size_t length = name ? strlen(name) : 0;
    bool valid = length > 0 && length < 256 && name[0] != ' ' && name[length - 1] != ' ' &&
                 name[length - 1] != '.';
    if (valid) {
        for (size_t i = 0; i < length; i++) {
            unsigned char c = (unsigned char)name[i];
            if (c < 32 || c == 127 || strchr("/\\:*?\"<>|", c)) {
                valid = false;
                break;
            }
        }
    }
    char base[256] = {0};
    if (valid) {
        snprintf(base, sizeof base, "%s", name);
        char *dot = strchr(base, '.');
        if (dot) {
            *dot = 0;
        }
    }
    if (!SDL_strcasecmp(base, "CON") || !SDL_strcasecmp(base, "PRN") ||
        !SDL_strcasecmp(base, "AUX") || !SDL_strcasecmp(base, "NUL") ||
        (strlen(base) == 4 &&
         (!SDL_strncasecmp(base, "COM", 3) || !SDL_strncasecmp(base, "LPT", 3)) && base[3] >= '1' &&
         base[3] <= '9')) {
        valid = false;
    }
    if (!valid) {
        snprintf(error, error_capacity,
                 "Ungueltiger Ordnername (keine Pfade oder reservierten Namen)");
        return false;
    }
    SDL_PathInfo info;
    if (!parent || !*parent || !SDL_GetPathInfo(parent, &info) ||
        info.type != SDL_PATHTYPE_DIRECTORY) {
        snprintf(error, error_capacity, "Kein gueltiger Zielordner / Projekt oeffnen");
        return false;
    }
    size_t parent_length = strlen(parent);
    bool separator = parent[parent_length - 1] == '/';
#ifdef _WIN32
    separator |= parent[parent_length - 1] == '\\';
#endif
    int n = snprintf(path, capacity, "%s%s%s", parent, separator ? "" : "/", name);
    if (n < 0 || (size_t)n >= capacity) {
        snprintf(error, error_capacity, "Ordnerpfad ist zu lang");
        return false;
    }
    if (SDL_GetPathInfo(path, &info)) {
        snprintf(error, error_capacity, "Ein Eintrag mit diesem Namen existiert bereits");
        return false;
    }
    if (!SDL_CreateDirectory(path)) {
        snprintf(error, error_capacity, "Ordner konnte nicht angelegt werden: %.150s",
                 SDL_GetError());
        return false;
    }
    return true;
}

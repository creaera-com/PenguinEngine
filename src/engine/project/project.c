#include "penguin/engine.h"
#include <stdio.h>
#include <string.h>

static bool has_manifest(const char *root) {
    char path[1024];
    int n = snprintf(path, sizeof path, "%s/project.json", root);
    if (n < 0 || n >= (int)sizeof path) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

bool pe_project_resolve(const char *explicit_root, const char *executable_directory, char *root,
                        size_t size, char *error, size_t error_size) {
    if (explicit_root) {
        if (strlen(explicit_root) >= size) {
            snprintf(error, error_size, "Project path too long");
            return false;
        }
        if (!has_manifest(explicit_root)) {
            snprintf(error, error_size,
                     "Cannot open %.180s/project.json. Use --project <project directory>.",
                     explicit_root);
            return false;
        }
        memcpy(root, explicit_root, strlen(explicit_root) + 1);
        return true;
    }

    const char *relative[] = {"example", "../example"};
    char candidate[1024];
    if (executable_directory && *executable_directory) {
        for (int i = 0; i < 2; i++) {
            int n =
                snprintf(candidate, sizeof candidate, "%s/%s", executable_directory, relative[i]);
            if (n >= 0 && n < (int)sizeof candidate && strlen(candidate) < size &&
                has_manifest(candidate)) {
                memcpy(root, candidate, strlen(candidate) + 1);
                return true;
            }
        }
    }

    const char *fallback[] = {"example", "examples/topdown", "."};
    for (int i = 0; i < 3; i++) {
        if (strlen(fallback[i]) < size && has_manifest(fallback[i])) {
            memcpy(root, fallback[i], strlen(fallback[i]) + 1);
            return true;
        }
    }
    snprintf(error, error_size,
             "No project found next to the executable or in the working directory. "
             "Use --project <directory containing project.json>.");
    return false;
}

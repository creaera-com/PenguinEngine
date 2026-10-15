#include "penguin/engine.h"
#include "shared/json/json_io.h"
#include <stdio.h>

bool pe_project_load(const char *root, char *scene_path, size_t size, char *error,
                     size_t error_size) {
    char path[1024];
    if (snprintf(path, sizeof path, "%s/project.json", root) >= (int)sizeof path)
        return pe_json_fail(error, error_size, "Project path too long");
    cJSON *j = pe_json_read(path, error, error_size);
    if (!j)
        return false;
    int version;
    cJSON *scene = cJSON_GetObjectItemCaseSensitive(j, "start_scene");
    bool ok = pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "version"), 1, 1, &version) &&
              cJSON_IsString(scene) && pe_json_relative_path(scene->valuestring, PE_PATH);
    if (ok)
        ok = snprintf(scene_path, size, "%s/%s", root, scene->valuestring) < (int)size;
    cJSON_Delete(j);
    return ok ? true : pe_json_fail(error, error_size, "Invalid project manifest");
}

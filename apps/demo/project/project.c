/** @file @brief Minimal versioned YAML project documents, loaded without activating editors. */
#include "project.h"
#include "platform/files.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

bool app_project_open(AppProject *project, const char *file, char *error, size_t capacity) {
    size_t length = 0;
    SDL_PathInfo info;
    if (!file || strlen(file) >= PROJECT_PATH_CAPACITY || !SDL_GetPathInfo(file, &info) ||
        info.type != SDL_PATHTYPE_FILE || info.size > 65536) {
        snprintf(error, capacity, "Projektdatei nicht gefunden oder zu gross");
        return false;
    }
    char *data = SDL_LoadFile(file, &length);
    if (!data) {
        snprintf(error, capacity, "Projektdatei kann nicht gelesen werden");
        return false;
    }
    yaml_parser_t parser;
    yaml_document_t doc;
    AppProject next = {0};
    bool initialized = yaml_parser_initialize(&parser), loaded = false, valid = false,
         schema = false, name = false;
    if (initialized) {
        yaml_parser_set_input_string(&parser, (unsigned char *)data, length);
        loaded = yaml_parser_load(&parser, &doc);
        yaml_node_t *root = loaded ? yaml_document_get_root_node(&doc) : NULL;
        valid = root && root->type == YAML_MAPPING_NODE;
        if (valid) {
            for (yaml_node_pair_t *p = root->data.mapping.pairs.start;
                 p < root->data.mapping.pairs.top; p++) {
                yaml_node_t *k = yaml_document_get_node(&doc, p->key),
                            *v = yaml_document_get_node(&doc, p->value);
                if (k->type != YAML_SCALAR_NODE || v->type != YAML_SCALAR_NODE ||
                    strlen((char *)k->data.scalar.value) != k->data.scalar.length ||
                    strlen((char *)v->data.scalar.value) != v->data.scalar.length) {
                    valid = false;
                    break;
                }
                if (!strcmp((char *)k->data.scalar.value, "schema")) {
                    valid &= !schema && !strcmp((char *)v->data.scalar.value, "pengui/project/v1");
                    schema = true;
                } else if (!strcmp((char *)k->data.scalar.value, "name")) {
                    valid &= !name && v->data.scalar.length > 0 &&
                             v->data.scalar.length < sizeof next.name;
                    if (valid) {
                        snprintf(next.name, sizeof next.name, "%s", v->data.scalar.value);
                    }
                    name = true;
                } else {
                    valid = false;
                }
            }
        }
        if (loaded) {
            yaml_document_delete(&doc);
        }
        if (valid) {
            yaml_document_t extra;
            bool read = yaml_parser_load(&parser, &extra);
            valid = read && yaml_document_get_root_node(&extra) == NULL;
            if (read) {
                yaml_document_delete(&extra);
            }
        }
        yaml_parser_delete(&parser);
    }
    SDL_free(data);
    if (!valid || !schema || !name) {
        snprintf(error, capacity, "Ungueltiges Projekt (erwartet: pengui/project/v1 mit Name)");
        return false;
    }
    // Dialogs return absolute paths; normalize relative command-line/test paths as well.
    char *cwd = SDL_GetCurrentDirectory();
    bool absolute = file[0] == '/' || file[0] == '\\' || (strlen(file) > 2 && file[1] == ':');
    int n = snprintf(next.file, sizeof next.file, "%s%s", absolute ? "" : (cwd ? cwd : ""), file);
    SDL_free(cwd);
    if (n < 0 || n >= (int)sizeof next.file) {
        snprintf(error, capacity, "Projektpfad zu lang");
        return false;
    }
    snprintf(next.root, sizeof next.root, "%s", next.file);
    char *slash = strrchr(next.root, '/'), *back = strrchr(next.root, '\\');
    if (back && (!slash || back > slash)) {
        slash = back;
    }
    if (!slash) {
        snprintf(error, capacity, "Projektordner nicht bestimmbar");
        return false;
    }
    // Preserve filesystem roots, including C:/ and /.
    if (slash == next.root || (slash == next.root + 2 && next.root[1] == ':')) {
        slash[1] = 0;
    } else {
        *slash = 0;
    }
    *project = next;
    return true;
}

bool app_project_create(AppProject *project, const char *folder, char *error, size_t capacity) {
    SDL_PathInfo info;
    char path[PROJECT_PATH_CAPACITY];
    if (!folder || !SDL_GetPathInfo(folder, &info) || info.type != SDL_PATHTYPE_DIRECTORY ||
        snprintf(path, sizeof path, "%s/project.pengui.yaml", folder) >= (int)sizeof path) {
        snprintf(error, capacity, "Kein gueltiger Projektordner");
        return false;
    }
    const char *body = "schema: pengui/project/v1\nname: Neues Projekt\n";
    if (!editor_file_create_exclusive(path, body, strlen(body))) {
        snprintf(error, capacity,
                 "Projektdatei existiert bereits oder konnte nicht geschrieben werden");
        return false;
    }
    return app_project_open(project, path, error, capacity);
}

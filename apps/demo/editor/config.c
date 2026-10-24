/** @file @brief Parse app configuration; this is not part of the pengUI schema. */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <yaml.h>

bool editor_config_load(EditorConfig *config, const char *path, char *error, size_t capacity) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        snprintf(error, capacity, "cannot open configuration: %s", path);
        return false;
    }
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fclose(file);
        snprintf(error, capacity, "could not initialize configuration parser");
        return false;
    }
    yaml_parser_set_input_file(&parser, file);
    yaml_document_t document;
    bool valid = yaml_parser_load(&parser, &document);
    EditorConfig next = {0};
    if (valid) {
        yaml_node_t *root = yaml_document_get_root_node(&document);
        valid = root && root->type == YAML_MAPPING_NODE &&
                root->data.mapping.pairs.top - root->data.mapping.pairs.start == 1;
        if (valid) {
            yaml_node_pair_t pair = root->data.mapping.pairs.start[0];
            yaml_node_t *key = yaml_document_get_node(&document, pair.key);
            yaml_node_t *value = yaml_document_get_node(&document, pair.value);
            valid = key->type == YAML_SCALAR_NODE && value->type == YAML_SCALAR_NODE &&
                    key->data.scalar.length == strlen("startup_editor") &&
                    !strcmp((char *)key->data.scalar.value, "startup_editor") &&
                    value->data.scalar.length > 0 &&
                    value->data.scalar.length < sizeof next.startup_editor &&
                    strlen((char *)value->data.scalar.value) == value->data.scalar.length;
            if (valid) {
                snprintf(next.startup_editor, sizeof next.startup_editor, "%s",
                         value->data.scalar.value);
            }
        }
        yaml_document_delete(&document);
    }
    if (valid) {
        yaml_document_t extra;
        if (yaml_parser_load(&parser, &extra)) {
            valid = yaml_document_get_root_node(&extra) == NULL;
            yaml_document_delete(&extra);
        } else {
            valid = false;
        }
    }
    yaml_parser_delete(&parser);
    fclose(file);
    if (!valid) {
        snprintf(error, capacity,
                 "expected one YAML document with startup_editor: vector, scripts or game");
        return false;
    }
    *config = next;
    return true;
}

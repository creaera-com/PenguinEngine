/** @file @brief Parse editor presentation separately from pengUI and the artwork model. */
#include "style.h"
#include "pengui/pengui.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

VectorWorkspaceStyle vector_workspace_style_default(void) {
    return (VectorWorkspaceStyle){true, 0x000000ff, .06f, 3, 3, 4};
}

static bool scalar(const yaml_node_t *node) {
    return node && node->type == YAML_SCALAR_NODE &&
           strlen((const char *)node->data.scalar.value) == node->data.scalar.length;
}

static bool read_style(yaml_document_t *document, VectorWorkspaceStyle *style) {
    yaml_node_t *root = yaml_document_get_root_node(document);
    if (!root || root->type != YAML_MAPPING_NODE ||
        root->data.mapping.pairs.top - root->data.mapping.pairs.start != 1) {
        return false;
    }
    yaml_node_pair_t pair = root->data.mapping.pairs.start[0];
    yaml_node_t *key = yaml_document_get_node(document, pair.key);
    yaml_node_t *settings = yaml_document_get_node(document, pair.value);
    if (!scalar(key) || strcmp((const char *)key->data.scalar.value, "artboard_shadow") ||
        !settings || settings->type != YAML_MAPPING_NODE) {
        return false;
    }
    const char *names[] = {"enabled", "color", "opacity", "offset_x", "offset_y", "softness"};
    unsigned seen = 0;
    for (yaml_node_pair_t *p = settings->data.mapping.pairs.start;
         p < settings->data.mapping.pairs.top; p++) {
        key = yaml_document_get_node(document, p->key);
        yaml_node_t *value = yaml_document_get_node(document, p->value);
        if (!scalar(key) || !scalar(value)) {
            return false;
        }
        int index = 0;
        while (index < 6 && strcmp((const char *)key->data.scalar.value, names[index])) {
            index++;
        }
        if (index == 6 || (seen & (1u << index))) {
            return false;
        }
        seen |= 1u << index;
        const char *text = (const char *)value->data.scalar.value;
        if (index == 0) {
            if (strcmp(text, "true") && strcmp(text, "false")) {
                return false;
            }
            style->enabled = !strcmp(text, "true");
        } else if (index == 1) {
            if (!pui_color_parse_hex(text, &style->color)) {
                return false;
            }
        } else {
            char *end;
            errno = 0;
            float number = strtof(text, &end);
            float min = index == 3 || index == 4 ? -100 : 0;
            float max = index == 2 ? 1 : index == 5 ? 32 : 100;
            if (!*text || *end || errno || !isfinite(number) || number < min || number > max) {
                return false;
            }
            float *fields[] = {&style->opacity, &style->offset_x, &style->offset_y,
                               &style->softness};
            *fields[index - 2] = number;
        }
    }
    return true;
}

bool vector_workspace_style_load(VectorWorkspaceStyle *style, const char *resources, char *error,
                                 size_t capacity) {
    char path[1600];
    if (snprintf(path, sizeof path, "%s/editors/vector/ui/workspace/style.yaml", resources) >=
        (int)sizeof path) {
        snprintf(error, capacity, "workspace style resource path too long");
        return false;
    }
    FILE *file = fopen(path, "rb");
    yaml_parser_t parser;
    if (!file || !yaml_parser_initialize(&parser)) {
        if (file) {
            fclose(file);
        }
        snprintf(error, capacity, "cannot load workspace style: %s", path);
        return false;
    }
    yaml_parser_set_input_file(&parser, file);
    yaml_document_t document;
    bool parsed = yaml_parser_load(&parser, &document);
    VectorWorkspaceStyle next = vector_workspace_style_default();
    bool valid = parsed && read_style(&document, &next);
    if (parsed) {
        yaml_document_delete(&document);
    }
    yaml_parser_delete(&parser);
    fclose(file);
    if (!valid) {
        snprintf(error, capacity,
                 "invalid workspace style: %s (check keys, color and numeric ranges)", path);
        return false;
    }
    *style = next;
    return true;
}

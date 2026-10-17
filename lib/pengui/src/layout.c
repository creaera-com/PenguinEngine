#include "pengui/layout.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef struct {
    yaml_document_t *document;
    UiLayout *layout;
    char *error;
    size_t capacity;
    bool failed;
} Reader;

static bool fail(Reader *r, yaml_node_t *n, const char *message) {
    if (!r->failed && r->error && r->capacity) {
        snprintf(r->error, r->capacity, "line %zu: %s", n ? n->start_mark.line + 1 : 1, message);
    }
    r->failed = true;
    return false;
}

static const char *scalar(Reader *r, yaml_node_t *n) {
    if (!n || n->type != YAML_SCALAR_NODE) {
        fail(r, n, "expected scalar");
        return "";
    }
    if (strlen((char *)n->data.scalar.value) != n->data.scalar.length) {
        fail(r, n, "embedded null is not allowed");
        return "";
    }
    return (char *)n->data.scalar.value;
}

static yaml_node_t *get(Reader *r, yaml_node_t *n, const char *key) {
    if (!n || n->type != YAML_MAPPING_NODE) {
        fail(r, n, "expected mapping");
        return NULL;
    }
    for (yaml_node_pair_t *p = n->data.mapping.pairs.start; p < n->data.mapping.pairs.top; p++) {
        if (!strcmp(scalar(r, yaml_document_get_node(r->document, p->key)), key)) {
            return yaml_document_get_node(r->document, p->value);
        }
    }
    return NULL;
}

static bool keys(Reader *r, yaml_node_t *n, const char *allowed) {
    if (!n || n->type != YAML_MAPPING_NODE) {
        return fail(r, n, "expected mapping");
    }
    for (yaml_node_pair_t *p = n->data.mapping.pairs.start; p < n->data.mapping.pairs.top; p++) {
        const char *key = scalar(r, yaml_document_get_node(r->document, p->key));
        char wrapped[80];
        if (snprintf(wrapped, sizeof wrapped, "|%s|", key) >= (int)sizeof wrapped ||
            !strstr(allowed, wrapped)) {
            return fail(r, n, "unknown property");
        }
        for (yaml_node_pair_t *q = n->data.mapping.pairs.start; q < p; q++) {
            if (!strcmp(key, scalar(r, yaml_document_get_node(r->document, q->key)))) {
                return fail(r, n, "duplicate property");
            }
        }
    }
    return !r->failed;
}

static void string(Reader *r, yaml_node_t *n, const char *key, char *out, size_t size) {
    yaml_node_t *value = get(r, n, key);
    if (value) {
        const char *text = scalar(r, value);
        if (strlen(text) >= size) {
            fail(r, value, "text exceeds field capacity");
        } else {
            snprintf(out, size, "%s", text);
        }
    }
}

static float number(Reader *r, yaml_node_t *n, const char *key, float fallback, float min,
                    float max) {
    yaml_node_t *value = get(r, n, key);
    if (!value) {
        return fallback;
    }
    const char *text = scalar(r, value);
    char *end;
    errno = 0;
    double parsed = strtod(text, &end);
    if (!*text || *end || errno || !isfinite(parsed) || parsed < min || parsed > max) {
        fail(r, value, "number outside allowed range");
        return fallback;
    }
    return (float)parsed;
}

static bool boolean(Reader *r, yaml_node_t *n, const char *key, bool fallback) {
    yaml_node_t *value = get(r, n, key);
    if (!value) {
        return fallback;
    }
    const char *text = scalar(r, value);
    if (strcmp(text, "true") && strcmp(text, "false")) {
        fail(r, value, "expected true or false");
    }
    return !strcmp(text, "true");
}

static int node(Reader *r, yaml_node_t *yaml, int depth) {
    static const char *types[] = {"column", "row",       "panel", "toolbar", "button",
                                  "label",  "separator", "tabs",  "tab",     "section",
                                  "input",  "tree",      "item",  "split",   "spacer"};
    if (depth >= UI_LAYOUT_DEPTH || r->layout->count >= UI_LAYOUT_NODES) {
        fail(r, yaml, "layout depth/node limit exceeded");
        return -1;
    }
    if (!keys(r, yaml,
              "|type|id|text|action|icon|image|width|height|gap|padding|initial|"
              "minimum_first|minimum_second|image_padding|enabled|open|axis|children|")) {
        return -1;
    }
    int index = r->layout->count++;
    UiNode *out = &r->layout->nodes[index];
    out->child = out->next = -1;
    const char *type = scalar(r, get(r, yaml, "type"));
    int kind = -1;
    for (size_t i = 0; i < sizeof types / sizeof types[0]; i++) {
        if (!strcmp(types[i], type)) {
            kind = (int)i;
        }
    }
    if (kind < 0) {
        fail(r, yaml, "unknown widget type");
        return -1;
    }
    out->type = (UiNodeType)kind;
    snprintf(out->name, sizeof out->name, "_node_%d", index);
    string(r, yaml, "id", out->name, sizeof out->name);
    out->id = ui_id(out->name);
    for (int i = 0; i < index; i++) {
        if (out->id == r->layout->nodes[i].id) {
            fail(r, yaml, "duplicate widget id");
        }
    }
    string(r, yaml, "text", out->text, sizeof out->text);
    string(r, yaml, "action", out->action, sizeof out->action);
    string(r, yaml, "icon", out->icon, sizeof out->icon);
    string(r, yaml, "image", out->image, sizeof out->image);
    out->width = number(r, yaml, "width", 0, 0, 10000);
    out->height = number(r, yaml, "height", 0, 0, 10000);
    out->gap = number(r, yaml, "gap", 3, 0, 128);
    out->padding = number(r, yaml, "padding", 8, 0, 128);
    out->initial = number(r, yaml, "initial", 240, 0, 10000);
    out->minimum_first = number(r, yaml, "minimum_first", 160, 0, 10000);
    out->minimum_second = number(r, yaml, "minimum_second", 160, 0, 10000);
    out->image_padding = number(r, yaml, "image_padding", 4, 2, 32);
    out->enabled = boolean(r, yaml, "enabled", true);
    out->open = boolean(r, yaml, "open", true);
    yaml_node_t *axis = get(r, yaml, "axis");
    if (axis) {
        const char *value = scalar(r, axis);
        if (strcmp(value, "horizontal") && strcmp(value, "vertical")) {
            fail(r, axis, "axis must be horizontal or vertical");
        }
        out->axis = !strcmp(value, "vertical") ? UI_AXIS_Y : UI_AXIS_X;
    }
    yaml_node_t *children = get(r, yaml, "children");
    int count = 0, previous = -1;
    if (children) {
        if (children->type != YAML_SEQUENCE_NODE) {
            fail(r, children, "children must be a sequence");
            return -1;
        }
        for (yaml_node_item_t *p = children->data.sequence.items.start;
             p < children->data.sequence.items.top && !r->failed; p++) {
            int child = node(r, yaml_document_get_node(r->document, *p), depth + 1);
            if (child < 0) {
                return -1;
            }
            if (previous < 0) {
                out->child = child;
            } else {
                r->layout->nodes[previous].next = child;
            }
            UiNodeType child_type = r->layout->nodes[child].type;
            if ((out->type == UI_NODE_TABS && child_type != UI_NODE_TAB) ||
                ((out->type == UI_NODE_TREE || out->type == UI_NODE_ITEM) &&
                 child_type != UI_NODE_ITEM) ||
                (child_type == UI_NODE_TAB && out->type != UI_NODE_TABS) ||
                (child_type == UI_NODE_ITEM && out->type != UI_NODE_TREE &&
                 out->type != UI_NODE_ITEM)) {
                fail(r, children, "invalid child widget for this parent");
            }
            previous = child;
            count++;
        }
    }
    if ((out->type == UI_NODE_SPLIT && count != 2) ||
        (out->type == UI_NODE_TABS && (count < 1 || count > UI_MAX_TABS)) ||
        ((out->type == UI_NODE_BUTTON || out->type == UI_NODE_LABEL || out->type == UI_NODE_INPUT ||
          out->type == UI_NODE_SPACER || out->type == UI_NODE_SEPARATOR) &&
         count)) {
        fail(r, yaml, "invalid child count");
    }
    return r->failed ? -1 : index;
}

UiLayout *ui_layout_parse(const char *text, size_t length, char *error, size_t capacity) {
    if (error && capacity) {
        *error = 0;
    }
    if (!text || !length || length > 256 * 1024) {
        if (error && capacity) {
            snprintf(error, capacity, "YAML must contain 1..262144 bytes");
        }
        return NULL;
    }
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        return NULL;
    }
    yaml_parser_set_input_string(&parser, (const unsigned char *)text, length);
    yaml_document_t document;
    if (!yaml_parser_load(&parser, &document)) {
        if (error && capacity) {
            snprintf(error, capacity, "line %zu: %s", parser.problem_mark.line + 1,
                     parser.problem ? parser.problem : "invalid YAML");
        }
        yaml_parser_delete(&parser);
        return NULL;
    }
    UiLayout *layout = calloc(1, sizeof *layout);
    if (!layout) {
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        return NULL;
    }
    Reader r = {&document, layout, error, capacity, false};
    yaml_node_t *root = yaml_document_get_root_node(&document);
    if (keys(&r, root, "|schema|window|theme|root|")) {
        if (strcmp(scalar(&r, get(&r, root, "schema")), "pengui/v1")) {
            fail(&r, root, "unsupported schema; expected pengui/v1");
        }
        strcpy(layout->title, "pengUI");
        layout->width = 1280;
        layout->height = 800;
        yaml_node_t *window = get(&r, root, "window");
        if (window && keys(&r, window, "|title|width|height|")) {
            string(&r, window, "title", layout->title, sizeof layout->title);
            layout->width = (int)number(&r, window, "width", 1280, 640, 4096);
            layout->height = (int)number(&r, window, "height", 800, 480, 4096);
        }
        uint32_t defaults[] = {0x101010ff, 0x202020ff, 0x292929ff, 0x393939ff,
                               0xc28e5cff, 0xe9e9e9ff, 0xa2a2a2ff};
        memcpy(layout->colors, defaults, sizeof defaults);
        yaml_node_t *theme = get(&r, root, "theme");
        if (theme && keys(&r, theme, "|background|panel|surface|hover|accent|text|muted|")) {
            const char *names[] = {"background", "panel", "surface", "hover",
                                   "accent",     "text",  "muted"};
            for (int i = 0; i < 7; i++) {
                yaml_node_t *value = get(&r, theme, names[i]);
                if (value) {
                    const char *color = scalar(&r, value);
                    char *end;
                    unsigned long rgb =
                        strlen(color) == 7 && *color == '#' ? strtoul(color + 1, &end, 16) : 0;
                    if (strlen(color) != 7 || *color != '#' || *end ||
                        strspn(color + 1, "0123456789abcdefABCDEF") != 6) {
                        fail(&r, value, "theme color must be #RRGGBB");
                    } else {
                        layout->colors[i] = (uint32_t)(rgb << 8) | 255;
                    }
                }
            }
        }
        if (!r.failed) {
            layout->root = node(&r, get(&r, root, "root"), 0);
            if (layout->root >= 0 && (layout->nodes[layout->root].type == UI_NODE_TAB ||
                                      layout->nodes[layout->root].type == UI_NODE_ITEM)) {
                fail(&r, root, "tab/item requires its container");
            }
        }
    }
    /* A second YAML document would otherwise be silently ignored. */
    yaml_document_t extra;
    if (!r.failed && yaml_parser_load(&parser, &extra)) {
        if (yaml_document_get_root_node(&extra)) {
            fail(&r, root, "only one YAML document is allowed");
        }
        yaml_document_delete(&extra);
    } else if (!r.failed) {
        fail(&r, root, "invalid trailing YAML");
    }
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    if (r.failed) {
        free(layout);
        return NULL;
    }
    return layout;
}

UiLayout *ui_layout_load(const char *path, char *error, size_t capacity) {
    if (!path) {
        return NULL;
    }
    FILE *file = fopen(path, "rb");
    if (!file) {
        if (error && capacity) {
            snprintf(error, capacity, "%s: cannot open YAML", path);
        }
        return NULL;
    }
    char *buffer = malloc(256 * 1024 + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    size_t size = fread(buffer, 1, 256 * 1024 + 1, file);
    bool read_error = ferror(file) != 0;
    fclose(file);
    UiLayout *layout = read_error ? NULL : ui_layout_parse(buffer, size, error, capacity);
    free(buffer);
    if (layout) {
        const char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
        if (back && (!slash || back > slash)) {
            slash = back;
        }
        size_t length = slash ? (size_t)(slash - path + 1) : 0;
        if (length >= sizeof layout->directory) {
            ui_layout_free(layout);
            return NULL;
        }
        memcpy(layout->directory, path, length);
    }
    return layout;
}

void ui_layout_free(UiLayout *layout) {
    free(layout);
}

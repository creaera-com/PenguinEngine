/** @file @brief Load and validate the SDL-free YAML widget description. */
#include "pengui/pengui.h"
#include "widgets/color_picker.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

typedef struct {
    yaml_document_t *document;
    PUILayout *layout;
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
    static const char *types[] = {
        "column", "row",    "panel",        "toolbar",      "button",   "label",  "separator",
        "tabs",   "tab",    "section",      "input",        "tree",     "item",   "split",
        "spacer", "toggle", "checkbox",     "button_group", "dropdown", "option", "popup_button",
        "slot",   "canvas", "color_picker", "radial_menu"};
    if (depth >= PUI_LAYOUT_DEPTH || r->layout->count >= PUI_LAYOUT_NODES) {
        fail(r, yaml, "layout depth/node limit exceeded");
        return -1;
    }
    if (!keys(
            r, yaml,
            "|type|id|text|action|icon|image|width|height|gap|padding|initial|"
            "initial_second|initial_last|minimum_first|minimum_second|minimum_last|flex|image_"
            "padding|enabled|open|checked|draggable|scroll|axis|children|value|popup_width|popup_"
            "height|"
            "placement|preview_action|close_action|filter_input|color_input|mode|input_type|radius|"
            "transition_ms|modal|modal_target|modal_title|")) {
        return -1;
    }
    int index = r->layout->count++;
    PUINode *out = &r->layout->nodes[index];
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
    out->type = (PUINodeType)kind;
    if (out->type == PUI_NODE_SEPARATOR) {
        out->axis = PUI_AXIS_Y; /* Preserve vertical toolbar separators by default. */
    }
    snprintf(out->name, sizeof out->name, "_node_%d", index);
    string(r, yaml, "id", out->name, sizeof out->name);
    out->generated_id = !get(r, yaml, "id");
    out->id = pui_id(out->name);
    for (int i = 0; i < index; i++) {
        if (out->id == r->layout->nodes[i].id) {
            fail(r, yaml, "duplicate widget id");
        }
    }
    string(r, yaml, "text", out->text, sizeof out->text);
    string(r, yaml, "action", out->action, sizeof out->action);
    yaml_node_t *input_type = get(r, yaml, "input_type");
    if (input_type) {
        const char *value = scalar(r, input_type);
        if (out->type != PUI_NODE_INPUT ||
            (strcmp(value, "string") && strcmp(value, "integer") && strcmp(value, "decimal"))) {
            fail(r, input_type, "input_type must be string, integer or decimal on an input");
        }
        out->input_type = !strcmp(value, "integer")   ? PUI_INPUT_INTEGER
                          : !strcmp(value, "decimal") ? PUI_INPUT_DECIMAL
                                                      : PUI_INPUT_STRING;
    }
    if (out->type == PUI_NODE_INPUT) {
        if (!pui_text_input_valid(out->input_type, out->text, false)) {
            fail(r, yaml, "initial input text does not match input_type");
        }
        if (out->input_type == PUI_INPUT_DECIMAL) {
            for (char *p = out->text; *p; p++) {
                if (*p == ',') {
                    *p = '.';
                }
            }
        }
    }
    string(r, yaml, "icon", out->icon, sizeof out->icon);
    string(r, yaml, "image", out->image, sizeof out->image);
    string(r, yaml, "value", out->value, sizeof out->value);
    out->popup_width = number(r, yaml, "popup_width", 0, 0, 10000);
    out->popup_height = number(r, yaml, "popup_height", 200, 28, 10000);
    yaml_node_t *placement = get(r, yaml, "placement");
    if (placement) {
        const char *value = scalar(r, placement);
        if (strcmp(value, "below") && strcmp(value, "right")) {
            fail(r, placement, "placement must be below or right");
        }
        out->placement = !strcmp(value, "right") ? PUI_POPUP_RIGHT : PUI_POPUP_BELOW;
    }
    out->modal = boolean(r, yaml, "modal", false);
    string(r, yaml, "modal_target", out->modal_target, sizeof out->modal_target);
    string(r, yaml, "modal_title", out->modal_title, sizeof out->modal_title);
    if ((out->modal || *out->modal_target || *out->modal_title) &&
        out->type != PUI_NODE_POPUP_BUTTON) {
        fail(r, yaml, "modal properties require a popup_button");
    }
    if (!out->modal && (*out->modal_target || *out->modal_title)) {
        fail(r, yaml, "modal_target and modal_title require modal: true");
    }
    string(r, yaml, "color_input", out->color_input, sizeof out->color_input);
    if (*out->color_input && out->type != PUI_NODE_POPUP_BUTTON) {
        fail(r, yaml, "color_input requires a popup_button");
    }
    string(r, yaml, "filter_input", out->filter_input, sizeof out->filter_input);
    if (*out->filter_input && out->type != PUI_NODE_TREE) {
        fail(r, yaml, "filter_input requires a tree");
    }
    string(r, yaml, "close_action", out->close_action, sizeof out->close_action);
    string(r, yaml, "preview_action", out->preview_action, sizeof out->preview_action);
    yaml_node_t *mode = get(r, yaml, "mode");
    if (mode) {
        const char *value = scalar(r, mode);
        if (out->type != PUI_NODE_COLOR_PICKER || (strcmp(value, "hsl") && strcmp(value, "rgba"))) {
            fail(r, mode, "mode must be hsl or rgba on a color_picker");
        }
        out->color_mode = !strcmp(value, "rgba");
    }
    out->width = number(r, yaml, "width", 0, 0, 10000);
    out->height = number(r, yaml, "height", 0, 0, 10000);
    out->gap = number(r, yaml, "gap", 3, 0, 128);
    out->padding = number(r, yaml, "padding", 8, 0, 128);
    out->initial = number(r, yaml, "initial", 240, 0, 10000);
    out->minimum_first = number(r, yaml, "minimum_first", 160, 0, 10000);
    out->minimum_second = number(r, yaml, "minimum_second", 160, 0, 10000);
    out->initial_second = number(r, yaml, "initial_second", 240, 0, 10000);
    out->initial_last = number(r, yaml, "initial_last", 280, 0, 10000);
    out->minimum_last = number(r, yaml, "minimum_last", 160, 0, 10000);
    out->radius = number(r, yaml, "radius", -1, 0, 1024);
    out->transition_ms = number(r, yaml, "transition_ms", -1, 0, 10000);
    out->image_padding = number(r, yaml, "image_padding", 4, 2, 32);
    out->enabled = boolean(r, yaml, "enabled", true);
    out->open = boolean(r, yaml, "open", true);
    out->checked = boolean(r, yaml, "checked", false);
    if (out->type == PUI_NODE_BUTTON_GROUP && !get(r, yaml, "padding")) {
        out->padding = 0;
    }
    out->draggable = boolean(r, yaml, "draggable", false);
    out->scrollable = boolean(r, yaml, "scroll", false);
    if (out->scrollable && out->type != PUI_NODE_PANEL) {
        fail(r, yaml, "scroll requires a panel");
        return -1;
    }
    yaml_node_t *axis = get(r, yaml, "axis");
    if (axis) {
        const char *value = scalar(r, axis);
        if (strcmp(value, "horizontal") && strcmp(value, "vertical")) {
            fail(r, axis, "axis must be horizontal or vertical");
        }
        out->axis = !strcmp(value, "vertical") ? PUI_AXIS_Y : PUI_AXIS_X;
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
            PUINodeType child_type = r->layout->nodes[child].type;
            if ((out->type == PUI_NODE_DROPDOWN && child_type != PUI_NODE_OPTION) ||
                (child_type == PUI_NODE_OPTION && out->type != PUI_NODE_DROPDOWN) ||
                (out->type == PUI_NODE_POPUP_BUTTON && child_type != PUI_NODE_PANEL) ||
                (out->type == PUI_NODE_TABS && child_type != PUI_NODE_TAB) ||
                (out->type == PUI_NODE_BUTTON_GROUP && child_type != PUI_NODE_BUTTON) ||
                (out->type == PUI_NODE_RADIAL_MENU && child_type != PUI_NODE_BUTTON &&
                 child_type != PUI_NODE_RADIAL_MENU) ||
                ((out->type == PUI_NODE_TREE || out->type == PUI_NODE_ITEM) &&
                 child_type != PUI_NODE_ITEM) ||
                (child_type == PUI_NODE_TAB && out->type != PUI_NODE_TABS) ||
                (child_type == PUI_NODE_ITEM && out->type != PUI_NODE_TREE &&
                 out->type != PUI_NODE_ITEM)) {
                fail(r, children, "invalid child widget for this parent");
            }
            previous = child;
            count++;
        }
    }
    if ((out->type == PUI_NODE_RADIAL_MENU && (count < 2 || count > 8)) ||
        (out->type == PUI_NODE_POPUP_BUTTON && count != 1) ||
        (out->type == PUI_NODE_SPLIT && (count < 2 || count > 3)) ||
        (out->type == PUI_NODE_TABS && count > PUI_MAX_TABS) ||
        ((out->type == PUI_NODE_BUTTON || out->type == PUI_NODE_LABEL ||
          out->type == PUI_NODE_INPUT || out->type == PUI_NODE_TOGGLE ||
          out->type == PUI_NODE_CHECKBOX || out->type == PUI_NODE_OPTION ||
          out->type == PUI_NODE_CANVAS || out->type == PUI_NODE_COLOR_PICKER ||
          out->type == PUI_NODE_SLOT || out->type == PUI_NODE_SPACER ||
          out->type == PUI_NODE_SEPARATOR) &&
         count)) {
        fail(r, yaml, "invalid child count");
    }
    if (out->type == PUI_NODE_OPTION && !*out->value) {
        fail(r, yaml, "option requires a non-empty value");
    }
    if (out->type == PUI_NODE_DROPDOWN) {
        bool enabled = false, found = !*out->value;
        for (int k = out->child; k >= 0; k = r->layout->nodes[k].next) {
            const PUINode *child = &r->layout->nodes[k];
            enabled |= child->enabled;
            found |= child->enabled && !strcmp(child->value, out->value);
            for (int j = out->child; j != k; j = r->layout->nodes[j].next) {
                if (!strcmp(r->layout->nodes[j].value, child->value)) {
                    fail(r, yaml, "dropdown option values must be unique");
                }
            }
        }
        if (!enabled || !found) {
            fail(r, yaml, "dropdown requires an enabled option and a valid initial value");
        }
    }
    if (out->type == PUI_NODE_BUTTON_GROUP) {
        int checked = 0, enabled = 0;
        for (int k = out->child; k >= 0; k = r->layout->nodes[k].next) {
            const PUINode *child = &r->layout->nodes[k];
            enabled += child->enabled;
            checked += child->checked;
            if (child->checked && !child->enabled) {
                fail(r, yaml, "selected group button must be enabled");
            }
        }
        if (!enabled || checked > 1) {
            fail(r, yaml, "button_group needs an enabled button and at most one checked button");
        }
    }
    if (out->type == PUI_NODE_SPLIT) {
        out->flexible = 1;
        if (count == 2) {
            if (!get(r, yaml, "initial_last")) {
                out->initial_last = out->initial;
            }
            if (!get(r, yaml, "minimum_last")) {
                out->minimum_last = out->minimum_second;
            }
        }
        yaml_node_t *flex = get(r, yaml, "flex");
        if (flex) {
            const char *value = scalar(r, flex);
            if (!strcmp(value, "first")) {
                out->flexible = 0;
            } else if (!strcmp(value, "last")) {
                out->flexible = count - 1;
            } else if (!strcmp(value, "second") || (count == 3 && !strcmp(value, "middle"))) {
                out->flexible = 1;
            } else {
                fail(r, flex, "flex must select an existing pane: first, second, middle or last");
            }
        }
    }
    if (!r->failed && out->type == PUI_NODE_COLOR_PICKER) {
        if (depth + 1 >= PUI_LAYOUT_DEPTH) {
            fail(r, yaml, "color_picker exceeds layout depth");
        } else if (!pui_color_picker_expand(r->layout, index, r->error, r->capacity)) {
            r->failed = true;
        }
    }
    return r->failed ? -1 : index;
}

PUILayout *pui_layout_parse(const char *text, size_t length, char *error, size_t capacity) {
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
    PUILayout *layout = calloc(1, sizeof *layout);
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
        if (theme && keys(&r, theme,
                          "|background|panel|surface|hover|accent|highlight_color|Highlight_Color|"
                          "text|muted|button_radius|button_"
                          "transition_ms|toolbar_radius|tab_radius|")) {
            layout->button_radius = number(&r, theme, "button_radius", 0, 0, 1024);
            layout->button_transition_ms = number(&r, theme, "button_transition_ms", 0, 0, 10000);
            layout->toolbar_radius =
                number(&r, theme, "toolbar_radius", layout->button_radius, 0, 1024);
            layout->tab_radius = number(&r, theme, "tab_radius", layout->button_radius, 0, 1024);
            yaml_node_t *highlight = get(&r, theme, "highlight_color");
            yaml_node_t *highlight_alias = get(&r, theme, "Highlight_Color");
            if (highlight && highlight_alias) {
                fail(&r, theme, "use only one highlight_color spelling");
            }
            if (!highlight) {
                highlight = highlight_alias;
            }
            const char *names[] = {"background", "panel", "surface", "hover",
                                   "accent",     "text",  "muted"};
            for (int i = 0; i < 7; i++) {
                yaml_node_t *value = i == 4 && highlight ? highlight : get(&r, theme, names[i]);
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
            if (layout->root >= 0 && (layout->nodes[layout->root].type == PUI_NODE_TAB ||
                                      layout->nodes[layout->root].type == PUI_NODE_ITEM ||
                                      layout->nodes[layout->root].type == PUI_NODE_OPTION)) {
                fail(&r, root, "tab/item/option requires its container");
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

PUILayout *pui_layout_load(const char *path, char *error, size_t capacity) {
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
    PUILayout *layout = read_error ? NULL : pui_layout_parse(buffer, size, error, capacity);
    free(buffer);
    if (layout) {
        const char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
        if (back && (!slash || back > slash)) {
            slash = back;
        }
        size_t length = slash ? (size_t)(slash - path + 1) : 0;
        if (length >= sizeof layout->directory) {
            pui_layout_free(layout);
            return NULL;
        }
        memcpy(layout->directory, path, length);
    }
    return layout;
}

void pui_layout_free(PUILayout *layout) {
    free(layout);
}

#include "layout/ui_layout.h"
#include "yaml.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *actions[] = {"document.save",
                                "document.undo",
                                "document.redo",
                                "scene.toggle_play",
                                "map.import_tileset",
                                "map.select_brush",
                                "map.select_eraser",
                                "map.select_rectangle",
                                "map.select_fill",
                                "map.select_entity",
                                "map.toggle_grid",
                                "map.toggle_collision",
                                "scene.add_root",
                                "scene.add_sibling",
                                "scene.add_child",
                                "scene.delete",
                                "script.check",
                                "window.minimize",
                                "window.maximize",
                                "window.collapse",
                                "window.close",
                                "vector.new",
                                "vector.open",
                                "vector.save_as",
                                "vector.select",
                                "vector.rectangle",
                                "vector.ellipse",
                                "vector.pen",
                                "vector.nodes",
                                "vector.artboard",
                                "vector.fit",
                                "vector.zoom_in",
                                "vector.zoom_out",
                                "vector.zoom_reset",
                                "vector.preview_select",
                                "vector.preview_clear",
                                "vector.toggle_grid",
                                "vector.toggle_snap",
                                "vector.grid_standard",
                                "vector.grid_isometric",
                                "vector.convert",
                                "vector.mirror_x",
                                "vector.mirror_y",
                                "vector.union",
                                "vector.intersection",
                                "vector.difference",
                                "vector.xor",
                                "vector.delete",
                                "vector.duplicate",
                                "vector.split",
                                "vector.close_path",
                                "vector.curve",
                                "vector.raise",
                                "vector.lower",
                                "vector.delete_node",
                                NULL};
static const char *bindings[] = {"document.can_save",
                                 "document.can_undo",
                                 "document.can_redo",
                                 "document.save_state",
                                 "editor.mode",
                                 "editor.details",
                                 "editor.message",
                                 "map.grid_visible",
                                 "map.collision_visible",
                                 "project.files",
                                 "scene.entities",
                                 "scene.active",
                                 "vector.active",
                                 "selection.active",
                                 "document.active",
                                 "terminal.shared",
                                 NULL};
static const char *icons[] = {
    "menu",     "save",      "undo",     "redo",    "play",      "stop",   "import",   "brush",
    "erase",    "rectangle", "fill",     "select",  "collision", "grid",   "refresh",  "folder",
    "file",     "image",     "layers",   "entity",  "plus",      "delete", "edit",     "up",
    "down",     "right",     "eye",      "eye_off", "check",     "close",  "terminal", "minimize",
    "maximize", "restore",   "collapse", NULL};

static bool contains(const char *const *names, const char *name) {
    for (int i = 0; names[i]; i++) {
        if (!strcmp(names[i], name)) {
            return true;
        }
    }

    return false;
}

bool pe_ui_action_known(const char *name) {
    return name && contains(actions, name);
}

bool pe_ui_binding_known(const char *name) {
    return name && contains(bindings, name);
}

const cJSON *pe_ui_get(const cJSON *o, const char *key) {
    return cJSON_GetObjectItemCaseSensitive(o, key);
}

const char *pe_ui_scalar(const cJSON *v, const char *fallback) {
    return cJSON_IsString(v) ? v->valuestring : fallback;
}

const char *pe_ui_string(const cJSON *o, const char *key, const char *fallback) {
    return pe_ui_scalar(pe_ui_get(o, key), fallback);
}

typedef struct {
    PeUiLayout *layout;
    char *error;
    size_t size;
    const char *ids[PE_UI_MAX_NODES];
    int ids_count, stateful, canvases;
    unsigned specialized;
    const char *module;
} Check;

static bool error_at(Check *c, const cJSON *value, const char *reason) {
    size_t line = 1, column = 1;
    for (int i = 0; i < c->layout->mark_count; i++) {
        if (c->layout->marks[i].value == value) {
            line = c->layout->marks[i].line;
            column = c->layout->marks[i].column;
            break;
        }
    }
    if (c->error && c->size) {
        snprintf(c->error, c->size, "%s:%zu:%zu: %s", c->layout->path, line, column, reason);
    }

    return false;
}

static bool fields(Check *c, const cJSON *o, const char *allowed) {
    if (!cJSON_IsObject(o)) {
        return error_at(c, o, "expected a mapping");
    }
    for (const cJSON *v = o->child; v; v = v->next) {
        char token[96];
        snprintf(token, sizeof token, "|%s|", v->string);
        if (!strstr(allowed, token)) {
            char reason[180];
            snprintf(reason, sizeof reason, "unknown field '%s'", v->string);
            return error_at(c, v, reason);
        }
    }

    return true;
}

static bool id(Check *c, const cJSON *v) {
    if (!cJSON_IsString(v) || !*v->valuestring || strlen(v->valuestring) > 63) {
        return error_at(c, v, "id must be a nonempty string (max 63 bytes)");
    }
    for (int i = 0; i < c->ids_count; i++) {
        if (!strcmp(c->ids[i], v->valuestring)) {
            return error_at(c, v, "duplicate id");
        }
    }
    if (c->ids_count >= PE_UI_MAX_NODES) {
        return error_at(c, v, "too many ids");
    }
    c->ids[c->ids_count++] = v->valuestring;
    return true;
}

static bool number(Check *c, const cJSON *v, double minimum, double maximum) {
    return (cJSON_IsNumber(v) && isfinite(v->valuedouble) && v->valuedouble >= minimum &&
            v->valuedouble <= maximum && floor(v->valuedouble) == v->valuedouble) ||
           error_at(c, v, "number out of range");
}

static bool boolean(Check *c, const cJSON *v) {
    if (!v || cJSON_IsBool(v)) {
        return true;
    }
    if (!fields(c, v, "|binding|")) {
        return false;
    }

    const char *b = pe_ui_string(v, "binding", "");
    const char *names[] = {"document.can_save", "document.can_undo",     "document.can_redo",
                           "map.grid_visible",  "map.collision_visible", NULL};
    return contains(names, b) || error_at(c, v, "unknown or non-Boolean binding");
}

static bool text_value(Check *c, const cJSON *v) {
    if (!v) {
        return true;
    }
    if (cJSON_IsString(v) && strlen(v->valuestring) <= 512) {
        return true;
    }
    if (!fields(c, v, "|binding|")) {
        return false;
    }

    const char *names[] = {"document.save_state", "editor.mode", "editor.details", "editor.message",
                           NULL};
    return contains(names, pe_ui_string(v, "binding", "")) ||
           error_at(c, v, "expected text or a text binding");
}

static bool widget(Check *, const cJSON *, int);

static bool widgets(Check *c, const cJSON *v, int depth) {
    if (!cJSON_IsArray(v)) {
        return error_at(c, v, "expected widget list");
    }
    for (const cJSON *n = v->child; n; n = n->next) {
        if (!widget(c, n, depth + 1)) {
            return false;
        }
    }

    return true;
}

static bool widget(Check *c, const cJSON *v, int depth) {
    if (depth > PE_UI_MAX_DEPTH) {
        return error_at(c, v, "widget nesting limit exceeded");
    }

    const char *type = pe_ui_string(v, "type", "");
    if ((!strcmp(type, "tabs") || !strcmp(type, "section") || !strcmp(type, "split") ||
         !strcmp(type, "tool_panel")) &&
        ++c->stateful > 64) {
        return error_at(c, v, "maximum 64 stateful containers");
    }
    if ((!strcmp(type, "map_canvas") || !strcmp(type, "script_editor") ||
         !strcmp(type, "vector_canvas")) &&
        ++c->canvases > 1) {
        return error_at(c, v, "only one interactive canvas per editor is supported");
    }

    const char *specialized[] = {
        "file_tree",        "entity_tree",    "map_canvas",    "script_editor",
        "inspector",        "scene_info",     "script_info",   "terminal",
        "map_layers",       "tile_palette",   "vector_canvas", "vector_tree",
        "vector_inspector", "vector_preview", "vector_grid",   NULL};
    for (int i = 0; specialized[i]; i++) {
        if (!strcmp(type, specialized[i])) {
            unsigned bit = 1u << i;
            if (c->specialized & bit) {
                return error_at(c, v, "this specialized widget may occur only once per editor");
            }
            c->specialized |= bit;
        }
    }

    const char *extra = "";
    if (!strcmp(type, "tabs")) {
        extra = "|tabs|";
    } else if (!strcmp(type, "entity_tree")) {
        extra = "|tools|";
    } else if (!strcmp(type, "tool_panel")) {
        extra = "|children|";
    } else if ((!strcmp(type, "vector_preview") || !strcmp(type, "vector_grid"))) {
        extra = "|height|";
    } else if (!strcmp(type, "section")) {
        extra = "|children||title||open|";
    } else if (!strcmp(type, "row") || !strcmp(type, "column")) {
        extra = "|children||gap|";
    } else if (!strcmp(type, "split")) {
        extra = "|axis||initial||minimum_first||minimum_second||gap||first||second|";
    } else if (!strcmp(type, "editor_switcher")) {
        extra = "|labels|";
    } else if (strcmp(type, "button") && strcmp(type, "separator") && strcmp(type, "label") &&
               strcmp(type, "spacer") && strcmp(type, "file_tree") && strcmp(type, "entity_tree") &&
               strcmp(type, "map_canvas") && strcmp(type, "script_editor") &&
               strcmp(type, "inspector") && strcmp(type, "scene_info") &&
               strcmp(type, "script_info") && strcmp(type, "terminal") &&
               strcmp(type, "map_layers") && strcmp(type, "tile_palette") &&
               strcmp(type, "vector_canvas") && strcmp(type, "vector_tree") &&
               strcmp(type, "vector_inspector") && strcmp(type, "vector_preview") &&
               strcmp(type, "vector_grid")) {
        return error_at(c, v, "unknown widget type");
    }

    char allowed[256];
    snprintf(allowed, sizeof allowed,
             "|type||id||label||icon||tooltip||enabled||visible||active||binding||action|%s",
             extra);
    if (!fields(c, v, allowed) || !id(c, pe_ui_get(v, "id"))) {
        return false;
    }
    if (!boolean(c, pe_ui_get(v, "enabled")) || !boolean(c, pe_ui_get(v, "visible")) ||
        !boolean(c, pe_ui_get(v, "active"))) {
        return false;
    }

    const cJSON *icon = pe_ui_get(v, "icon");
    if (icon && (!cJSON_IsString(icon) || !contains(icons, icon->valuestring))) {
        return error_at(c, icon, "unknown icon");
    }
    if (!text_value(c, pe_ui_get(v, "label"))) {
        return false;
    }

    const cJSON *tip = pe_ui_get(v, "tooltip");
    if (tip && (!cJSON_IsString(tip) || strlen(tip->valuestring) > 512)) {
        return error_at(c, tip, "tooltip must be text");
    }

    const cJSON *action = pe_ui_get(v, "action");
    if (action && (strcmp(type, "button") || !cJSON_IsString(action) ||
                   !pe_ui_action_known(action->valuestring))) {
        return error_at(c, action, "unknown action or action on non-button");
    }
    if (action && c->module) {
        const char *a0 = action->valuestring;
        if ((!strncmp(a0, "vector.", 7) && strcmp(c->module, "vector")) ||
            (!strcmp(c->module, "vector") &&
             (!strncmp(a0, "map.", 4) || !strncmp(a0, "scene.", 6) ||
              !strncmp(a0, "script.", 7)))) {
            return error_at(c, action, "action unavailable in this editor");
        }
        const char *a = action->valuestring;
        if ((!strcmp(c->module, "scripts") &&
             (!strncmp(a, "map.", 4) || !strncmp(a, "scene.", 6))) ||
            (!strcmp(c->module, "map") && !strncmp(a, "script.", 7))) {
            return error_at(c, action, "action is unavailable in this editor");
        }
    }

    const cJSON *tools = pe_ui_get(v, "tools");
    if (tools) {
        if (!cJSON_IsArray(tools) || cJSON_GetArraySize(tools) > 4) {
            return error_at(c, tools, "entity_tree tools requires at most four buttons");
        }
        for (const cJSON *tool = tools->child; tool; tool = tool->next) {
            if (strcmp(pe_ui_string(tool, "type", ""), "button") || !widget(c, tool, depth + 1)) {
                return error_at(c, tool, "entity_tree tools only supports buttons");
            }
        }
    }

    const char *binding = pe_ui_string(v, "binding", "");
    if (pe_ui_get(v, "binding") && !pe_ui_binding_known(binding)) {
        return error_at(c, v, "unknown binding");
    }

    const char *required = !strcmp(type, "terminal")      ? "terminal.shared"
                           : !strcmp(type, "file_tree")   ? "project.files"
                           : !strcmp(type, "entity_tree") ? "scene.entities"
                           : (!strcmp(type, "map_canvas") || !strcmp(type, "scene_info") ||
                              !strcmp(type, "map_layers") || !strcmp(type, "tile_palette"))
                               ? "scene.active"
                           : (!strcmp(type, "vector_canvas") || !strcmp(type, "vector_tree") ||
                              !strcmp(type, "vector_inspector") ||
                              !strcmp(type, "vector_preview") || !strcmp(type, "vector_grid"))
                               ? "vector.active"
                           : !strcmp(type, "inspector") ? "selection.active"
                           : (!strcmp(type, "script_editor") || !strcmp(type, "script_info"))
                               ? "document.active"
                               : NULL;
    if (!required && pe_ui_get(v, "binding")) {
        return error_at(c, v, "binding property is unavailable on this widget");
    }
    if (required && strcmp(binding, required)) {
        return error_at(c, v, "incompatible or missing widget binding");
    }

    bool map_only = !strcmp(type, "entity_tree") || !strcmp(type, "map_canvas") ||
                    !strcmp(type, "scene_info") || !strcmp(type, "inspector") ||
                    !strcmp(type, "map_layers") || !strcmp(type, "tile_palette");
    bool vector_only = !strcmp(type, "vector_canvas") || !strcmp(type, "vector_tree") ||
                       !strcmp(type, "vector_inspector") || !strcmp(type, "vector_preview") ||
                       !strcmp(type, "vector_grid");
    bool script_only = !strcmp(type, "script_editor") || !strcmp(type, "script_info");
    if ((map_only && (!c->module || strcmp(c->module, "map"))) ||
        (script_only && (!c->module || strcmp(c->module, "scripts"))) ||
        (vector_only && (!c->module || strcmp(c->module, "vector")))) {
        return error_at(c, v, "widget unavailable in this editor");
    }
    if (!strcmp(type, "editor_switcher") && (c->module || !cJSON_IsBool(pe_ui_get(v, "labels")))) {
        return error_at(c, v, "editor_switcher is shell-only and requires labels");
    }
    if (!strcmp(type, "terminal") && !c->module) {
        return error_at(c, v, "terminal is editor-only");
    }
    if (!strcmp(type, "split")) {
        const char *axis = pe_ui_string(v, "axis", "");
        if (!c->module || (strcmp(axis, "vertical") && strcmp(axis, "horizontal"))) {
            return error_at(c, v, "split is editor-only and requires vertical or horizontal axis");
        }
        if (!number(c, pe_ui_get(v, "initial"), 1, 2000) ||
            !number(c, pe_ui_get(v, "minimum_first"), 1, 2000) ||
            !number(c, pe_ui_get(v, "minimum_second"), 1, 2000) ||
            !number(c, pe_ui_get(v, "gap"), 4, 16) ||
            !widget(c, pe_ui_get(v, "first"), depth + 1) ||
            !widget(c, pe_ui_get(v, "second"), depth + 1)) {
            return false;
        }
    }
    if (!strcmp(type, "tabs")) {
        const cJSON *tabs = pe_ui_get(v, "tabs");
        if (!cJSON_IsArray(tabs) || cJSON_GetArraySize(tabs) < 1 || cJSON_GetArraySize(tabs) > 16) {
            return error_at(c, tabs, "tabs requires 1..16 entries");
        }
        for (const cJSON *tab = tabs->child; tab; tab = tab->next) {
            if (!fields(c, tab, "|id||title||icon||content|") || !id(c, pe_ui_get(tab, "id")) ||
                !cJSON_IsString(pe_ui_get(tab, "title"))) {
                return error_at(c, tab, "invalid tab");
            }
            const cJSON *i = pe_ui_get(tab, "icon");
            if (i && (!cJSON_IsString(i) || !contains(icons, i->valuestring))) {
                return error_at(c, i, "unknown tab icon");
            }
            if (!widget(c, pe_ui_get(tab, "content"), depth + 1)) {
                return false;
            }
        }
    }
    if (!strcmp(type, "tool_panel")) {
        const cJSON *children = pe_ui_get(v, "children");
        if (!c->module || !widgets(c, children, depth)) {
            return error_at(c, v, "tool_panel requires editor section children");
        }
        for (const cJSON *section = children->child; section; section = section->next) {
            if (strcmp(pe_ui_string(section, "type", ""), "section")) {
                return error_at(c, section, "tool_panel children must be sections");
            }
            const cJSON *buttons = pe_ui_get(section, "children");
            for (const cJSON *button = buttons->child; button; button = button->next) {
                const char *child_type = pe_ui_string(button, "type", "");
                if (strcmp(child_type, "button") &&
                    ((strcmp(child_type, "vector_preview") && strcmp(child_type, "vector_grid")) ||
                     button->next)) {
                    return error_at(c, button,
                                    "tool_panel sections only support buttons and a final "
                                    "vector_preview/vector_grid");
                }
            }
        }
    }
    if ((!strcmp(type, "vector_preview") || !strcmp(type, "vector_grid")) &&
        !number(c, pe_ui_get(v, "height"), !strcmp(type, "vector_grid") ? 128 : 64, 512)) {
        return false;
    }
    if (!strcmp(type, "section")) {
        if (!cJSON_IsString(pe_ui_get(v, "title")) || !cJSON_IsBool(pe_ui_get(v, "open"))) {
            return error_at(c, v, "section requires title and open Boolean");
        }
    }
    if (!strcmp(type, "row") || !strcmp(type, "column") || !strcmp(type, "section")) {
        const cJSON *gap = pe_ui_get(v, "gap");
        if (gap && !number(c, gap, 0, 64)) {
            return false;
        }
        if (!widgets(c, pe_ui_get(v, "children"), depth)) {
            return false;
        }
    }

    return true;
}

static bool toolbar(Check *c, const cJSON *list) {
    if (!cJSON_IsArray(list)) {
        return error_at(c, list, "toolbar must be a list");
    }
    for (const cJSON *v = list->child; v; v = v->next) {
        const char *t = pe_ui_string(v, "type", "");
        if (strcmp(t, "button") && strcmp(t, "separator") && strcmp(t, "editor_switcher")) {
            return error_at(c, v, "invalid toolbar element");
        }
        if (!widget(c, v, 0)) {
            return false;
        }
    }

    return true;
}

static bool validate(Check *c, const char *kind) {
    const cJSON *r = c->layout->root;
    if (strcmp(pe_ui_string(r, "schema", ""), "penguin.ui/v1") ||
        strcmp(pe_ui_string(r, "kind", ""), kind)) {
        return error_at(c, r, "unsupported schema or kind");
    }
    if (!id(c, pe_ui_get(r, "id"))) {
        return false;
    }
    if (!strcmp(kind, "editor")) {
        c->module = pe_ui_string(r, "id", "");
        if (strcmp(c->module, "map") && strcmp(c->module, "scripts") &&
            strcmp(c->module, "vector")) {
            return error_at(c, r, "editor module is not registered");
        }
        if (!fields(c, r, "|schema||kind||id||document_provider||toolbar||columns|")) {
            return false;
        }
        const char *provider = pe_ui_string(r, "document_provider", "");
        if (strcmp(provider, !strcmp(c->module, "map")      ? "scene"
                             : !strcmp(c->module, "vector") ? "vector"
                                                            : "script")) {
            return error_at(c, r, "document provider does not match editor");
        }
        if (!toolbar(c, pe_ui_get(r, "toolbar"))) {
            return false;
        }
        const cJSON *cols = pe_ui_get(r, "columns");
        if (!fields(c, cols, "|left||center||right|")) {
            return false;
        }
        return widget(c, pe_ui_get(cols, "left"), 0) && widget(c, pe_ui_get(cols, "center"), 0) &&
               widget(c, pe_ui_get(cols, "right"), 0);
    }
    if (!fields(c, r,
                "|schema||kind||id||layout||theme||toolbar||editors||editor_policy||statusbar||"
                "window_controls|") ||
        strcmp(pe_ui_string(r, "editor_policy", ""), "unload_on_leave")) {
        return error_at(c, r, "shell requires unload_on_leave policy");
    }

    const cJSON *controls = pe_ui_get(r, "window_controls");
    if (controls) {
        if (!cJSON_IsArray(controls) || cJSON_GetArraySize(controls) > 4) {
            return error_at(c, controls, "window_controls requires at most four buttons");
        }
        for (const cJSON *v = controls->child; v; v = v->next) {
            const char *action = pe_ui_string(v, "action", "");
            if (strcmp(pe_ui_string(v, "type", ""), "button") || strncmp(action, "window.", 7) ||
                !widget(c, v, 0)) {
                return error_at(c, v, "window_controls only supports window action buttons");
            }
        }
    }

    const cJSON *layout = pe_ui_get(r, "layout");
    if (!fields(c, layout, "|gap||tab_gap||columns|") ||
        !number(c, pe_ui_get(layout, "gap"), 0, 12) ||
        !number(c, pe_ui_get(layout, "tab_gap"), 0, 16)) {
        return false;
    }

    const cJSON *cols = pe_ui_get(layout, "columns");
    if (!fields(c, cols, "|left||center||right|")) {
        return false;
    }

    const char *names[] = {"left", "center", "right"};
    for (int i = 0; i < 3; i++) {
        const cJSON *col = pe_ui_get(cols, names[i]);
        if (!fields(c, col, i == 1 ? "|minimum|" : "|initial||minimum|") ||
            !number(c, pe_ui_get(col, "minimum"), 64, 2000)) {
            return false;
        }
        if (i != 1 &&
            !number(c, pe_ui_get(col, "initial"), pe_ui_get(col, "minimum")->valuedouble, 2000)) {
            return false;
        }
    }

    const cJSON *theme = pe_ui_get(r, "theme");
    if (!fields(c, theme, "|background||panel||surface||hover||accent||text||muted|")) {
        return false;
    }
    for (const cJSON *v = theme->child; v; v = v->next) {
        if (!cJSON_IsString(v) || strlen(v->valuestring) != 7 || v->valuestring[0] != '#') {
            return error_at(c, v, "expected quoted #RRGGBB color");
        }
        for (int i = 1; i < 7; i++) {
            if (!isxdigit((unsigned char)v->valuestring[i])) {
                return error_at(c, v, "invalid hex color");
            }
        }
    }
    if (!toolbar(c, pe_ui_get(r, "toolbar"))) {
        return false;
    }

    const cJSON *shell_toolbar = pe_ui_get(r, "toolbar");
    int selectors = 0;
    for (const cJSON *v = shell_toolbar->child; v; v = v->next) {
        if (!strcmp(pe_ui_string(v, "type", ""), "editor_switcher")) {
            selectors++;
            if (v->next) {
                return error_at(c, v, "editor_switcher must be the last toolbar element");
            }
        }
    }
    if (selectors != 1) {
        return error_at(c, shell_toolbar, "shell toolbar requires one editor_switcher");
    }

    const cJSON *editors = pe_ui_get(r, "editors");
    if (!cJSON_IsArray(editors) || cJSON_GetArraySize(editors) < 1 ||
        cJSON_GetArraySize(editors) > 16) {
        return error_at(c, editors, "requires 1..16 editors");
    }
    for (const cJSON *e = editors->child; e; e = e->next) {
        if (!fields(c, e, "|id||title||icon||module||file||separator_before|") ||
            !id(c, pe_ui_get(e, "id"))) {
            return false;
        }
        const cJSON *separator = pe_ui_get(e, "separator_before");
        if (separator && !cJSON_IsBool(separator)) {
            return error_at(c, separator, "separator_before requires boolean");
        }
        const char *module = pe_ui_string(e, "module", "");
        const char *file = pe_ui_string(e, "file", "");
        if ((strcmp(module, "map") && strcmp(module, "scripts") && strcmp(module, "vector")) ||
            strcmp(module, pe_ui_string(e, "id", "")) || !cJSON_IsString(pe_ui_get(e, "title")) ||
            !contains(icons, pe_ui_string(e, "icon", "")) || !*file || *file == '/' ||
            strstr(file, "..") || strchr(file, '\\') || strchr(file, ':')) {
            return error_at(c, e, "invalid or unregistered editor entry");
        }
    }

    const cJSON *status = pe_ui_get(r, "statusbar");
    if (!fields(c, status, "|id||lines||fields|") || !id(c, pe_ui_get(status, "id")) ||
        !number(c, pe_ui_get(status, "lines"), 1, 1)) {
        return false;
    }

    const cJSON *values = pe_ui_get(status, "fields");
    if (!cJSON_IsArray(values) || cJSON_GetArraySize(values) < 1 ||
        cJSON_GetArraySize(values) > 16) {
        return error_at(c, values, "status fields must be a list");
    }
    for (const cJSON *v = values->child; v; v = v->next) {
        if (!text_value(c, v)) {
            return false;
        }
    }

    return true;
}

static cJSON *convert(Check *check, yaml_document_t *doc, int index, int depth) {
    yaml_node_t *n = yaml_document_get_node(doc, index);
    if (!n || depth > PE_UI_MAX_DEPTH) {
        return NULL;
    }
    cJSON *value = NULL;
    if (n->type == YAML_SCALAR_NODE) {
        const char *s = (const char *)n->data.scalar.value;
        if (memchr(s, 0, n->data.scalar.length)) {
            error_at(check, NULL, "embedded NUL in YAML scalar");
            return NULL;
        }
        if (n->data.scalar.style == YAML_PLAIN_SCALAR_STYLE && !strcmp(s, "true")) {
            value = cJSON_CreateBool(true);
        } else if (n->data.scalar.style == YAML_PLAIN_SCALAR_STYLE && !strcmp(s, "false")) {
            value = cJSON_CreateBool(false);
        } else {
            char *end;
            double number = strtod(s, &end);
            if (*s && !*end && isfinite(number) &&
                n->data.scalar.style == YAML_PLAIN_SCALAR_STYLE) {
                value = cJSON_CreateNumber(number);
            } else {
                value = cJSON_CreateString(s);
            }
        }
    } else if (n->type == YAML_SEQUENCE_NODE) {
        value = cJSON_CreateArray();
        for (yaml_node_item_t *i = n->data.sequence.items.start;
             value && i < n->data.sequence.items.top; i++) {
            cJSON *child = convert(check, doc, *i, depth + 1);
            if (!child) {
                cJSON_Delete(value);
                return NULL;
            }
            if (!cJSON_AddItemToArray(value, child)) {
                cJSON_Delete(child);
                cJSON_Delete(value);
                return NULL;
            }
        }
    } else if (n->type == YAML_MAPPING_NODE) {
        value = cJSON_CreateObject();
        for (yaml_node_pair_t *i = n->data.mapping.pairs.start;
             value && i < n->data.mapping.pairs.top; i++) {
            yaml_node_t *key = yaml_document_get_node(doc, i->key);
            if (!key || key->type != YAML_SCALAR_NODE ||
                !strcmp((char *)key->data.scalar.value, "<<") ||
                cJSON_HasObjectItem(value, (char *)key->data.scalar.value)) {
                if (check->error && check->size) {
                    snprintf(check->error, check->size,
                             "%s:%zu:%zu: duplicate or invalid mapping key", check->layout->path,
                             key ? key->start_mark.line + 1 : 1,
                             key ? key->start_mark.column + 1 : 1);
                }
                cJSON_Delete(value);
                return NULL;
            }
            cJSON *child = convert(check, doc, i->value, depth + 1);
            if (!child) {
                cJSON_Delete(value);
                return NULL;
            }
            if (!cJSON_AddItemToObject(value, (char *)key->data.scalar.value, child)) {
                cJSON_Delete(child);
                cJSON_Delete(value);
                return NULL;
            }
        }
    }
    if (value && check->layout->mark_count < PE_UI_MAX_NODES) {
        check->layout->marks[check->layout->mark_count++] =
            (PeUiMark){value, n->start_mark.line + 1, n->start_mark.column + 1};
    }

    return value;
}

void pe_ui_layout_free(PeUiLayout *l) {
    if (l) {
        cJSON_Delete(l->root);
        free(l->marks);
        free(l);
    }
}

PeUiLayout *pe_ui_layout_load(const char *path, const char *kind, char *error, size_t size) {
    if (error && size) {
        *error = 0;
    }
    if (strlen(path) >= 1024) {
        if (error && size) {
            snprintf(error, size, "UI path exceeds 1023 bytes");
        }
        return NULL;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (error && size) {
            snprintf(error, size, "%s:1:1: cannot open UI file", path);
        }
        return NULL;
    }

    bool ok = !fseek(f, 0, SEEK_END);
    long length = ok ? ftell(f) : -1;
    rewind(f);
    if (length < 0 || length > PE_UI_MAX_FILE) {
        fclose(f);
        if (error && size) {
            snprintf(error, size, "%s:1:1: UI file size limit exceeded", path);
        }
        return NULL;
    }
    unsigned char *data = malloc((size_t)length + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }
    ok = fread(data, 1, (size_t)length, f) == (size_t)length;
    fclose(f);
    data[length] = 0;
    yaml_parser_t parser;
    bool initialized = yaml_parser_initialize(&parser) != 0;
    ok = ok && initialized;
    if (initialized) {
        yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
    }
    if (initialized) {
        yaml_parser_set_input_string(&parser, data, (size_t)length);
    }

    int depth = 0, nodes = 0, documents = 0;
    while (ok) {
        yaml_event_t e;
        if (!yaml_parser_parse(&parser, &e)) {
            ok = false;
            break;
        }
        bool end = e.type == YAML_STREAM_END_EVENT;
        if (e.type == YAML_DOCUMENT_START_EVENT) {
            documents++;
        }
        if (e.type == YAML_MAPPING_START_EVENT || e.type == YAML_SEQUENCE_START_EVENT) {
            depth++;
            nodes++;
        }
        if (e.type == YAML_MAPPING_END_EVENT || e.type == YAML_SEQUENCE_END_EVENT) {
            depth--;
        }
        if (e.type == YAML_SCALAR_EVENT) {
            nodes++;
        }
        bool special =
            e.type == YAML_ALIAS_EVENT ||
            (e.type == YAML_SCALAR_EVENT && (e.data.scalar.anchor || e.data.scalar.tag)) ||
            (e.type == YAML_MAPPING_START_EVENT &&
             (e.data.mapping_start.anchor || e.data.mapping_start.tag)) ||
            (e.type == YAML_SEQUENCE_START_EVENT &&
             (e.data.sequence_start.anchor || e.data.sequence_start.tag));
        if (depth > PE_UI_MAX_DEPTH || nodes > PE_UI_MAX_NODES || documents > 1 || special) {
            if (error && size) {
                snprintf(error, size,
                         "%s:%zu:%zu: YAML limits exceeded or tags/anchors/aliases are unsupported",
                         path, e.start_mark.line + 1, e.start_mark.column + 1);
            }
            ok = false;
        }
        yaml_event_delete(&e);
        if (end) {
            break;
        }
    }
    if (!ok && error && size && !*error) {
        snprintf(error, size, "%s:%zu:%zu: %s", path,
                 initialized ? parser.problem_mark.line + 1 : 1,
                 initialized ? parser.problem_mark.column + 1 : 1,
                 initialized && parser.problem ? parser.problem : "YAML parse failed");
    }
    if (initialized) {
        yaml_parser_delete(&parser);
    }

    PeUiLayout *layout = NULL;
    if (ok) {
        layout = calloc(1, sizeof *layout);
        if (layout) {
            snprintf(layout->path, sizeof layout->path, "%s", path);
            layout->marks = calloc(PE_UI_MAX_NODES, sizeof(PeUiMark));
        }
        ok = layout && layout->marks && yaml_parser_initialize(&parser);
        if (ok) {
            yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
            yaml_parser_set_input_string(&parser, data, (size_t)length);
            yaml_document_t document;
            ok = yaml_parser_load(&parser, &document) != 0;
            if (ok) {
                Check check = {0};
                check.layout = layout;
                check.error = error;
                check.size = size;
                layout->root = convert(&check, &document, 1, 0);
                ok = layout->root && validate(&check, kind);
                yaml_document_delete(&document);
            }
            yaml_parser_delete(&parser);
        }
    }
    free(data);
    if (!ok) {
        if (error && size && !*error) {
            snprintf(error, size, "%s:1:1: invalid or empty UI document", path);
        }
        pe_ui_layout_free(layout);
        return NULL;
    }

    return layout;
}

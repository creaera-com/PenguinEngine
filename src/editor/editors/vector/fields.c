#include "app/editor_internal.h"
#include "editors/vector/internal.h"

void vector_apply_field(int value) {
    VectorEditor *v = app.vector;
    Field field = (Field)value;
    if (field == FIELD_V_OPEN) {
        char path[1024];
        if (!vector_resolve_path(app.input, path, sizeof path)) {
            editor_message("Invalid SVG path");
            return;
        }
        editor_field_stop();
        editor_request(REQUEST_VECTOR_OPEN, path);
        return;
    }
    if (field == FIELD_V_SAVE) {
        char full[1024];
        if (!vector_resolve_path(app.input, full, sizeof full)) {
            editor_message("Invalid SVG path");
            return;
        }
        char error[256];
        if (!ve_save(&v->document, full, error, sizeof error)) {
            editor_message(error);
            return;
        }
        int after = v->after_save;
        snprintf(v->view.path, sizeof v->view.path, "%s", full);
        v->history.saved = v->history.ids[v->history.position];
        v->view.exists = 1;
        editor_field_stop();
        if (after) {
            editor_request((Request)after, NULL);
        }
        return;
    }
    int index = vector_first(), n = vector_selected_count();
    VeShape *s = index >= 0 ? &v->document.shapes[index] : NULL;
    VeBoard *b = &v->document.boards[v->view.board];
    if (field == FIELD_V_NAME) {
        if (!pe_entity_name_valid(app.input)) {
            editor_message("Name: 1-63 UTF-8 bytes");
            return;
        }
        memcpy(s ? s->name : b->name, app.input, strlen(app.input) + 1);
    } else if (field == FIELD_V_FILL || field == FIELD_V_STROKE || field == FIELD_V_BACKGROUND) {
        const char *hex = app.input + (app.input[0] == '#');
        if (strlen(hex) != 8 || strspn(hex, "0123456789abcdefABCDEF") != 8) {
            editor_message("Enter eight RGBA hex digits");
            return;
        }
        uint32_t rgba = (uint32_t)strtoul(hex, NULL, 16);
        if (field == FIELD_V_BACKGROUND) {
            b->background = rgba;
        } else {
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    if (field == FIELD_V_FILL) {
                        v->document.shapes[i].fill = rgba;
                    } else {
                        v->document.shapes[i].stroke = rgba;
                    }
                }
            }
        }
    } else {
        char *end;
        errno = 0;
        float number = strtof(app.input, &end);
        if (errno || end == app.input || *end || !isfinite(number) || fabsf(number) > 100000 ||
            ((field == FIELD_V_W || field == FIELD_V_H) && number <= 0) ||
            (field == FIELD_V_STROKE_WIDTH && (number < 0 || number > 4096))) {
            editor_message("Enter finite px value; size >0, stroke 0..4096");
            return;
        }
        if (field == FIELD_V_STROKE_WIDTH) {
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    v->document.shapes[i].stroke_width = number;
                }
            }
        } else if (n > 1) {
            editor_message("Select one object for geometry editing");
            return;
        } else if (s) {
            if (field == FIELD_V_X) {
                ve_move(s, number - s->x, 0);
            } else if (field == FIELD_V_Y) {
                ve_move(s, 0, number - s->y);
            } else {
                float old = field == FIELD_V_W ? s->w : s->h;
                if (old <= 0) {
                    editor_message("Cannot resize a zero-extent path on this axis");
                    return;
                }
                float scale = number / old;
                if (s->kind == VE_PATH) {
                    for (int i = 0; i < s->count; i++) {
                        VeNode *node = &s->nodes[i];
                        if (node->command == VE_CLOSE) {
                            continue;
                        }
                        VePoint *p[] = {&node->p, &node->c1, &node->c2};
                        for (int k = 0; k < (node->command == VE_CUBIC ? 3 : 1); k++) {
                            if (field == FIELD_V_W) {
                                p[k]->x = s->x + (p[k]->x - s->x) * scale;
                            } else {
                                p[k]->y = s->y + (p[k]->y - s->y) * scale;
                            }
                        }
                    }
                }
                if (field == FIELD_V_W) {
                    s->w = number;
                } else {
                    s->h = number;
                }
            }
        } else {
            if (field == FIELD_V_X) {
                b->x = number;
            } else if (field == FIELD_V_Y) {
                b->y = number;
            } else if (field == FIELD_V_W) {
                b->w = number;
            } else if (field == FIELD_V_H) {
                b->h = number;
            }
        }
    }
    vector_commit();
    editor_field_stop();
}

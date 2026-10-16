#include "app/editor_internal.h"
#include "editors/vector/internal.h"

bool vector_action(const char *action) {
    if (!action || strncmp(action, "vector.", 7)) {
        return false;
    }
    if (!editor_vector_active()) {
        return true;
    }
    VectorEditor *v = app.vector;
    const char *name = action + 7;
    int selected = vector_first();
    if (!strcmp(name, "new")) {
        editor_request(REQUEST_VECTOR_NEW, NULL);
        return true;
    }
    if (!strcmp(name, "open")) {
        editor_field_start(FIELD_V_OPEN, "assets/vector.svg");
        return true;
    }
    if (!strcmp(name, "save_as")) {
        editor_field_start(FIELD_V_SAVE, *v->view.path ? v->view.path : "assets/vector.svg");
        return true;
    }
    const char *tools[] = {"select", "rectangle", "ellipse", "pen", "nodes", "artboard"};
    for (int i = 0; i < 6; i++) {
        if (!strcmp(name, tools[i])) {
            v->view.tool = i;
            v->view.node = -1;
            return true;
        }
    }
    if (!strcmp(name, "fit")) {
        VeBoard *b = &v->document.boards[v->view.board];
        v->view.zoom =
            fminf(fmaxf(.02f, (v->canvas.w - 80) / b->w), fmaxf(.02f, (v->canvas.h - 80) / b->h));
        v->view.zoom = fminf(32, v->view.zoom);
        v->view.x = b->x - 40 / v->view.zoom;
        v->view.y = b->y - 40 / v->view.zoom;
        v->render_dirty = true;
        return true;
    }
    if (!strcmp(name, "delete")) {
        if (v->view.selected) {
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    v->document.shapes[i].used = false;
                }
            }
        } else {
            int count = 0;
            for (int i = 0; i < VE_BOARDS; i++) {
                count += v->document.boards[i].used;
            }
            if (count <= 1) {
                editor_message("Keep at least one artboard");
                return true;
            }
            v->document.boards[v->view.board].used = false;
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->document.shapes[i].board == v->view.board) {
                    v->document.shapes[i].used = false;
                }
            }
        }
        vector_commit();
        return true;
    }
    if (!strcmp(name, "duplicate")) {
        int available = 0;
        for (int i = 0; i < VE_SHAPES; i++) {
            available += !v->document.shapes[i].used;
        }
        if (available < vector_selected_count()) {
            editor_message("Object limit reached");
            return true;
        }
        uint64_t result = 0;
        for (int i = 0; i < VE_SHAPES; i++) {
            if (!(v->view.selected & (UINT64_C(1) << i))) {
                continue;
            }
            int slot = -1;
            for (int j = 0; j < VE_SHAPES; j++) {
                if (!v->document.shapes[j].used) {
                    slot = j;
                    break;
                }
            }
            if (slot < 0) {
                editor_message("Object limit reached");
                return true;
            }
            v->document.shapes[slot] = v->document.shapes[i];
            v->document.shapes[slot].id = v->document.next_id++;
            ve_move(&v->document.shapes[slot], 16, 16);
            result |= UINT64_C(1) << slot;
        }
        v->view.selected = result;
        vector_commit();
        return true;
    }
    const char *booleans[] = {"union", "mask", "subtract", "xor"};
    for (int k = 0; k < 4; k++) {
        if (!strcmp(name, booleans[k])) {
            int a = -1, b = -1, count = 0;
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    if (a < 0) {
                        a = i;
                    } else {
                        b = i;
                    }
                    count++;
                }
            }
            char error[256];
            if (count != 2) {
                editor_message("Shift+click exactly two objects");
                return true;
            }
            if (!ve_boolean(&v->document, a, b, (VeBoolean)k, error, sizeof error)) {
                editor_message(error);
                return true;
            }
            v->view.selected = v->document.shapes[a].used ? (UINT64_C(1) << a) : 0;
            vector_commit();
            editor_message("Boolean applied (curves flattened to 0.25 px)");
            return true;
        }
    }
    if (selected < 0) {
        editor_message("Select an object first");
        return true;
    }
    VeShape *s = &v->document.shapes[selected];
    if (!strcmp(name, "convert")) {
        for (int i = 0; i < VE_SHAPES; i++) {
            if (v->view.selected & (UINT64_C(1) << i)) {
                ve_to_path(&v->document.shapes[i]);
            }
        }
    } else if (!strcmp(name, "mirror_x") || !strcmp(name, "mirror_y")) {
        bool horizontal = !strcmp(name, "mirror_x");
        float low = INFINITY, high = -INFINITY;
        for (int i = 0; i < VE_SHAPES; i++) {
            VeShape *o = &v->document.shapes[i];
            if (v->view.selected & (UINT64_C(1) << i)) {
                if (o->board != s->board) {
                    editor_message("Select objects on one artboard");
                    return true;
                }
                float a = horizontal ? o->x : o->y;
                low = fminf(low, a);
                high = fmaxf(high, a + (horizontal ? o->w : o->h));
            }
        }
        for (int i = 0; i < VE_SHAPES; i++) {
            if (v->view.selected & (UINT64_C(1) << i)) {
                ve_mirror(&v->document.shapes[i], horizontal, (low + high) * .5f);
            }
        }
    } else if (!strcmp(name, "curve")) {
        if (s->kind == VE_PATH && v->view.node > 0 && v->view.node < s->count) {
            VeNode *n = &s->nodes[v->view.node];
            if (n->command == VE_LINE) {
                VePoint p = s->nodes[v->view.node - 1].p;
                n->command = VE_CUBIC;
                n->c1 = (VePoint){p.x + (n->p.x - p.x) / 3, p.y + (n->p.y - p.y) / 3};
                n->c2 = (VePoint){p.x + 2 * (n->p.x - p.x) / 3, p.y + 2 * (n->p.y - p.y) / 3};
            }
        }
    } else if (!strcmp(name, "split")) {
        if (!ve_split_node(s, v->view.node)) {
            editor_message("Select a line/curve endpoint; path node limit may be reached");
            return true;
        }
    } else if (!strcmp(name, "close_path")) {
        if (s->kind == VE_PATH && s->count > 2 && s->count < VE_NODES &&
            s->nodes[s->count - 1].command != VE_CLOSE) {
            s->nodes[s->count++] = (VeNode){.command = VE_CLOSE};
        }
    } else if (!strcmp(name, "delete_node")) {
        int node = v->view.node;
        if (s->kind == VE_PATH && node > 0 && node < s->count && s->count > 2) {
            memmove(s->nodes + node, s->nodes + node + 1,
                    (size_t)(s->count - node - 1) * sizeof(VeNode));
            s->count--;
            v->view.node = -1;
            ve_bounds(s);
        }
    } else if (!strcmp(name, "raise") || !strcmp(name, "lower")) {
        int other = selected + (!strcmp(name, "raise") ? 1 : -1);
        if (other >= 0 && other < VE_SHAPES) {
            VeShape temp = *s;
            *s = v->document.shapes[other];
            v->document.shapes[other] = temp;
            v->view.selected = UINT64_C(1) << other;
        }
    }
    vector_commit();
    return true;
}

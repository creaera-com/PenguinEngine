#include "app/editor_internal.h"
#include "editors/vector/internal.h"

static VePoint world(float x, float y) {
    VectorEditor *v = app.vector;
    return (VePoint){v->view.x + (x - v->canvas.x) / v->view.zoom,
                     v->view.y + (y - v->canvas.y) / v->view.zoom};
}

static int board_at(VePoint p) {
    VectorEditor *v = app.vector;
    for (int i = VE_BOARDS - 1; i >= 0; i--) {
        VeBoard *b = &v->document.boards[i];
        if (b->used && p.x >= b->x && p.y >= b->y && p.x <= b->x + b->w && p.y <= b->y + b->h) {
            return i;
        }
    }
    return -1;
}

static int hit(VePoint p) {
    VectorEditor *v = app.vector;
    VeFlat *flat = malloc(sizeof *flat);
    if (!flat) {
        return -1;
    }
    int found = -1;
    for (int i = VE_SHAPES - 1; i >= 0; i--) {
        VeShape *s = &v->document.shapes[i];
        if (!s->used) {
            continue;
        }
        VeBoard *b = &v->document.boards[s->board];
        float x = p.x - b->x, y = p.y - b->y;
        if (!ve_flatten(s, flat)) {
            continue;
        }
        if (s->fill_enabled && ve_inside(flat, x, y)) {
            found = i;
            break;
        }
        if (s->stroke_enabled) {
            float tolerance = s->stroke_width * .5f + 4 / v->view.zoom;
            for (int c = 0; c < flat->contours && found < 0; c++) {
                int start = flat->start[c], count = flat->length[c];
                int segments = flat->closed[c] ? count : count - 1;
                for (int k = 0; k < segments; k++) {
                    VePoint a = flat->points[start + k], b = flat->points[start + (k + 1) % count];
                    float dx = b.x - a.x, dy = b.y - a.y, l = dx * dx + dy * dy;
                    float t = l > 0 ? fmaxf(0, fminf(1, ((x - a.x) * dx + (y - a.y) * dy) / l)) : 0;
                    if (hypotf(x - a.x - t * dx, y - a.y - t * dy) <= tolerance) {
                        found = i;
                        break;
                    }
                }
            }
            if (found >= 0) {
                break;
            }
        }
    }
    free(flat);
    return found;
}

static void move_one_anchor(VeShape *s, int index, float dx, float dy) {
    VeNode *n = &s->nodes[index];
    n->p.x += dx;
    n->p.y += dy;
    if (n->command == VE_CUBIC) {
        n->c2.x += dx;
        n->c2.y += dy;
    }
    if (index + 1 < s->count && s->nodes[index + 1].command == VE_CUBIC) {
        s->nodes[index + 1].c1.x += dx;
        s->nodes[index + 1].c1.y += dy;
    }
}

static void move_anchor(VeShape *s, int index, float dx, float dy) {
    int start = index, end = index;
    while (start > 0 && s->nodes[start].command != VE_MOVE) {
        start--;
    }
    while (end + 1 < s->count && s->nodes[end + 1].command != VE_MOVE &&
           s->nodes[end + 1].command != VE_CLOSE) {
        end++;
    }
    bool closed = end + 1 < s->count && s->nodes[end + 1].command == VE_CLOSE;
    VePoint first = s->nodes[start].p, last = s->nodes[end].p;
    bool duplicate = closed && end != start && fabsf(first.x - last.x) < 1e-5f &&
                     fabsf(first.y - last.y) < 1e-5f;
    move_one_anchor(s, index, dx, dy);
    if (duplicate && index == start) {
        move_one_anchor(s, end, dx, dy);
    } else if (duplicate && index == end) {
        move_one_anchor(s, start, dx, dy);
    }
}

static bool preview_event(SDL_Event *e, bool inside) {
    VectorEditor *v = app.vector;
    bool escape = e->type == SDL_EVENT_KEY_DOWN && e->key.key == SDLK_ESCAPE;
    bool cancel = escape || e->type == SDL_EVENT_WINDOW_FOCUS_LOST || e->type == SDL_EVENT_QUIT ||
                  e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    if (v->preview_drag && cancel) {
        v->view.preview = v->preview_before;
        v->view.preview_board = v->preview_before_board;
        v->view.preview_enabled = v->preview_before_enabled;
        v->preview_drag = false;
        return escape;
    }
    if (v->preview_drag && e->type == SDL_EVENT_KEY_DOWN) {
        return true;
    }
    VePoint p = world(app.mx, app.my);
    bool release = e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT;
    if (v->preview_drag && (e->type == SDL_EVENT_MOUSE_MOTION || release)) {
        VeBoard *b = &v->document.boards[v->view.preview_board];
        float x = fmaxf(0, fminf(b->w, p.x - b->x));
        float y = fmaxf(0, fminf(b->h, p.y - b->y));
        float left = floorf(fminf(v->start_x, x) + .0001f),
              top = floorf(fminf(v->start_y, y) + .0001f);
        v->view.preview =
            (VeRegion){left, top, fminf(b->w, ceilf(fmaxf(v->start_x, x) - .0001f)) - left,
                       fminf(b->h, ceilf(fmaxf(v->start_y, y) - .0001f)) - top};
        if (release) {
            v->preview_drag = false;
            if (v->view.preview.w < 1 || v->view.preview.h < 1) {
                v->view.preview = v->preview_before;
                v->view.preview_board = v->preview_before_board;
                v->view.preview_enabled = v->preview_before_enabled;
            }
        }
        return true;
    }
    if (v->view.tool == VT_PREVIEW && inside && !app.ui.blocked &&
        e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT) {
        int board = board_at(p);
        if (board < 0) {
            editor_message("Start the preview selection inside an artboard");
            return true;
        }
        v->preview_before = v->view.preview;
        v->preview_before_board = v->view.preview_board;
        v->preview_before_enabled = v->view.preview_enabled;
        v->view.preview_board = board;
        v->view.preview_enabled = true;
        v->preview_drag = true;
        VeBoard *b = &v->document.boards[board];
        v->start_x = p.x - b->x;
        v->start_y = p.y - b->y;
        v->view.preview = (VeRegion){floorf(v->start_x), floorf(v->start_y), 0, 0};
        return true;
    }
    return false;
}

bool vector_event(SDL_Event *e) {
    if (!editor_vector_active() || app.request != REQUEST_NONE || app.field != FIELD_NONE) {
        return false;
    }
    VectorEditor *v = app.vector;
    bool inside = editor_inside(app.mx, app.my, v->canvas);
    if (preview_event(e, inside)) {
        return true;
    }
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST || e->type == SDL_EVENT_QUIT ||
        e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        if (v->dragging) {
            char error[128];
            ve_from_json(&v->document, v->history.states[v->history.position], error, sizeof error);
            v->dragging = false;
            vector_sync();
        }
        v->panning = false;
        return false;
    }
    if (e->type == SDL_EVENT_KEY_DOWN && !(e->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
        if (e->key.key == SDLK_ESCAPE) {
            if (v->dragging) {
                char error[128];
                ve_from_json(&v->document, v->history.states[v->history.position], error,
                             sizeof error);
                v->dragging = false;
                vector_sync();
            }
            v->panning = false;
            v->view.tool = VT_SELECT;
            return true;
        }
        if (e->key.repeat) {
            return false;
        }
        if (e->key.key == SDLK_DELETE || e->key.key == SDLK_BACKSPACE) {
            if (v->view.tool == VT_PREVIEW) {
                vector_action("vector.preview_clear");
                return true;
            }
            vector_action(v->view.tool == VT_NODE && v->view.node > 0 ? "vector.delete_node"
                                                                      : "vector.delete");
            return true;
        }
        if (e->key.key == SDLK_RETURN && v->view.tool == VT_PEN) {
            vector_action("vector.close_path");
            v->view.tool = VT_SELECT;
            return true;
        }
        const SDL_Keycode keys[] = {SDLK_V, SDLK_R, SDLK_C, SDLK_P, SDLK_N, SDLK_A};
        for (int i = 0; i < 6; i++) {
            if (e->key.key == keys[i]) {
                v->view.tool = i;
                return true;
            }
        }
    }
    if (e->type == SDL_EVENT_MOUSE_WHEEL && inside) {
        vector_zoom(v->view.zoom * powf(1.15f, e->wheel.y), app.mx, app.my);
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && inside && e->button.button == SDL_BUTTON_MIDDLE) {
        v->panning = true;
        v->last_x = app.mx;
        v->last_y = app.my;
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_MIDDLE &&
        v->panning) {
        v->panning = false;
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION && v->panning) {
        v->view.x -= (app.mx - v->last_x) / v->view.zoom;
        v->view.y -= (app.my - v->last_y) / v->view.zoom;
        v->last_x = app.mx;
        v->last_y = app.my;
        v->render_dirty = true;
        return true;
    }
    VePoint p = world(app.mx, app.my);
    if (e->type == SDL_EVENT_MOUSE_MOTION && v->dragging) {
        float dx = p.x - v->last_x, dy = p.y - v->last_y;
        v->last_x = p.x;
        v->last_y = p.y;
        if (v->drag_board >= 0) {
            VeBoard *b = &v->document.boards[v->drag_board];
            if (v->view.tool == VT_BOARD) {
                b->x = fminf(v->start_x, p.x);
                b->y = fminf(v->start_y, p.y);
                b->w = fmaxf(.01f, fabsf(p.x - v->start_x));
                b->h = fmaxf(.01f, fabsf(p.y - v->start_y));
            } else {
                b->x += dx;
                b->y += dy;
            }
        } else if (v->drag_node >= 0 && v->drag_shape >= 0) {
            VeShape *s = &v->document.shapes[v->drag_shape];
            VeNode *node = &s->nodes[v->drag_node];
            if (v->drag_handle == 0) {
                move_anchor(s, v->drag_node, dx, dy);
            } else if (v->drag_handle == 1) {
                node->c1.x += dx;
                node->c1.y += dy;
            } else {
                node->c2.x += dx;
                node->c2.y += dy;
            }
            ve_bounds(s);
        } else if (v->view.tool == VT_RECT || v->view.tool == VT_ELLIPSE) {
            VeShape *s = &v->document.shapes[v->drag_shape];
            VeBoard *b = &v->document.boards[s->board];
            float endx = p.x, endy = p.y;
            if (SDL_GetModState() & SDL_KMOD_SHIFT) {
                float extent = fmaxf(fabsf(endx - v->start_x), fabsf(endy - v->start_y));
                endx = v->start_x + copysignf(extent, endx - v->start_x);
                endy = v->start_y + copysignf(extent, endy - v->start_y);
            }
            s->x = fminf(v->start_x, endx) - b->x;
            s->y = fminf(v->start_y, endy) - b->y;
            s->w = fmaxf(.01f, fabsf(endx - v->start_x));
            s->h = fmaxf(.01f, fabsf(endy - v->start_y));
        } else {
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    ve_move(&v->document.shapes[i], dx, dy);
                }
            }
        }
        v->render_dirty = true;
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT &&
        v->dragging) {
        if (v->view.tool == VT_SELECT && v->drag_board < 0 && v->drag_node < 0) {
            for (int i = 0; i < VE_SHAPES; i++) {
                VeShape *s = &v->document.shapes[i];
                if (!(v->view.selected & (UINT64_C(1) << i))) {
                    continue;
                }
                VeBoard *old = &v->document.boards[s->board];
                int target =
                    board_at((VePoint){old->x + s->x + s->w * .5f, old->y + s->y + s->h * .5f});
                if (target >= 0 && target != s->board) {
                    VeBoard *next = &v->document.boards[target];
                    ve_move(s, old->x - next->x, old->y - next->y);
                    s->board = target;
                    v->view.board = target;
                }
            }
        }
        v->dragging = false;
        vector_commit();
        return true;
    }
    if (e->type != SDL_EVENT_MOUSE_BUTTON_DOWN || e->button.button != SDL_BUTTON_LEFT || !inside ||
        app.ui.blocked) {
        return false;
    }
    int board = board_at(p);
    v->drag_board = v->drag_shape = v->drag_node = -1;
    v->start_x = v->last_x = p.x;
    v->start_y = v->last_y = p.y;
    if ((SDL_GetModState() & SDL_KMOD_ALT) && board >= 0) {
        v->view.board = board;
        v->view.selected = 0;
        v->drag_board = board;
        v->dragging = true;
        vector_sync();
        return true;
    }
    if (v->view.tool == VT_BOARD) {
        int b = ve_add_board(&v->document, p.x, p.y, .01f, .01f);
        if (b < 0) {
            editor_message("Artboard limit reached");
            return true;
        }
        v->view.board = b;
        v->view.selected = 0;
        v->view.expanded[b] = 1;
        v->drag_board = b;
        v->dragging = true;
        vector_sync();
        return true;
    }
    if (v->view.tool == VT_RECT || v->view.tool == VT_ELLIPSE) {
        if (board < 0) {
            editor_message("Draw inside an artboard");
            return true;
        }
        VeBoard *b = &v->document.boards[board];
        int i = ve_add_shape(&v->document, board, v->view.tool == VT_RECT ? VE_RECT : VE_ELLIPSE,
                             p.x - b->x, p.y - b->y, .01f, .01f);
        if (i < 0) {
            editor_message("Object limit reached");
            return true;
        }
        v->view.board = board;
        v->view.selected = UINT64_C(1) << i;
        v->drag_shape = i;
        v->dragging = true;
        vector_sync();
        return true;
    }
    if (v->view.tool == VT_PEN) {
        if (board < 0) {
            return true;
        }
        int i = vector_first();
        VeShape *s = i >= 0 ? &v->document.shapes[i] : NULL;
        if (e->button.clicks >= 2 && s && s->kind == VE_PATH && s->count > 2) {
            vector_action("vector.close_path");
            v->view.tool = VT_SELECT;
            return true;
        }
        if (!s || s->kind != VE_PATH || s->board != board ||
            s->nodes[s->count - 1].command == VE_CLOSE) {
            i = ve_add_shape(&v->document, board, VE_PATH, 0, 0, 1, 1);
            if (i < 0) {
                return true;
            }
            s = &v->document.shapes[i];
            s->count = 0; /* The first click supplies the initial Move. */
            s->stroke_enabled = true;
            s->fill_enabled = false;
            v->view.selected = UINT64_C(1) << i;
            v->view.board = board;
        }
        if (s->count >= VE_NODES) {
            editor_message("Path node limit reached");
            return true;
        }
        VeBoard *b = &v->document.boards[board];
        s->nodes[s->count] =
            (VeNode){.command = s->count ? VE_LINE : VE_MOVE, .p = {p.x - b->x, p.y - b->y}};
        s->count++;
        ve_bounds(s);
        vector_commit();
        return true;
    }
    if (v->view.tool == VT_NODE) {
        int i = vector_first();
        if (i >= 0) {
            VeShape *s = &v->document.shapes[i];
            if (s->kind != VE_PATH) {
                editor_message("Convert the shape to a path first");
                return true;
            }
            VeBoard *b = &v->document.boards[s->board];
            for (int n = 0; n < s->count; n++) {
                VeNode *node = &s->nodes[n];
                if (node->command == VE_CLOSE) {
                    continue;
                }
                VePoint points[] = {node->p, node->c1, node->c2};
                for (int k = 0; k < (node->command == VE_CUBIC ? 3 : 1); k++) {
                    if (hypotf(points[k].x + b->x - p.x, points[k].y + b->y - p.y) <
                        8 / v->view.zoom) {
                        v->drag_shape = i;
                        v->drag_node = v->view.node = n;
                        v->drag_handle = k;
                        v->dragging = true;
                        return true;
                    }
                }
            }
        }
    }
    int i = hit(p);
    if (i >= 0) {
        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            v->view.selected ^= UINT64_C(1) << i;
        } else {
            if (!(v->view.selected & (UINT64_C(1) << i))) {
                v->view.selected = UINT64_C(1) << i;
            }
            v->dragging = v->view.tool == VT_SELECT;
        }
        v->view.board = v->document.shapes[i].board;
    } else {
        v->view.selected = 0;
        if (board >= 0) {
            v->view.board = board;
        }
    }
    v->view.node = -1;
    vector_sync();
    return true;
}

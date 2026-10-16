#include "app/editor_internal.h"
#include "editors/vector/internal.h"
#include "nanosvg.h"
#include "nanosvgrast.h"

bool editor_vector_active(void) {
    return app.loaded_view == UI_VECTOR_VIEW;
}

bool vector_init(VectorEditor *v) {
    ve_init(&v->document);
    v->view = (VeView){.board = 0, .node = -1, .zoom = 1, .x = -64, .y = -64};
    memset(v->view.expanded, 1, sizeof v->view.expanded);
    v->render_dirty = true;
    v->drag_shape = v->drag_board = v->drag_node = -1;
    return ve_history_init(&v->history, &v->document);
}

void vector_free(VectorEditor *v) {
    ve_history_free(&v->history);
    SDL_DestroyTexture(v->texture);
    nsvgDeleteRasterizer(v->rasterizer);
    free(v->pixels);
    v->texture = NULL;
    v->rasterizer = NULL;
    v->pixels = NULL;
}

void vector_sync(void) {
    VectorEditor *v = app.vector;
    for (int i = 0; i < VE_SHAPES; i++) {
        if (!v->document.shapes[i].used) {
            v->view.selected &= ~(UINT64_C(1) << i);
        }
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        bool found = false;
        for (int j = 0; j < VE_SHAPES; j++) {
            VeShape *shape = &v->document.shapes[j];
            if (shape->used && shape->kind == VE_PATH && shape->id == v->view.expanded_paths[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            v->view.expanded_paths[i] = 0;
        }
    }
    if (v->view.board < 0 || v->view.board >= VE_BOARDS ||
        !v->document.boards[v->view.board].used) {
        for (int i = 0; i < VE_BOARDS; i++) {
            if (v->document.boards[i].used) {
                v->view.board = i;
                break;
            }
        }
    }
    app.selected = (uint32_t)(10000000 + v->view.board);
    for (int i = 0; i < VE_SHAPES; i++) {
        if (v->view.selected & (UINT64_C(1) << i)) {
            app.selected = v->document.shapes[i].id;
            break;
        }
    }
    v->render_dirty = true;
    app.needs_layout = true;
}

void vector_commit(void) {
    VectorEditor *v = app.vector;
    bool valid = ve_valid(&v->document);
    if (!valid || !ve_history_commit(&v->history, &v->document)) {
        char error[256];
        ve_from_json(&v->document, v->history.states[v->history.position], error, sizeof error);
        if (!valid) {
            editor_message("Geometry limit exceeded; edit restored");
        }
    }
    vector_sync();
}

void vector_zoom(float zoom, float x, float y) {
    VectorEditor *v = app.vector;
    if (!isfinite(zoom)) {
        return;
    }
    float before_x = v->view.x + (x - v->canvas.x) / v->view.zoom;
    float before_y = v->view.y + (y - v->canvas.y) / v->view.zoom;
    v->view.zoom = fmaxf(.02f, fminf(32, zoom));
    v->view.x = before_x - (x - v->canvas.x) / v->view.zoom;
    v->view.y = before_y - (y - v->canvas.y) / v->view.zoom;
    v->render_dirty = true;
}

void vector_undo(bool redo) {
    ve_history_step(&app.vector->history, &app.vector->document, redo);
    app.vector->view.node = -1;
    vector_sync();
}

int vector_first(void) {
    for (int i = 0; i < VE_SHAPES; i++) {
        if (app.vector->view.selected & (UINT64_C(1) << i)) {
            return i;
        }
    }
    return -1;
}

int vector_selected_count(void) {
    int n = 0;
    for (int i = 0; i < VE_SHAPES; i++) {
        n += (app.vector->view.selected & (UINT64_C(1) << i)) != 0;
    }
    return n;
}

bool vector_resolve_path(const char *path, char *out, size_t size) {
    if (!*path) {
        return false;
    }
    bool absolute = path[0] == '/' || path[0] == '\\' || (strlen(path) > 1 && path[1] == ':');
    return snprintf(out, size, "%s%s%s", absolute ? "" : app.root, absolute ? "" : "/", path) <
           (int)size;
}

bool vector_open(const char *path) {
    VectorEditor *v = app.vector;
    VeDocument *next = malloc(sizeof *next);
    VeHistory history;
    char full[1024], error[256];
    if (!next || !vector_resolve_path(path, full, sizeof full)) {
        free(next);
        editor_message("Invalid SVG path");
        return false;
    }
    if (!ve_load(next, full, error, sizeof error)) {
        free(next);
        editor_message(error);
        return false;
    }
    if (!ve_history_init(&history, next)) {
        free(next);
        return false;
    }
    ve_history_free(&v->history);
    v->history = history;
    v->document = *next;
    free(next);
    snprintf(v->view.path, sizeof v->view.path, "%s", full);
    v->view.exists = 1;
    memset(v->view.expanded_paths, 0, sizeof v->view.expanded_paths);
    v->view.selected = 0;
    v->view.node = -1;
    vector_sync();
    v->view.x = v->document.boards[v->view.board].x - 48;
    v->view.y = v->document.boards[v->view.board].y - 48;
    editor_message("SVG loaded");
    return true;
}

void vector_new(void) {
    VectorEditor *v = app.vector;
    ve_history_free(&v->history);
    ve_init(&v->document);
    ve_history_init(&v->history, &v->document);
    v->view.path[0] = 0;
    v->view.exists = 0;
    memset(v->view.expanded_paths, 0, sizeof v->view.expanded_paths);
    v->view.selected = 0;
    v->view.board = 0;
    v->view.node = -1;
    v->view.x = v->view.y = -64;
    v->view.zoom = 1;
    vector_sync();
    editor_message("New SVG document");
}

bool vector_save(void) {
    VectorEditor *v = app.vector;
    if (!*v->view.path) {
        v->after_save = app.request;
        app.request = REQUEST_NONE;
        editor_field_start(FIELD_V_SAVE, "assets/vector.svg");
        return false;
    }
    char error[256];
    if (!ve_save(&v->document, v->view.path, error, sizeof error)) {
        editor_message(error);
        return false;
    }
    v->history.saved = v->history.ids[v->history.position];
    v->view.exists = 1;
    editor_message("SVG saved");
    return true;
}

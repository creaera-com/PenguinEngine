#include "editors/vector/editor.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool vector_checkpoint(VectorEditor *v, PeSession *io) {
    VeView view = v->view;
    int count = v->history.count, position = v->history.position;
    uint64_t next = v->history.next, saved = v->history.saved;
    if (!pe_session_block(io, &view, sizeof view) || !pe_session_block(io, &count, sizeof count) ||
        !pe_session_block(io, &position, sizeof position) ||
        !pe_session_block(io, &next, sizeof next) || !pe_session_block(io, &saved, sizeof saved)) {
        return false;
    }
    if (count < 1 || count > VE_HISTORY || position < 0 || position >= count || !next ||
        !memchr(view.path, 0, sizeof view.path) || view.board < 0 || view.board >= VE_BOARDS ||
        view.tool < 0 || view.tool > VT_PREVIEW || view.node < -1 || view.node >= VE_NODES ||
        !isfinite(view.x) || !isfinite(view.y) || !isfinite(view.zoom) || view.zoom < .02f ||
        view.zoom > 32) {
        return false;
    }
    if (!isfinite(view.snap_tolerance) || view.snap_tolerance < 0 || view.snap_tolerance > 100) {
        return false;
    }
    if (!isfinite(view.grid_width) || !isfinite(view.grid_height) || view.grid_width < 1 ||
        view.grid_width > 100000 || view.grid_height < 1 || view.grid_height > 100000) {
        return false;
    }
    if (view.preview_enabled &&
        (view.preview_board < 0 || view.preview_board >= VE_BOARDS || !isfinite(view.preview.x) ||
         !isfinite(view.preview.y) || !isfinite(view.preview.w) || !isfinite(view.preview.h) ||
         view.preview.x < 0 || view.preview.y < 0 || view.preview.w < 1 || view.preview.h < 1 ||
         view.preview.x + view.preview.w > 100000 || view.preview.y + view.preview.h > 100000)) {
        return false;
    }
    for (int i = 0; i < VE_BOARDS; i++) {
        if (view.expanded[i] > 1) {
            return false;
        }
    }
    if (io->reading) {
        ve_history_free(&v->history);
        v->history.count = count;
        v->history.position = position;
        v->history.next = next;
        v->history.saved = saved;
    }
    for (int i = 0; i < count; i++) {
        if (!pe_session_block(io, &v->history.ids[i], sizeof(uint64_t)) ||
            v->history.ids[i] >= next ||
            !pe_session_string(io, &v->history.states[i], 4 * 1024 * 1024)) {
            return false;
        }
    }
    if (io->reading) {
        char error[256];
        if (!ve_from_json(&v->document, v->history.states[position], error, sizeof error) ||
            !v->document.boards[view.board].used ||
            (view.preview_enabled && !v->document.boards[view.preview_board].used)) {
            return false;
        }
        v->view = view;
        v->render_dirty = true;
        v->drag_shape = v->drag_node = v->drag_board = -1;
    }
    return true;
}

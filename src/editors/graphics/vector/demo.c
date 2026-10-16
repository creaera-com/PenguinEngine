#include "editors/graphics/vector/internal.h"

void vector_demo(void) {
    VectorEditor *v = app.workspace.vector;
    vector_new();
    snprintf(v->document.boards[0].name, 64, "Game icons");
    v->document.boards[0].w = 448;
    v->document.boards[0].h = 384;
    int second = ve_add_board(&v->document, 488, 0, 224, 384);
    snprintf(v->document.boards[second].name, 64, "Variants");
    v->document.boards[second].background = 0xe7ecefff;
    int a = ve_add_shape(&v->document, 0, VE_RECT, 48, 56, 220, 144);
    v->document.shapes[a].fill = 0x517e95ff;
    v->document.shapes[a].stroke_enabled = true;
    v->document.shapes[a].stroke_width = 3;
    snprintf(v->document.shapes[a].name, 64, "Panel");
    int b = ve_add_shape(&v->document, 0, VE_ELLIPSE, 228, 160, 144, 144);
    v->document.shapes[b].fill = 0xc28e5cff;
    v->document.shapes[b].stroke_enabled = true;
    snprintf(v->document.shapes[b].name, 64, "Orb");
    int c = ve_add_shape(&v->document, 0, VE_PATH, 64, 228, 120, 100);
    VeShape *p = &v->document.shapes[c];
    p->fill = 0x79936dff;
    p->stroke_enabled = true;
    p->count = 5;
    p->nodes[0] = (VeNode){.command = VE_MOVE, .p = {68, 306}};
    p->nodes[1] = (VeNode){.command = VE_CUBIC, .p = {138, 218}, .c1 = {74, 262}, .c2 = {104, 218}};
    p->nodes[2] =
        (VeNode){.command = VE_CUBIC, .p = {202, 306}, .c1 = {176, 218}, .c2 = {196, 264}};
    p->nodes[3] = (VeNode){.command = VE_LINE, .p = {68, 306}};
    p->nodes[4] = (VeNode){.command = VE_CLOSE};
    ve_bounds(p);
    snprintf(p->name, 64, "Bezier leaf");
    int moon = ve_add_shape(&v->document, second, VE_ELLIPSE, 28, 72, 160, 160),
        cut = ve_add_shape(&v->document, second, VE_ELLIPSE, 66, 56, 144, 144);
    char error[256];
    ve_boolean(&v->document, moon, cut, VE_DIFFERENCE, error, sizeof error);
    snprintf(v->document.shapes[moon].name, 64, "Cut contour");
    ve_history_free(&v->history);
    ve_history_init(&v->history, &v->document);
    v->view.grid_visible = true;
    v->view.preview_enabled = true;
    v->view.preview_board = 0;
    v->view.preview = (VeRegion){220, 152, 160, 160};
    v->view.selected = UINT64_C(1) << c;
    v->view.board = 0;
    v->view.tool = VT_NODE;
    v->view.node = 1;
    v->view.expanded_paths[0] = p->id;
    v->view.zoom = .85f;
    v->view.x = -48;
    v->view.y = -44;
    app.shell.right_tabs.selected = ui_id("vector_tools_tab");
    v->render_dirty = true;
    app.shell.selected = p->id;
    editor_message("Vector Art | editable SVG, artboards and Bezier paths");
}

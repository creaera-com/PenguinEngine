#include "editors/graphics/vector/viewport/grid.h"
#include "editors/graphics/vector/internal.h"

void vector_draw_grid_settings(SDL_FRect area) {
    VectorEditor *v = pe_vector_active_instance();
    float x = area.x + 6, y = area.y + 4, w = fmaxf(0, area.w - 12);
    const char *labels[] = {"Width px", "Height px", "RGBA", "Snap px"};
    Field fields[] = {FIELD_V_GRID_WIDTH, FIELD_V_GRID_HEIGHT, FIELD_V_GRID_COLOR,
                      FIELD_V_SNAP_TOLERANCE};
    for (int i = 0; i < 4; i++) {
        char text[32];
        if (i == 2) {
            snprintf(text, sizeof text, "%08X", v->view.grid_color);
        } else {
            snprintf(text, sizeof text, "%.2f",
                     i == 3 ? v->view.snap_tolerance
                            : (i ? v->view.grid_height : v->view.grid_width));
        }
        UiPropertyRow row = ui_property_row((UiRect){x, y, w, 25}, .42f, 8);
        editor_text(row.label.x, row.label.y + 10, labels[i], UI_MUTED);
        editor_draw_input(fields[i], text, text, row.value, true);
        y += 29;
    }
}

void vector_draw_grid(void) {
    VectorEditor *v = pe_vector_active_instance();
    if (!v->view.grid_visible) {
        return;
    }
    VeGridLine lines[2048];
    SDL_BlendMode previous = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(app.platform.renderer, &previous);
    SDL_SetRenderDrawBlendMode(app.platform.renderer, SDL_BLENDMODE_BLEND);
    editor_draw_color(v->view.grid_color);
    for (int i = 0; i < VE_BOARDS; i++) {
        VeBoard *b = &v->document.boards[i];
        if (!b->used) {
            continue;
        }
        float left = fmaxf(0, v->view.x - b->x);
        float top = fmaxf(0, v->view.y - b->y);
        float right = fminf(b->w, v->view.x + v->canvas.w / v->view.zoom - b->x);
        float bottom = fminf(b->h, v->view.y + v->canvas.h / v->view.zoom - b->y);
        VeRegion visible = {left, top, right - left, bottom - top};
        int count = ve_grid_lines(visible, v->view.grid_width, v->view.grid_height,
                                  v->view.grid_isometric, v->view.zoom, lines, 2048);
        for (int n = 0; n < count; n++) {
            SDL_RenderLine(app.platform.renderer,
                           v->canvas.x + (b->x + lines[n].a.x - v->view.x) * v->view.zoom,
                           v->canvas.y + (b->y + lines[n].a.y - v->view.y) * v->view.zoom,
                           v->canvas.x + (b->x + lines[n].b.x - v->view.x) * v->view.zoom,
                           v->canvas.y + (b->y + lines[n].b.y - v->view.y) * v->view.zoom);
        }
    }
    SDL_SetRenderDrawBlendMode(app.platform.renderer, previous);
}

/** @file @brief Color scheme controls, wheel interaction and clipboard export. */
#include "palette.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int colors(VectorPaletteUI *s, uint32_t out[5]) {
    return vector_palette_generate(s->base, s->harmony, s->spread, out);
}

static float radius(PUIRect a) {
    return fmaxf(1, fminf(a.w - 24, a.h - 78) * .5f);
}

static void sync(VectorPaletteUI *s, PUIView *v) {
    pui_view_set_color(v, "palette_base", s->base);
}

bool vector_palette_ui_action(VectorPaletteUI *s, PUIView *v, const char *action) {
    if (strncmp(action, "vector.palette.", 15)) {
        return false;
    }
    if (!strcmp(action, "vector.palette.base")) {
        pui_view_color(v, "palette_base", &s->base);
    } else if (!strcmp(action, "vector.palette.harmony")) {
        const char *value = pui_view_value(v, "palette_harmony");
        s->harmony = value ? (VectorHarmony)atoi(value) : VECTOR_HARMONY_MONO;
    } else if (!strcmp(action, "vector.palette.spread")) {
        const char *value = pui_view_text(v, "palette_spread");
        float spread = value ? strtof(value, NULL) : 30;
        s->spread = isfinite(spread) ? fmaxf(5, fminf(90, spread)) : 30;
        char text[32];
        snprintf(text, sizeof text, "%.0f", (double)s->spread);
        pui_view_set_text(v, "palette_spread", text);
    } else if (!strcmp(action, "vector.palette.copy")) {
        uint32_t palette[5];
        int count = colors(s, palette);
        char text[64] = {0};
        for (int i = 0; i < count; i++) {
            snprintf(text + strlen(text), sizeof text - strlen(text), "%s#%08X", i ? " " : "",
                     palette[i]);
        }
        SDL_SetClipboardText(text);
        pui_view_set_text(v, "status", "Palette als Hex-Werte kopiert");
    }
    return true;
}

void vector_palette_ui_draw(VectorPaletteUI *s, SDL_Renderer *r, PUIRect a) {
    s->area = a;
    float rad = radius(a), cx = a.x + a.w / 2, cy = a.y + rad + 8;
    PUIHSLColor base = pui_color_to_hsl(s->base);
    SDL_Vertex vertices[3];
    for (int degree = 0; degree < 360; degree++) {
        for (int k = 0; k < 3; k++) {
            float angle = (degree + (k == 2)) * .017453293f;
            PUIHSLColor sample = {
                .h = (float)(degree + (k == 2)), .s = k ? 1 : 0, .l = base.l, .a = 1};
            uint32_t c = pui_color_from_hsl(sample);
            vertices[k] = (SDL_Vertex){
                .position = {cx + (k ? cosf(angle) * rad : 0), cy + (k ? sinf(angle) * rad : 0)},
                .color = {(c >> 24) / 255.f, ((c >> 16) & 255) / 255.f, ((c >> 8) & 255) / 255.f,
                          1}};
        }
        SDL_RenderGeometry(r, NULL, vertices, 3, NULL, 0);
    }
    uint32_t palette[5];
    int count = colors(s, palette);
    for (int i = 0; i < count; i++) {
        PUIHSLColor c = pui_color_to_hsl(palette[i]);
        float angle = c.h * .017453293f;
        float x = cx + cosf(angle) * rad * c.s, y = cy + sinf(angle) * rad * c.s;
        SDL_SetRenderDrawColor(r, 20, 20, 20, 255);
        SDL_RenderFillRect(r, &(SDL_FRect){x - 5, y - 5, 10, 10});
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderRect(r, &(SDL_FRect){x - 4, y - 4, 8, 8});
        float w = (a.w - 8) / count, sx = a.x + 4 + i * w, sy = a.y + a.h - 58;
        SDL_SetRenderDrawColor(r, palette[i] >> 24, (palette[i] >> 16) & 255,
                               (palette[i] >> 8) & 255, 255);
        SDL_RenderFillRect(r, &(SDL_FRect){sx, sy, w - 4, 30});
        char text[10];
        snprintf(text, sizeof text, "#%06X", palette[i] >> 8);
        pui_text_scaled(r, sx, sy + 36, text, 0xccccccff, .85f);
    }
}

bool vector_palette_ui_event(VectorPaletteUI *s, PUIView *v, const SDL_Event *e) {
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (e->type == SDL_EVENT_KEY_DOWN && e->key.key == SDLK_ESCAPE)) {
        s->dragging = false;
        return false;
    }
    bool down = e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT;
    bool up = e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT;
    bool motion = e->type == SDL_EVENT_MOUSE_MOTION;
    if (!down && !up && !motion) {
        return false;
    }
    float x = motion ? e->motion.x : e->button.x, y = motion ? e->motion.y : e->button.y;
    if (!pui_view_hit(v, "palette_wheel", x, y) && !s->dragging) {
        return false;
    }
    float rad = radius(s->area), dx = x - s->area.x - s->area.w / 2, dy = y - s->area.y - rad - 8;
    float distance = hypotf(dx, dy);
    if (down && y >= s->area.y + s->area.h - 58) {
        uint32_t palette[5];
        int count = colors(s, palette);
        int i = (int)((x - s->area.x - 4) / ((s->area.w - 8) / count));
        if (i >= 0 && i < count) {
            char text[12];
            snprintf(text, sizeof text, "#%08X", palette[i]);
            SDL_SetClipboardText(text);
            pui_view_set_text(v, "status", "Farbe als Hex-Wert kopiert");
        }
        return true;
    }
    if (down && distance <= rad) {
        s->dragging = true;
    }
    if (s->dragging && (motion || down || up)) {
        PUIHSLColor c = pui_color_to_hsl(s->base);
        c.h = fmodf(atan2f(dy, dx) * 57.29578f + 360, 360);
        c.s = fminf(1, distance / rad);
        s->base = pui_color_from_hsl(c);
        sync(s, v);
        if (up) {
            s->dragging = false;
        }
        return true;
    }
    return false;
}

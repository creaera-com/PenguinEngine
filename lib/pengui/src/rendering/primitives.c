/** @file @brief Basic surface drawing and render clipping. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

void pui_render_color(SDL_Renderer *r, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}

void pui_panel(SDL_Renderer *r, SDL_FRect rect, Uint32 c) {
    pui_render_color(r, c);
    SDL_RenderFillRect(r, &rect);
}

void pui_apply_clip(SDL_Renderer *r, const PUIContext *c) {
    if (c->clip_depth) {
        PUIRect p = c->clips[c->clip_depth - 1];
        SDL_Rect clip = {(int)p.x, (int)p.y, (int)p.w, (int)p.h};
        SDL_SetRenderClipRect(r, &clip);
    } else {
        SDL_SetRenderClipRect(r, NULL);
    }
}

void pui_rounded_panel(SDL_Renderer *r, SDL_FRect rect, Uint32 color, float radius,
                       bool upper_only) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    radius = fmaxf(0, fminf(radius, fminf(rect.w, rect.h) / 2));
    if (radius <= 0) {
        pui_panel(r, rect, color);
        return;
    }
    SDL_Vertex vertices[38];
    int indices[108];
    SDL_FColor tint = {(color >> 24) / 255.f, ((color >> 16) & 255) / 255.f,
                       ((color >> 8) & 255) / 255.f, (color & 255) / 255.f};
    vertices[0] =
        (SDL_Vertex){.position = {rect.x + rect.w / 2, rect.y + rect.h / 2}, .color = tint};
    int count = 0;
    for (int corner = 0; corner < 4; corner++) {
        float rad = upper_only && corner < 2 ? 0 : radius;
        float cx = rect.x + (corner == 0 || corner == 3 ? rect.w - rad : rad);
        float cy = rect.y + (corner < 2 ? rect.h - rad : rad);
        for (int step = 0; step <= 8; step++) {
            float angle = (corner * 90 + step * 90.f / 8) * .017453293f;
            vertices[++count] = (SDL_Vertex){
                .position = {cx + cosf(angle) * rad, cy + sinf(angle) * rad}, .color = tint};
        }
    }
    vertices[count + 1] = vertices[1];
    for (int i = 0; i < count; i++) {
        indices[3 * i] = 0;
        indices[3 * i + 1] = i + 1;
        indices[3 * i + 2] = i + 2;
    }
    SDL_RenderGeometry(r, NULL, vertices, count + 2, indices, count * 3);
}

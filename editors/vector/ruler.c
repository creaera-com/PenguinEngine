/** @file @brief Artboard rulers; ticks use document pixels, thickness uses screen pixels. */
#include "ruler.h"
#include <math.h>
#include <stdio.h>

static const float ruler_scale = .7f;
static const float top_height = 16 * .7f, left_width = 24 * .7f;
static const float text_scale = .625f * .7f;

static float major_step(float zoom) {
    float target = 48 / zoom;
    float magnitude = powf(10, floorf(log10f(target)));
    float unit = target / magnitude;
    return (unit <= 1 ? 1 : unit <= 2 ? 2 : unit <= 5 ? 5 : 10) * magnitude;
}

static void draw_axis(SDL_Renderer *renderer, SDL_FRect board, float zoom, bool vertical,
                      SDL_Rect outer) {
    SDL_FRect strip = vertical ? (SDL_FRect){board.x - left_width, board.y, left_width, board.h}
                               : (SDL_FRect){board.x, board.y - top_height, board.w, top_height};
    SDL_Rect requested = {(int)floorf(strip.x), (int)floorf(strip.y), (int)ceilf(strip.w + 1),
                          (int)ceilf(strip.h + 1)},
             clip;
    if (!SDL_GetRectIntersection(&outer, &requested, &clip)) {
        return;
    }
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 83, 83, 83, 255);
    SDL_RenderFillRect(renderer, &strip);

    float origin = vertical ? board.y : board.x;
    float end = origin + (vertical ? board.h : board.w);
    float visible_start = vertical ? clip.y : clip.x;
    float visible_end = vertical ? clip.y + clip.h : clip.x + clip.w;
    float major = major_step(zoom), minor = major / 5;
    int first = (int)fmaxf(0, ceilf((visible_start - origin) / (minor * zoom)));
    int last = (int)floorf((fminf(end, visible_end) - origin) / (minor * zoom));
    for (int index = first; index <= last; index++) {
        bool labeled = index % 5 == 0;
        float position = roundf(origin + index * minor * zoom);
        float length = (labeled ? 5 : 3) * ruler_scale;
        SDL_SetRenderDrawColor(renderer, 151, 151, 151, 255);
        if (vertical) {
            SDL_RenderLine(renderer, board.x - length, position, board.x - 1, position);
        } else {
            SDL_RenderLine(renderer, position, board.y - length, position, board.y - 1);
        }
        if (labeled) {
            char label[32];
            snprintf(label, sizeof label, "%.0f", index * minor);
            float x = vertical ? strip.x + 2 * ruler_scale : position + 2 * ruler_scale;
            float y = vertical ? position + 2 * ruler_scale : strip.y + ruler_scale;
            if (x + pui_text_width(label) * text_scale <= strip.x + strip.w - 1 &&
                y + PUI_FONT_HEIGHT * text_scale <= strip.y + strip.h) {
                pui_text_scaled(renderer, x, y, label, 0xCDCDCDFF, text_scale);
            }
        }
    }
    SDL_SetRenderClipRect(renderer, &outer);
}

void vector_ruler_draw(SDL_Renderer *renderer, SDL_FRect board, float zoom) {
    bool clipped = SDL_RenderClipEnabled(renderer);
    SDL_Rect outer;
    if (clipped) {
        SDL_GetRenderClipRect(renderer, &outer);
    } else {
        int width, height;
        SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
        outer = (SDL_Rect){0, 0, width, height};
    }
    draw_axis(renderer, board, zoom, false, outer);
    draw_axis(renderer, board, zoom, true, outer);
    SDL_FRect corner = {board.x - left_width, board.y - top_height, left_width, top_height};
    SDL_SetRenderDrawColor(renderer, 83, 83, 83, 255);
    SDL_RenderFillRect(renderer, &corner);
    pui_text_scaled(renderer, corner.x + 5 * ruler_scale, corner.y + 2 * ruler_scale, "px",
                    0xCDCDCDFF, text_scale);
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

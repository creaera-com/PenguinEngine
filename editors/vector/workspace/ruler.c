/** @file @brief Artboard rulers; ticks use document pixels, thickness uses screen pixels. */
#include "ruler.h"
#include <math.h>
#include <stdio.h>

static const float ruler_scale = .7f;
static const float ruler_thickness = 16 * .7f;
static const float text_scale = .625f * .7f;

static float major_step(float zoom) {
    float target = 48 / zoom;
    float magnitude = powf(10, floorf(log10f(target)));
    float unit = target / magnitude;
    return (unit <= 1 ? 1 : unit <= 2 ? 2 : unit <= 5 ? 5 : 10) * magnitude;
}

static void draw_axis(SDL_Renderer *renderer, SDL_FRect board, float zoom, bool vertical,
                      SDL_Rect outer) {
    SDL_FRect strip =
        vertical ? (SDL_FRect){board.x - ruler_thickness, board.y, ruler_thickness, board.h}
                 : (SDL_FRect){board.x, board.y - ruler_thickness, board.w, ruler_thickness};
    SDL_Rect requested = {(int)floorf(strip.x), (int)floorf(strip.y), (int)ceilf(strip.w + 1),
                          (int)ceilf(strip.h + 1)},
             clip;
    if (!SDL_GetRectIntersection(&outer, &requested, &clip)) {
        return;
    }
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
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
        SDL_SetRenderDrawColor(renderer, 85, 85, 85, 255);
        if (vertical) {
            SDL_RenderLine(renderer, board.x - length, position, board.x - 1, position);
        } else {
            SDL_RenderLine(renderer, position, board.y - length, position, board.y - 1);
        }
        if (labeled) {
            char label[32];
            snprintf(label, sizeof label, "%.0f", index * minor);
            float padding = vertical ? .5f * ruler_scale : 2 * ruler_scale;
            float x = vertical ? strip.x + padding : position + padding;
            float label_scale =
                vertical ? fminf(text_scale, (strip.w - 2 * padding) / pui_text_width(label))
                         : text_scale;
            float y = vertical ? position + 2 * ruler_scale : strip.y + ruler_scale;
            if (x + pui_text_width(label) * label_scale <=
                    strip.x + strip.w - (vertical ? padding : 1) &&
                y + PUI_FONT_HEIGHT * label_scale <= strip.y + strip.h) {
                pui_text_scaled(renderer, x, y, label, 0x252525FF, label_scale);
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
    SDL_FRect corner = {board.x - ruler_thickness, board.y - ruler_thickness, ruler_thickness,
                        ruler_thickness};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &corner);
    pui_text_scaled(renderer, corner.x + 5 * ruler_scale, corner.y + 2 * ruler_scale, "px",
                    0x252525FF, text_scale);
    SDL_SetRenderClipRect(renderer, clipped ? &outer : NULL);
}

void vector_ruler_draw_pointer(SDL_Renderer *renderer, SDL_FRect board, float x, float y) {
    if (!isfinite(x) || !isfinite(y) || x < board.x || y < board.y || x >= board.x + board.w ||
        y >= board.y + board.h) {
        return;
    }
    /* Board and pointer already use screen coordinates: pan and zoom need no second conversion. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    float column = floorf(x), row = floorf(y);
    SDL_FRect top = {column, board.y - ruler_thickness, 1, ruler_thickness};
    SDL_FRect left = {board.x - ruler_thickness, row, ruler_thickness, 1};
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &left);
}

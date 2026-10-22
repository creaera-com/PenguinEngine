/** @file @brief Rasterize object alpha, dilate, soften, tint and composite behind its content. */
#include "pengdraw/pengdraw.h"
#include <math.h>
#include <stdlib.h>

/* Sliding box averages approximate a Gaussian in three passes. Transparent borders are retained,
 * never clamped to the edge alpha. Dilation uses a monotone deque, also linear in image size. */
static void filter(const unsigned char *src, unsigned char *dst, int w, int h, int radius,
                   bool vertical, bool maximum, int *queue) {
    int lines = vertical ? w : h, length = vertical ? h : w;
    int step = vertical ? w : 1;
    for (int line = 0; line < lines; line++) {
        int base = vertical ? line : line * w;
        int sum = 0, head = 0, tail = 0;
        for (int i = -radius; i < length + radius; i++) {
            if (i >= 0 && i < length) {
                unsigned char v = src[base + i * step];
                sum += v;
                if (maximum) {
                    while (tail > head && src[base + queue[tail - 1] * step] <= v) {
                        tail--;
                    }
                    queue[tail++] = i;
                }
            }
            int old = i - 2 * radius - 1;
            if (old >= 0 && old < length) {
                sum -= src[base + old * step];
            }
            while (tail > head && queue[head] <= old) {
                head++;
            }
            int out = i - radius;
            if (out >= 0 && out < length) {
                dst[base + out * step] = maximum
                                             ? (tail > head ? src[base + queue[head] * step] : 0)
                                             : (unsigned char)(sum / (2 * radius + 1));
            }
        }
    }
}

bool pdraw_shadow_draw(SDL_Renderer *r, const PDrawShadow *s, float zoom, PDrawRect b,
                       PDrawShadowPaint paint, void *context) {
    if (!s->enabled || !(s->color & 255u) || !pdraw_shadow_valid(s) || !paint || !isfinite(zoom) ||
        zoom <= 0) {
        return false;
    }
    int ow, oh;
    if (!SDL_GetCurrentRenderOutputSize(r, &ow, &oh)) {
        return false;
    }
    SDL_Rect clip;
    bool clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderClipRect(r, &clip);
    if (!clipped) {
        clip = (SDL_Rect){0, 0, ow, oh};
    }
    float ox = s->offset_x * zoom, oy = s->offset_y * zoom;
    /* Limit raster resolution, not effect size: distant zoomed shapes use a coarser silhouette. */
    float scale = fminf(1, 2048.f / fmaxf(ow + 2 * (s->blur + s->spread) * zoom,
                                          oh + 2 * (s->blur + s->spread) * zoom));
    int radius = (int)ceilf(s->blur * zoom * scale / 3);
    int spread = (int)ceilf(s->spread * zoom * scale);
    float margin = (3 * radius + spread + 2) / scale;
    float left = fmaxf(b.x - margin, clip.x - ox - margin);
    float top = fmaxf(b.y - margin, clip.y - oy - margin);
    float right = fminf(b.x + b.w + margin, clip.x + clip.w - ox + margin);
    float bottom = fminf(b.y + b.h + margin, clip.y + clip.h - oy + margin);
    if (right <= left || bottom <= top) {
        return true;
    }
    left = floorf(left);
    top = floorf(top);
    int w = (int)ceilf((right - left) * scale), h = (int)ceilf((bottom - top) * scale);
    if (w < 1 || h < 1 || w > 4096 || h > 4096) {
        return false;
    }
    /* Paint in original pixel coordinates; SDL renderer scale applies the sampling reduction. */
    SDL_Texture *previous = SDL_GetRenderTarget(r);
    SDL_Texture *target =
        SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_Surface *surface = NULL, *rgba = NULL;
    unsigned char *alpha = NULL, *tmp = NULL;
    int *queue = NULL;
    Uint8 cr, cg, cb, ca;
    SDL_BlendMode blend;
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    SDL_GetRenderDrawColor(r, &cr, &cg, &cb, &ca);
    SDL_GetRenderDrawBlendMode(r, &blend);
    bool ok = target && SDL_SetRenderTarget(r, target);
    if (ok) {
        SDL_SetRenderClipRect(r, NULL);
        SDL_SetRenderScale(r, scale, scale);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
        paint(context, -left, -top);
        surface = SDL_RenderReadPixels(r, NULL);
        rgba = surface ? SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32) : NULL;
        ok = rgba != NULL;
    }
    SDL_SetRenderTarget(r, previous);
    SDL_SetRenderScale(r, sx, sy);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_SetRenderDrawBlendMode(r, blend);
    size_t count = (size_t)w * h;
    if (ok) {
        alpha = malloc(count);
        tmp = malloc(count);
        queue = malloc((size_t)(w > h ? w : h) * sizeof *queue);
        ok = alpha && tmp && queue;
    }
    if (ok) {
        for (int y = 0; y < h; y++) {
            const unsigned char *pixels = (unsigned char *)rgba->pixels + y * rgba->pitch;
            for (int x = 0; x < w; x++) {
                alpha[(size_t)y * w + x] = pixels[x * 4 + 3];
            }
        }
        if (spread) {
            filter(alpha, tmp, w, h, spread, false, true, queue);
            filter(tmp, alpha, w, h, spread, true, true, queue);
        }
        if (radius) {
            for (int pass = 0; pass < 3; pass++) {
                filter(alpha, tmp, w, h, radius, false, false, queue);
                filter(tmp, alpha, w, h, radius, true, false, queue);
            }
        }
        for (int y = 0; y < h; y++) {
            unsigned char *pixels = (unsigned char *)rgba->pixels + y * rgba->pitch;
            for (int x = 0; x < w; x++) {
                pixels[x * 4] = s->color >> 24;
                pixels[x * 4 + 1] = s->color >> 16;
                pixels[x * 4 + 2] = s->color >> 8;
                pixels[x * 4 + 3] =
                    (unsigned char)(alpha[(size_t)y * w + x] * (s->color & 255) / 255);
            }
        }
        SDL_Texture *shadow = SDL_CreateTextureFromSurface(r, rgba);
        if (shadow) {
            SDL_SetTextureBlendMode(shadow, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(shadow, SDL_SCALEMODE_LINEAR);
            SDL_FRect dest = {left + ox, top + oy, w / scale, h / scale};
            ok = SDL_RenderTexture(r, shadow, NULL, &dest);
            SDL_DestroyTexture(shadow);
        } else {
            ok = false;
        }
    }
    free(queue);
    free(tmp);
    free(alpha);
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(target);
    return ok;
}

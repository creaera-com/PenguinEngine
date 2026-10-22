/** @file @brief Export the committed model at one document pixel per output pixel. */
#include "render.h"
#include "editor_host/editor.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/rendering/artwork.h"
#include "editors/vector/rendering/objects.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool selected_content(const VectorDocument *d, unsigned group) {
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *s = &d->rectangles[i];
        if (group && s->group_id == group && !s->clip_mask &&
            vector_document_is_selected(d, s->id)) {
            return true;
        }
    }
    return false;
}

static void prepare(VectorDocument *copy, const VectorDocument *d, bool selection) {
    *copy = *d;
    copy->history = (VectorHistory){0};
    copy->viewport.zoom = 1;
    copy->selection = (VectorRectangleSelection){0};
    copy->active_rectangle = 0;
    copy->selection_tool = (VectorSelectionTool){0};
    copy->rectangle_tool = (VectorRectangleTool){0};
    copy->artboard_tool = (VectorArtboardTool){0};
    copy->curve_tool = (VectorCurveTool){0};
    copy->text_tool = (VectorTextTool){0};
    copy->gradient_tool = (VectorGradientTool){0};
    copy->fill_preview = (VectorFillPreview){0};
    if (!selection) {
        return;
    }
    copy->rectangle_count = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape s = d->rectangles[i];
        bool content = selected_content(d, s.group_id);
        if (!vector_document_is_selected(d, s.id) && !(s.clip_mask && content)) {
            continue;
        }
        if (s.clip_mask && !content) {
            s.clip_mask = 0;
            s.group_id = 0;
        }
        copy->rectangles[copy->rectangle_count++] = s;
    }
}

SDL_Surface *vector_export_render(const VectorDocument *d, const VectorExportPlan *p, unsigned item,
                                  SDL_Renderer *r, VectorTextRenderer *text,
                                  VectorImageRenderer *images, bool transparent) {
    if (!d || !p || p->count > VECTOR_MAX_ARTBOARDS || item >= p->count) {
        return NULL;
    }
    PUIRect area = p->items[item].bounds;
    if (area.w < 1 || area.h < 1 || area.w > 8192 || area.h > 8192 ||
        (double)area.w * area.h > 16777216) {
        return NULL;
    }
    VectorDocument *copy = malloc(sizeof *copy);
    if (!copy) {
        return NULL;
    }
    prepare(copy, d, p->scope == VECTOR_EXPORT_SELECTION);
    SDL_Texture *target = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                            (int)area.w, (int)area.h);
    SDL_Texture *previous = SDL_GetRenderTarget(r);
    SDL_Rect clip, viewport;
    bool clipped = SDL_RenderClipEnabled(r), view_set = SDL_RenderViewportSet(r);
    SDL_GetRenderClipRect(r, &clip);
    SDL_GetRenderViewport(r, &viewport);
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    Uint8 red, green, blue, alpha;
    SDL_GetRenderDrawColor(r, &red, &green, &blue, &alpha);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    SDL_Surface *pixels = NULL, *rgba = NULL;
    VectorArtworkRenderer artwork = {0};
    if (target && SDL_SetRenderTarget(r, target)) {
        SDL_SetRenderScale(r, 1, 1);
        SDL_SetRenderViewport(r, NULL);
        SDL_SetRenderClipRect(r, NULL);
        SDL_SetRenderDrawColor(r, transparent ? 0 : 255, transparent ? 0 : 255,
                               transparent ? 0 : 255, transparent ? 0 : 255);
        SDL_RenderClear(r);
        for (int i = 0; i < copy->artboard_count; i++) {
            VectorArtboard *b = &copy->artboards[i];
            if (p->scope != VECTOR_EXPORT_SELECTION && b->id != p->items[item].board) {
                continue;
            }
            SDL_FRect board = {b->rect.x - area.x, b->rect.y - area.y, b->rect.w, b->rect.h};
            vector_artwork_draw(&artwork, copy, b, r, board, text, images);
        }
        pixels = SDL_RenderReadPixels(r, NULL);
        rgba = pixels ? SDL_ConvertSurface(pixels, SDL_PIXELFORMAT_RGBA32) : NULL;
    }
    SDL_SetRenderTarget(r, previous);
    SDL_SetRenderScale(r, sx, sy);
    SDL_SetRenderViewport(r, view_set ? &viewport : NULL);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_SetRenderDrawBlendMode(r, blend);
    SDL_DestroySurface(pixels);
    vector_artwork_renderer_destroy(&artwork);
    SDL_DestroyTexture(target);
    free(copy);
    if (rgba && transparent) {
        /* SDL target accumulation is premultiplied; PNG stores straight alpha. */
        for (int y = 0; y < rgba->h; y++) {
            unsigned char *row = (unsigned char *)rgba->pixels + y * rgba->pitch;
            for (int x = 0; x < rgba->w; x++) {
                unsigned a = row[x * 4 + 3];
                for (int c = 0; c < 3; c++) {
                    unsigned v = a ? (row[x * 4 + c] * 255u + a / 2) / a : 0;
                    row[x * 4 + c] = (unsigned char)(v > 255 ? 255 : v);
                }
            }
        }
    }
    return rgba;
}

static void safe_name(const char *s, char *out, size_t capacity) {
    size_t n = 0;
    for (; *s && n + 1 < capacity; s++) {
        unsigned char c = (unsigned char)*s;
        out[n++] = (c < 32 || strchr("/\\:*?\"<>|", c)) ? '_' : (char)c;
    }
    while (n && (out[n - 1] == ' ' || out[n - 1] == '.')) {
        n--;
    }
    out[n] = 0;
}

bool vector_export_write(const VectorDocument *d, const VectorExportPlan *p, SDL_Renderer *r,
                         VectorTextRenderer *text, VectorImageRenderer *images,
                         const char *directory, const char *stem, bool jpeg, bool transparent,
                         int quality, char *error, size_t capacity) {
    SDL_PathInfo info;
    if (!d || !p || !p->count || p->count > VECTOR_MAX_ARTBOARDS) {
        snprintf(error, capacity, "Kein Exportbereich");
        return false;
    }
    if (!directory || !SDL_GetPathInfo(directory, &info) || info.type != SDL_PATHTYPE_DIRECTORY ||
        !stem || !*stem || quality < 1 || quality > 100) {
        snprintf(error, capacity, "Exportordner, Dateiname oder JPEG-Qualitaet ungueltig");
        return false;
    }
    char base[96];
    safe_name(stem, base, sizeof base);
    char *suffix = strrchr(base, '.');
    if (suffix && (!SDL_strcasecmp(suffix, ".png") || !SDL_strcasecmp(suffix, ".jpg") ||
                   !SDL_strcasecmp(suffix, ".jpeg"))) {
        *suffix = 0;
    }
    if (!*base) {
        snprintf(error, capacity, "Dateinamen eingeben");
        return false;
    }
    unsigned done = 0;
    for (; done < p->count; done++) {
        char name[256], path[4096], staged[4096];
        const char *extension = jpeg ? "jpg" : "png";
        if (p->scope == VECTOR_EXPORT_ALL) {
            const VectorArtboard *board = NULL;
            for (int i = 0; i < d->artboard_count; i++) {
                if (d->artboards[i].id == p->items[done].board) {
                    board = &d->artboards[i];
                }
            }
            char label[80];
            safe_name(board ? board->name : "Artboard", label, sizeof label);
            snprintf(name, sizeof name, "%s-%u-%s.%s", base, p->items[done].board, label,
                     extension);
        } else {
            snprintf(name, sizeof name, "%s.%s", base, extension);
        }
        if (snprintf(path, sizeof path, "%s/%s", directory, name) >= (int)sizeof path ||
            snprintf(staged, sizeof staged, "%s.tmp-%llu", path,
                     (unsigned long long)SDL_GetPerformanceCounter()) >= (int)sizeof staged) {
            break;
        }
        SDL_Surface *surface =
            vector_export_render(d, p, done, r, text, images, transparent && !jpeg);
        bool ok = surface && editor_image_write(surface, staged, jpeg, quality);
        SDL_DestroySurface(surface);
        if (ok) {
            ok = SDL_RenamePath(staged, path);
        }
        if (!ok) {
            SDL_RemovePath(staged);
            break;
        }
    }
    if (done != p->count) {
        snprintf(error, capacity, "Export fehlgeschlagen (%u/%u Dateien geschrieben): %.120s", done,
                 p->count, SDL_GetError());
        return false;
    }
    snprintf(error, capacity, "%u Datei(en) exportiert", done);
    return true;
}

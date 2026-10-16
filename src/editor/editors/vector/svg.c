#include "editors/vector/document.h"
#include "nanosvg.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    char *text;
    size_t length, capacity;
    bool failed;
} Buffer;

static void append(Buffer *b, const char *format, ...) {
    if (b->failed) {
        return;
    }
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int n = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (n < 0 || b->length + (size_t)n > 16 * 1024 * 1024) {
        b->failed = true;
        va_end(args);
        return;
    }
    size_t needed = b->length + (size_t)n + 1;
    if (needed > b->capacity) {
        size_t cap = needed * 2;
        char *text = realloc(b->text, cap);
        if (!text) {
            b->failed = true;
            va_end(args);
            return;
        }
        b->text = text;
        b->capacity = cap;
    }
    vsnprintf(b->text + b->length, b->capacity - b->length, format, args);
    b->length += (size_t)n;
    va_end(args);
}

static void escaped(Buffer *b, const char *text) {
    for (; *text; text++) {
        if (*text == '&') {
            append(b, "&amp;");
        } else if (*text == '<') {
            append(b, "&lt;");
        } else if (*text == '>') {
            append(b, "&gt;");
        } else if (*text == '\"') {
            append(b, "&quot;");
        } else {
            append(b, "%c", *text);
        }
    }
}

static void paint(Buffer *b, const char *key, uint32_t rgba, bool enabled) {
    if (!enabled) {
        append(b, " %s=\"none\"", key);
        return;
    }
    append(b, " %s=\"#%06X\" %s-opacity=\"%.9g\"", key, rgba >> 8, key, (rgba & 255) / 255.0);
}

char *ve_svg(const VeDocument *d, bool metadata, float *ox, float *oy) {
    if (!ve_valid(d)) {
        return NULL;
    }
    Buffer b = {0};
    float x = INFINITY, y = INFINITY, right = -INFINITY, bottom = -INFINITY;
    for (int i = 0; i < VE_BOARDS; i++) {
        const VeBoard *a = &d->boards[i];
        if (a->used) {
            x = fminf(x, a->x);
            y = fminf(y, a->y);
            right = fmaxf(right, a->x + a->w);
            bottom = fmaxf(bottom, a->y + a->h);
        }
    }
    if (ox) {
        *ox = x;
    }
    if (oy) {
        *oy = y;
    }
    append(&b,
           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" "
           "width=\"%.9g\" height=\"%.9g\" viewBox=\"%.9g %.9g %.9g %.9g\">\n",
           right - x, bottom - y, x, y, right - x, bottom - y);
    if (metadata) {
        char *json = ve_json(d);
        if (!json) {
            free(b.text);
            return NULL;
        }
        append(&b, "<metadata id=\"penguin-vector\">");
        escaped(&b, json);
        append(&b, "</metadata>\n");
        free(json);
    }
    for (int i = 0; i < VE_BOARDS; i++) {
        const VeBoard *a = &d->boards[i];
        if (!a->used) {
            continue;
        }
        append(&b,
               "<g id=\"artboard-%d\" data-penguin-artboard=\"true\" transform=\"translate(%.9g "
               "%.9g)\">\n<title>",
               i, a->x, a->y);
        escaped(&b, a->name);
        append(&b, "</title>\n<rect width=\"%.9g\" height=\"%.9g\"", a->w, a->h);
        paint(&b, "fill", a->background, true);
        append(&b, "/>\n");
        for (int k = 0; k < VE_SHAPES; k++) {
            const VeShape *s = &d->shapes[k];
            if (!s->used || s->board != i) {
                continue;
            }
            if (s->kind == VE_RECT) {
                append(&b, "<rect x=\"%.9g\" y=\"%.9g\" width=\"%.9g\" height=\"%.9g\"", s->x, s->y,
                       s->w, s->h);
            } else if (s->kind == VE_ELLIPSE) {
                append(&b, "<ellipse cx=\"%.9g\" cy=\"%.9g\" rx=\"%.9g\" ry=\"%.9g\"",
                       s->x + s->w * .5f, s->y + s->h * .5f, s->w * .5f, s->h * .5f);
            } else {
                append(&b, "<path fill-rule=\"%s\" d=\"", s->evenodd ? "evenodd" : "nonzero");
                for (int n = 0; n < s->count; n++) {
                    const VeNode *p = &s->nodes[n];
                    if (p->command == VE_MOVE) {
                        append(&b, "M %.9g %.9g ", p->p.x, p->p.y);
                    } else if (p->command == VE_LINE) {
                        append(&b, "L %.9g %.9g ", p->p.x, p->p.y);
                    } else if (p->command == VE_CUBIC) {
                        append(&b, "C %.9g %.9g %.9g %.9g %.9g %.9g ", p->c1.x, p->c1.y, p->c2.x,
                               p->c2.y, p->p.x, p->p.y);
                    } else {
                        append(&b, "Z ");
                    }
                }
                append(&b, "\"");
            }
            append(&b, " id=\"shape-%u\"", s->id);
            paint(&b, "fill", s->fill, s->fill_enabled);
            paint(&b, "stroke", s->stroke, s->stroke_enabled);
            append(&b, " stroke-width=\"%.9g\"><title>", s->stroke_width);
            escaped(&b, s->name);
            append(&b, "</title></%s>\n",
                   s->kind == VE_RECT      ? "rect"
                   : s->kind == VE_ELLIPSE ? "ellipse"
                                           : "path");
        }
        append(&b, "</g>\n");
    }
    append(&b, "</svg>\n");
    if (b.failed) {
        free(b.text);
        return NULL;
    }
    return b.text;
}

static char *unescape(const char *p, size_t length) {
    char *out = malloc(length + 1);
    if (!out) {
        return NULL;
    }
    size_t n = 0;
    for (size_t i = 0; i < length; i++) {
        if (p[i] == '&') {
            const char *tokens[] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;"};
            const char values[] = "&<>\"'";
            bool found = false;
            for (int k = 0; k < 5; k++) {
                size_t l = strlen(tokens[k]);
                if (i + l <= length && !strncmp(p + i, tokens[k], l)) {
                    out[n++] = values[k];
                    i += l - 1;
                    found = true;
                    break;
                }
            }
            if (!found) {
                free(out);
                return NULL;
            }
        } else {
            out[n++] = p[i];
        }
    }
    out[n] = 0;
    return out;
}

static uint32_t rgba(unsigned c, float opacity) {
    return ((c & 255) << 24) | ((c & 0xff00) << 8) | ((c >> 8) & 0xff00) |
           (uint32_t)fminf(255, fmaxf(0, (c >> 24) * opacity));
}

bool ve_load(VeDocument *d, const char *path, char *error, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(error, size, "Cannot open SVG: %s", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    if (length <= 0 || length > 16 * 1024 * 1024) {
        fclose(f);
        snprintf(error, size, "SVG must contain 1 byte to 16 MiB");
        return false;
    }
    char *text = malloc((size_t)length + 1);
    if (!text) {
        fclose(f);
        return false;
    }
    bool read = fread(text, 1, (size_t)length, f) == (size_t)length;
    fclose(f);
    text[length] = 0;
    if (!read || memchr(text, 0, (size_t)length)) {
        free(text);
        snprintf(error, size, "Invalid SVG file");
        return false;
    }
    char *meta = strstr(text, "<metadata id=\"penguin-vector\">");
    if (meta) {
        meta += strlen("<metadata id=\"penguin-vector\">");
        char *end = strstr(meta, "</metadata>");
        char *json = end ? unescape(meta, (size_t)(end - meta)) : NULL;
        bool ok = json && ve_from_json(d, json, error, size);
        free(json);
        free(text);
        if (!ok) {
            snprintf(error, size, "Invalid Penguin SVG metadata");
        }
        return ok;
    }
    const char *unsupported[] = {"<text",
                                 "<image",
                                 "<use",
                                 "<mask",
                                 "<clipPath",
                                 "<filter",
                                 "<linearGradient",
                                 "<radialGradient",
                                 "<style",
                                 "<pattern",
                                 "<symbol",
                                 "<foreignObject",
                                 "<animate",
                                 "<script",
                                 "<!ENTITY",
                                 "<!DOCTYPE",
                                 NULL};
    for (int i = 0; unsupported[i]; i++) {
        if (strstr(text, unsupported[i])) {
            snprintf(error, size, "Unsupported SVG element: %s", unsupported[i]);
            free(text);
            return false;
        }
    }
    NSVGimage *image = nsvgParse(text, "px", 96);
    free(text);
    if (!image || !isfinite(image->width) || !isfinite(image->height) || image->width <= 0 ||
        image->height <= 0) {
        nsvgDelete(image);
        snprintf(error, size, "Cannot parse supported SVG geometry");
        return false;
    }
    VeDocument *next = calloc(1, sizeof *next);
    if (!next) {
        nsvgDelete(image);
        return false;
    }
    ve_init(next);
    next->boards[0].w = image->width;
    next->boards[0].h = image->height;
    next->boards[0].background = 0xffffff00;
    bool ok = true;
    for (NSVGshape *shape = image->shapes; shape && ok; shape = shape->next) {
        if (!(shape->flags & NSVG_FLAGS_VISIBLE)) {
            continue;
        }
        if (shape->fill.type > NSVG_PAINT_COLOR || shape->stroke.type > NSVG_PAINT_COLOR) {
            ok = false;
            break;
        }
        if (shape->stroke.type == NSVG_PAINT_COLOR &&
            (shape->strokeLineCap != NSVG_CAP_BUTT || shape->strokeLineJoin != NSVG_JOIN_MITER ||
             shape->strokeDashCount > 0 || fabsf(shape->miterLimit - 4) > 1e-5f)) {
            ok = false;
            break;
        }
        int index = ve_add_shape(next, 0, VE_PATH, 0, 0, 1, 1);
        if (index < 0) {
            ok = false;
            break;
        }
        VeShape *s = &next->shapes[index];
        s->count = 0;
        s->evenodd = shape->fillRule == NSVG_FILLRULE_EVENODD;
        s->fill_enabled = shape->fill.type == NSVG_PAINT_COLOR;
        s->stroke_enabled = shape->stroke.type == NSVG_PAINT_COLOR;
        s->fill = rgba(shape->fill.color, shape->opacity);
        s->stroke = rgba(shape->stroke.color, shape->opacity);
        s->stroke_width = shape->strokeWidth;
        for (NSVGpath *p = shape->paths; p && ok; p = p->next) {
            if (p->npts < 1 || s->count + 2 + (p->npts - 1) / 3 > VE_NODES) {
                ok = false;
                break;
            }
            s->nodes[s->count++] = (VeNode){.command = VE_MOVE, .p = {p->pts[0], p->pts[1]}};
            for (int i = 1; i < p->npts; i += 3) {
                s->nodes[s->count++] =
                    (VeNode){.command = VE_CUBIC,
                             .c1 = {p->pts[i * 2], p->pts[i * 2 + 1]},
                             .c2 = {p->pts[(i + 1) * 2], p->pts[(i + 1) * 2 + 1]},
                             .p = {p->pts[(i + 2) * 2], p->pts[(i + 2) * 2 + 1]}};
            }
            if (p->closed) {
                s->nodes[s->count++] = (VeNode){.command = VE_CLOSE};
            }
        }
        ve_bounds(s);
    }
    ok = ok && ve_valid(next);
    if (ok) {
        *d = *next;
    } else {
        snprintf(error, size, "Unsupported SVG paint/stroke style or document complexity limit");
    }
    free(next);
    nsvgDelete(image);
    return ok;
}

bool ve_save(const VeDocument *d, const char *path, char *error, size_t size) {
    char *text = ve_svg(d, true, NULL, NULL);
    if (!text) {
        snprintf(error, size, "Invalid vector document or out of memory");
        return false;
    }
    char temp[1600];
    if (snprintf(temp, sizeof temp, "%s.tmp", path) >= (int)sizeof temp) {
        free(text);
        return false;
    }
    FILE *f = fopen(temp, "wb");
    bool ok = false;
    if (f) {
        size_t n = strlen(text);
        ok = fwrite(text, 1, n, f) == n;
        if (fclose(f) != 0) {
            ok = false;
        }
    }
    free(text);
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temp, path) == 0;
#endif
    }
    if (!ok) {
        remove(temp);
        snprintf(error, size, "Cannot save SVG: %s", strerror(errno));
    }
    return ok;
}

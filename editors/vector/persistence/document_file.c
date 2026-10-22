/** @file @brief Explicit little-endian fields preserve editable geometry, style and text values. */
#include "document_file.h"
#include "editors/vector/image/assets.h"
#include "editors/vector/object/clipboard.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24,
               "format requires binary32 floats");

typedef struct {
    SDL_IOStream *io;
    bool read, ok;
    unsigned version;
    char image_root[4096];
} Codec;

static void bytes(Codec *c, void *data, size_t length) {
    if (!c->ok) {
        return;
    }
    c->ok =
        (c->read ? SDL_ReadIO(c->io, data, length) : SDL_WriteIO(c->io, data, length)) == length;
}

static void integer(Codec *c, unsigned *value) {
    unsigned char b[4] = {(unsigned char)*value, (unsigned char)(*value >> 8),
                          (unsigned char)(*value >> 16), (unsigned char)(*value >> 24)};
    bytes(c, b, 4);
    if (c->read && c->ok) {
        *value = (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) |
                 ((unsigned)b[3] << 24);
    }
}

static void real(Codec *c, float *value) {
    uint32_t bits = 0;
    if (!c->read) {
        memcpy(&bits, value, 4);
    }
    unsigned v = bits;
    integer(c, &v);
    bits = v;
    if (c->read && c->ok) {
        memcpy(value, &bits, 4);
    }
    c->ok &= isfinite(*value);
}

static void string(Codec *c, char *s, size_t capacity) {
    unsigned length = c->read ? 0 : (unsigned)strlen(s);
    integer(c, &length);
    if (!c->ok || length >= capacity) {
        c->ok = false;
        return;
    }
    bytes(c, s, length);
    if (c->read) {
        s[length] = 0;
        c->ok &= memchr(s, 0, length) == NULL;
    }
}

static void rect(Codec *c, PUIRect *r) {
    real(c, &r->x);
    real(c, &r->y);
    real(c, &r->w);
    real(c, &r->h);
}

static void point(Codec *c, VectorPoint *p) {
    real(c, &p->x);
    real(c, &p->y);
}

static void shape(Codec *c, VectorShape *s) {
    integer(c, &s->id);
    integer(c, &s->parent_id);
    integer(c, &s->flags);
    integer(c, &s->group_id);
    if (c->version >= 2) {
        integer(c, &s->clip_mask);
    }
    integer(c, &s->fill);
    integer(c, &s->stroke.color);
    real(c, &s->stroke.width);
    unsigned alignment = s->stroke.alignment;
    integer(c, &alignment);
    if (c->read) {
        s->stroke.alignment = (VectorStrokeAlignment)alignment;
    }
    integer(c, &s->curve_kind);
    string(c, s->name, sizeof s->name);
    rect(c, &s->rect);
    real(c, &s->angle);
    integer(c, &s->curve.count);
    integer(c, &s->curve.closed);
    if (s->curve.count > VECTOR_BEZIER_NODES) {
        c->ok = false;
        return;
    }
    for (unsigned i = 0; i < s->curve.count; i++) {
        point(c, &s->curve.nodes[i].point);
        point(c, &s->curve.nodes[i].incoming);
        point(c, &s->curve.nodes[i].outgoing);
    }
    integer(c, &s->path.count);
    integer(c, &s->path.contours);
    if (s->path.count > VECTOR_PATH_POINTS || s->path.contours > VECTOR_PATH_CONTOURS) {
        c->ok = false;
        return;
    }
    for (unsigned i = 0; i < s->path.count; i++) {
        point(c, &s->path.points[i]);
    }
    for (unsigned i = 0; i < s->path.contours; i++) {
        integer(c, &s->path.ends[i]);
    }
    integer(c, &s->text.enabled);
    if (s->text.enabled) {
        string(c, s->text.content, sizeof s->text.content);
        string(c, s->text.font, sizeof s->text.font);
        integer(c, &s->text.face);
        integer(c, &s->text.bold);
        integer(c, &s->text.italic);
        integer(c, &s->text.alignment);
        integer(c, &s->text.columns);
        real(c, &s->text.size);
        real(c, &s->text.line_height);
        real(c, &s->text.letter_spacing);
        real(c, &s->text.column_gap);
    }
    if (c->version >= 3) {
        unsigned enabled = *s->image.source != 0;
        integer(c, &enabled);
        if (enabled > 1) {
            c->ok = false;
            return;
        }
        if (enabled) {
            char relative[4096] = {0};
            if (!c->read &&
                !vector_image_export(&s->image, c->image_root, relative, sizeof relative)) {
                c->ok = false;
                return;
            }
            string(c, relative, sizeof relative);
            if (!vector_image_relative_valid(relative)) {
                c->ok = false;
                return;
            }
            if (c->read && snprintf(s->image.source, sizeof s->image.source, "%s/%s", c->image_root,
                                    relative) >= (int)sizeof s->image.source) {
                c->ok = false;
                return;
            }
            integer(c, &s->image.width);
            integer(c, &s->image.height);
            integer(c, &s->image.flip_x);
            integer(c, &s->image.flip_y);
        }
    }
    if (c->version >= 4) {
        integer(c, &s->repeat.enabled);
        if (s->repeat.enabled > 1) {
            c->ok = false;
            return;
        }
        if (s->repeat.enabled) {
            integer(c, &s->repeat.columns);
            integer(c, &s->repeat.rows);
            real(c, &s->repeat.gap_x);
            real(c, &s->repeat.gap_y);
            for (int i = 0; i < VECTOR_REPEAT_PROPERTIES; i++) {
                real(c, &s->repeat.column.value[i]);
                real(c, &s->repeat.row.value[i]);
            }
        }
    }
    if (c->version >= 5) {
        integer(c, &s->shadow.enabled);
        integer(c, &s->shadow.color);
        real(c, &s->shadow.offset_x);
        real(c, &s->shadow.offset_y);
        real(c, &s->shadow.blur);
        real(c, &s->shadow.spread);
    }
    if (c->version >= 6) {
        integer(c, &s->gradient.kind);
        integer(c, &s->gradient.count);
        real(c, &s->gradient.x1);
        real(c, &s->gradient.y1);
        real(c, &s->gradient.x2);
        real(c, &s->gradient.y2);
        if (s->gradient.count > VECTOR_GRADIENT_STOPS) {
            c->ok = false;
            return;
        }
        for (unsigned i = 0; i < s->gradient.count; i++) {
            real(c, &s->gradient.stops[i].position);
            integer(c, &s->gradient.stops[i].color);
        }
    }
    if (c->version >= 8) {
        integer(c, &s->instance_source);
        real(c, &s->instance_basis_w);
        real(c, &s->instance_basis_h);
    }
    if (c->version >= 9) {
        integer(c, &s->ellipse.mode);
        real(c, &s->ellipse.start);
        real(c, &s->ellipse.end);
    }
    if (c->version >= 7) {
        real(c, &s->corner_radius);
    }
}

static bool content(Codec *c, VectorClipboard *values) {
    char magic[8] = {'P', 'E', 'N', 'G', 'V', 'E', 'C', 0}, actual[8];
    memcpy(actual, magic, 8);
    bytes(c, actual, 8);
    unsigned version = 9;
    integer(c, &version);
    if (memcmp(actual, magic, 8) || (version < 1 || version > 9)) {
        return false;
    }
    c->version = version;
    unsigned boards = values->board_count, shapes = values->rectangle_count;
    integer(c, &boards);
    integer(c, &shapes);
    if (boards > VECTOR_MAX_ARTBOARDS || shapes > VECTOR_MAX_RECTANGLES) {
        return false;
    }
    values->board_count = (int)boards;
    values->rectangle_count = (int)shapes;
    for (unsigned i = 0; i < boards; i++) {
        VectorArtboard *b = &values->boards[i];
        integer(c, &b->id);
        integer(c, &b->flags);
        string(c, b->name, sizeof b->name);
        rect(c, &b->rect);
        if (b->id == UINT_MAX || fabsf(b->rect.x) > VECTOR_MAX_ARTBOARD_SIZE ||
            fabsf(b->rect.y) > VECTOR_MAX_ARTBOARD_SIZE) {
            return false;
        }
    }
    for (unsigned i = 0; i < shapes; i++) {
        shape(c, &values->rectangles[i]);
        if (values->rectangles[i].id == UINT_MAX) {
            return false;
        }
    }
    return c->ok && vector_clipboard_valid(values);
}

static bool absolute_path(const char *path, char *out, size_t capacity) {
    char *cwd = SDL_GetCurrentDirectory();
    bool absolute = path[0] == '/' || path[0] == '\\' || (strlen(path) > 2 && path[1] == ':');
    int n = snprintf(out, capacity, "%s%s", absolute ? "" : (cwd ? cwd : ""), path);
    SDL_free(cwd);
    return n > 0 && (size_t)n < capacity;
}

static void identity(VectorDocument *d, const char *path) {
    vector_document_mark_saved(d);
    snprintf(d->filename, sizeof d->filename, "%s", path);
    const char *name = strrchr(path, '/'), *back = strrchr(path, '\\');
    if (back && (!name || back > name)) {
        name = back;
    }
    snprintf(d->title, sizeof d->title, "%.63s", name ? name + 1 : path);
}

bool vector_document_file_load(VectorDocument *d, const char *path, char *error, size_t capacity) {
    SDL_PathInfo info;
    char full[4096];
    if (!path || !*path || !absolute_path(path, full, sizeof full) ||
        !SDL_GetPathInfo(full, &info) || info.size > 4 * 1024 * 1024) {
        snprintf(error, capacity, "Dokumentdatei nicht lesbar oder zu gross");
        return false;
    }
    SDL_IOStream *io = SDL_IOFromFile(full, "rb");
    VectorClipboard *values = calloc(1, sizeof *values);
    Codec c = {.io = io, .read = true, .ok = io && values};
    bool ok = c.ok && vector_image_project_root(full, c.image_root, sizeof c.image_root) &&
              content(&c, values);
    unsigned char extra;
    if (ok) {
        ok = SDL_ReadIO(io, &extra, 1) == 0 && SDL_GetIOStatus(io) == SDL_IO_STATUS_EOF;
    }
    if (io) {
        ok = SDL_CloseIO(io) && ok;
    }
    if (ok) {
        ok = vector_objects_initialize(d, values);
    }
    free(values);
    if (!ok) {
        snprintf(error, capacity, "Ungueltiges oder nicht unterstuetztes .pengvec-Dokument");
        return false;
    }
    identity(d, full);
    return true;
}

bool vector_document_file_save(VectorDocument *d, const char *path, char *error, size_t capacity) {
    char full[4096], staged[4096];
    if (!path || !*path || !absolute_path(path, full, sizeof full) ||
        snprintf(staged, sizeof staged, "%s.tmp-%llu", full,
                 (unsigned long long)SDL_GetPerformanceCounter()) >= (int)sizeof staged) {
        snprintf(error, capacity, "Dokumentpfad ist zu lang");
        return false;
    }
    VectorClipboard *values = calloc(1, sizeof *values);
    if (!values) {
        return false;
    }
    values->board_count = d->artboard_count;
    values->rectangle_count = d->rectangle_count;
    memcpy(values->boards, d->artboards, d->artboard_count * sizeof *d->artboards);
    memcpy(values->rectangles, d->rectangles, d->rectangle_count * sizeof *d->rectangles);
    SDL_IOStream *io = SDL_IOFromFile(staged, "wb");
    Codec c = {.io = io, .ok = io != NULL};
    bool ok = io && vector_image_project_root(full, c.image_root, sizeof c.image_root) &&
              content(&c, values);
    if (io) {
        ok = SDL_CloseIO(io) && ok;
    }
    if (ok) {
        ok = SDL_RenamePath(staged, full);
    }
    if (!ok) {
        SDL_RemovePath(staged);
        snprintf(error, capacity, "Dokument konnte nicht gespeichert werden");
    }
    free(values);
    if (ok) {
        identity(d, full);
    }
    return ok;
}

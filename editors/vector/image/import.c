/** @file @brief File validation/copy precedes one reversible shape insertion. */
#include "import.h"
#include "assets.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

bool vector_image_import(void *instance, PUIView *view, unsigned document, unsigned artboard,
                         const char *source, const char *project, char *error, size_t capacity) {
    VectorEditor *editor = instance;
    VectorDocument *d = vector_documents_find(&editor->documents, document);
    VectorArtboard *b = d ? vector_artboard_find(d, artboard) : NULL;
    if (!b || b->flags & VECTOR_OBJECT_STATE_MASK || d->rectangle_count == VECTOR_MAX_RECTANGLES ||
        d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        snprintf(error, capacity, "Kein bearbeitbares Artboard oder Objektlimit erreicht");
        return false;
    }
    SDL_Surface *surface = editor_image_decode(source);
    if (!surface) {
        snprintf(error, capacity, "PNG/JPEG ungueltig oder zu gross (8192 px, 16 MP, 32 MiB)");
        return false;
    }
    VectorShape shape = {.parent_id = b->id};
    shape.image.width = (unsigned)surface->w;
    shape.image.height = (unsigned)surface->h;
    SDL_DestroySurface(surface);
    if (!vector_image_copy(source, project, shape.image.source, sizeof shape.image.source)) {
        snprintf(error, capacity, "Bild konnte nicht ins Projekt kopiert werden");
        return false;
    }
    const char *name = strrchr(source, '/'), *back = strrchr(source, '\\');
    if (back && (!name || back > name)) {
        name = back;
    }
    snprintf(shape.name, sizeof shape.name, "%.63s", name ? name + 1 : source);
    float scale = fminf(1, fminf(b->rect.w / shape.image.width, b->rect.h / shape.image.height));
    shape.rect.w = fmaxf(1, shape.image.width * scale);
    shape.rect.h = fmaxf(1, shape.image.height * scale);
    shape.rect.x = (b->rect.w - shape.rect.w) / 2;
    shape.rect.y = (b->rect.h - shape.rect.h) / 2;
    vector_artboard_tool_cancel(d);
    if (!vector_shape_add(d, &shape)) {
        SDL_RemovePath(shape.image.source);
        snprintf(error, capacity, "Bildobjekt konnte nicht angelegt werden");
        return false;
    }
    pui_view_set_text(view, "status", "Bild importiert: verschieben und mit Greifern skalieren");
    return true;
}

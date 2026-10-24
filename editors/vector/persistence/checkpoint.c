/** @file @brief Pointer-free session snapshots; candidate restoration is transactional. */
#include "checkpoint.h"
#include "commands.h"
#include "editors/vector/instance/instance.h"
#include "editors/vector/internal.h"
#include "editors/vector/workspace/canvas.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Native integer/float representation is sufficient for a private file in this process.
 * Changing this schema requires a new signature. This is not an SVG or interchange format. */
static const char signature[8] = {'P', 'V', 'C', 'K', 'P', 'T', '3', '4'};

#define WRITE(value) (fwrite(&(value), sizeof(value), 1, file) == 1)
#define READ(value) (fread(&(value), sizeof(value), 1, file) == 1)

static bool fail(char *error, size_t capacity) {
    if (error && capacity) {
        snprintf(error, capacity, "could not save or restore Vector session checkpoint");
    }
    return false;
}

static void tab_name(char *name, size_t capacity, unsigned id) {
    snprintf(name, capacity, "vector_document_%u", id);
}

static bool capture_tabs(const VectorDocuments *documents, const PUIView *view,
                         unsigned order[PUI_MAX_TABS], unsigned *selected) {
    if (pui_view_tab_count(view, "vector_document_tabs") != documents->count) {
        return false;
    }
    const char *active = pui_view_selected_tab(view, "vector_document_tabs");
    if (!documents->count) {
        *selected = 0;
        return active == NULL;
    }
    bool seen[PUI_MAX_TABS + 1] = {0}, found = false;
    for (int i = 0; i < documents->count; i++) {
        const char *tab = pui_view_tab_at(view, "vector_document_tabs", i);
        bool matched = false;
        for (int j = 0; j < documents->count; j++) {
            unsigned id = documents->items[j].id;
            char name[64];
            tab_name(name, sizeof name, id);
            if (tab && !strcmp(tab, name) && !seen[j]) {
                order[i] = id;
                seen[j] = true;
                matched = true;
                if (active && !strcmp(active, name)) {
                    *selected = id;
                    found = true;
                }
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return found;
}

static bool write_document(const VectorDocument *document, FILE *file) {
    /* A marquee is a selection preview, just like a live geometry gesture. */
    const VectorSelectionTool *tool = &document->selection_tool;
    VectorRectangleSelection selection = tool->marquee ? tool->before : document->selection;
    unsigned active_board = tool->marquee ? tool->previous_board : document->active_artboard;
    unsigned active_rectangle =
        tool->marquee ? tool->previous_rectangle : document->active_rectangle;
    unsigned grid_visible = document->grid.visible, grid_snap = document->grid.snap_enabled;
    unsigned element_snap = document->element_snap_enabled, rulers_hidden = document->rulers_hidden;
    unsigned has_saved_content = document->has_saved_content;
    if (!WRITE(document->id) || !WRITE(document->saved_content) || !WRITE(has_saved_content) ||
        !WRITE(document->title) || !WRITE(document->filename) || !WRITE(document->history.floor) ||
        !WRITE(document->next_artboard_id) || !WRITE(active_board) ||
        !WRITE(document->artboard_count) ||
        fwrite(document->artboards, sizeof *document->artboards, document->artboard_count, file) !=
            (size_t)document->artboard_count ||
        !WRITE(document->next_rectangle_id) || !WRITE(active_rectangle) ||
        !WRITE(document->rectangle_count) ||
        fwrite(document->rectangles, sizeof *document->rectangles, document->rectangle_count,
               file) != (size_t)document->rectangle_count ||
        !WRITE(document->viewport.x) || !WRITE(document->viewport.y) ||
        !WRITE(document->viewport.zoom) || !WRITE(grid_visible) || !WRITE(grid_snap) ||
        !WRITE(document->grid.snap_tolerance) || !WRITE(document->grid.type) ||
        !WRITE(document->grid.width) || !WRITE(document->grid.height) ||
        !WRITE(document->grid.color) || !WRITE(element_snap) || !WRITE(rulers_hidden) ||
        !WRITE(document->preview.board) || !WRITE(document->preview.region) ||
        !WRITE(document->history.count) || !WRITE(document->history.cursor) ||
        !WRITE(selection.count) ||
        fwrite(selection.ids, sizeof *selection.ids, selection.count, file) !=
            (size_t)selection.count) {
        return false;
    }
    for (int i = 0; i < document->history.count; i++) {
        if (!vector_command_write(&document->history.commands[i], file)) {
            return false;
        }
    }
    return true;
}

bool vector_editor_checkpoint(const void *instance, const PUIView *view, FILE *file, char *error,
                              size_t capacity) {
    const VectorEditor *editor = instance;
    const VectorDocuments *documents = &editor->documents;
    unsigned order[PUI_MAX_TABS], selected;
    unsigned initialized = editor->document_tabs.initialized;
    if (editor->document_tabs.pending || !capture_tabs(documents, view, order, &selected) ||
        !WRITE(signature) || !WRITE(documents->next_id) || !WRITE(initialized) ||
        !WRITE(documents->count) || !WRITE(selected) ||
        fwrite(order, sizeof *order, documents->count, file) != (size_t)documents->count) {
        return fail(error, capacity);
    }
    for (int i = 0; i < documents->count; i++) {
        if (!write_document(&documents->items[i], file)) {
            return fail(error, capacity);
        }
    }
    char navigation[64] = {0};
    const char *tab = pui_view_selected_tab(view, "vector_navigation_tabs");
    if (!tab || strlen(tab) >= sizeof navigation) {
        return fail(error, capacity);
    }
    snprintf(navigation, sizeof navigation, "%s", tab);
    return (WRITE(navigation) && WRITE(editor->object_actions.clipboard)) || fail(error, capacity);
}

/* Replaying validates command order, limits and agreement with the saved current state.
 * It never modifies the candidate document or takes ownership of command payloads. */
typedef struct {
    VectorDocument replay;
    VectorRectangle rectangles_before[VECTOR_MAX_RECTANGLES];
    VectorArtboard artboards_before[VECTOR_MAX_ARTBOARDS];
} HistoryValidation;

static bool validate_history(const VectorDocument *document, HistoryValidation *workspace) {
    VectorDocument *replay = &workspace->replay;
    VectorRectangle *rectangles_before = workspace->rectangles_before;
    VectorArtboard *before = workspace->artboards_before;
    for (int i = 0; i < document->history.count; i++) {
        VectorCommand command = document->history.commands[i];
        int rectangle_count = replay->rectangle_count;
        memcpy(rectangles_before, replay->rectangles, rectangle_count * sizeof *rectangles_before);
        int count = replay->artboard_count;
        memcpy(before, replay->artboards, count * sizeof *before);
        if (!command.apply(replay, command.data)) {
            return false;
        }
        for (int j = 0; j < replay->artboard_count; j++) {
            if (!replay->artboards[j].id || replay->artboards[j].id >= document->next_artboard_id) {
                return false;
            }
            for (int k = 0; k < j; k++) {
                if (replay->artboards[k].id == replay->artboards[j].id) {
                    return false;
                }
            }
        }
        for (int j = 0; j < replay->rectangle_count; j++) {
            VectorRectangle *rectangle = &replay->rectangles[j];
            if (!rectangle->id || rectangle->id >= document->next_rectangle_id ||
                !vector_artboard_find(replay, rectangle->parent_id)) {
                return false;
            }
            for (int k = 0; k < j; k++) {
                if (replay->rectangles[k].id == rectangle->id) {
                    return false;
                }
            }
        }
        if (!command.revert(replay, command.data) || replay->rectangle_count != rectangle_count ||
            memcmp(rectangles_before, replay->rectangles,
                   rectangle_count * sizeof *rectangles_before) ||
            replay->artboard_count != count ||
            memcmp(before, replay->artboards, count * sizeof *before) ||
            !command.apply(replay, command.data)) {
            return false;
        }
    }
    for (int i = document->history.count; i > document->history.cursor; i--) {
        VectorCommand command = document->history.commands[i - 1];
        if (!command.revert(replay, command.data)) {
            return false;
        }
    }
    if (replay->rectangle_count != document->rectangle_count ||
        memcmp(replay->rectangles, document->rectangles,
               replay->rectangle_count * sizeof *replay->rectangles) ||
        replay->artboard_count != document->artboard_count ||
        (document->active_artboard && !document->artboard_count)) {
        return false;
    }
    for (int i = 0; i < replay->artboard_count; i++) {
        const VectorArtboard *a = &replay->artboards[i], *b = &document->artboards[i];
        if (a->flags != b->flags || !memchr(b->name, 0, sizeof b->name) ||
            strcmp(a->name, b->name) || a->id != b->id || a->rect.x != b->rect.x ||
            a->rect.y != b->rect.y || a->rect.w != b->rect.w || a->rect.h != b->rect.h) {
            return false;
        }
    }
    if (document->active_rectangle) {
        bool found = false;
        for (int i = 0; i < document->rectangle_count; i++) {
            if (document->rectangles[i].id == document->active_rectangle &&
                document->rectangles[i].parent_id == document->active_artboard) {
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    if (document->active_artboard) {
        for (int i = 0; i < document->artboard_count; i++) {
            if (document->artboards[i].id == document->active_artboard) {
                return true;
            }
        }
        return false;
    }
    return true;
}

/* The replay plus geometry snapshots exceed the default Windows stack budget.
 * One owned workspace also guarantees cleanup on every validation failure. */
static bool valid_history(const VectorDocument *document) {
    HistoryValidation *workspace = calloc(1, sizeof *workspace);
    if (!workspace) {
        return false;
    }
    bool valid = validate_history(document, workspace);
    free(workspace);
    return valid;
}

static bool read_document(VectorDocument *document, unsigned next_id, FILE *file) {
    int history_count;
    unsigned grid_visible, grid_snap, element_snap, rulers_hidden, has_saved_content;
    if (!READ(document->id) || !document->id || document->id >= next_id ||
        !READ(document->saved_content) || !READ(has_saved_content) || has_saved_content > 1 ||
        !READ(document->title) || !READ(document->filename) ||
        !memchr(document->filename, 0, sizeof document->filename) ||
        !READ(document->history.floor) || document->history.floor < 0 ||
        !memchr(document->title, 0, sizeof document->title) || !READ(document->next_artboard_id) ||
        !document->next_artboard_id || !READ(document->active_artboard) ||
        !READ(document->artboard_count) || document->artboard_count < 0 ||
        document->artboard_count > VECTOR_MAX_ARTBOARDS ||
        fread(document->artboards, sizeof *document->artboards, document->artboard_count, file) !=
            (size_t)document->artboard_count ||
        !READ(document->next_rectangle_id) || !document->next_rectangle_id ||
        !READ(document->active_rectangle) || !READ(document->rectangle_count) ||
        document->rectangle_count < 0 || document->rectangle_count > VECTOR_MAX_RECTANGLES ||
        fread(document->rectangles, sizeof *document->rectangles, document->rectangle_count,
              file) != (size_t)document->rectangle_count ||
        !READ(document->viewport.x) || !READ(document->viewport.y) ||
        !READ(document->viewport.zoom) || !isfinite(document->viewport.x) ||
        !isfinite(document->viewport.y) || !isfinite(document->viewport.zoom) ||
        document->viewport.zoom < .1f || document->viewport.zoom > 4 || !READ(grid_visible) ||
        grid_visible > 1 || !READ(grid_snap) || grid_snap > 1 ||
        !READ(document->grid.snap_tolerance) || !isfinite(document->grid.snap_tolerance) ||
        document->grid.snap_tolerance < 0 || document->grid.snap_tolerance > 100 ||
        !READ(document->grid.type) || !READ(document->grid.width) || !READ(document->grid.height) ||
        !READ(document->grid.color) || !READ(element_snap) || element_snap > 1 ||
        !READ(rulers_hidden) || rulers_hidden > 1 || !READ(document->preview.board) ||
        !READ(document->preview.region) || document->preview.board >= document->next_artboard_id ||
        !vector_preview_valid(document->preview.board, document->preview.region) ||
        (document->grid.type != VECTOR_GRID_STANDARD &&
         document->grid.type != VECTOR_GRID_HEXAGONAL &&
         document->grid.type != VECTOR_GRID_ISOMETRIC) ||
        !isfinite(document->grid.width) || !isfinite(document->grid.height) ||
        document->grid.width < 1 || document->grid.width > 10000 || document->grid.height < 1 ||
        document->grid.height > 10000 || !READ(history_count) || history_count < 0 ||
        history_count > VECTOR_HISTORY_CAPACITY || !READ(document->history.cursor) ||
        document->history.cursor < document->history.floor ||
        document->history.floor > history_count || document->history.cursor > history_count) {
        return false;
    }
    if (!READ(document->selection.count) || document->selection.count < 0 ||
        document->selection.count > VECTOR_MAX_RECTANGLES ||
        fread(document->selection.ids, sizeof *document->selection.ids, document->selection.count,
              file) != (size_t)document->selection.count) {
        return false;
    }
    bool primary = document->selection.count == 0;
    for (int i = 0; i < document->selection.count; i++) {
        if (!vector_rectangle_find(document, document->selection.ids[i])) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (document->selection.ids[j] == document->selection.ids[i]) {
                return false;
            }
        }
        primary |= document->selection.ids[i] == document->active_rectangle;
    }
    if (!primary) {
        return false;
    }
    document->has_saved_content = has_saved_content != 0;
    document->rulers_hidden = rulers_hidden != 0;
    document->element_snap_enabled = element_snap != 0;
    document->grid.visible = grid_visible != 0;
    document->grid.snap_enabled = grid_snap != 0;
    for (int i = 0; i < history_count; i++) {
        if (!vector_command_read(&document->history.commands[i], file)) {
            return false;
        }
        document->history.count++;
    }
    return vector_instances_valid(document->rectangles, document->rectangle_count) &&
           valid_history(document);
}

static bool read_documents(VectorDocuments *documents, unsigned order[PUI_MAX_TABS],
                           unsigned *selected, unsigned *initialized, char navigation[64],
                           VectorClipboard *clipboard, FILE *file) {
    char actual[sizeof signature];
    if (!READ(actual) || memcmp(actual, signature, sizeof signature) || !READ(documents->next_id) ||
        !documents->next_id || !READ(*initialized) || *initialized > 1 || !READ(documents->count) ||
        documents->count < 0 || documents->count > VECTOR_MAX_DOCUMENTS || !READ(*selected) ||
        *selected >= documents->next_id || (documents->count && !*selected) ||
        (!documents->count && *selected) ||
        fread(order, sizeof *order, documents->count, file) != (size_t)documents->count) {
        /* Keep cleanup bounded even for an invalid count. */
        documents->count = 0;
        return false;
    }
    bool found_selected = !documents->count;
    for (int i = 0; i < documents->count; i++) {
        if (!read_document(&documents->items[i], documents->next_id, file)) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (documents->items[j].id == documents->items[i].id || order[i] == order[j]) {
                return false;
            }
        }
        found_selected |= documents->items[i].id == *selected;
    }
    for (int i = 0; i < documents->count; i++) {
        if (!vector_documents_find(documents, order[i])) {
            return false;
        }
    }
    if (!found_selected) {
        return false;
    }
    if (fread(navigation, 64, 1, file) != 1 || !memchr(navigation, 0, 64) ||
        (strcmp(navigation, "vector_explorer_tab") &&
         strcmp(navigation, "vector_document_tree_tab"))) {
        return false;
    }
    return READ(*clipboard) && vector_clipboard_valid(clipboard) && fgetc(file) == EOF &&
           !ferror(file);
}

static bool restore_tab_order(PUILayout *layout, const VectorDocuments *documents,
                              const unsigned order[PUI_MAX_TABS], unsigned selected) {
    for (int i = 0; i < layout->count; i++) {
        PUINode *tabs = &layout->nodes[i];
        if (tabs->type != PUI_NODE_TABS || strcmp(tabs->name, "vector_document_tabs")) {
            continue;
        }
        if (!documents->count) {
            tabs->child = -1;
            return selected == 0;
        }
        int indices[PUI_MAX_TABS];
        for (int j = 0; j < documents->count; j++) {
            char name[64];
            tab_name(name, sizeof name, order[j]);
            indices[j] = -1;
            for (int k = tabs->child; k >= 0; k = layout->nodes[k].next) {
                if (!strcmp(layout->nodes[k].name, name)) {
                    indices[j] = k;
                    layout->nodes[k].checked = order[j] == selected;
                    break;
                }
            }
            if (indices[j] < 0) {
                return false;
            }
        }
        tabs->child = indices[0];
        for (int j = 0; j < documents->count; j++) {
            layout->nodes[indices[j]].next = j + 1 < documents->count ? indices[j + 1] : -1;
        }
        return true;
    }
    return false;
}

static PUILayout *restore_layout(const PUILayout *base, const VectorDocuments *documents,
                                 const unsigned order[PUI_MAX_TABS], unsigned selected, char *error,
                                 size_t capacity) {
    PUILayout *layout = malloc(sizeof *layout);
    if (!layout) {
        return NULL;
    }
    *layout = *base;
    PUILayout *canvas = vector_canvas_layout(error, capacity);
    if (!canvas) {
        pui_layout_free(layout);
        return NULL;
    }
    for (int i = 0; i < documents->count; i++) {
        const VectorDocument *document = &documents->items[i];
        char name[64];
        tab_name(name, sizeof name, document->id);
        PUILayout *next = pui_layout_add_tab(layout, "vector_document_tabs", name, document->title,
                                             canvas, error, capacity);
        pui_layout_free(layout);
        layout = next;
        if (!layout) {
            break;
        }
    }
    pui_layout_free(canvas);
    if (layout && !restore_tab_order(layout, documents, order, selected)) {
        pui_layout_free(layout);
        layout = NULL;
    }
    return layout;
}

bool vector_editor_restore(void *instance, FILE *file, char *error, size_t capacity) {
    VectorEditor *editor = instance;
    VectorDocuments *loaded = calloc(1, sizeof *loaded);
    if (!loaded) {
        return fail(error, capacity);
    }
    unsigned order[PUI_MAX_TABS], selected, initialized;
    char navigation[64];
    VectorClipboard clipboard;
    PUILayout *layout = NULL;
    if (read_documents(loaded, order, &selected, &initialized, navigation, &clipboard, file)) {
        layout = restore_layout(editor->layout, loaded, order, selected, error, capacity);
    }
    if (layout) {
        for (int i = 0; i < layout->count; i++) {
            PUINode *node = &layout->nodes[i];
            if (node->type == PUI_NODE_TAB && (!strcmp(node->name, "vector_explorer_tab") ||
                                               !strcmp(node->name, "vector_document_tree_tab"))) {
                node->checked = !strcmp(node->name, navigation);
            }
        }
    }
    if (!layout) {
        vector_documents_destroy(loaded);
        free(loaded);
        return fail(error, capacity);
    }
    vector_documents_destroy(&editor->documents);
    editor->documents = *loaded;
    editor->document_tabs.initialized = initialized != 0;
    editor->object_actions.clipboard = clipboard;
    free(loaded); /* Payload ownership has moved into editor->documents. */
    pui_layout_free(editor->layout);
    editor->layout = layout;
    return true;
}

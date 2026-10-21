/** @file @brief File tree state is app-owned and independent of concrete editor modules. */
#include "explorer.h"
#include "directories.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char path_character(char c) {
#ifdef _WIN32
    if (c == '\\') {
        return '/';
    }
    return (char)SDL_tolower((unsigned char)c);
#else
    return c;
#endif
}

static bool path_prefix(const char *path, const char *parent) {
    size_t i = 0;
    while (parent[i]) {
        if (!path[i] || path_character(path[i]) != path_character(parent[i])) {
            return false;
        }
        i++;
    }
    return !path[i] || path_character(path[i]) == '/' ||
           (i && path_character(parent[i - 1]) == '/');
}

struct ProjectEntry {
    char *path, *name;
    int parent;
    unsigned id;
    int page;
    bool directory, loaded, open;
};

static void clear(ProjectExplorer *e) {
    for (int i = 0; i < e->count; i++) {
        SDL_free(e->entries[i].path);
        SDL_free(e->entries[i].name);
    }
    free(e->entries);
    e->entries = NULL;
    e->count = e->allocated = 0;
    e->search_cursor = 0;
}

bool project_explorer_init(ProjectExplorer *e, const char *resources, char *error,
                           size_t capacity) {
    *e = (ProjectExplorer){.dirty = true};
    char path[PROJECT_PATH_CAPACITY];
    snprintf(path, sizeof path, "%s/project-explorer.yaml", resources);
    e->layout = pui_layout_load(path, error, capacity);
    return e->layout != NULL;
}

void project_explorer_destroy(ProjectExplorer *e) {
    clear(e);
    pui_layout_free(e->layout);
    *e = (ProjectExplorer){0};
}

static bool add(ProjectExplorer *e, const char *path, const char *name, int parent,
                bool directory) {
    if (*e->filter && e->count >= 8192) {
        return false;
    }
    if (e->count == e->allocated) {
        int n = e->allocated + 128;
        ProjectEntry *entries = realloc(e->entries, (size_t)n * sizeof *entries);
        if (!entries) {
            return false;
        }
        e->entries = entries;
        e->allocated = n;
    }
    char *p = SDL_strdup(path), *label = SDL_strdup(name);
    if (!p || !label) {
        SDL_free(p);
        SDL_free(label);
        return false;
    }
    e->entries[e->count] = (ProjectEntry){.path = p,
                                          .name = label,
                                          .parent = parent,
                                          .id = (unsigned)e->count + 1,
                                          .directory = directory};
    e->count++;
    return true;
}

void project_explorer_set(ProjectExplorer *e, const AppProject *project) {
    clear(e);
    e->create_pending = false;
    snprintf(e->root, sizeof e->root, "%s", project->root);
    e->dirty = true;
    e->reset = true;
    if (*project->root && add(e, project->root, project->name, -1, true)) {
        e->entries[0].open = true;
    }
}

typedef struct {
    ProjectExplorer *explorer;
    int parent;
    bool ok;
} Scan;

static SDL_EnumerationResult visit(void *context, const char *directory, const char *name) {
    Scan *s = context;
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        return SDL_ENUM_CONTINUE;
    }
    char path[PROJECT_PATH_CAPACITY];
    SDL_PathInfo info;
    int n = snprintf(path, sizeof path, "%s%s", directory, name);
    if (n < 0 || n >= (int)sizeof path || !SDL_GetPathInfo(path, &info)) {
        return SDL_ENUM_CONTINUE;
    }
    s->ok = add(s->explorer, path, name, s->parent, info.type == SDL_PATHTYPE_DIRECTORY);
    return s->ok ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}

static int compare(const void *a, const void *b) {
    const ProjectEntry *x = a, *y = b;
    if (x->directory != y->directory) {
        return x->directory ? -1 : 1;
    }
    return SDL_strcasecmp(x->name, y->name);
}

static bool load(ProjectExplorer *e, int i) {
    if (e->entries[i].loaded) {
        return true;
    }
    int start = e->count;
    Scan scan = {e, i, true};
    bool ok = SDL_EnumerateDirectory(e->entries[i].path, visit, &scan) && scan.ok;
    if (ok) {
        qsort(e->entries + start, (size_t)(e->count - start), sizeof *e->entries, compare);
        e->entries[i].loaded = true;
    } else {
        for (int j = start; j < e->count; j++) {
            SDL_free(e->entries[j].path);
            SDL_free(e->entries[j].name);
        }
        e->count = start;
    }
    return ok;
}

const char *project_explorer_file(const ProjectExplorer *e, const char *row) {
    unsigned id = 0;
    if (!row || sscanf(row, "project_entry_%u", &id) != 1) {
        return NULL;
    }
    for (int i = 0; i < e->count; i++) {
        if (e->entries[i].id == id && !e->entries[i].directory) {
            return e->entries[i].path;
        }
    }
    return NULL;
}

bool project_explorer_action(ProjectExplorer *e, PUIView *view, const char *action,
                             const char *widget) {
    if (!strcmp(action, "project.folder.create")) {
        if (!*e->root) {
            pui_view_set_text(view, "status", "Zuerst ein Projekt oeffnen");
            return true;
        }
        const char *name = pui_view_text(view, "project_folder_name");
        snprintf(e->pending_name, sizeof e->pending_name, "%s", name ? name : "");
        const char *selected = pui_view_selected_tree_item(view, "project_files");
        unsigned id = 0;
        snprintf(e->pending_parent, sizeof e->pending_parent, "%s", e->root);
        if (selected && sscanf(selected, "project_entry_%u", &id) == 1) {
            for (int i = 0; i < e->count; i++) {
                if (e->entries[i].id == id) {
                    int parent = e->entries[i].directory ? i : e->entries[i].parent;
                    if (parent >= 0) {
                        snprintf(e->pending_parent, sizeof e->pending_parent, "%s",
                                 e->entries[parent].path);
                    }
                    break;
                }
            }
        }
        e->create_pending = true;
        return true;
    }
    if (!strcmp(action, "project.refresh")) {
        AppProject p = {0};
        snprintf(p.root, sizeof p.root, "%s", e->root);
        if (e->count) {
            snprintf(p.name, sizeof p.name, "%s", e->entries[0].name);
        }
        project_explorer_set(e, &p);
        return true;
    }
    if (!strcmp(action, "project.page")) {
        unsigned id = 0;
        if (sscanf(widget, "project_page_%u", &id) == 1) {
            for (int i = 0; i < e->count; i++) {
                if (e->entries[i].id == id) {
                    e->entries[i].page++;
                    e->dirty = true;
                    break;
                }
            }
        }
        return true;
    }
    if (strcmp(action, "project.entry")) {
        return false;
    }
    unsigned id = 0;
    if (sscanf(widget, "project_entry_%u", &id) != 1) {
        return true;
    }
    for (int i = 0; i < e->count; i++) {
        if (e->entries[i].id == id) {
            if (e->entries[i].directory) {
                if (*e->filter) {
                    return true;
                }
                e->entries[i].open = !e->entries[i].open;
                e->dirty = true;
            } else {
                pui_view_set_text(view, "status", e->entries[i].path);
            }
            break;
        }
    }
    return true;
}

static bool rows(ProjectExplorer *e, int i, int depth, PUITreeItem *items, int *count) {
    if (*count >= PUI_LAYOUT_NODES - 1 || depth > 32) {
        return false;
    }
    // Cache can grow while loading: never keep pointers into it across a scan.
    if (e->entries[i].directory && e->entries[i].open && !load(e, i)) {
        return false;
    }
    ProjectEntry *n = &e->entries[i];
    PUITreeItem *row = &items[(*count)++];
    snprintf(row->id, sizeof row->id, "project_entry_%u", n->id);
    snprintf(row->text, sizeof row->text, "%s", n->name);
    snprintf(row->action, sizeof row->action, "project.entry");
    snprintf(row->icon, sizeof row->icon, "%s", n->directory ? "tabler:folder" : "tabler:file");
    row->enabled = true;
    row->open = n->open;
    row->depth = depth;
    if (n->directory && n->open) {
        int total = e->count, children = 0;
        for (int j = 0; j < total; j++) {
            children += e->entries[j].parent == i;
        }
        const int page_size = 32;
        if (n->page * page_size >= children) {
            n->page = 0;
        }
        int page = n->page;
        if (children > page_size && *count < PUI_LAYOUT_NODES - 1) {
            PUITreeItem *more = &items[(*count)++];
            more->depth = depth + 1;
            more->enabled = true;
            snprintf(more->id, sizeof more->id, "project_page_%u", n->id);
            snprintf(more->text, sizeof more->text, "Seite %d/%d - weiter", page + 1,
                     (children + page_size - 1) / page_size);
            snprintf(more->action, sizeof more->action, "project.page");
        }
        int position = 0;
        for (int j = 0; j < total; j++) {
            if (e->entries[j].parent != i) {
                continue;
            }
            int child = position++;
            if (child >= page * page_size && child < (page + 1) * page_size &&
                !rows(e, j, depth + 1, items, count)) {
                return false;
            }
        }
    } else if (n->directory && *count < PUI_LAYOUT_NODES - 1) {
        PUITreeItem *placeholder = &items[(*count)++];
        placeholder->depth = depth + 1;
        snprintf(placeholder->id, sizeof placeholder->id, "project_placeholder_%u", n->id);
        snprintf(placeholder->text, sizeof placeholder->text, "Ordner aufklappen");
    }
    return true;
}

/* Search indexes closed folders in bounded batches between frames. Normal paging
 * and folder disclosure remain unchanged when the search is cleared. */
static bool search_rows(ProjectExplorer *e, int parent, int depth, const bool *keep,
                        PUITreeItem *items, int *count) {
    if (!keep[parent]) {
        return true;
    }
    if (*count >= PUI_LAYOUT_NODES - 1 || depth > 32) {
        return false;
    }
    ProjectEntry *entry = &e->entries[parent];
    PUITreeItem *row = &items[(*count)++];
    snprintf(row->id, sizeof row->id, "project_entry_%u", entry->id);
    snprintf(row->text, sizeof row->text, "%s", entry->name);
    snprintf(row->action, sizeof row->action, "project.entry");
    snprintf(row->icon, sizeof row->icon, "%s", entry->directory ? "tabler:folder" : "tabler:file");
    row->enabled = row->open = true;
    row->depth = depth;
    for (int i = 0; i < e->count; i++) {
        if (e->entries[i].parent == parent && !search_rows(e, i, depth + 1, keep, items, count)) {
            return false;
        }
    }
    return true;
}

static bool filtered_rows(ProjectExplorer *e, PUITreeItem *items, int *count) {
    bool *keep = calloc((size_t)e->count, sizeof *keep);
    if (!keep && e->count) {
        return false;
    }
    for (int i = 0; i < e->count; i++) {
        if (SDL_strcasestr(e->entries[i].name, e->filter)) {
            for (int j = i; j >= 0; j = e->entries[j].parent) {
                keep[j] = true;
            }
        }
    }
    bool ok = !e->count || search_rows(e, 0, 0, keep, items, count);
    free(keep);
    return ok;
}

bool project_explorer_commit(ProjectExplorer *e, PUIView *view, PUIContext *input, char *error,
                             size_t capacity) {
    if (pui_view_tree_item_count(view, "project_files") < 0) {
        if (!pui_view_replace_slot(view, input, "project_explorer", e->layout, error, capacity)) {
            return false;
        }
        e->dirty = true;
    }
    const char *query = pui_view_text(view, "project_search");
    if (strcmp(e->filter, query ? query : "")) {
        snprintf(e->filter, sizeof e->filter, "%s", query ? query : "");
        e->dirty = true;
        e->search_cursor = 0;
    }
    if (*e->filter) {
        for (int work = 0; work < 16 && e->search_cursor < e->count; work++) {
            int i = e->search_cursor++;
            if (e->entries[i].directory) {
                int depth = 0;
                for (int j = i; j >= 0; j = e->entries[j].parent) {
                    depth++;
                }
                if (depth <= 32 && !load(e, i)) {
                    pui_view_set_text(view, "status", "Suchbereich teilweise nicht lesbar");
                }
            }
            e->dirty = true;
        }
    }
    char created[PROJECT_PATH_CAPACITY] = {0};
    if (e->create_pending) {
        e->create_pending = false;
        char message[256];
        if (project_directory_create(e->pending_parent, e->pending_name, created, sizeof created,
                                     message, sizeof message)) {
            AppProject p = {0};
            snprintf(p.root, sizeof p.root, "%s", e->root);
            if (e->count) {
                snprintf(p.name, sizeof p.name, "%s", e->entries[0].name);
            }
            project_explorer_set(e, &p);
            // Expand only ancestors of the newly created folder, selecting the correct sibling
            // page.
            for (int i = 0; i < e->count; i++) {
                if (e->entries[i].directory && path_prefix(created, e->entries[i].path)) {
                    e->entries[i].open = true;
                    if (load(e, i)) {
                        int position = 0;
                        for (int j = 0; j < e->count; j++) {
                            if (e->entries[j].parent != i) {
                                continue;
                            }
                            if (path_prefix(created, e->entries[j].path)) {
                                e->entries[i].page = position / 32;
                                break;
                            }
                            position++;
                        }
                    }
                }
            }
            pui_view_close_popups(view, input);
            pui_view_set_text(view, "project_folder_name", "");
            pui_view_set_text(view, "status", "Ordner angelegt");
        } else {
            pui_view_set_text(view, "status", message);
        }
    }
    if (!e->dirty) {
        return true;
    }
    if (e->reset) {
        pui_view_set_tree_items(view, "project_files", NULL, 0);
        e->reset = false;
    }
    PUITreeItem *items = calloc(PUI_LAYOUT_NODES, sizeof *items);
    if (!items) {
        return false;
    }
    int count = 0;
    bool complete =
        *e->filter ? filtered_rows(e, items, &count) : !e->count || rows(e, 0, 0, items, &count);
    if (!complete && count < PUI_LAYOUT_NODES) {
        PUITreeItem *last = &items[count++];
        snprintf(last->id, sizeof last->id, "project_limit");
        snprintf(last->text, sizeof last->text, "Weitere Ordner zuklappen / neu laden");
        pui_view_set_text(view, "status",
                          "Explorer: Zeilenlimit oder Lesefehler; Ordner zuklappen / neu laden");
    }
    pui_view_set_text(view, "project_hint", e->count ? "" : "Kein Projekt geoeffnet");
    bool ok = pui_view_set_tree_items(view, "project_files", items, count);
    free(items);
    if (ok && *created) {
        for (int i = 0; i < e->count; i++) {
            if (strlen(e->entries[i].path) == strlen(created) &&
                path_prefix(created, e->entries[i].path)) {
                char row[64];
                snprintf(row, sizeof row, "project_entry_%u", e->entries[i].id);
                pui_view_select_tree_item(view, "project_files", row);
                break;
            }
        }
    }
    e->dirty = !ok;
    return ok;
}

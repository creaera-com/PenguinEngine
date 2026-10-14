#ifndef PENGUIN_SCRIPT_EDITOR_H
#define PENGUIN_SCRIPT_EDITOR_H
#include "penguin/script.h"
#include "ui.h"
typedef struct {
    SDL_Window *window;
    PeText document;
    PeJsToken tokens[PE_TEXT_MAX];
    PeJsDiagnostic diagnostic;
    PeCompletions completions;
    char root[768], path[1024], status[256];
    bool focused, selecting, exists, show_completion;
    int top_line, left_column, completion_index, goal_column;
    uint64_t highlighted_revision, checked_revision;
    Uint64 changed_at;
    SDL_FRect body, popup;
} PeScriptEditor;
bool se_init(PeScriptEditor *editor, const char *root);
void se_free(PeScriptEditor *editor);
void se_focus(PeScriptEditor *editor, SDL_Window *window, bool focus);
bool se_open(PeScriptEditor *editor, const char *path);
bool se_save(PeScriptEditor *editor);
bool se_dirty(const PeScriptEditor *editor);
void se_check(PeScriptEditor *editor);
void se_changed(PeScriptEditor *editor, SDL_FRect area, bool completion);
/* Returns true when input belongs to script editor rather than scene tools. */
bool se_event(PeScriptEditor *editor, SDL_Window *window, const SDL_Event *event, SDL_FRect area);
void se_draw(PeScriptEditor *editor, SDL_Renderer *renderer, SDL_FRect area, float mx, float my,
             bool click);
#endif

#ifndef PENGUIN_SCRIPT_CHECKPOINT_H
#define PENGUIN_SCRIPT_CHECKPOINT_H
#include "editors/scripts/script_editor.h"
#include "workspace/session.h"

size_t se_checkpoint_header_size(void);
bool se_checkpoint(PeScriptEditor *editor, const char *project_root, SDL_Window *window,
                   PeSession *io);
#endif

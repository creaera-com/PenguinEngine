#ifndef PENGUIN_BROWSER_FILE_TREE_H
#define PENGUIN_BROWSER_FILE_TREE_H
#include "penguin/engine.h"

typedef struct {
    char path[PE_PATH];
    char name[128];
    int depth;
    bool directory, expanded;
} TreeRow;

void editor_draw_tree(void);
bool editor_suffix(const char *p, const char *s);
void editor_tree_refresh(void);
#endif

/** @file
 * @brief Optional typography form and searchable installed-font selection. */
#ifndef PE_VECTOR_TEXT_INSPECTOR_H
#define PE_VECTOR_TEXT_INSPECTOR_H
#include "editors/vector/text/render.h"

typedef struct {
    PUILayout *layout;
    PUIFonts *fonts;
    VectorTextRenderer *renderer;
    VectorText presented;
    unsigned id, dirty;
    char search[128];
    int page;
    bool mounted, rebuild_fonts;
} VectorTextInspector;

/** @brief Route typography controls to validated document commands. */
bool vector_text_inspector_action(VectorTextInspector *inspector, VectorDocument *document,
                                  unsigned id, PUIView *view, const char *action);
/** @brief Mount optional fields and font choices between frames. */
bool vector_text_inspector_sync(VectorTextInspector *inspector, VectorDocument *document,
                                VectorShape *shape, PUIView *view, PUIContext *input,
                                bool remounted, char *error, size_t capacity);
#endif

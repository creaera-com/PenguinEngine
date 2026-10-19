/** @file @brief Transient text editing cursor; no font or window resources. */
#ifndef PE_VECTOR_TEXT_TOOL_STATE_H
#define PE_VECTOR_TEXT_TOOL_STATE_H
#include "pengui/pengui.h"

typedef struct {
    unsigned editing_id;
    PUITextEdit edit;
    int history_start;
    bool selecting;
} VectorTextTool;
#endif

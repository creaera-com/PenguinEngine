/** @file @brief SDL-free, validated YAML widget descriptions. */
#ifndef PENGUI_LAYOUT_H
#define PENGUI_LAYOUT_H
#include "pengui/ui.h"
#define UI_LAYOUT_NODES 128
#define UI_LAYOUT_DEPTH 12

/** @brief Generic widget kinds; no application or document domains. */
typedef enum {
    UI_NODE_COLUMN,
    UI_NODE_ROW,
    UI_NODE_PANEL,
    UI_NODE_TOOLBAR,
    UI_NODE_BUTTON,
    UI_NODE_LABEL,
    UI_NODE_SEPARATOR,
    UI_NODE_TABS,
    UI_NODE_TAB,
    UI_NODE_SECTION,
    UI_NODE_INPUT,
    UI_NODE_TREE,
    UI_NODE_ITEM,
    UI_NODE_SPLIT,
    UI_NODE_SPACER
} UiNodeType;

/** @brief Owned description; children form a linked list of indices (-1 ends it). */
typedef struct {
    UiNodeType type;
    UiId id;
    char name[64], text[256], action[64], icon[32], image[512];
    float width, height, gap, padding, initial, minimum_first, minimum_second, image_padding;
    bool enabled, open;
    bool draggable; ///< Empty space in this widget may move a borderless window.
    UiAxis axis;
    int child, next;
} UiNode;

/** @brief Entire owned layout, allocated by the loader and released with ui_layout_free. */
typedef struct {
    char title[128], directory[1024];
    int width, height, count, root;
    uint32_t colors[7];
    UiNode nodes[UI_LAYOUT_NODES];
} UiLayout;

/** @brief Load from a file; NULL on error with a path/line diagnostic in error. */
UiLayout *ui_layout_load(const char *path, char *error, size_t capacity);
/** @brief Parse UTF-8 YAML in memory; NULL on schema, duplicate-ID or resource-limit errors. */
UiLayout *ui_layout_parse(const char *yaml, size_t length, char *error, size_t capacity);
/** @brief Release a layout; NULL is accepted. Views must stop borrowing it first. */
void ui_layout_free(UiLayout *layout);
#endif

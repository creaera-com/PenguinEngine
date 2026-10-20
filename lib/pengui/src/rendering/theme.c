/** @file @brief Shared palette and applying YAML theme colors. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

PUITheme pui_theme = {0x101010ffu,
                      0x202020ffu,
                      0x292929ffu,
                      0x393939ffu,
                      0xc28e5cffu,
                      0xe9e9e9ffu,
                      0xa2a2a2ffu,
                      0x686868ffu,
                      0x303030ffu,
                      0,
                      0};

void pui_set_theme(const PUITheme *theme) {
    pui_theme = *theme;
}

PUITheme pui_dark_theme(void) {
    return (PUITheme){0x101010ffu,
                      0x202020ffu,
                      0x292929ffu,
                      0x393939ffu,
                      0xc28e5cffu,
                      0xe9e9e9ffu,
                      0xa2a2a2ffu,
                      0x686868ffu,
                      0x303030ffu,
                      0,
                      0};
}

void pui_view_apply_theme(const PUILayout *layout) {
    PUITheme theme = pui_dark_theme();
    theme.canvas = layout->colors[0];
    theme.panel = layout->colors[1];
    theme.surface = layout->colors[2];
    theme.hover = layout->colors[3];
    theme.accent = layout->colors[4];
    theme.text = layout->colors[5];
    theme.muted = layout->colors[6];
    theme.button_radius = layout->button_radius;
    theme.button_transition_ms = layout->button_transition_ms;
    pui_set_theme(&theme);
}

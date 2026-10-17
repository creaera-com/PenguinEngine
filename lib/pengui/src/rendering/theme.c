/** @file @brief Shared palette and applying YAML theme colors. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

UiTheme ui_theme = {0x101010ffu, 0x202020ffu, 0x292929ffu, 0x393939ffu, 0xc28e5cffu,
                    0xe9e9e9ffu, 0xa2a2a2ffu, 0x686868ffu, 0x303030ffu};

void ui_set_theme(const UiTheme *theme) {
    ui_theme = *theme;
}

UiTheme ui_dark_theme(void) {
    return (UiTheme){0x101010ffu, 0x202020ffu, 0x292929ffu, 0x393939ffu, 0xc28e5cffu,
                     0xe9e9e9ffu, 0xa2a2a2ffu, 0x686868ffu, 0x303030ffu};
}

void ui_view_apply_theme(const UiLayout *layout) {
    UiTheme theme = ui_dark_theme();
    theme.canvas = layout->colors[0];
    theme.panel = layout->colors[1];
    theme.surface = layout->colors[2];
    theme.hover = layout->colors[3];
    theme.accent = layout->colors[4];
    theme.text = layout->colors[5];
    theme.muted = layout->colors[6];
    ui_set_theme(&theme);
}

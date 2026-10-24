/** @file @brief Color harmonies on the HSL hue circle. */
#include "palette.h"
#include <math.h>

int vector_palette_generate(uint32_t base, VectorHarmony harmony, float spread, uint32_t out[5]) {
    PUIHSLColor hsl = pui_color_to_hsl(base);
    spread = isfinite(spread) ? fmaxf(5, fminf(90, spread)) : 30;
    float offsets[5] = {0};
    int count = 5;
    switch (harmony) {
    case VECTOR_HARMONY_ANALOGOUS:
        offsets[1] = -spread;
        offsets[2] = spread;
        offsets[3] = -2 * spread;
        offsets[4] = 2 * spread;
        break;
    case VECTOR_HARMONY_TRIAD:
        count = 3;
        offsets[1] = 120;
        offsets[2] = 240;
        break;
    case VECTOR_HARMONY_TETRAD:
        count = 4;
        offsets[1] = spread;
        offsets[2] = 180;
        offsets[3] = 180 + spread;
        break;
    case VECTOR_HARMONY_COMPLEMENT:
        count = 2;
        offsets[1] = 180;
        break;
    case VECTOR_HARMONY_SPLIT:
        count = 3;
        offsets[1] = 180 - spread;
        offsets[2] = 180 + spread;
        break;
    case VECTOR_HARMONY_SQUARE:
        count = 4;
        offsets[1] = 90;
        offsets[2] = 180;
        offsets[3] = 270;
        break;
    default:
        break;
    }
    for (int i = 0; i < count; i++) {
        PUIHSLColor color = hsl;
        color.h = fmodf(hsl.h + offsets[i] + 720, 360);
        if (harmony == VECTOR_HARMONY_MONO && i > 0) {
            const float lightness[] = {0, .15f, .35f, .65f, .85f};
            color.l = lightness[i];
        }
        out[i] = i == 0 ? base : pui_color_from_hsl(color);
    }
    return count;
}

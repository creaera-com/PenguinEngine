/** @file @brief SDL-free tree hierarchy guide calculations. */
#include "core/internal.h"
#include <string.h>

bool ui_tree_continues(const int *depths, int count, int row, int level) {
    if (!depths || row < 0 || row >= count || level < 0 || level > depths[row]) {
        return false;
    }
    for (int i = row + 1; i < count; i++) {
        if (depths[i] < level) {
            return false;
        }
        if (depths[i] == level) {
            return true;
        }
    }
    return false;
}

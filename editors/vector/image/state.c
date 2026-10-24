/** @file @brief Bounded image metadata validation without file or rendering dependencies. */
#include "state.h"
#include <string.h>

bool vector_image_valid(const VectorImage *i) {
    return memchr(i->source, 0, sizeof i->source) && i->flip_x <= 1 && i->flip_y <= 1 &&
           (*i->source ? i->width > 0 && i->height > 0 && i->width <= 8192 && i->height <= 8192 &&
                             (unsigned long long)i->width * i->height <= 16777216
                       : !i->width && !i->height && !i->flip_x && !i->flip_y);
}

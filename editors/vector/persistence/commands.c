/** @file @brief Command type tags belong to persistence, not to an object model. */
#include "commands.h"
#include "editors/vector/artboard/artboard.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/order.h"
#include "editors/vector/rectangle/rectangle.h"
#include "editors/vector/rectangle/transform.h"

bool vector_command_write(const VectorCommand *command, FILE *file) {
    return vector_artboard_command_write(command, file) ||
           vector_rectangle_command_write(command, file) ||
           vector_rectangles_transform_write(command, file) ||
           vector_objects_command_write(command, file) || vector_order_command_write(command, file);
}

bool vector_command_read(VectorCommand *command, FILE *file) {
    unsigned kind;
    if (fread(&kind, sizeof kind, 1, file) != 1) {
        return false;
    }
    if (kind == 7) {
        return vector_order_command_read(command, file);
    }
    if (kind == 6) {
        return vector_objects_command_read(command, file);
    }
    if (kind == 5) {
        return vector_rectangles_transform_read(command, file);
    }
    return kind <= 2 ? vector_artboard_command_read(command, file, kind)
                     : vector_rectangle_command_read(command, file, kind);
}

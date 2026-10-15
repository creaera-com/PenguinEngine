#ifndef PENGUIN_DIALOGS_FIELDS_H
#define PENGUIN_DIALOGS_FIELDS_H

typedef enum {
    FIELD_NONE,
    FIELD_IMPORT,
    FIELD_TILESIZE,
    FIELD_LAYER,
    FIELD_X,
    FIELD_Y,
    FIELD_W,
    FIELD_H
} Field;

void editor_draw_modal(void);
void editor_field_start(Field f, const char *value);
void editor_field_stop(void);
#endif

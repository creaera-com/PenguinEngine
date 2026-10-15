#include "app/editor_internal.h"

bool editor_script_action(const char *name) {
    if (!strcmp(name, "script.check")) {
        if (app.script) {
            se_check(app.script);
            ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
            se_focus(app.script, app.window, true);
        }
        return true;
    }
    return false;
}

#include "editors/script/internal.h"

bool editor_script_action(const char *name) {
    if (!strcmp(name, "script.check")) {
        if (app.workspace.script) {
            se_check(app.workspace.script);
            ui_focus(&app.shell.ui, UI_SCRIPT_DOCUMENT);
            se_focus(app.workspace.script, app.platform.window, true);
        }
        return true;
    }

    return false;
}

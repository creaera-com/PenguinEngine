#include "editors/script/internal.h"

bool editor_script_action(const char *name) {
    if (!strcmp(name, "script.check")) {
        if (pe_script_active_instance()) {
            se_check(pe_script_active_instance());
            ui_focus(&app.shell.ui, UI_SCRIPT_DOCUMENT);
            se_focus(pe_script_active_instance(), app.platform.window, true);
        }
        return true;
    }

    return false;
}

/**
 * @file
 * @brief Privater Frontend-Host: gemeinsame Dienste und getrennte Zustandsbereiche; keine
 * Dokumentmodelle.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_EDITOR_HOST_H
#define PENGUIN_EDITOR_HOST_H

#include "application/run_state.h"
#include "editor/ids.h"
#include "editor/layout/runtime.h"
#include "editor/layout/ui_layout.h"
#include "editor/module.h"
#include "editor/registry.h"
#include "editor/workspace/session.h"
#include "editor/workspace/state.h"
#include "editor/workspace/workspace.h"
#include "penguin/render.h"
#include "penguin/ui_sdl.h"
#include "platform/window/state.h"
#include "platform/window/window.h"
#include "services/project/state.h"
#include "shell/console/view.h"
#include "shell/dialogs/fields.h"
#include "shell/explorer/file_tree.h"
#include "shell/shell.h"
#include "shell/state.h"
#include "shell/widgets.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Interne Layout-/Darstellungsvorgabe LEFT. */
#define LEFT ((int)app.shell.left_width)
/** @brief Interne Layout-/Darstellungsvorgabe RIGHT. */
#define RIGHT ((int)app.shell.right_width)
/** @brief Interne Layout-/Darstellungsvorgabe TOP. */
#define TOP 50
/** @brief Interne Layout-/Darstellungsvorgabe TABS. */
#define TABS 32
/** @brief Interne Layout-/Darstellungsvorgabe BOTTOM. */
#define BOTTOM 28
/** @brief Interne Layout-/Darstellungsvorgabe PANEL_GAP. */
#define PANEL_GAP (app.shell.gap)
/** @brief Interne Layout-/Darstellungsvorgabe GLOBAL_NAV_X. */
#define GLOBAL_NAV_X 232
/** @brief Interne Layout-/Darstellungsvorgabe SIDE_TOP. */
#define SIDE_TOP (TOP + TABS)
/** @brief Interne Layout-/Darstellungsvorgabe TREE_MAX. */
#define TREE_MAX 256

/** @brief Privater Composition Root; verbindet klar getrennte Zustandsbereiche. */
typedef struct {
    PeWindowState platform;     ///< Besitzender Zustandsbereich platform.
    PeProjectState project;     ///< Besitzender Zustandsbereich project.
    PeWorkspaceState workspace; ///< Besitzender Zustandsbereich workspace.
    PeRunState runtime;         ///< Besitzender Zustandsbereich runtime.
    PeShellState shell;         ///< Besitzender Zustandsbereich shell.
} Editor;

/* Private integration state. Engine and reusable UI never include this header. */
/** @brief Änderungszustand einer registrierten aktiven oder ausgelagerten Sitzung. */
bool editor_document_dirty(UiId id);
/** @brief Stabiler Index der drei Dokument-/Editor-Sitzungen. */
int editor_session_index(UiId id);
/** @brief Dirty-Zustand des aktiven oder ausgelagerten Vector-Dokuments. */
bool editor_vector_dirty(void);
/** @brief Private gemeinsame Editor-Anwendungsinstanz. */
extern Editor app;

/** @brief Registrierte Editoradapter; Reihenfolge ist keine Dokumentidentität. */
extern const EditorView *const editor_views[];

/** @brief Anzahl registrierter Editoradapter. */
extern const int editor_view_count;
/** @brief Geliehene Registrierung der Anwendung. */
extern const PeEditorRegistry editor_registry;

/** @brief Geliehener Tooltipzeiger für den aktuellen Frame. */
extern const char *editor_tooltip;

/** @brief Geliehener optionaler BMP-Ausgabepfad aus den Programmargumenten. */
extern const char *editor_screenshot_path;

/**
 * @brief Prüft, ob eine Map-Transaktion noch offen ist.
 */
bool editor_edit_pending(void);

/**
 * @brief Liefert die registrierte Editorbeschreibung; unbekannte IDs fallen auf Map zurück.
 */
const EditorView *editor_view(UiId id);

/**
 * @brief Prüft die ID des tatsächlich geladenen Editors auf Map.
 */
bool editor_map_active(void);
/** @brief Prüft den Play-Modus des aktiven Editors; false ohne Map-Payload. */
bool editor_playing(void);

/**
 * @brief Liest den Map-Änderungszustand aus Dokument oder entladenem Sitzungsstatus.
 */
bool editor_map_dirty(void);

/**
 * @brief Kopiert eine Statusmeldung in Shell und gegebenenfalls aktive Scriptansicht.
 */
void editor_message(const char *s);

/**
 * @brief Liest den Script-Änderungszustand aus Dokument oder entladenem Sitzungsstatus.
 */
bool editor_script_dirty(void);

/**
 * @brief Prüft die ID des tatsächlich geladenen Editors auf Scripts.
 */
bool editor_scripts_active(void);

/* File identity prevents equal line numbers in different modules sharing IDs. */
/** @brief Erzeugt eine auf Dateiname und Zeile basierende interne Button-ID. */
#define icon_button(...) editor_icon_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)
/** @brief Erzeugt eine auf Dateiname und Zeile basierende interne Button-ID. */
#define button(...) editor_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)

/** @} */
#endif

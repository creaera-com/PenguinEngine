/**
 * @file
 * @brief Private Modulverträge für src/editor/browser.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_BROWSER_FILE_TREE_H
#define PENGUIN_BROWSER_FILE_TREE_H
#include "penguin/engine.h"

/** @brief Ein sichtbarer Projektbaumeintrag mit projektbezogenem Pfad und Einrückungstiefe. */
typedef struct {
    char path[PE_PATH]; ///< Nullterminierter Datei- oder Projektpfad.
    char name[128];     ///< Nullterminierter Anzeigename.
    int depth;          ///< Einrückungstiefe im Projektbaum.
    bool directory;     ///< Ob der Eintrag ein Verzeichnis ist.
    bool expanded;      ///< Aufklappzustand bzw. Liste der aufgeklappten Projektpfade.
} TreeRow;

/**
 * @brief Zeichnet den Projektbaum und verarbeitet Datei- und Ordnerauswahl.
 */
void editor_draw_tree(void);

/**
 * @brief Prüft eine Dateiendung ohne Beachtung der ASCII-Groß-/Kleinschreibung.
 */
bool editor_suffix(const char *p, const char *s);

/**
 * @brief Liest den sichtbaren Projektbaum anhand der aufgeklappten Ordner erneut ein.
 */
void editor_tree_refresh(void);
/** @} */
#endif

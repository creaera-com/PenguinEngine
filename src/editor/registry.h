/** @file
 * @brief Geliehene Editorregistrierung ohne Abhängigkeit von konkreten Domänen.
 */
#ifndef PENGUIN_EDITOR_REGISTRY_H
#define PENGUIN_EDITOR_REGISTRY_H
#include "editor/module.h"

/** @brief Unveränderliche Liste; Beschreibungen und Zeiger leben länger als die Registrierung. */
typedef struct {
    const EditorView *const *views; ///< Geliehene Beschreibungen, keine Instanzdaten.
    size_t count;                   ///< Zahl gültiger Einträge.
} PeEditorRegistry;

/** @brief Sucht eine stabile ID; unbekannte IDs liefern NULL. */
const EditorView *pe_editor_registry_find(PeEditorRegistry registry, UiId id);
/** @brief Sucht einen YAML-Modulnamen; NULL und unbekannte Namen liefern NULL. */
const EditorView *pe_editor_registry_module(PeEditorRegistry registry, const char *module);
/** @brief Sucht den Besitzer eines Dokumentbefehls; 0 ist immer unregistriert. */
const EditorView *pe_editor_registry_request(PeEditorRegistry registry, int command);
/** @brief Liefert den Listenslot einer ID oder -1; Slots sind keine dauerhaften Identitäten. */
int pe_editor_registry_index(PeEditorRegistry registry, UiId id);
#endif

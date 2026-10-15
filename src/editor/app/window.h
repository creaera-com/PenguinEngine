/**
 * @file
 * @brief Fensterrahmen, Betriebssystem-Hit-Test und Fensteraktionen des Editors.
 * @ingroup editor_api
 */
#ifndef PENGUIN_EDITOR_WINDOW_H
#define PENGUIN_EDITOR_WINDOW_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/** @brief Aktiviert den eigenen Rahmen; behält bei fehlendem Hit-Test native Dekorationen. */
void editor_window_init(void);
/**
 * @brief Klassifiziert einen Punkt für Verschieben und Größenänderung.
 * @param x Horizontale Fensterkoordinate in Pixeln.
 * @param y Vertikale Fensterkoordinate in Pixeln.
 */
SDL_HitTestResult editor_window_hit_test(int x, int y);
/** @brief Linke Grenze der rechtsbündigen Fensterbuttons in Fensterkoordinaten. */
float editor_window_controls_left(void);
/** @brief Zeichnet und bedient die Fensterbuttons aus der Shell-YAML. */
void editor_window_draw_controls(void);
/**
 * @brief Führt eine window.*-Aktion aus; liefert false bei unbekannten Namen.
 * @param name Registrierter, nullterminierter Aktionsname.
 */
bool editor_window_action(const char *name);

#endif

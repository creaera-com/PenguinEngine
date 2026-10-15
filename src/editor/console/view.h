/**
 * @file
 * @brief Gemeinsame SDL-Terminalansicht und Shellservice der Anwendungshülle. */
#ifndef PENGUIN_CONSOLE_VIEW_H
#define PENGUIN_CONSOLE_VIEW_H
#include "console/terminal.h"
#include "penguin/ui_sdl.h"
/** @brief Kontextweit reservierte Scrollbar-ID der gemeinsamen Konsole. */
#define PE_CONSOLE_SCROLL_ID 0x10012u
/** @brief Besitzender Shellservice; lebt unabhängig von Map-/Scriptinstanzen. */
typedef struct PeConsole PeConsole;
/** @brief Erstellt den Service; spawn=false unterbindet Prozessstarts in GUI-Smoke-Tests. */
PeConsole *pe_console_new(const char *root, bool spawn);
/** @brief Beendet die Shell und gibt Terminalmodell sowie Ausgabeverlauf frei. */
void pe_console_free(PeConsole *console);
/** @brief Liest/schreibt pro Frame begrenzt und nichtblockierend zur Shell. */
void pe_console_tick(PeConsole *console);
/** @brief Markiert zu Framebeginn die Ansicht als unsichtbar; der Service läuft weiter. */
void pe_console_begin_frame(PeConsole *console);
/** @brief Zeichnet Terminalzellen, Auswahl, Cursor und Status im YAML-Bereich. */
void pe_console_draw(PeConsole *console, SDL_Renderer *renderer, UiContext *ui, SDL_FRect area,
                     bool enabled);
/** @brief Verarbeitet fokussierte Eingaben; true verhindert Dokument-/Map-Tastenkürzel. */
bool pe_console_event(PeConsole *console, SDL_Window *window, const SDL_Event *event, bool enabled);
/** @brief Prüft den Tastaturfokus der Konsole. */
bool pe_console_focused(const PeConsole *console);
/** @brief Liefert das geliehene VT-Modell für Integrationsprüfungen. */
PeTerminal *pe_console_terminal(PeConsole *console);
/** @brief Liefert den zuletzt gezeichneten Zellbereich in Fensterpixeln. */
SDL_FRect pe_console_area(const PeConsole *console);
#endif

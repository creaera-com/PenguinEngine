/**
 * @file
 * @brief Begrenzter, SDL-unabhängiger VT-Terminalzustand mit Scrollback. */
#ifndef PENGUIN_TERMINAL_H
#define PENGUIN_TERMINAL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/** @brief Maximale Anzahl gespeicherter Ausgabezeilen. */
#define PE_TERMINAL_HISTORY 512
/** @brief Maximaler noch nicht an die Shell geschriebener Eingabepuffer. */
#define PE_TERMINAL_OUTPUT 65536
/** @brief Besitzender VT-Zustand; mit pe_terminal_free freigeben. */
typedef struct PeTerminal PeTerminal;

/** @brief Gerenderte Terminalzelle mit bis zu sechs Unicode-Codepoints. */
typedef struct {
    uint32_t chars[6]; ///< Grundzeichen und kombinierende Zeichen; 0 beendet die Folge.
    uint32_t fg;       ///< Vordergrundfarbe als RGBA.
    uint32_t bg;       ///< Hintergrundfarbe als RGBA.
    int width;         ///< Zellbreite, 0 für Fortsetzung eines breiten Zeichens.
    bool bold;         ///< Fettattribut.
    bool underline;    ///< Unterstreichungsattribut.
} PeTerminalCell;

/** @brief Terminaltasten; F1..F12 beginnen bei PE_TERM_F1. */
typedef enum {
    PE_TERM_ENTER,
    PE_TERM_TAB,
    PE_TERM_BACKSPACE,
    PE_TERM_ESCAPE,
    PE_TERM_UP,
    PE_TERM_DOWN,
    PE_TERM_LEFT,
    PE_TERM_RIGHT,
    PE_TERM_INSERT,
    PE_TERM_DELETE,
    PE_TERM_HOME,
    PE_TERM_END,
    PE_TERM_PAGEUP,
    PE_TERM_PAGEDOWN,
    PE_TERM_F1
} PeTerminalKey;

/** @brief Erzeugt ein UTF-8-Terminal mit 1..100 Zeilen und 2..240 Spalten. */
PeTerminal *pe_terminal_new(int rows, int cols);
/** @brief Gibt VT-Bildschirm, Eingabepuffer und Scrollback frei; NULL ist erlaubt. */
void pe_terminal_free(PeTerminal *terminal);
/** @brief Übergibt Shellbytes an den VT-Parser; inklusive ANSI-/OSC-/Alternativbildschirm. */
void pe_terminal_feed(PeTerminal *terminal, const char *bytes, size_t length);
/** @brief Ändert die begrenzte Zellgeometrie und bewahrt den VT-Zustand. */
bool pe_terminal_resize(PeTerminal *terminal, int rows, int cols);
/** @brief Gibt die aktuelle Zeilenzahl zurück. */
int pe_terminal_rows(const PeTerminal *terminal);
/** @brief Gibt die aktuelle Spaltenzahl zurück. */
int pe_terminal_cols(const PeTerminal *terminal);
/** @brief Liest eine sichtbare Zelle unter Berücksichtigung des Scrolloffsets. */
bool pe_terminal_cell(PeTerminal *terminal, int row, int col, PeTerminalCell *cell);
/** @brief Liest Cursorposition und Sichtbarkeit; im Scrollback ist der Cursor verborgen. */
bool pe_terminal_cursor(PeTerminal *terminal, int *row, int *col);
/** @brief Scrollt um Zeilen; positive Werte bewegen sich in die Vergangenheit. */
void pe_terminal_scroll(PeTerminal *terminal, int lines);
/** @brief Gibt die Anzahl gespeicherter Scrollbackzeilen zurück. */
int pe_terminal_history(const PeTerminal *terminal);
/** @brief Gibt den Abstand zur neuesten Ausgabe in Zeilen zurück. */
int pe_terminal_scroll_offset(const PeTerminal *terminal);
/** @brief Codiert eine Sondertaste; Modifierbits: Shift=1, Alt=2, Ctrl=4. */
void pe_terminal_key(PeTerminal *terminal, PeTerminalKey key, unsigned modifiers);
/** @brief Codiert einen Unicode-Codepoint mit Modifierbits, etwa Ctrl+C. */
void pe_terminal_unichar(PeTerminal *terminal, uint32_t character, unsigned modifiers);
/** @brief Puffert Pastebytes mit VT-Bracketed-Paste, falls vom Shellprogramm aktiviert.
 * @return false, falls die gesamte Eingabe nicht in den begrenzten Puffer passt.
 */
bool pe_terminal_paste(PeTerminal *terminal, const char *text, bool bracketed);
/** @brief Liefert geliehene noch ausstehende Shellbytes und deren Länge. */
const char *pe_terminal_output(const PeTerminal *terminal, size_t *length);
/** @brief Entfernt tatsächlich an die Shell geschriebene Bytes aus dem Eingabepuffer. */
void pe_terminal_consume(PeTerminal *terminal, size_t length);
/** @brief Meldet und löscht einen Eingabepufferüberlauf. */
bool pe_terminal_overflow(PeTerminal *terminal);
/** @brief Kopiert eine inklusive sichtbare Zellenauswahl als UTF-8; Ergebnis mit free freigeben. */
char *pe_terminal_copy(PeTerminal *terminal, int row0, int col0, int row1, int col1);
/** @brief Codiert einen Unicode-Codepoint nach UTF-8; buffer benötigt vier Bytes. */
size_t pe_terminal_utf8(uint32_t codepoint, char *buffer);
#endif

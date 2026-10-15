/**
 * @file
 * @brief Plattformvertrag für eine interaktive Shell an einem Pseudoterminal. */
#ifndef PENGUIN_TERMINAL_PROCESS_H
#define PENGUIN_TERMINAL_PROCESS_H
#include <stdbool.h>
#include <stddef.h>
/** @brief Besitzender, plattformspezifischer Shellprozess; nur im Aufruferthread verwenden. */
typedef struct PeTerminalProcess PeTerminalProcess;
/** @brief Startet shell (absoluter Pfad oder NULL für die Standardshell) im Projektverzeichnis.
 * @return Besitzender Prozess oder NULL mit Fehlertext; Windows benötigt künftig einen
 * ConPTY-Adapter.
 */
PeTerminalProcess *pe_terminal_process_start(const char *root, const char *shell, int rows,
                                             int cols, char *error, size_t size);
/** @brief Liest ohne Blockieren; >0 Bytes, 0 momentan keine Daten, -1 Ende/Fehler. */
int pe_terminal_process_read(PeTerminalProcess *process, char *buffer, size_t capacity);
/** @brief Schreibt ohne Blockieren; >0 Bytes, 0 momentan voll, -1 Fehler. */
int pe_terminal_process_write(PeTerminalProcess *process, const char *bytes, size_t length);
/** @brief Aktualisiert die PTY-Fenstergröße und benachrichtigt das Vordergrundprogramm. */
bool pe_terminal_process_resize(PeTerminalProcess *process, int rows, int cols);
/** @brief Prüft und reapet den eigenen Shellprozess; wartet nicht auf dessen Ende. */
bool pe_terminal_process_running(PeTerminalProcess *process);
/** @brief Beendet Shell/Vordergrundgruppe, reapet die Shell und gibt die Prozessdaten frei. */
void pe_terminal_process_free(PeTerminalProcess *process);
#endif

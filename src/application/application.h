/** @file
 * @brief Öffentlicher Startpunkt der Editor-Anwendung. */
#ifndef PENGUIN_APPLICATION_H
#define PENGUIN_APPLICATION_H
/** @brief Startet die Anwendung; besitzt Initialisierung, Hauptschleife und Freigabe.
 * @param argc Anzahl der Programmargumente.
 * @param argv Geliehene UTF-8-Programmargumente für die gesamte Laufzeit.
 * @return Prozess-Exitcode, 0 bei Erfolg.
 */
int pe_application_run(int argc, char **argv);
#endif

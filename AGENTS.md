# Vorgaben für die Weiterentwicklung

## Fachliche Trennung beibehalten

Diese Architektur ist eine ausdrückliche Projektvorgabe. Sie gilt für neue Funktionen,
Refactorings und Fehlerkorrekturen:

- Code zuerst nach fachlichem Bereich und darin nach Verantwortung organisieren.
  Zusammengehörige Daten, Operationen und Schnittstellen bleiben nah beieinander.
- Keine globalen Sammelverzeichnisse wie `actions/` oder `functions/`, die eine fachliche
  Funktion über mehrere unverbundene Bereiche verteilen. Keine Datei pro Einzelfunktion.
- Die fachliche Dokumentverwaltung enthält keine Buttons, Tabs, Statusmeldungen oder
  native Eingabeverarbeitung. UI-Anbindungen übersetzen Ereignisse in fachliche Operationen.
- Modelle und History dürfen keine UI-Anbindung, Editorinstanz oder Host-Interna einbinden.
  Reine Zustands- und Geometrieheader sind zulässig; sie dürfen keine UI-Verarbeitung einschleusen.
- Werkzeuge halten vorläufige Interaktionszustände. Abgeschlossene, rückgängig machbare
  Dokumentänderungen laufen über fachliche Änderungsbefehle und die History.
- Rendering liest den Dokumentzustand. Persistenz und temporäre Checkpoints speichern
  Werte und rekonstruieren Ressourcen; sie speichern keine Heap- oder Funktionszeiger.
- `editor.c` übernimmt Lebenszyklus und Weiterleitung. Fachliche Implementierungen gehören
  in die zuständigen Bereiche und werden nicht wieder in dieser Datei gesammelt.
- Editor-Module kommunizieren mit dem Host über den gemeinsamen Adapter-Vertrag.
  Nur die Registry kennt alle konkreten Adapter. Editoren greifen nicht auf Interna
  anderer Editoren zu.
- pengUI bleibt eine eigenständige Bibliothek ohne Abhängigkeiten von Apps oder Editoren.
- Gemeinsamen Code nur dann auslagern, wenn er tatsächlich fachlich unabhängig ist.
  Keine unspezifischen Sammeldateien oder `shared`-Ordner als Ablage für unklaren Code.

## Orientierung im Vektoreditor

`editors/vector/` ist das aktuelle Beispiel für die Aufteilung:

- `document/`: Dokumentdaten, IDs und Speicherbesitz.
- `artboard/`: Artboards, Änderungsbefehle und zugehöriges Mauswerkzeug.
- `workspace/`: Zeichenfläche, Lineale, Anbindung der Dokument-Tabs und gemeinsame
  Aufzieh-Geste für Rechtecke/Ellipsen (`box_tool.c`, reine Werte in `box_tool_state.h`).
- `ellipse/`: Ellipsen-Geometrie und Erzeugung; keine eigene Kopie der Mausverarbeitung.
- `history/`: Ausführung von Befehlen sowie Undo/Redo.
- `persistence/`: Temporäres Sichern und Wiederherstellen.
- `ui/`: YAML-Beschreibungen und UI-Anbindungen nach Bereichen:
  `inspector/`, `document_tree/`, `toolbar/`, `tools/`, `context_menu/`.
  C-Dateien nicht flach in `ui/` ablegen. `ui/editor.yaml` bleibt der Layout-Einstieg.

Andere Editoren nach ihren tatsächlichen Aufgaben gliedern. Leere Verzeichnisse und
Dateien nicht vorsorglich anlegen und diese Struktur nicht mechanisch kopieren.

## Änderungen prüfen

- Bei Verschiebungen Includes, CMake-Quellen, Tests und Dokumentationsverweise anpassen.
- `python3 tools/check_architecture.py` ausführen. Neue fachliche Grenzen bei Bedarf
  dort absichern; bestehende Regeln nicht aufweichen, um eine Abhängigkeit zu erlauben.
- Zur Änderung passende Tests ausführen. Reine Markdown-Änderungen benötigen keine
  erneuten Programmtests; lokale Verweise und `git diff --check` prüfen.
- Bei Refactorings ist kein Screenshot nötig. Bei sichtbaren UI-Änderungen einen
  aktuellen Screenshot erstellen und zeigen.

Weitere Erläuterungen: [Editor-Architektur](docs/EDITOREN.md),
[Programmfluss und Datenmodell](docs/VEKTOR_PROGRAMMFLUSS_UND_DATENMODELL.md) und
[pengUI-Architektur](docs/ARCHITEKTUR.md).

# Oberfläche des Vektoreditors

Die UI-Dateien sind nach sichtbaren Bereichen geordnet. Fachliche Operationen bleiben
in `object/`, `artboard/`, `curve/` usw.; die Dateien hier verbinden Widgets mit diesen
Operationen. YAML und die zugehörige C-Anbindung liegen möglichst nah zusammen.

| Bereich | Dateien und Aufgabe |
| --- | --- |
| `editor.yaml` | Einstieg: Spalten, Tabs, Toolbar, radiales Menü und Tools-Panel. |
| `inspector/inspector.c/.h` | Auswahl auswerten, Property-Panel zusammensetzen und Aktionen weiterleiten. |
| `inspector/inspector.yaml` | Äußerer Inspector mit Innenabstand und `selection_properties`-Slot. |
| `inspector/artboard.yaml` | Eigenschaften eines Artboards. Die Anbindung liegt in `inspector.c`. |
| `inspector/shape.c/.h` und `shape.yaml` | Gemeinsame Eigenschaften von Rechtecken, Polygonpfaden und Bézierkurven. |
| `inspector/selection.yaml` | Anzeige einer Mehrfachauswahl. |
| `document_tree/document_tree.c/.h` | Dokumentdaten in Tree-Zeilen übersetzen, Auswahl und Objektstatus anbinden. |
| `document_tree/groups.c/.h` | Gruppenzeilen identifizieren und ihre Auswahl weiterleiten. |
| `toolbar/toolbar.c/.h` | Aktiven Werkzeugzustand mit den Toolbar-Buttons synchronisieren. |
| `tools/grid.c/.h` | Raster-Einstellungen im Tools-Panel validieren und synchronisieren. |
| `context_menu/object_actions.c/.h` | Kontextmenü, Clipboard und Objekt-Tastenkürzel anbinden. |
| `context_menu/path_actions.c/.h` | Verfügbarkeit und Aktionen der Flächenoperationen. |
| `context_menu/orientation_actions.c/.h` | Spiegeln und 90-Grad-Drehungen anbinden. |

## Typische Änderungen

- **Weniger Rand im Inspector:** `inspector/inspector.yaml`, `padding` am äußeren Panel.
- **Formularfelder ändern:** `inspector/artboard.yaml` oder `inspector/shape.yaml`.
- **Toolbar-Button oder Menüeintrag ergänzen:** `editor.yaml`; die zugehörige Aktion
  in der zuständigen UI-Anbindung weiterleiten, den Fachbefehl im jeweiligen Domänenmodul umsetzen.
- **Raster-Einstellung ergänzen:** Beschreibung in `editor.yaml`, Anbindung in `tools/grid.c`.
- **Neue Tree-Zeile oder Zellaktion:** `document_tree/`.

`editor.yaml` bleibt bewusst die zentrale Layoutbeschreibung. pengUI lädt die
Inspector-Dateien über die bestehende C-Komposition mit Slots; es gibt keine impliziten
Datei-Includes. Bestehende Widget-IDs, Aktionsnamen und C-Schnittstellen bleiben erhalten.
Die historischen `VectorRectangleInspector`-Namen beschreiben weiterhin den gemeinsamen
Form-Inspector; die Datei heißt nun entsprechend ihrer Zuständigkeit `shape.c`.

Neue C-Dateien gehören in den passenden Bereich statt direkt in `ui/`.
Weitere Zusammenhänge stehen in der [Editorarchitektur](../../../docs/EDITOREN.md).

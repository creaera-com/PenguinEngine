# Oberfläche des Vektoreditors

Die UI-Dateien sind nach sichtbaren Bereichen geordnet. Fachliche Operationen bleiben
in `object/`, `artboard/`, `curve/` usw.; die Dateien hier verbinden Widgets mit diesen
Operationen. YAML und die zugehörige C-Anbindung liegen möglichst nah zusammen.

| Bereich | Dateien und Aufgabe |
| --- | --- |
| `editor.yaml` | Einstieg: Spalten, Tabs, Toolbar, radiales Menü und Tools-Panel. |
| `inspector/inspector.c/.h` | Auswahl auswerten, Property-Panel zusammensetzen und Aktionen weiterleiten. |
| `inspector/inspector.yaml` | Äußerer Inspector mit Innenabstand und `selection_properties`-Slot. |
| `inspector/artboard/artboard.yaml` | Eigenschaften eines Artboards. Die Anbindung liegt im selben Unterverzeichnis in `artboard.c`. |
| `inspector/object/object.c/.h` und `object.yaml` | Gemeinsame Eigenschaften von Rechtecken, Polygonpfaden und Bézierkurven. |
| `inspector/text/text.c/.h` und `text.yaml` | Optionaler Typografie-Popup: Schriftwahl, Abstaende, Ausrichtung und Spalten. |
| `inspector/selection/selection.yaml` | Anzeige einer Mehrfachauswahl. |
| `document_tree/document_tree.c/.h` | Dokumentdaten in Tree-Zeilen übersetzen, Auswahl und Objektstatus anbinden. |
| `document_tree/groups.c/.h` | Gruppenzeilen identifizieren und ihre Auswahl weiterleiten. |
| `toolbar/toolbar.c/.h` | Aktiven Werkzeugzustand mit den Toolbar-Buttons synchronisieren. |
| `tools/grid.c/.h` | Raster-Einstellungen im Tools-Panel validieren und synchronisieren. |
| `context_menu/object_actions.c/.h` | Kontextmenü, Clipboard und Objekt-Tastenkürzel anbinden. |
| `context_menu/path_actions.c/.h` | Verfügbarkeit und Aktionen der Flächenoperationen. |
| `context_menu/orientation_actions.c/.h` | Spiegeln und 90-Grad-Drehungen anbinden. |

## Typische Änderungen

- **Weniger Rand im Inspector:** `inspector/inspector.yaml`, `padding` am äußeren Panel.
- **Formularfelder ändern:** `inspector/artboard/artboard.yaml` oder `inspector/object/object.yaml`.
- **Toolbar-Button oder Menüeintrag ergänzen:** `editor.yaml`; die zugehörige Aktion
  in der zuständigen UI-Anbindung weiterleiten, den Fachbefehl im jeweiligen Domänenmodul umsetzen.
- **Raster-Einstellung ergänzen:** Beschreibung in `editor.yaml`, Anbindung in `tools/grid.c`.
- **Neue Tree-Zeile oder Zellaktion:** `document_tree/`.

`editor.yaml` bleibt bewusst die zentrale Layoutbeschreibung. pengUI lädt die
Inspector-Dateien über die bestehende C-Komposition mit Slots; es gibt keine impliziten
Datei-Includes. Bestehende Widget-IDs und Aktionsnamen bleiben erhalten.
Der gemeinsame Form-Inspector heißt `VectorObjectInspector`. Historische Widget-IDs
und Aktionsnamen mit `rectangle` bleiben unverändert, damit bestehende YAML-Dateien
und gespeicherte UI-Zustände weiterhin funktionieren.

## Inspector intern

- `inspector.c/.h`: gemeinsame Schnittstelle, Auswahl auswerten und Panels wechseln.
- `lifecycle.c`: Layouts laden, zusammensetzen und vollständig freigeben; auch bei
  einem Fehler nach teilweise erfolgreichem Laden. Schrift-Service und Text-Renderer
  werden ausgeliehen und weiterhin vom Editor verwaltet.
- `artboard/`: Name, Größe und Ebenenstatus des Artboards.
- `selection/`: Zusammenfassung bei Mehrfachauswahl.
- `object/`: gemeinsame Objektfelder und Einhängen der optionalen Abschnitte.
- `appearance/fill/`: einfarbige Füllung, Live-Vorschau und Verläufe.
- `appearance/stroke/`: Konturbreite, Farbe und Ausrichtung.
- `appearance/effects/`: Schatten-Einstellungen.
- `rectangle/`: Eckenradius; `polygon/`: parametrische Eckenzahl.
- `text/`: Typografie und Schriftensuche; `image/`: Bildformular.

C-Anbindung, Header und YAML liegen beim jeweiligen Bereich. Bereiche ohne eigene
Logik (aktuell `image/`) benötigen nur YAML. Der Editor verwendet ausschließlich
`vector_inspector_init()`, `vector_inspector_destroy()` und die gemeinsame
Routing-Schnittstelle. Er lädt keine einzelnen Inspector-Formulare mehr.
Fachliche Änderungen bleiben in den bestehenden Domänenbefehlen und der History.

Neue C-Dateien gehören in den passenden Bereich statt direkt in `ui/`.
Weitere Zusammenhänge stehen in der [Editorarchitektur](../../../docs/EDITOREN.md).

# 1:1-Vorschau

Die **Vorschau** steht in einem eigenen Tab neben **Klonen** und **Raster** im unteren Bereich der rechten Spalte.
Mit **Bereich wählen** oder dem Scan-Icon in der Zeichen-Toolbar das Werkzeug aktivieren,
dann mit der linken Maustaste einen Bereich innerhalb eines Artboards aufziehen.
Der Bereich ist auf dieses Artboard begrenzt und wird auf der Zeichenfläche gestrichelt
markiert. Nach dem Loslassen kehrt das Auswahlwerkzeug zurück.

Die Vorschau zeigt einen Dokument-Pixel pro UI-Pixel, unabhängig vom Zoom der
Zeichenfläche. Kleine Bereiche werden zentriert und nicht vergrößert. Ist die Auswahl
größer als das Vorschaufenster, wird nur ein Ausschnitt gezeichnet: Er folgt dem
Mittelpunkt der Zeichenansicht, bleibt aber innerhalb der gewählten Region. Zum
Verschieben der Zeichenansicht die mittlere Maustaste verwenden. Bei einem Display
mit Betriebssystem-Skalierung gilt die Pixelgröße im Koordinatensystem der Anwendung.

Text, Rahmen, Deckkraft und Schnittmasken werden mit denselben Renderern wie auf
der Zeichenfläche dargestellt. Raster, Lineale, Greifer, Textcursor und andere
Bearbeitungshilfen erscheinen in der Vorschau nicht. Laufende Objekttransformationen
und Farbänderungsvorschauen werden übernommen.

- **Esc**, Rechtsklick, erneutes Betätigen des Auswahlbuttons oder ein anderes
  Zeichenwerkzeug brechen die neue Bereichsauswahl ab. Der bisherige Bereich bleibt erhalten.
- Das **X** neben „Bereich wählen“ entfernt die Vorschauauswahl.
- Jeder Dokument-Tab hält seinen eigenen Bereich. Beim Editorwechsel bleibt er über
  den temporären Sitzungs-Checkpoint erhalten; unfertige Auswahlgesten werden verworfen.
- Der Bereich ist Ansichtszustand und erzeugt keine Dokument-/Undo-Änderung. Er wird
  wie Zoom und Raster nicht in der `.pengvec`-Datei gespeichert.
- Wird das Artboard verkleinert, wird der Bereich für die Anzeige begrenzt. Nach Undo
  ist der ursprüngliche Ausschnitt wieder verfügbar. Gelöschte/ausgeblendete Artboards
  liefern keine Vorschau; ihre Region bleibt für ein mögliches Undo vorgemerkt.
- Die Vorschau nutzt die verfügbare Tab-Fläche; das Raster besitzt ebenfalls einen eigenen Tab. Die Höhe des unteren Panels bleibt
  über den vorhandenen horizontalen Greifer veränderbar.

## Aufteilung

- `preview/state.h`, `preview/region.c`: reine Werte, Validierung und Berechnung des
  gleich großen Quell-/Zielausschnitts; keine SDL- oder UI-Verarbeitung.
- `preview/tool.c`: YAML-Aktionen, Mausgeste und Synchronisation der Werkzeugbuttons.
- `preview/render.c`: 1:1-Ansicht und gestrichelte Markierung. Ein kurzlebiger Wert-Snapshot
  enthält die angezeigte Geometrie; das Originaldokument und sein Zoom bleiben unverändert.
  Es wird nur in das sichtbare Vorschaufenster gezeichnet, kein riesiges Artboardbild erzeugt.
- `document/document.h`: besitzt den Vorschaubereich pro Dokument.
- `ui/editor.yaml`: beschreibt Werkzeugbutton, Vorschau-Tab und Canvas
  `vector_preview_canvas`; deren feste Höhe lässt sich hier ändern.
- `editor.c`: leitet Aktionen, Events und die zusätzliche Canvas-Zeichenanfrage weiter.
- `persistence/checkpoint.c`: Schema **PVCKPT29** sichert ausschließlich den bestätigten
  Bereich samt Artboard-ID; dauerhafte Dokumentdateien bleiben **PENGVEC Version 6**.

Die Aktionen heißen `vector.preview.select` und `vector.preview.clear`. Die Vorschau
kann deshalb auch an anderer Stelle in der YAML-Oberfläche angeordnet werden.

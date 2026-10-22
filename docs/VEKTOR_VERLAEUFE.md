# Flächenfüllungen und Verläufe

Rechtecke, Ellipsen, Polygone und geschlossene Pfade können einfarbig oder mit
einem Verlauf gefüllt werden. Offene Kurven bleiben Linien. Die Rahmenfarbe
bleibt eine eigenständige Einstellung.

## Bedienung

1. Ein Objekt mit dem Auswahlwerkzeug oder über den Dokumentbaum auswählen.
2. Im Inspector **Füllung / Verlauf** öffnen und den Typ wählen:
   - **Einfarbig:** verwendet die bisherige Füllfarbe unter **Einstellungen → Füllung**.
   - **Linear:** Verlauf vom ersten zum zweiten Griff, außerhalb werden die Randfarben fortgesetzt.
   - **Radial:** erster Griff ist das Zentrum, zweiter Griff bestimmt den Kreisradius.
   - **Konisch:** Farbkreis um den ersten Griff; der zweite bestimmt den Ausgangswinkel.
     Unterschiedliche erste/letzte Farben erzeugen am Ausgangswinkel eine sichtbare Naht.
   - **Reflektiert:** linearer Verlauf symmetrisch zu beiden Seiten des ersten Griffs.
3. Das Verlaufswerkzeug in der Toolbar oder **Auf Leinwand bearbeiten** aktivieren.
4. Auf dem ausgewählten Objekt eine Strecke aufziehen oder die Endpunkte ziehen.
   Andere Objekte lassen sich im Werkzeug zunächst durch Anklicken auswählen.
5. Innere Farbstopps direkt auf der Linie verschieben. Anfangs-/Endgriffe ändern
   die Geometrie des Verlaufs. **Esc** oder Rechtsklick verwirft die aktuelle
   Geste und beendet das Werkzeug.

Der Verlauf ändert sich während des Ziehens live. Erst das Loslassen erzeugt
einen History-Schritt; Zwischenstände verändern das dauerhafte Dokument nicht.
Sperren von Objekt oder Artboard verhindern Änderungen.

## Farbstopps

Ein Verlauf enthält zwei bis acht Stopps mit eigener Farbe und Deckkraft. Über die
Pfeile neben **Farbstopp** wird der bearbeitete Stopp ausgewählt. **Stopp +** fügt
im größten noch freien Farbabstand einen Stopp mit der dortigen Verlaufsfarbe ein.
**Stopp −** entfernt den ausgewählten Stopp, solange mindestens zwei bleiben.

Die Positionsangabe ist in Prozent. Stopps bleiben streng aufsteigend geordnet;
der erste liegt bei 0 %, der letzte bei 100 %. Innere Stopps können zwischen ihren
Nachbarn verschoben werden. Ungültige Eingaben werden auf den bestehenden Wert
zurückgesetzt. **Farben umkehren** spiegelt Positionen und Farben der Stoppliste.

Die Füllfarbe unter **Einstellungen** ist auch für geschlossene Bezierkurven und
Pfade verfügbar. Das Bestätigen dieser Farbe aktiviert eine einfarbige Füllung.
Der zuvor konfigurierte Verlauf bleibt erhalten und kann über die Typauswahl
wieder aktiviert werden. Alle bestätigten Änderungen unterstützen Undo/Redo.

## Transformationen und bestehende Funktionen

Die Endpunkte liegen relativ im ungedrehten Objektkoordinatensystem. Beim
Verschieben, Skalieren und Drehen folgt der Verlauf dem Objekt; die Berechnung
von Richtung und Radius berücksichtigt dessen tatsächliche Breite und Höhe.
Spiegeln spiegelt die Füllung einschließlich konischer Farbrichtung.

Die tatsächlichen Konturen und Löcher begrenzen den Verlauf. Schnittmasken,
Vorschau und Schlagschatten verwenden dieselben Füllwerte. Insbesondere bestimmen
transparente Stopps auch die Alpha-Silhouette des Schattens. Verknüpfte Kopien
übernehmen die aktuellen Stopps ihres Originals; die H/S/L/A-Schritte des
Wiederholungswerkzeugs verändern dabei alle Stoppfarben.

## Architektur und Programmfluss

- `gradient/state.h` und `state.c`: reine Werte, Validierung, Stopplisten und
  Farbauswertung ohne SDL-/UI-Abhängigkeiten. RGB wird mit Alpha gewichtet
  interpoliert, damit transparente Stopps keine fremden Farbsäume erzeugen.
- `gradient/tool_state.h`: temporärer Gestenzustand pro Dokument.
- `gradient/tool.c`: Auswahl und Bildschirm-/Objektkoordinaten, Maus-Capture,
  Vorschau sowie ein `vector_shape_update` beim Loslassen.
- `gradient/render.c`: erzeugt eine auf sichtbaren Bereich begrenzte Verlaufstextur
  und zeichnet sie durch tessellierte Formkonturen. Teilweise sichtbare Dreiecke
  werden vor dem Texturieren zugeschnitten; Löcher bleiben ausgespart.
- `ui/inspector/appearance/fill/gradient.yaml`: eigenständiger Formularbaustein. Der Inspector
  lädt ihn über den Slot `object_gradient` nur für geschlossene geometrische Formen.
- `ui/inspector/appearance/fill/gradient.c`: Formularaktionen → validierter Formwert → bestehende
  atomare Shape-History. Die Auswahl des gerade bearbeiteten Stopps ist nur UI-Zustand.
- `rectangle/render.c`: zeichnet die Verlaufsfüllung vor Rahmen und Auswahlkontur.
  Derselbe Ablauf wird in Schatten-Silhouetten, Masken und Vorschau verwendet.

Große Texturen werden auf maximal 2048 Pixel je Achse reduziert, ohne den Verlauf
im Dokumentkoordinatensystem zu verändern. Die erste Implementierung berechnet
sichtbare Verläufe auf der CPU neu; ein dauerhafter Texturcache und GPU-Auswertung
sind noch nicht enthalten. Viele große Verlaufsflächen können Rechenzeit benötigen.

## Speicherung

`VectorShape.gradient` enthält ausschließlich Werte. Deshalb bleiben Verläufe in
Clipboard, Duplikaten, History und Editor-Checkpoints erhalten. Während einer
Mausgeste bleibt der vorläufige Verlauf getrennt vom gespeicherten Modell.

**PENGVEC Version 6** ergänzt nach den Schattenwerten jedes Objekts:

- Typ und Stoppanzahl als `u32`.
- Anfang X/Y und Ende X/Y als vier `f32`, relativ zum Objekt.
- Je Stopp Position (`f32`) und Farbe (`u32`, `0xRRGGBBAA`).

Auch deaktivierte Verläufe können konfigurierte Stopps behalten. Leere Verläufe
haben Typ 0 und Anzahl 0. Versionen 1–5 bleiben lesbar und erhalten eine einfarbige
Füllung ohne Verlauf. Ältere Programmversionen können Version 6 nicht öffnen.
Der private Sitzungs-Checkpoint heißt **PVCKPT29** und erhält Werte samt History
beim Editorwechsel; Gesten und Werkzeugmodus werden nicht wiederhergestellt.

# Ebeneneffekte: Schlagschatten

Ein einzelnes Objekt kann einen nicht destruktiven Schlagschatten erhalten. Die
Geometrie bleibt editierbar. Im Inspector den Abschnitt **Effekte** öffnen und
**Schlagschatten** aktivieren.

- **Versatz X/Y:** in Dokumentpixeln, positiv nach rechts/unten; jeweils −4096 bis 4096.
- **Weichzeichnung:** 0 bis 128 Dokumentpixel; 0 erzeugt einen scharfen Schatten.
- **Ausbreitung:** erweitert die sichtbare Silhouette um 0 bis 64 Dokumentpixel.
- **Farbe / Deckkraft:** derselbe Farbwähler wie bei Füllung und Rahmen. Sein
  Alpha-Wert steuert die Deckkraft.

Beim ersten Aktivieren werden ein schwarzer Schatten mit 35 % Deckkraft,
8 px Versatz pro Achse und 12 px Weichzeichnung verwendet. Ausschalten behält
konfigurierte Werte. Feldänderungen werden beim Bestätigen oder Fokuswechsel
übernommen; der Farbwähler bestätigt nach Abschluss seiner Änderung.
Jede bestätigte Änderung ist mit Undo/Redo rückgängig zu machen. Gesperrte
Objekte und Objekte auf gesperrten Artboards können nicht verändert werden.

## Darstellung

Der Schatten folgt der tatsächlich gezeichneten Alpha-Silhouette, einschließlich
Rahmen, Löchern in Pfaden, Text und transparenter Bildbereiche. Auswahlrahmen,
Greifer und Textcursor gehören nicht dazu. Er wird direkt hinter seinem Objekt
gezeichnet und unterliegt dessen Ebenenreihenfolge und Schnittmaske. Artboards
begrenzen weiterhin die sichtbare Fläche.

Zoom, Vorschau und verknüpfte Wiederholungen verwenden dieselbe Darstellung.
Jede virtuelle Instanz übernimmt die Schattenwerte ihres Originals. Effekte sind
an einzelnen Objekten gespeichert; Gruppen haben noch keinen gemeinsamen Effekt.

## Aufbau und Datenfluss

- `effects/shadow.h` und `shadow.c`: reine Werte, Grenzen und Standardwerte.
- `effects/render.c`: zeichnet die Objekt-Alphaform in ein begrenztes temporäres
  Renderziel, erweitert sie, glättet sie mit drei separaten Boxfilter-Pässen,
  färbt sie und zeichnet den Schatten. Alle SDL-Ressourcen bleiben beim Rendering.
- `ui/inspector/appearance/effects/effects.yaml`: wiederverwendbare Formularbeschreibung, über einen
  Slot sowohl in den Form- als auch in den Bild-Inspector eingebunden.
- `ui/inspector/appearance/effects/effects.c`: übersetzt Formulareingaben in `vector_shape_update`.
- `object/commands.c`: bestehender atomarer Vorher/Nachher-Befehl samt History.
- `rectangle/render.c`: ruft den Effekt vor dem normalen Objektinhalt auf; die
  vorhandenen Form-, Text- und Bildrenderer erzeugen auch dessen Silhouette.

Temporäre Alpha-Puffer werden auf den sichtbaren Bereich mit Filterrand begrenzt.
Bei großen Ansichten wird die Abtastauflösung reduziert; die Effektgröße in
Dokumentpixeln bleibt erhalten. Dies ist eine erste CPU-Filterimplementierung mit
Render-Readback pro sichtbarem Schatten. Viele große Schatten oder Wiederholungen
können dadurch merklich Rechenzeit benötigen; ein persistenter Effektcache und
GPU-Filter sind noch nicht implementiert.

## Speicherung

`VectorShape.shadow` enthält nur Werte, keine Texturen oder Zeiger. Deshalb tragen
Kopieren, Duplizieren, History und verknüpfte Instanzen den Effekt mit.
**PENGVEC Version 5** ergänzt pro Objekt sechs 32-Bit-Felder nach der Wiederholung:
aktiviert, Farbe, Versatz X/Y, Weichzeichnung, Ausbreitung. Auch bei ausgeschaltetem
Effekt werden alle Werte gespeichert. Versionen 1–4 bleiben lesbar und erhalten
einen ausgeschalteten Schatten mit Nullwerten. Ältere Programmversionen können
Version 5 nicht öffnen. Temporäre Sitzungsdaten nutzen **PVCKPT29** und behalten
Effekte samt Undo/Redo beim Editorwechsel.

Das aktuelle Dokumentformat ist Version 6; es ergänzt die
[Verlaufsfüllung](VEKTOR_VERLAEUFE.md) nach den Schattenwerten.

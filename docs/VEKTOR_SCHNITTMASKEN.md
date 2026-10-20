# Schnittmasken im Vektoreditor

Eine Schnittmaske funktioniert geometrisch wie in Illustrator: Die geschlossene
Fläche des Maskenpfads begrenzt den sichtbaren Inhalt. Farbe, Deckkraft und Rahmen
der Maske beeinflussen diesen Ausschnitt nicht. Löcher in zusammengesetzten Pfaden
bleiben durchsichtig. Inhalt, Text und Rahmen werden gemeinsam beschnitten; die
Originaldaten bleiben vollständig erhalten.

## Bedienung

1. Ein Objekt oder eine vorhandene Gruppe als Inhalt vorbereiten.
2. Darüber eine unabhängige geschlossene Form platzieren: Rechteck, Ellipse,
   Polygon oder geschlossener Bézier-/zusammengesetzter Pfad.
3. Inhalt und Maskenform gemeinsam auswählen. Die **oberste ausgewählte Form in
   der Ebenenreihenfolge** wird die Maske; die Klickreihenfolge ist unerheblich.
4. Rechtsklick → **Ebenen → Schnittmaske**.

Im Dokumentbaum erscheint eine **Schnittgruppe**, darin der Eintrag **Maske: …**
und die ursprünglichen Inhalte. Ein Klick auf die Schnittgruppe oder ihren sichtbaren
Inhalt wählt die ganze Einheit: Verschieben, Skalieren und Drehen verändern auch die
Maske. Ein Klick auf **Maske: …** im Baum wählt dagegen nur den Maskenpfad. Er lässt
sich über Inspector, Greifer und Maus bearbeiten; für Bézier-Knoten bleibt das
vorhandene Kurvenwerkzeug zuständig. Der Inspector bezeichnet ihn als Schnittmaske.

Zum Aufheben die ganze Schnittgruppe auswählen und **Ebenen → Maske loesen**
verwenden. Die Maske wird wieder als normale Form mit ihrem ursprünglichen Stil
angezeigt. Die Inhalte bleiben zusammen in einer gewöhnlichen Gruppe, die danach
bei Bedarf aufgelöst werden kann. Wird nur das Masken-Kind gelöscht, wird der
Inhalt ebenfalls wieder vollständig sichtbar; Undo stellt die Maske wieder her.

Ein ausgeblendetes Masken-Kind blendet den beschnittenen Inhalt aus. Gesperrte Kinder
verhindern Änderungen an der gesamten Gruppe. Unsichtbare Teile des Inhalts werden
beim normalen Klicken und bei der Bereichsauswahl nicht getroffen.

## Grenzen der ersten Version

Die bestehenden Gruppen sind flach. Erzeugen einer Schnittgruppe fasst die ausgewählten
Inhalte und die Maske in einer gemeinsamen Gruppe zusammen; frühere Untergruppen
werden nicht als verschachtelte Gruppen erhalten. Undo stellt die ursprünglichen
Gruppenzuordnungen wieder her. Verschachtelte Schnittmasken werden vorerst abgelehnt.
Es gibt keine Transparenz-/Luminanzmaske, keinen Weichzeichner und kein Feathering.
Offene Kurven und Text sind als Masken nicht zugelassen; Text darf Inhalt sein.

Boolesche Mehrfachoperationen und Umgruppieren einer aktiven Schnittgruppe sind
bewusst gesperrt, bis die Maske gelöst wird. Einzelne Maskenpfade lassen sich weiterhin
in Pfade umwandeln oder über die vorhandenen parametrischen Eigenschaften bearbeiten.
Die vorhandenen Kapazitätsgrenzen für Formen und abgeleitete Pfadgeometrie gelten weiter.

## Fachliche Aufteilung

- `mask/mask.c`: Prüfung, Erzeugen/Lösen als struktureller History-Befehl,
  maskenbewusste geometrische Trefferprüfung; ohne UI- oder SDL-Verarbeitung.
- `object/shape.h`: `clip_mask` ist eine gespeicherte Rolle eines Gruppen-Kindes.
  `group_id` verbindet Maske und Inhalt, `parent_id` bleibt das Artboard.
- `object/clipboard.c`: validiert Beziehungen; Kopieren/Duplizieren ordnet neue
  Gruppen-IDs zu. Eine einzeln kopierte Maske wird als gewöhnlicher Pfad eingefügt.
- `mask/render.c`: temporäre Render-Targets beschneiden vorhandene Shape-/Text-Renderer
  mit triangulierter Maskengeometrie. Premultipliziertes Alpha bleibt korrekt;
  GPU-Texturen gehören ausschließlich zum Rendering und nie zum Dokument.
- `ui/editor.yaml`, `ui/context_menu/object_actions.c`: deklarierte Menüeinträge
  und Übersetzung der Aktionen `vector.objects.mask_create` / `mask_release`.
- `ui/document_tree/`: zeigt Gruppe und bearbeitbares Masken-Kind.

Erzeugen und Lösen sind jeweils ein Undo-Schritt. Kopieren zwischen Dokument-Tabs,
Dateispeicherung und temporäres Entladen beim Editorwechsel erhalten die Maskenrolle.
Das dauerhafte Format ist jetzt **PENGVEC Version 5** mit Reader für Version 1;
der private Sitzungs-Checkpoint heißt **PVCKPT28**.

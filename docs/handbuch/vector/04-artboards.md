# 04 – Zeichenfläche, Artboards und Vorschau

Ein Artboard begrenzt einen Zeichen- und Exportbereich. Sein Name steht links über
der Fläche sowie im Document-Tree. Objektkoordinaten gehören zum jeweiligen
Artboard. Der Dokument-Tab behält seinen eigenen Namen; eine Umbenennung des
Artboards benennt den Tab nicht um.

## Anlegen und bearbeiten

Aktiviere den Artboard-Button und ziehe mit der linken Maustaste ein Rechteck auf.
Loslassen bestätigt ein Artboard. Ein erneuter Klick auf den aktiven Button oder
Esc bricht das Werkzeug ab. Nach einer erfolgreichen Anlage endet es ebenfalls.

Mit dem Auswahlwerkzeug wählst du ein Artboard aus. Ziehe **seinen Namen oberhalb
der Fläche**, um es zu verschieben. Die acht kleinen Griffe an Ecken und Kantenmitten
ändern seine Größe. Der Inspector bietet Name, Breite und Höhe; Enter oder der
Wechsel in ein anderes Feld übernimmt die Änderung. Größenänderungen werden live
angezeigt und beim Loslassen als eine History-Aktion bestätigt.

## Navigieren

Mit gedrückter mittlerer Maustaste verschiebst du die Ansicht. Das Mausrad zoomt
weich um die Mausposition; der Bereich beträgt 10–400 Prozent. Die Toolbar hat
Zoom minus, Zoom zurücksetzen auf 100 Prozent und Zoom plus. Ein weiterer Button
führt zurück zum aktiven Artboard. Der Zoom steht zusätzlich in der Statuszeile.

Lineale sind zunächst ausgeblendet. Der Lineal-Button schaltet sie pro Ansicht ein.
Jedes Artboard hat eigene Pixel-Lineale. Innerhalb des Artboards zeigt eine weiße
Markierung die Mausposition. Lineale sind Bearbeitungshilfen und werden nicht exportiert.

## Vorschau ohne Zeichenflächenzoom

Öffne rechts unten **Vorschau**, klicke **Bereich wählen** oder den zugehörigen
Toolbar-Button und ziehe einen Bereich innerhalb eines Artboards auf. Dort wird
mit einem Dokumentpixel pro UI-Pixel dargestellt. Kleine Bereiche bleiben klein;
ist der Bereich größer als das Panel, folgt sein sichtbarer Ausschnitt der
Zeichenansicht. Betriebssystem-Skalierung kann UI-Pixel von physischen Pixeln unterscheiden.
Das X bei der Bereichswahl entfernt die Region. Eine abgebrochene neue Wahl erhält
die vorherige. Vorschau und Zoom werden nicht als Artwork-Änderung behandelt.

## Übungen und Abnahme

### VEC-ART-01: Artboard bearbeiten

Lege ein zweites Artboard an. Nenne es im Inspector `Testboard`, wechsle mit Tab ins Breitenfeld und trage 640 ein, danach Höhe 480. Verschiebe es am Namenslabel und ändere seine Größe an einem Eckgriff. Nutze Undo.

**Erwartet:** Name über dem Artboard und Tree stimmen überein, der Dokument-Tab bleibt unverändert. Formularwerte werden bei Tab übernommen. Undo nimmt die letzte abgeschlossene Größenänderung zurück.

### VEC-ART-02: Zoom und Vorschau

Zeichne ein Objekt. Wähle einen Vorschau-Bereich darüber. Ändere den Zeichenflächenzoom und verschiebe die Ansicht mit der mittleren Maustaste. Schalte die Lineale ein.

**Erwartet:** Die Vorschau bleibt bei 1:1. Ein zu großer Vorschau-Bereich zeigt einen mitwandernden Ausschnitt; die Lineale berücksichtigen den Zeichenflächenzoom.

[Zur Kapitelübersicht](index.md) · [Ergebnis protokollieren](testcheckliste.md)

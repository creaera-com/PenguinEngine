# Projekte und Benutzerzustand

Das Hamburger-Menü der Shell bietet **Neues Projekt** und **Projekt öffnen**.
Beide Einträge sind in `resources/shell.yaml` als normales Popup-Menü beschrieben.
Die nativen Dateidialoge stellt SDL3 für Linux, Windows und macOS bereit.

**Neues Projekt** wählt einen vorhandenen Ordner. Dort wird exklusiv
`project.pengui.yaml` angelegt; eine bereits vorhandene Datei wird nicht überschrieben.
Zum Öffnen eine Projektdatei wählen. Die Datei wird vor dem Wechsel geprüft:

```yaml
schema: pengui/project/v1
name: Neues Projekt
```

Der Projektordner ist der übergeordnete Ordner dieser Datei. Die Projektdatei
speichert derzeit nur diese Metadaten. Neue/ungeöffnete Projekte dürfen weiterhin
neben den vorhandenen Editor-Dokumenten bestehen: Ein Projektwechsel löscht keine
ungespeicherten Editor-Inhalte und speichert sie auch noch nicht dauerhaft.

## Explorer

Vector-, Script- und Game-Editor stellen den gemeinsamen Slot `project_explorer`
bereit. Die App setzt dort `resources/project-explorer.yaml` ein. Projektzustand und
Dateibaum gehören zur App, nicht zu einem konkreten Editor; Editorwechsel und
Reloads behalten deshalb das geöffnete Projekt.

Der Baum zeigt Ordner vor Dateien, einschließlich versteckter Namen. Unterordner
werden beim Aufklappen eingelesen und zwischengespeichert. Der Refresh-Button liest
den Baum neu ein, etwa nach Änderungen durch andere Programme. Dateien werden beim
Anklicken markiert und ihr Pfad erscheint in der Statuszeile; das Öffnen ihrer
Inhalte in einem Editor ist ein späterer Schritt.

Verzeichnisse mit mehr als 32 Einträgen haben eine Zeile **Seite … – weiter**.
Sie blättert zyklisch durch alle Einträge des Verzeichnisses. So verbraucht ein
voller Ordner nicht alle 256 momentan möglichen Tree-Zeilen. Bei zu vielen zugleich
aufgeklappten Verzeichnissen zeigt der Explorer einen Hinweis; dann andere Ordner
zuklappen. Die maximale Baumtiefe ist 32.

## Letztes Projekt beim Neustart

Nach erfolgreichem Öffnen wird der absolute Projektpfad in `last-project.txt`
gespeichert. Ein fehlgeschlagener Wechsel verändert das aktuelle Projekt nicht.
Die Zustandsdatei wird zunächst separat geschrieben und erst bei Erfolg ersetzt.

- **Linux:** `$XDG_STATE_HOME/penguengine/last-project.txt`, standardmäßig
  `~/.local/state/penguengine/last-project.txt`. Relative XDG-Werte werden ignoriert.
- **Windows:** das von SDL ermittelte Benutzer-AppData-Verzeichnis für `penguengine`.
- **macOS:** das von SDL ermittelte Benutzerverzeichnis unter Application Support
  für `penguengine`.

`penguengine` ist bewusst der Verzeichnisname für die künftige Engine/App.
Es wird kein `pengui`-Zustandsverzeichnis angelegt. Allgemeine Benutzereinstellungen
und ein dauerhafter Recovery-Speicher werden in diesem Schritt noch nicht eingeführt.

Beim Start wird die letzte Projektdatei erneut geprüft und geöffnet. Fehlt sie,
startet die App ohne Projekt und zeigt einen Hinweis. Mit `--smoke` wird der echte
Benutzerzustand weder gelesen noch geschrieben, damit automatisierte Tests keine
persönlichen Projekte beeinflussen.

Die vorhandenen Editor-Checkpoints sind weiterhin temporäre Dateien (`tmpfile()`
unter Linux/macOS, Windows-Temp-Dateien mit Delete-on-Close). Sie halten entladene
Editor-Daten innerhalb der laufenden Sitzung und werden beim Beenden entfernt.
Das Wiederöffnen des Projektordners stellt somit noch keine ungespeicherten
Vektordokumente oder deren Undo-History nach einem App-Neustart wieder her.

## Fachliche Aufteilung

- `apps/demo/project/project.c`: validierte Projektidentität, Erzeugen/Lesen der Metadaten.
- `state.c`: plattformgerechter Benutzerzustand und letzter Projektpfad.
- `controller.c`: Shell-Aktionen, native Dialoge, Projektwechsel. Dialog-Callbacks
  geben ihre Ergebnisse als SDL-Ereignisse an den Hauptthread zurück und greifen
  weder auf UI noch auf Editorinstanzen zu.
- `explorer.c`: App-eigene Dateibaum-Anbindung und verzögertes Einlesen.
- `editor/application.c`: Lebenszyklus und Weiterleitung; keine Dateibaum- oder YAML-Fachlogik.


## Dokumentdateien

Vektordokumente können jetzt separat als `.pengvec` gespeichert und per Klick im
Explorer in einen Tab geladen werden. Die Projektdatei bleibt reine Metadatenverwaltung.
[Dokumentformat und Speichern](VEKTOR_DATEIFORMAT.md).

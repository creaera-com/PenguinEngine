# Doxygen-Dokumentation erzeugen

Die Doxygen-Konfiguration und Doxygen Awesome mit Sidebar-Navigation wurden aus
der früheren Engine-Version wiederhergestellt und für die aktuelle Struktur
angepasst. Theme und Lizenz liegen lokal unter `third_party/doxygen-awesome-css/`;
der Dokumentationsbuild benötigt keine Downloads.

Doxygen ab Version 1.9.7 installieren (z.B. Ubuntu: `sudo apt install doxygen`), dann im Projektordner:

```sh
cmake -S . -B build
cmake --build build --target api_docs
```

Bei Mehrkonfigurations-Generatoren kann `--config Debug` ergänzt werden.
Die Dokumentation im Browser öffnen:

```text
build/docs/html/index.html
```

Die Vorlage liegt unter `docs/doxygen/Doxyfile.in`. CMake erzeugt daraus
`build/Doxyfile` mit absoluten Pfaden. Nicht die generierte Datei bearbeiten;
Änderungen gehören in die Vorlage. Die generierte Konfiguration kann auch direkt
mit `doxygen build/Doxyfile` ausgeführt werden.

Ist kein ausreichend neues Doxygen beim Konfigurieren installiert, gibt es das optionale Target
`api_docs` nicht. Nach der Installation CMake erneut konfigurieren. In CLion den
CMake-Reload ausführen und anschließend das Target `api_docs` bauen. Normale
App-Builds und Tests benötigen Doxygen nicht.

Die Navigation unter **Handbuch** enthält die Markdown-Anleitungen. Die Datei
`docs/doxygen/mainpage.dox` pflegt deren Gruppierung; neue Anleitungen dort mit
`@subpage` aufnehmen. Die API-Dateiliste umfasst die öffentlichen Header von
pengUI und pengDraw sowie die Header der App und Editoren. Drittanbieterquellen,
generierte Font-/Icon-Dateien und Build-Verzeichnisse werden nicht eingelesen.

Warnungen liegen in `build/doxygen-warnings.log`. Dokumentationsfehler lassen den
Dokumentationsbuild fehlschlagen. Fehlende Einzelkommentare werden nicht als
vollständige Dokumentationsabdeckung geprüft (`EXTRACT_ALL=YES`), damit auch die
noch nicht einzeln kommentierten Schnittstellen auffindbar bleiben.

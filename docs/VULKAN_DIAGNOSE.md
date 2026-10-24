# Vulkan-Speicherdiagnose

Diese Diagnose beobachtet die native SDL-Vulkan-Implementierung zusätzlich zu den
bestehenden pengDraw-, SDL-Heap- und Prozesszählern. Sie ändert weder Poolgrößen noch
Freigabezeitpunkte. Zusätzliche Zähloperationen können allerdings das Timing verändern.

## Aktivieren

In CLion dem CMake-Profil diese Option hinzufügen und CMake neu laden:

```text
-DPENGUI_VULKAN_DIAGNOSTICS=ON
```

Oder ein separates Build-Verzeichnis verwenden:

```sh
cmake -S . -B build-vulkan-diagnostics -DPENGUI_VULKAN_DIAGNOSTICS=ON
cmake --build build-vulkan-diagnostics -j
```

Das Profil baut absichtlich das festgelegte SDL 3.4.18 aus Quellen, auch wenn SDL
bereits installiert ist. Dafür wird beim ersten Konfigurieren ein Download benötigt.
Die Instrumentierung erfolgt in einer privaten Kopie im Build-Verzeichnis.
Installierte Bibliotheken und lokale SDL-Quellen bleiben unverändert. Der Patch
bricht bei unerwartetem Quelltext ab. Ohne Option bleibt die normale SDL-Auswahl
bestehen. Nach dem Test die Option wieder auf `OFF` setzen.

Im Editor das Projekt öffnen und im Monitor das Performance-Logging aktivieren.
Jede JSONL-Zeile enthält anschließend `vulkan.available`. Nur bei `true` stammen
die Werte aus dem instrumentierten Backend; andere Backends liefern `null`.
Die Option erzwingt nicht den Vulkan-Treiber. Unter Linux verwendet der GPU-Renderer
üblicherweise Vulkan; auf anderen Plattformen können andere Backends aktiv sein.

## Bedeutung der Zähler

Alle Zähler gelten kumulativ für das aktuelle GPU-Gerät:

- `descriptor_pools_created` / `descriptor_pools_destroyed`: erfolgreiche native
  Pool-Anlagen und ausgeführte Zerstörungen. Die Differenz ist die Poolanzahl.
- `descriptor_sets_allocated`: erfolgreich angelegte native Sets. SDL legt pro Pool
  128 Sets an. `descriptor_sets_fetched` zählt deren Nutzung, einschließlich erneuter
  Nutzung bereits angelegter Sets; es ist keine Zahl gleichzeitig aktiver Sets.
- `descriptor_caches_returned` / `descriptor_caches_reused`: Rückgaben an den SDL-Pool
  und erneute Entnahmen. Eine Rückgabe setzt die Set-Indizes zurück und behält die
  nativen Pools. SDL ruft dabei kein `vkResetDescriptorPool` auf.
- `command_buffers_allocated` / `command_buffers_freed`: erfolgreiche native Anlagen
  und implizite Freigaben durch Zerstören eines Command-Pools.
  `command_buffers_acquired` zählt Entnahmen, einschließlich frisch angelegter Buffer.
- `memory_allocations` / `memory_frees` und `memory_bytes_allocated` /
  `memory_bytes_freed`: erfolgreiche `VkDeviceMemory`-Anlagen und Freigaben mit ihren
  angeforderten Größen. Die Byte-Differenz ist **kein** vollständiger VRAM-Verbrauch.
- `memory_maps`: erfolgreiche persistente Mappings. `memory_implicit_unmaps` zählt
  deren Aufhebung durch `vkFreeMemory`; SDL verwendet hier kein explizites Unmap.
- `failures`: Fehler der beobachteten Allocate-/Create-/Map-Aufrufe, keine allgemeine
  Vulkan-Fehlerstatistik.

Treiberinterne Descriptor-Verwaltung und Reservierungen müssen nicht durch
`vkAllocateMemory` laufen. Deshalb können diese Byte-Zähler stabil bleiben, während
RSS und `/dev/nvidiactl`-Mappings steigen. Die Geräte-Zähler werden gemeinsam unter
SDLs Property-Lock gelesen; verschiedene Beobachtungsbereiche im Log können zeitlich
leicht auseinanderliegen. Das Logging wartet nicht auf die GPU.

## Vergleichstest

Eine Datei mit derselben Szene und z. B. 80 verknüpften Kopien verwenden. Jeden
Durchgang in einem frischen Prozess starten und dieselben Bewegungen ausführen:

1. Schatten aus.
2. Schatten an, ohne Weichzeichnung.
3. Schatten an, mit Weichzeichnung.

„Auf GPU warten“ für diese drei Durchgänge zunächst ausgeschaltet lassen. Einen
zusätzlichen Vergleich mit dieser Option separat aufzeichnen. Jeweils den
Startzustand, mehrere Verschiebungen und einige Sekunden Leerlauf protokollieren.
Interessant ist, ob neue Descriptor-Pools zeitgleich mit dem RSS-Anstieg entstehen
und ob Cache-/Command-Buffer-Wiederverwendung danach ein Plateau erreicht.

Das kann einen Zusammenhang eingrenzen, beweist allein aber keinen Treiberfehler.

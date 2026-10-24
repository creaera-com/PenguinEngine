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

## Erweiterte Aufschlüsselung (Diagnoseversion 2)

Nach Aktualisieren des Branches CMake neu laden und neu bauen. Die bisherige Option
`PENGUI_VULKAN_DIAGNOSTICS=ON` bleibt bestehen. Im Log muss `vulkan.version` den Wert
`2` haben. Ältere instrumentierte Bibliotheken bleiben lesbar; neue Detailbereiche
werden dort als `null` ausgegeben.

`allocation_origins` enthält vier Klassen: `gpu_buffer` (z. B. Vertex-/Indexdaten),
`uniform_buffer` (Shaderparameter), `transfer_buffer` (Datentransfer) und `image`
(native Bilder einschließlich interner Renderziele). Pro Klasse werden erfasst:

- `allocations`, `frees`, `allocated_bytes`, `freed_bytes`: native Speicherblöcke,
  zugeordnet zu der Ressource, die ihre Anlage ausgelöst hat. Das ist **keine
  exklusive Besitzzuordnung**: SDL kann später andere Ressourcen im selben Block
  unterbringen. Die Zuordnung bleibt bis zur Freigabe des Blocks erhalten.
- `dedicated_allocations`: Anlagen, für die SDL einen eigenen Speicherblock anfordert.
- `bindings`: erfolgreiche Speicherbindungen, einschließlich Unterbelegungen eines
  vorhandenen Blocks. Wiederverwendung muss also keine neue native Anlage erzeugen.
- `resource_bytes`: kumulierte angeforderte Ressourcengröße bei diesen Bindungen.
  Bei Buffern ist das ihre logische Größe; bei Bildern verwendet SDL bereits die
  Vulkan-Speicheranforderung. Dies sind weder aktive Bytes noch reine Bildpixel.
- `requirement_bytes`: kumulierte Vulkan-Speicheranforderungen derselben Bindungen,
  vor zusätzlicher Blockrundung durch SDL. So können kleine logische Buffer mit
  großen Vulkan-Anforderungen und zusätzliche Reserven auseinandergehalten werden.

`memory_types` enthält alle vom Gerät gemeldeten Speichertypen mit `index`,
`heap_index` und Vulkan-`flags` sowie Anlagen/Freigaben, angeforderten Bytezahlen,
`maps` und `implicit_unmaps`. Die wichtigsten Flag-Bits sind:

| Bit | Bedeutung |
| --- | --- |
| 1 | Device-local: GPU-lokaler Speicher |
| 2 | Host-visible: für CPU-Zugriff abbildbar |
| 4 | Host-coherent: kohärenter CPU/GPU-Zugriff |
| 8 | Host-cached: CPU-seitig gecacht |

Flags können kombiniert sein: GPU-lokal und CPU-zugänglich schließen sich nicht aus.
Die Bytes verschiedener Speichertypen sind API-Anforderungen, keine RSS- oder
VRAM-Messung. Die Summe der Anlagen/Freigaben und Bytezahlen über die Klassen bzw.
Speichertypen entspricht den vorhandenen Gesamtsummen, sobald die jeweilige native
Operation vollständig verbucht wurde. Kurzzeitige Abweichungen sind möglich, wenn
parallel eine Operation erfasst wird. Es wird kein unbegrenztes Einzelereignis-Log
und keine neue Liste pro GPU-Objekt im Monitor gehalten.

/** @file
 * @brief SDL-unabhängiges Vektordokument, SVG-Austausch und geometrische Operationen. */
#ifndef PENGUIN_VECTOR_DOCUMENT_H
#define PENGUIN_VECTOR_DOCUMENT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/** @brief Feste Grenzen; Überschreitungen werden atomar abgewiesen. */
#define VE_BOARDS 16
/** @brief Maximale Objektzahl pro Dokument. */
#define VE_SHAPES 64
/** @brief Maximale Segmentzahl pro Objekt. */
#define VE_NODES 256
/** @brief Maximale polygonalisierte Punktzahl pro Objekt. */
#define VE_FLAT 2048
/** @brief Anzahl Verlaufssnapshots einschließlich Ausgangszustand. */
#define VE_HISTORY 33

/** @brief SVG-Pfadsegment; C enthält zwei Kontrollpunkte und den Endpunkt. */
typedef enum { VE_MOVE, VE_LINE, VE_CUBIC, VE_CLOSE } VeCommand;

/** @brief Punkt in Artboard-Pixeln. */
typedef struct {
    float x; ///< Horizontale Position in Pixeln.
    float y; ///< Vertikale Position in Pixeln.
} VePoint;

/** @brief Pfadsegment in lokalen Artboard-Pixeln. */
typedef struct {
    int command; ///< Befehl aus VeCommand.
    VePoint p;   ///< Endpunkt bzw. Beginn eines Teilpfads.
    VePoint c1;  ///< Erster kubischer Kontrollpunkt.
    VePoint c2;  ///< Zweiter kubischer Kontrollpunkt.
} VeNode;

/** @brief Bearbeitbare Grundform oder zusammengesetzter Pfad. */
typedef enum { VE_RECT, VE_ELLIPSE, VE_PATH } VeKind;

/** @brief Eigenständiges Artboard; Kinderkoordinaten bleiben beim Verschieben lokal. */
typedef struct {
    bool used;           ///< Ob dieser Slot belegt ist.
    char name[64];       ///< Nullterminierter UTF-8-Anzeigename.
    float x;             ///< Horizontale Position in Pixeln.
    float y;             ///< Vertikale Position in Pixeln.
    float w;             ///< Breite in Pixeln.
    float h;             ///< Höhe in Pixeln.
    uint32_t background; ///< Artboard-Hintergrund als RRGGBBAA.
} VeBoard;

/** @brief Vektorobjekt mit einheitlicher Füllung/Kontur im Format RRGGBBAA. */
typedef struct {
    bool used;              ///< Ob dieser Slot belegt ist.
    bool fill_enabled;      ///< Ob die Fläche gefüllt wird.
    bool stroke_enabled;    ///< Ob die Kontur gezeichnet wird.
    bool evenodd;           ///< Even-Odd statt Nonzero-Winding.
    int kind;               ///< Grundform aus VeKind.
    int board;              ///< Index des zugehörigen aktiven Artboards.
    int count;              ///< Anzahl belegter Einträge.
    uint32_t id;            ///< Stabile Dokument-ID.
    uint32_t fill;          ///< Füllfarbe als RRGGBBAA.
    uint32_t stroke;        ///< Konturfarbe als RRGGBBAA.
    float x;                ///< Horizontale Position in Pixeln.
    float y;                ///< Vertikale Position in Pixeln.
    float w;                ///< Breite in Pixeln.
    float h;                ///< Höhe in Pixeln.
    float stroke_width;     ///< Konturbreite in lokalen Pixeln.
    char name[64];          ///< Nullterminierter UTF-8-Anzeigename.
    VeNode nodes[VE_NODES]; ///< Besitzende Pfadsegmente.
} VeShape;

/** @brief Besitzendes Dokument ohne Renderer- oder UI-Zustand. */
typedef struct {
    uint32_t next_id;          ///< Nächste freie stabile Objekt-ID.
    VeBoard boards[VE_BOARDS]; ///< Besitzende Artboard-Slots.
    VeShape shapes[VE_SHAPES]; ///< Besitzende Objekt-Slots in Zeichenreihenfolge.
} VeDocument;

/** @brief Polygonalisierte Teilpfade, auch für Even-Odd-Löcher. */
typedef struct {
    VePoint points[VE_FLAT]; ///< Polygonalisierte Punkte.
    int start[VE_NODES];     ///< Erster Punktindex je Teilpfad.
    int length[VE_NODES];    ///< Punktzahl je Teilpfad.
    int count;               ///< Anzahl belegter Einträge.
    int contours;            ///< Anzahl Teilpfade.
    bool closed[VE_NODES];   ///< Geschlossener Teilpfad je Eintrag.
    bool evenodd;            ///< Even-Odd statt Nonzero-Winding.
} VeFlat;

/** @brief Boolesche Flächenoperation; Differenz ist A minus B. */
typedef enum { VE_UNION, VE_INTERSECTION, VE_DIFFERENCE, VE_XOR } VeBoolean;

/** @brief Begrenzte, besitzende JSON-Snapshots mit separater gespeicherter Revision. */
typedef struct {
    char *states[VE_HISTORY]; ///< Besitzende JSON-Snapshots; mit free freigeben.
    uint64_t ids[VE_HISTORY]; ///< Eindeutige Revision je Snapshot.
    uint64_t next;            ///< Nächste freie Revisions-ID.
    uint64_t saved;           ///< Zuletzt gespeicherte Revisions-ID, auch nach Eviction.
    int count;                ///< Anzahl belegter Einträge.
    int position;             ///< Aktive Position im Verlauf.
} VeHistory;

/** @brief Initialisiert ein Dokument mit einem 512x384-Pixel-Artboard. */
void ve_init(VeDocument *doc);
/** @brief Fügt Artboard/Objekt hinzu; -1 bei Limit oder ungültigen Maßen. */
int ve_add_board(VeDocument *doc, float x, float y, float w, float h);
/** @brief Fügt eine Grundform in lokalen Artboard-Koordinaten hinzu. */
int ve_add_shape(VeDocument *doc, int board, VeKind kind, float x, float y, float w, float h);
/** @brief Validiert vollständige Dokumente, einschließlich Indizes, Farben und endlicher Geometrie.
 */
bool ve_valid(const VeDocument *doc);
/** @brief Wandelt Rechtecke in Linien und Ellipsen in vier kubische Näherungsbögen um. */
bool ve_to_path(VeShape *shape);
/** @brief Verschiebt Objektgeometrie einschließlich Kontrollpunkten. */
void ve_move(VeShape *shape, float dx, float dy);
/** @brief Spiegelt um eine gemeinsame Achse in lokalen Artboard-Pixeln. */
void ve_mirror(VeShape *shape, bool horizontal, float axis);
/** @brief Aktualisiert die Kontrollhüllen-Grenzen eines Pfads. */
void ve_bounds(VeShape *shape);
/** @brief Teilt ein Linien-/Kurvensegment bei t=0.5 atomar. */
bool ve_split_node(VeShape *shape, int node);
/** @brief Polygonalisiert kubische Kurven mit höchstens 0.25 px Kontrollabstand. */
bool ve_flatten(const VeShape *shape, VeFlat *flat);
/** @brief Flächentest nach gespeicherter Füllregel in lokalen Artboard-Koordinaten. */
bool ve_inside(const VeFlat *flat, double x, double y);
/** @brief Boolesche Operation, atomar; Ergebnis ersetzt A, B wird entfernt. */
bool ve_boolean(VeDocument *doc, int a, int b, VeBoolean operation, char *error, size_t size);
/** @brief Serialisiert native Metadaten; malloc-Ergebnis mit free freigeben. */
char *ve_json(const VeDocument *doc);
/** @brief Liest validierte Metadaten atomar. */
bool ve_from_json(VeDocument *doc, const char *text, char *error, size_t size);
/** @brief Erzeugt interoperables SVG; Metadaten optional, malloc-Ergebnis. */
char *ve_svg(const VeDocument *doc, bool metadata, float *origin_x, float *origin_y);
/** @brief Liest SVG atomar; fremde unterstützte SVG-Formen werden zu Bézierpfaden. */
bool ve_load(VeDocument *doc, const char *path, char *error, size_t size);
/** @brief Speichert SVG atomar neben der Zieldatei; alte Datei bleibt bei Fehler erhalten. */
bool ve_save(const VeDocument *doc, const char *path, char *error, size_t size);
/** @brief Initialisiert den ersten Verlaufssnapshot. */
bool ve_history_init(VeHistory *history, const VeDocument *doc);
/** @brief Gibt alle Verlaufssnapshots frei. */
void ve_history_free(VeHistory *history);
/** @brief Übernimmt einen geänderten Dokumentzustand; false bei unverändert/OOM. */
bool ve_history_commit(VeHistory *history, const VeDocument *doc);
/** @brief Undo/Redo; lädt den Snapshot vor Änderung der Position. */
bool ve_history_step(VeHistory *history, VeDocument *doc, bool redo);
/** @brief Prüft die aktuelle Revision gegen die zuletzt gespeicherte Revision. */
bool ve_dirty(const VeHistory *history);
#endif

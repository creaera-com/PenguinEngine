/** @file
 * @brief Public pengUI API: SDL-free core and optional SDL3 presentation.
 * Link pengUI::sdl to expose the SDL API through PENGUI_WITH_SDL=1.
 */
#ifndef PENGUI_H
#define PENGUI_H

/* Core: input, layout geometry and YAML. No SDL dependency. */

/**
 * @file
 * @brief Wiederverwendbare UI-Eingabe, Layoutzustände und Widgetinteraktionen ohne SDL.
 * @ingroup ui_api
 */

/** @addtogroup ui_api
 * @{
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Maximale Tiefe des UI-Clip-Stacks. */
#define UI_MAX_CLIPS 16
/** @brief Maximale Anzahl Tabs pro Gruppe. */
#define UI_MAX_TABS 16
/** @brief Standardabstand zwischen Tabs in Pixeln. */
#define UI_TAB_GAP 2
/** @brief Kontextweit eindeutiger, stabiler Widgetbezeichner; 0 bedeutet keine Auswahl. */
typedef uint32_t UiId;

/** @brief Achsenparalleler UI-Bereich in Pixeln. */
typedef struct {
    float x; ///< Horizontale Position.
    float y; ///< Vertikale Position.
    float w; ///< Breite.
    float h; ///< Höhe.
} UiRect;

/** @brief Achse einer Zieh- oder Scrollinteraktion. */
typedef enum { UI_AXIS_X, UI_AXIS_Y } UiAxis;

/** @brief Zustand eines UI-Eingabekontexts einschließlich Fokus, Capture und Clip-Stack. */
typedef struct {
    float x;           ///< Horizontale Position.
    float y;           ///< Vertikale Position.
    float press_x;     ///< Horizontale Zeigerposition beim Drücken.
    float press_y;     ///< Vertikale Zeigerposition beim Drücken.
    float release_x;   ///< Horizontale Zeigerposition beim Loslassen.
    float release_y;   ///< Vertikale Zeigerposition beim Loslassen.
    bool down;         ///< Ob die primäre Zeigertaste gehalten wird.
    bool pressed;      ///< Ob seit dem letzten Frame eine Druckflanke eingetroffen ist.
    bool released;     ///< Ob seit dem letzten Frame eine Loslassflanke eingetroffen ist.
    bool activate;     ///< Ausstehende Tastaturaktivierung des fokussierten Widgets.
    bool blocked;      ///< Ob die UI-Interaktion aktuell gesperrt ist.
    bool active_seen;  ///< Ob das eingefangene Widget im aktuellen Frame verarbeitet wurde.
    UiId active;       ///< ID des Widgets mit aktiver Zeigerinteraktion; 0 bedeutet keines.
    UiId focus;        ///< ID des Tastaturfokus; 0 bedeutet keiner.
    float drag_origin; ///< Zeigerkoordinate beim Beginn des Ziehens.
    float drag_value;  ///< Ausgangswert beim Beginn des Ziehens.
    bool dragging;     ///< Ob der aktive Drag-Schwellwert überschritten wurde.
    UiRect clips[UI_MAX_CLIPS]; ///< Begrenzter Stack geschnittener UI-Cliprechtecke.
    int clip_depth;             ///< Aktuelle Tiefe des Clip-Stacks.
} UiContext;

/** @brief Scrollzustand in Pixeln für Inhalt und sichtbaren Viewport. */
typedef struct {
    float offset;   ///< Aktueller Scrolloffset in Pixeln.
    float content;  ///< Gesamtausdehnung des Inhalts in Pixeln.
    float viewport; ///< Sichtbarer Inhaltsbereich bzw. dessen Ausdehnung in Pixeln.
} UiScroll;

/** @brief Stabile Tab-ID, Bedienbarkeit und optionale individuelle Slotbreite. */
typedef struct {
    UiId id;      ///< Stabile Widget- oder Editor-ID.
    bool enabled; ///< Ob Interaktionen erlaubt sind.
    float width;  ///< Slotbreite einschließlich Abstand; 0 verwendet die Fallbackbreite.
} UiTab;

/** @brief Besitzende Kopie der Tabreihenfolge mit Auswahl, Abstand und Scroll-/Dragzustand. */
typedef struct {
    UiTab items[UI_MAX_TABS]; ///< Besitzende Kopie der Tab-Einträge in aktueller Reihenfolge.
    int count;                ///< Anzahl gültiger Einträge.
    UiId selected;            ///< Stabile ID der aktuellen Auswahl; 0 bedeutet keine Auswahl.
    float scroll;             ///< Horizontaler Tab-Scrolloffset in Pixeln.
    float gap;                ///< Abstand zwischen UI-Flächen bzw. Tabs in Pixeln.
    bool dragging;            ///< Zustand der laufenden Ziehinteraktion.
} UiTabs;

/**
 * @brief Bildet einen stabilen, von 0 verschiedenen Bezeichner aus einer Zeichenkette.
 *
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 */
UiId ui_id(const char *label);

/**
 * @brief Setzt einen UI-Kontext in seinen leeren Ausgangszustand zurück.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_init(UiContext *context);
/* Feed transitions in event order. A press and release may occur before the next draw. */
/**
 * @brief Verarbeitet Zeigerposition und Tastenzustandswechsel in Ereignisreihenfolge.
 *
 * Press und Release dürfen beide vor dem nächsten Zeichnen eintreffen; Ereignisse nicht
 * zusammenfassen.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param down Aktueller Zustand der primären Zeigertaste.
 */
void ui_pointer(UiContext *context, float x, float y, bool down);

/**
 * @brief Bricht die aktuelle Zeigerinteraktion und Aktivierung ab.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_cancel(UiContext *context);

/**
 * @brief Setzt den Tastaturfokus; ID 0 entfernt den Fokus.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 */
void ui_focus(UiContext *context, UiId id);

/**
 * @brief Fordert die Tastaturaktivierung des fokussierten Elements an.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_activate(UiContext *context);

/**
 * @brief Schließt einen UI-Frame ab und verbraucht dessen kurzlebige Eingabezustände.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_end_frame(UiContext *context);

/**
 * @brief Prüft einen Punkt gegen ein nichtleeres Rechteck mit exklusiver rechter/unterer Grenze.
 *
 * @param rect Elementrechteck in UI-Pixeln.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 */
bool ui_contains(UiRect rect, float x, float y);

/**
 * @brief Prüft Zeigerposition, Clip und globalen Eingabeblock für ein Element.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 */
bool ui_hovered(const UiContext *context, UiRect rect);

/**
 * @brief Legt den Schnitt mit dem bisherigen Clip auf den begrenzten Clip-Stack.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param clip Neuer Clipbereich in UI-Pixeln.
 */
bool ui_clip_push(UiContext *context, UiRect clip);

/**
 * @brief Entfernt den obersten Clip, sofern einer vorhanden ist.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_clip_pop(UiContext *context);
/* Stable, nonzero IDs must be unique within a context. Activation is on release. */
/**
 * @brief Verarbeitet eine Buttoninteraktion mit Aktivierung beim Loslassen oder per Tastatur.
 *
 * IDs müssen stabil, von 0 verschieden und innerhalb eines UiContext eindeutig sein.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 */
bool ui_button(UiContext *context, UiId id, UiRect rect, bool enabled);

/**
 * @brief Verarbeitet einen Button und schaltet den übergebenen Aufklappzustand um.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param open Ein-/Ausgabe des Aufklappzustands.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 */
bool ui_disclosure(UiContext *context, UiId id, UiRect rect, bool *open, bool enabled);

/**
 * @brief Verändert einen begrenzten Wert durch Ziehen entlang einer Achse.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param value Ein-/Ausgabe des veränderbaren Werts bzw. zu prüfender Modellknoten.
 * @param minimum Inklusive Untergrenze.
 * @param maximum Inklusive Obergrenze bzw. maximale Stringlänge in Bytes.
 * @param axis Achse für Ziehen oder Scrollen.
 * @param scale Faktor zwischen Zeigerbewegung und Wertänderung.
 */
bool ui_drag_value(UiContext *context, UiId id, UiRect rect, float *value, float minimum,
                   float maximum, UiAxis axis, float scale);

/**
 * @brief Passt linke und rechte Spaltenbreite an die Gesamtbreite und Mindestbreiten an.
 *
 * @param total Verfügbare Gesamtbreite in Pixeln.
 * @param left Ein-/Ausgabe der linken Spaltenbreite in Pixeln.
 * @param right Ein-/Ausgabe der rechten Spaltenbreite in Pixeln.
 * @param min_left Mindestbreite der linken Spalte in Pixeln.
 * @param min_right Mindestbreite der rechten Spalte in Pixeln.
 * @param min_center Mindestbreite des mittleren Bereichs in Pixeln.
 */
void ui_columns_fit(float total, float *left, float *right, float min_left, float min_right,
                    float min_center);

/**
 * @brief Liefert den nichtnegativen maximalen Scrolloffset.
 *
 * @param scroll Scrollzustand in Pixeln.
 */
float ui_scroll_max(const UiScroll *scroll);

/**
 * @brief Beschränkt den Scrolloffset auf den verfügbaren Inhaltsbereich.
 *
 * @param scroll Scrollzustand in Pixeln.
 */
void ui_scroll_clamp(UiScroll *scroll);

/**
 * @brief Addiert eine Pixeldistanz und begrenzt anschließend den Scrolloffset.
 *
 * @param scroll Scrollzustand in Pixeln.
 * @param delta Additive Scrolldistanz in Pixeln.
 */
void ui_scroll_by(UiScroll *scroll, float delta);

/**
 * @brief Berechnet das Scrollbar-Griffrechteck aus Inhalt, Viewport und Track.
 *
 * @param scroll Scrollzustand in Pixeln.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
UiRect ui_scroll_thumb(const UiScroll *scroll, UiRect track, UiAxis axis);

/**
 * @brief Verarbeitet Scrollbar-Ziehen und Track-Klicks und aktualisiert den Scrolloffset.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param scroll Scrollzustand in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
bool ui_scrollbar(UiContext *context, UiId id, UiRect track, UiScroll *scroll, UiAxis axis);

/**
 * @brief Validiert und kopiert eine begrenzte Liste eindeutiger, nichtleerer Tab-IDs.
 *
 * @return false bei ungültiger Anzahl, doppelten IDs oder fehlender/gesperrter Auswahl.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param items Zu kopierende Liste der Tab-Einträge.
 * @param count Anzahl zu kopierender Einträge, höchstens UI_MAX_TABS.
 * @param selected ID des anfangs ausgewählten, bedienbaren Tabs.
 */
bool ui_tabs_init(UiTabs *tabs, const UiTab *items, int count, UiId selected);

/**
 * @brief Setzt die individuelle Slotbreite eines Tabs.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param width Breite in den von der Funktion beschriebenen Einheiten.
 */
bool ui_tabs_set_width(UiTabs *tabs, UiId id, float width);

/**
 * @brief Berechnet das sichtbare Tabrechteck ohne den konfigurierten Zwischenraum.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param bar Sichtbarer Bereich der Tabgruppe in Pixeln.
 * @param width Fallback-Slotbreite einschließlich Tababstand in Pixeln.
 * @param index Nullbasierter Index des gewünschten Eintrags.
 */
UiRect ui_tab_rect(const UiTabs *tabs, UiRect bar, float width, int index);

/**
 * @brief Scrollt eine Tabgruppe horizontal innerhalb ihrer Inhaltsgrenzen.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param viewport Sichtbarer Zeichenbereich bzw. seine Ausdehnung in Pixeln.
 * @param tab_width Fallback-Slotbreite einschließlich Abstand in Pixeln.
 * @param delta Additive Scrolldistanz in Pixeln.
 */
void ui_tabs_scroll(UiTabs *tabs, float viewport, float tab_width, float delta);
/* Reorders IDs while dragging; selected ID and the associated content remain stable. */
/**
 * @brief Verarbeitet Auswahl und Drag-Reihenfolge, während IDs und Inhalte stabil bleiben.
 *
 * @return true bei geänderter Auswahl; eine reine Umordnung kann auch bei false stattfinden.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param tabs Tabzustand mit stabilen IDs.
 * @param bar Sichtbarer Bereich der Tabgruppe in Pixeln.
 * @param tab_width Fallback-Slotbreite einschließlich Abstand in Pixeln.
 */
bool ui_tabs_update(UiContext *context, UiTabs *tabs, UiRect bar, float tab_width);

/** @brief Zustand eines einzeiligen UTF-8-Eingabefelds; der Textpuffer gehört dem Aufrufer. */
typedef struct {
    size_t cursor; ///< Cursorposition als Byteoffset an einer UTF-8-Grenze.
    size_t anchor; ///< Anfang der Auswahl als Byteoffset; cursor == anchor bedeutet keine Auswahl.
    float scroll;  ///< Horizontaler Darstellungsversatz in Pixeln.
} UiTextEdit;

/** @brief Renderer-unabhängige Bearbeitungstasten eines einzeiligen Felds. */
typedef enum {
    UI_TEXT_LEFT,
    UI_TEXT_RIGHT,
    UI_TEXT_HOME,
    UI_TEXT_END,
    UI_TEXT_BACKSPACE,
    UI_TEXT_DELETE,
    UI_TEXT_SELECT_ALL
} UiTextKey;

/** @brief Initialisiert den Cursor am Textende und wählt optional den ganzen Text aus.
 * @param edit Besitzender Feldzustand.
 * @param text Nullterminierter UTF-8-Text; wird nicht verändert.
 * @param select_all Ob der Anfangswert zum direkten Ersetzen markiert wird.
 */
void ui_text_edit_init(UiTextEdit *edit, const char *text, bool select_all);
/** @brief Fügt UTF-8-Text am Cursor ein oder ersetzt die Auswahl atomar.
 * @param edit Feldzustand, dessen Cursor und Auswahl aktualisiert werden.
 * @param text Veränderbarer, nullterminierter Puffer mit mindestens capacity Bytes.
 * @param capacity Gesamtkapazität einschließlich Nullbyte.
 * @param inserted Geliehener UTF-8-Text; darf nicht mit text überlappen.
 * @return false bei Platzmangel, ungültigem UTF-8 oder Steuerzeichen; Text bleibt unverändert.
 */
bool ui_text_edit_insert(UiTextEdit *edit, char *text, size_t capacity, const char *inserted);
/** @brief Bewegt Cursor/Auswahl oder löscht ganze UTF-8-Zeichen.
 * @param edit Feldzustand.
 * @param text Veränderbarer, nullterminierter UTF-8-Puffer.
 * @param key Bearbeitungsaktion.
 * @param shift Ob Navigation die bestehende Auswahl erweitert.
 * @return true, wenn der Text durch Löschen verändert wurde.
 */
bool ui_text_edit_key(UiTextEdit *edit, char *text, UiTextKey key, bool shift);
/** @brief Setzt den Cursor auf eine nullbasierte Zeichenspalte, begrenzt auf das Textende.
 * @param edit Feldzustand.
 * @param text Nullterminierter UTF-8-Text.
 * @param column Zeichenspalte, unabhängig von der UTF-8-Bytelänge.
 * @param shift Ob die Auswahl erweitert wird.
 */
void ui_text_edit_click(UiTextEdit *edit, const char *text, size_t column, bool shift);
/** @brief Ermittelt die Zeichenspalte zu einem auf die Textlänge begrenzten Byteoffset.
 * @param text Nullterminierter UTF-8-Text.
 * @param offset Byteoffset; wird auf Textlänge und Zeichengrenze begrenzt.
 */
size_t ui_text_edit_column(const char *text, size_t offset);

/** @brief Prüft die Fortsetzung einer Hierarchieebene in einer aufgeklappten Baum-Liste.
 * @param depths Geliehene Preorder-Liste nichtnegativer Einrückungstiefen.
 * @param count Anzahl sichtbarer (nicht eingeklappter) Einträge, einschließlich außerhalb des
 * Viewports.
 * @param row Nullbasierter aktueller Zeilenindex.
 * @param level Eigene oder übergeordnete Tiefe, deren nächstes Geschwister gesucht wird.
 * @return true bei einem folgenden Eintrag derselben Ebene vor dem Ende des Teilbaums.
 */
bool ui_tree_continues(const int *depths, int count, int row, int level);

/** @brief Zweispaltige Eigenschaftszeile ohne eigenen Zustand oder Rendering. */
typedef struct {
    UiRect label; ///< Linke Spalte für die Bezeichnung.
    UiRect value; ///< Rechte Spalte für Wert oder Eingabefeld.
} UiPropertyRow;

/**
 * @brief Teilt eine Eigenschaftszeile in bündige Bezeichnungs- und Wertspalten.
 * @param area Zeilenbereich in Pixeln; negative Ausdehnungen werden auf 0 begrenzt.
 * @param label_fraction Anteil der linken Spalte an der Breite ohne Zwischenraum (0 bis 1).
 * @param gap Zwischenraum in Pixeln; wird auf die verfügbare Breite begrenzt.
 * @return Nicht überlappende Spalten innerhalb des begrenzten Zeilenbereichs.
 * Gleiche Breite, Anteil und Abstand ergeben über mehrere Zeilen dieselben Spaltenkanten.
 */
UiPropertyRow ui_property_row(UiRect area, float label_fraction, float gap);

/** @brief Geometrie eines teilbaren Bereichs; second_size ist die begrenzte zweite Paneelgröße. */
typedef struct {
    UiRect first;      ///< Erstes Panel.
    UiRect grip;       ///< Ziehbarer Zwischenraum.
    UiRect second;     ///< Zweites Panel.
    float second_size; ///< Tatsächliche zweite Panelgröße in Pixeln.
} UiSplit;

/** @brief Teilt einen Bereich horizontal (X) oder vertikal (Y), auch bei zu kleinen Fenstern.
 * Mindestgrößen werden bei Platzmangel proportional reduziert; keine negative Geometrie.
 */
UiSplit ui_split_rects(UiRect area, UiAxis axis, float second_size, float minimum_first,
                       float minimum_second, float gap);

/** @} */
/** @brief Passt ein Bild proportional in einen Bereich mit sichtbarem Innenabstand ein.
 * @param bounds Verfügbarer Bereich in UI-Pixeln.
 * @param width Positive Quellbreite.
 * @param height Positive Quellhöhe.
 * @param padding Innenabstand je Seite; wird auf mindestens 2 Pixel begrenzt.
 * @return Zentrierter Bildbereich; leer bei ungültigen oder zu kleinen Abmessungen.
 */
UiRect ui_image_fit(UiRect bounds, float width, float height, float padding);

/** @file @brief SDL-free, validated YAML widget descriptions. */
#define UI_LAYOUT_NODES 128
#define UI_LAYOUT_DEPTH 12

/** @brief Generic widget kinds; no application or document domains. */
typedef enum {
    UI_NODE_COLUMN,
    UI_NODE_ROW,
    UI_NODE_PANEL,
    UI_NODE_TOOLBAR,
    UI_NODE_BUTTON,
    UI_NODE_LABEL,
    UI_NODE_SEPARATOR,
    UI_NODE_TABS,
    UI_NODE_TAB,
    UI_NODE_SECTION,
    UI_NODE_INPUT,
    UI_NODE_TREE,
    UI_NODE_ITEM,
    UI_NODE_SPLIT,
    UI_NODE_SPACER
} UiNodeType;

/** @brief Owned description; children form a linked list of indices (-1 ends it). */
typedef struct {
    UiNodeType type;
    UiId id;
    char name[64], text[256], action[64], icon[32], image[512];
    float width, height, gap, padding, initial, minimum_first, minimum_second, image_padding;
    bool enabled, open;
    bool draggable; ///< Empty space in this widget may move a borderless window.
    UiAxis axis;
    int child, next;
} UiNode;

/** @brief Entire owned layout, allocated by the loader and released with ui_layout_free. */
typedef struct {
    char title[128], directory[1024];
    int width, height, count, root;
    uint32_t colors[7];
    UiNode nodes[UI_LAYOUT_NODES];
} UiLayout;

/** @brief Load from a file; NULL on error with a path/line diagnostic in error. */
UiLayout *ui_layout_load(const char *path, char *error, size_t capacity);
/** @brief Parse UTF-8 YAML in memory; NULL on schema, duplicate-ID or resource-limit errors. */
UiLayout *ui_layout_parse(const char *yaml, size_t length, char *error, size_t capacity);
/** @brief Release a layout; NULL is accepted. Views must stop borrowing it first. */
void ui_layout_free(UiLayout *layout);

/* SDL support is enabled by the pengUI::sdl CMake target, not by the core target. */
#if defined(PENGUI_WITH_SDL) && PENGUI_WITH_SDL

/**
 * @file
 * @brief SDL-Darstellung, Schrift, Symbole und zur Laufzeit veränderbares UI-Theme.
 * @ingroup ui_api
 */

/** @addtogroup ui_api
 * @{
 */
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
/** @brief Aktueller Theme-Farbwert UI_CANVAS als RGBA. */
#define UI_CANVAS (ui_theme.canvas)
/** @brief Aktueller Theme-Farbwert UI_PANEL als RGBA. */
#define UI_PANEL (ui_theme.panel)
/** @brief Aktueller Theme-Farbwert UI_SURFACE als RGBA. */
#define UI_SURFACE (ui_theme.surface)
/** @brief Aktueller Theme-Farbwert UI_BORDER als RGBA. */
#define UI_BORDER (ui_theme.border)
/** @brief Aktueller Theme-Farbwert UI_HOVER als RGBA. */
#define UI_HOVER (ui_theme.hover)
/** @brief Aktueller Theme-Farbwert UI_ACCENT als RGBA. */
#define UI_ACCENT (ui_theme.accent)
/** @brief Aktueller Theme-Farbwert UI_ACCENT_BG als RGBA. */
#define UI_ACCENT_BG (ui_theme.accent)
/** @brief Aktueller Theme-Farbwert UI_TEXT als RGBA. */
#define UI_TEXT (ui_theme.text)
/** @brief Aktueller Theme-Farbwert UI_MUTED als RGBA. */
#define UI_MUTED (ui_theme.muted)
/** @brief Aktueller Theme-Farbwert UI_DISABLED als RGBA. */
#define UI_DISABLED (ui_theme.disabled)

/**
 * @brief Zeichnet gestrichelte Baumverbindungen für eine Zeile; erhält den aktiven Clip.
 * @param renderer Geliehener SDL-Renderer.
 * @param depths Preorder-Tiefen aller aufgeklappten Zeilen, auch außerhalb des Viewports.
 * @param count Anzahl Einträge in depths.
 * @param row Nullbasierter aktueller Zeilenindex.
 * @param area Zeilenbereich; x ist der Ursprung der Wurzel-Einrückung, h die Zeilenhöhe.
 * @param indent Abstand pro Hierarchieebene in Pixeln.
 * @param marker_center Horizontale Mitte des Aufklappsymbols relativ zum Knotenanfang.
 * @param max_indent Maximale Einrückung; tiefere Linien werden begrenzt.
 * @param branch_end Horizontales Ende des Asts in Fensterpixeln, vor Symbol oder Label.
 * @param rgba Linienfarbe aus dem aktuellen Theme.
 */
void ui_draw_tree_guides(SDL_Renderer *renderer, const int *depths, int count, int row, UiRect area,
                         float indent, float marker_center, float max_indent, float branch_end,
                         Uint32 rgba);

/**
 * @brief Zeichnet ein einzeiliges Eingabefeld mit Auswahl, horizontalem Scrollen und Cursor.
 * @param renderer Geliehener SDL-Renderer.
 * @param rect Feldbereich in Fensterpixeln; der bisherige Render-Clip bleibt erhalten.
 * @param text Nullterminierter UTF-8-Text.
 * @param edit Feldzustand; scroll wird für den sichtbaren Cursor angepasst.
 * @param focused Ob Auswahl und orange Fokusumrandung sichtbar sind.
 * @param enabled Ob das Feld bedienbar ist.
 * @param invalid Ob eine abgewiesene Eingabe rot markiert wird.
 * @param cursor_visible Blinkphase des Cursors; wird vom Aufrufer gesteuert.
 */
void ui_draw_text_input(SDL_Renderer *renderer, SDL_FRect rect, const char *text, UiTextEdit *edit,
                        bool focused, bool enabled, bool invalid, bool cursor_visible);

/** @brief Interne Layout-/Darstellungsvorgabe UI_FONT_WIDTH. */
#define UI_FONT_WIDTH 8
/** @brief Interne Layout-/Darstellungsvorgabe UI_FONT_HEIGHT. */
#define UI_FONT_HEIGHT 16
/* UTF-8, Latin-1 glyphs; unsupported codepoints use ?. Texture cached per renderer. */
/**
 * @brief Misst UTF-8-Text mit der eingebetteten Schrift in Pixeln.
 *
 * @param text Nullterminierter UTF-8-Text.
 */
float ui_text_width(const char *text);

/**
 * @brief Zeichnet UTF-8-Text mit Latin-1-Glyphen; unbekannte Zeichen erscheinen als Fragezeichen.
 *
 * Die Schrifttextur wird pro Renderer zwischengespeichert und von SDL mit dem Renderer freigegeben.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param text Nullterminierter UTF-8-Text.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
bool ui_text(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color);

/**
 * @brief Zeichnet Atlastext mit einem positiven, endlichen Größenfaktor.
 * @param renderer Geliehener SDL-Renderer.
 * @param x Horizontale Position in Bildschirm-Pixeln.
 * @param y Vertikale Position in Bildschirm-Pixeln.
 * @param text Nullterminierter UTF-8-Text.
 * @param color Gepacktes RGBA.
 * @param scale Größenfaktor; 1 entspricht ui_text, 0.75 einer kompakten Beschriftung.
 * @return false bei ungültigen Argumenten oder einem SDL-Renderfehler.
 */
bool ui_text_scaled(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color,
                    float scale);

/** @brief UI-Palette als gepackte RGBA-Werte im Format 0xRRGGBBAA. */
typedef struct {
    Uint32 canvas;   ///< Hintergrundfarbe der Zeichenfläche als RGBA.
    Uint32 panel;    ///< Panelfarbe als RGBA.
    Uint32 surface;  ///< Farbe angehobener Bedienflächen als RGBA.
    Uint32 hover;    ///< Hoverfarbe als RGBA.
    Uint32 accent;   ///< Akzentfarbe als RGBA.
    Uint32 text;     ///< Standardtextfarbe als RGBA.
    Uint32 muted;    ///< Farbe zurückhaltender Beschriftungen als RGBA.
    Uint32 disabled; ///< Farbe deaktivierter Elemente als RGBA.
    Uint32 border;   ///< Rahmenfarbe als RGBA.
} UiTheme;

/** @brief Gemeinsame UI-Palette; über ui_set_theme aktualisieren. */
extern UiTheme ui_theme;

/**
 * @brief Kopiert ein Theme in den gemeinsam verwendeten UI-Farbzustand.
 *
 * @param theme Palette; Werte werden beim Setzen kopiert.
 */
void ui_set_theme(const UiTheme *theme);

/**
 * @brief Liefert die dunkelgraue Standardpalette mit gedämpftem orangefarbenem Akzent.
 */
UiTheme ui_dark_theme(void);

/** @brief Eingebautes, mit SDL-Primitiven gezeichnetes UI-Symbol. */
typedef enum {
    ICON_NONE,
    ICON_SAVE,
    ICON_UNDO,
    ICON_REDO,
    ICON_PLAY,
    ICON_STOP,
    ICON_IMPORT,
    ICON_BRUSH,
    ICON_ERASE,
    ICON_RECTANGLE,
    ICON_FILL,
    ICON_SELECT,
    ICON_COLLISION,
    ICON_GRID,
    ICON_REFRESH,
    ICON_FOLDER,
    ICON_FILE,
    ICON_IMAGE,
    ICON_LAYERS,
    ICON_ENTITY,
    ICON_PLUS,
    ICON_DELETE,
    ICON_EDIT,
    ICON_UP,
    ICON_DOWN,
    ICON_RIGHT,
    ICON_EYE,
    ICON_EYE_OFF,
    ICON_CHECK,
    ICON_CLOSE,
    ICON_MENU,
    ICON_TERMINAL, ///< Shell-/Konsolensymbol.
    ICON_MINIMIZE, ///< Fenster minimieren.
    ICON_MAXIMIZE, ///< Fenster maximieren.
    ICON_RESTORE,  ///< Fenstergröße wiederherstellen.
    ICON_COLLAPSE  ///< Fenster auf Titelleiste reduzieren.
} UiIcon;

/**
 * @brief Zeichnet einen flachen Icon-/Textbutton mit expliziter Palette.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param theme Palette; Werte werden beim Setzen kopiert.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_button_themed(SDL_Renderer *renderer, const UiTheme *theme, SDL_FRect rect,
                           UiIcon icon, const char *label, bool active, bool enabled, bool hover);

/**
 * @brief Füllt eine rechteckige Panelfläche mit einer RGBA-Farbe.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
void ui_panel(SDL_Renderer *renderer, SDL_FRect rect, Uint32 color);

/**
 * @brief Zeichnet ein skalierbares Symbol aus SDL-Linien und -Flächen.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param size Symbolkantenlänge in Pixeln.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
void ui_icon(SDL_Renderer *renderer, UiIcon icon, float x, float y, float size, Uint32 color);

/**
 * @brief Zeichnet einen flachen Icon-/Textbutton mit der aktuellen Palette.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_button(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                    bool active, bool enabled, bool hover);

/**
 * @brief Berechnet die benötigte Tabbreite für Icon, Beschriftung und Innenabstände.
 *
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 */
float ui_tab_content_width(UiIcon icon, const char *label);

/**
 * @brief Zeichnet einen Panel-Tab mit aktuellem Theme.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 * @param active Ob das Element als ausgewählt gezeichnet wird.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 * @param hover Ob das Element als unter dem Zeiger gezeichnet wird.
 */
void ui_draw_tab(SDL_Renderer *renderer, SDL_FRect rect, UiIcon icon, const char *label,
                 bool active, bool enabled, bool hover);

/**
 * @brief Zeichnet Track und Griff passend zum Interaktionszustand.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param scroll Scrollzustand in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
void ui_draw_scrollbar(SDL_Renderer *renderer, const UiContext *context, UiId id, UiRect track,
                       const UiScroll *scroll, UiAxis axis);

/**
 * @brief Überträgt den aktuellen UI-Clip auf den SDL-Renderer.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void ui_apply_clip(SDL_Renderer *renderer, const UiContext *context);
/** @} */
/** @brief Zeichnet einen Bildbutton mit sichtbarem Zustandsrand und optionaler Beschriftung.
 * @param renderer Geliehener Renderer der Textur.
 * @param rect Buttonbereich in UI-Pixeln.
 * @param image Geliehene Bildtextur; Farbe und Alpha werden nach dem Zeichnen wiederhergestellt.
 * @param label Optionaler nullterminierter Text.
 * @param padding Innenabstand in Pixeln, mindestens 2; empfohlen 4.
 * @param active Ausgewähltes Werkzeug.
 * @param enabled Bedienbarkeit; deaktivierte Bilder werden abgeschwächt.
 * @param hover Zeiger über dem Button.
 * @param pressed Laufender Tastendruck.
 * @param focused Tastaturfokus; zeichnet einen Akzentrahmen.
 */
void ui_draw_image_button(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                          const char *label, float padding, bool active, bool enabled, bool hover,
                          bool pressed, bool focused);

/** @file @brief SDL presentation of generic YAML widgets. */
/** @brief Application callback. Both strings are borrowed for the duration of the call. */
typedef void (*UiAction)(void *context, const char *action, const char *widget);
/** @brief Opaque view owns widget state and cached textures, borrows its layout and renderer. */
typedef struct UiView UiView;
/** @brief Create one independent view; layout/renderer must outlive it. NULL on failure. */
UiView *ui_view_new(const UiLayout *layout, SDL_Renderer *renderer);
/** @brief Destroy before the renderer/layout; NULL is accepted. */
void ui_view_free(UiView *view);
/** @brief Apply the seven theme colors from YAML to the existing renderer theme. */
void ui_view_apply_theme(const UiLayout *layout);
/** @brief Feed SDL input on the main thread; true when a text edit consumed the event. */
bool ui_view_event(UiView *view, UiContext *ui, SDL_Window *window, const SDL_Event *event);
/** @brief Draw a frame; callbacks may change app state but must not free the active view/layout. */
void ui_view_draw(UiView *view, UiContext *ui, UiRect area, UiAction action, void *context);
/** @brief Read an input value by YAML id; NULL for unknown/non-input widgets. Borrowed until free.
 */
const char *ui_view_text(const UiView *view, const char *id);
/** @brief Set input/label text without modifying YAML; false for wrong kinds or excessive text. */
bool ui_view_set_text(UiView *view, const char *id, const char *text);
/** @brief Load a bounded PNG image as a renderer-owned texture; caller must destroy it. */
SDL_Texture *ui_image_load(SDL_Renderer *renderer, const char *path);

/** @file @brief Optional borderless SDL window controls, independent of application actions. */

/** @brief Stable callback context. Must outlive the attached window or be detached first. */
typedef struct {
    SDL_Window *window;  ///< Borrowed window.
    const UiView *view;  ///< Borrowed view; update after YAML reload.
    const UiContext *ui; ///< Borrowed interaction context.
} UiWindow;

/** @brief Attach hit testing and remove native borders; retain native borders if unsupported. */
bool ui_window_attach(UiWindow *chrome, SDL_Window *window, const UiView *view,
                      const UiContext *ui);
/** @brief Remove callback before releasing its borrowed view/context. */
void ui_window_detach(UiWindow *chrome);
/** @brief Compute resize/drag regions using the last drawn YAML view; coordinates are window-local.
 */
SDL_HitTestResult ui_window_hit_test(const UiWindow *chrome, int x, int y);
/** @brief Handle window.minimize/maximize/close. Close queues a close request for the application.
 */
bool ui_window_action(SDL_Window *window, const char *action);

#endif /* PENGUI_WITH_SDL */

#endif /* PENGUI_H */

/** @file
 * @brief Public pengUI API: SDL-free core and optional SDL3 presentation.
 * Link pengUI::sdl to expose the SDL API through PUI_WITH_SDL=1.
 */
#ifndef PUI_H
#define PUI_H

/* Core: input, layout geometry and YAML. No SDL dependency. */

/**
 * @file
 * @brief Wiederverwendbare UI-Eingabe, Layoutzustände und Widgetinteraktionen ohne SDL.
 * @ingroup pui_api
 */

/** @addtogroup pui_api
 * @{
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Maximale Tiefe des UI-Clip-Stacks. */
#define PUI_MAX_CLIPS 16
/** @brief Maximale Anzahl Tabs pro Gruppe. */
#define PUI_MAX_TABS 16
/** @brief Standardabstand zwischen Tabs in Pixeln. */
#define PUI_TAB_GAP 2
/** @brief Kontextweit eindeutiger, stabiler Widgetbezeichner; 0 bedeutet keine Auswahl. */
typedef uint32_t PUIId;

/** @brief Achsenparalleler UI-Bereich in Pixeln. */
typedef struct {
    float x; ///< Horizontale Position.
    float y; ///< Vertikale Position.
    float w; ///< Breite.
    float h; ///< Höhe.
} PUIRect;

/** @brief Achse einer Zieh- oder Scrollinteraktion. */
typedef enum { PUI_AXIS_X, PUI_AXIS_Y } PUIAxis;

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
    PUIId active;      ///< ID des Widgets mit aktiver Zeigerinteraktion; 0 bedeutet keines.
    PUIId focus;       ///< ID des Tastaturfokus; 0 bedeutet keiner.
    float drag_origin; ///< Zeigerkoordinate beim Beginn des Ziehens.
    float drag_value;  ///< Ausgangswert beim Beginn des Ziehens.
    bool dragging;     ///< Ob der aktive Drag-Schwellwert überschritten wurde.
    PUIRect clips[PUI_MAX_CLIPS]; ///< Begrenzter Stack geschnittener UI-Cliprechtecke.
    int clip_depth;               ///< Aktuelle Tiefe des Clip-Stacks.
} PUIContext;

/** @brief Scrollzustand in Pixeln für Inhalt und sichtbaren Viewport. */
typedef struct {
    float offset;   ///< Aktueller Scrolloffset in Pixeln.
    float content;  ///< Gesamtausdehnung des Inhalts in Pixeln.
    float viewport; ///< Sichtbarer Inhaltsbereich bzw. dessen Ausdehnung in Pixeln.
} PUIScroll;

/** @brief Stabile Tab-ID, Bedienbarkeit und optionale individuelle Slotbreite. */
typedef struct {
    PUIId id;     ///< Stabile Widget- oder Editor-ID.
    bool enabled; ///< Ob Interaktionen erlaubt sind.
    float width;  ///< Slotbreite einschließlich Abstand; 0 verwendet die Fallbackbreite.
} PUITab;

/** @brief Besitzende Kopie der Tabreihenfolge mit Auswahl, Abstand und Scroll-/Dragzustand. */
typedef struct {
    PUITab items[PUI_MAX_TABS]; ///< Besitzende Kopie der Tab-Einträge in aktueller Reihenfolge.
    int count;                  ///< Anzahl gültiger Einträge.
    PUIId selected;             ///< Stabile ID der aktuellen Auswahl; 0 bedeutet keine Auswahl.
    float scroll;               ///< Horizontaler Tab-Scrolloffset in Pixeln.
    float gap;                  ///< Abstand zwischen UI-Flächen bzw. Tabs in Pixeln.
    bool dragging;              ///< Zustand der laufenden Ziehinteraktion.
} PUITabs;

/**
 * @brief Bildet einen stabilen, von 0 verschiedenen Bezeichner aus einer Zeichenkette.
 *
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 */
PUIId pui_id(const char *label);

/**
 * @brief Setzt einen UI-Kontext in seinen leeren Ausgangszustand zurück.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_init(PUIContext *context);
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
void pui_pointer(PUIContext *context, float x, float y, bool down);

/**
 * @brief Bricht die aktuelle Zeigerinteraktion und Aktivierung ab.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_cancel(PUIContext *context);

/**
 * @brief Setzt den Tastaturfokus; ID 0 entfernt den Fokus.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 */
void pui_focus(PUIContext *context, PUIId id);

/**
 * @brief Fordert die Tastaturaktivierung des fokussierten Elements an.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_activate(PUIContext *context);

/**
 * @brief Schließt einen UI-Frame ab und verbraucht dessen kurzlebige Eingabezustände.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_end_frame(PUIContext *context);

/**
 * @brief Prüft einen Punkt gegen ein nichtleeres Rechteck mit exklusiver rechter/unterer Grenze.
 *
 * @param rect Elementrechteck in UI-Pixeln.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 */
bool pui_contains(PUIRect rect, float x, float y);

/**
 * @brief Prüft Zeigerposition, Clip und globalen Eingabeblock für ein Element.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 */
bool pui_hovered(const PUIContext *context, PUIRect rect);

/**
 * @brief Legt den Schnitt mit dem bisherigen Clip auf den begrenzten Clip-Stack.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param clip Neuer Clipbereich in UI-Pixeln.
 */
bool pui_clip_push(PUIContext *context, PUIRect clip);

/**
 * @brief Entfernt den obersten Clip, sofern einer vorhanden ist.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_clip_pop(PUIContext *context);
/* Stable, nonzero IDs must be unique within a context. Activation is on release. */
/**
 * @brief Verarbeitet eine Buttoninteraktion mit Aktivierung beim Loslassen oder per Tastatur.
 *
 * IDs müssen stabil, von 0 verschieden und innerhalb eines PUIContext eindeutig sein.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 */
bool pui_button(PUIContext *context, PUIId id, PUIRect rect, bool enabled);

/** @brief Toggle a caller-owned value on activation. Returns true only when changed.
 * A NULL value is ignored. Uses the same pointer, keyboard and disabled rules as buttons.
 */
bool pui_toggle(PUIContext *context, PUIId id, PUIRect rect, bool *checked, bool enabled);

/** @brief Select one ID in a caller-owned group; clicking the active choice cannot clear it.
 * Returns true only when selection changes. A NULL selection is ignored.
 */
bool pui_radio(PUIContext *context, PUIId id, PUIRect rect, PUIId *selected, bool enabled);

/**
 * @brief Verarbeitet einen Button und schaltet den übergebenen Aufklappzustand um.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param open Ein-/Ausgabe des Aufklappzustands.
 * @param enabled Ob Eingabe für das Element erlaubt ist.
 */
bool pui_disclosure(PUIContext *context, PUIId id, PUIRect rect, bool *open, bool enabled);

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
bool pui_drag_value(PUIContext *context, PUIId id, PUIRect rect, float *value, float minimum,
                    float maximum, PUIAxis axis, float scale);

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
void pui_columns_fit(float total, float *left, float *right, float min_left, float min_right,
                     float min_center);

/**
 * @brief Liefert den nichtnegativen maximalen Scrolloffset.
 *
 * @param scroll Scrollzustand in Pixeln.
 */
float pui_scroll_max(const PUIScroll *scroll);

/**
 * @brief Beschränkt den Scrolloffset auf den verfügbaren Inhaltsbereich.
 *
 * @param scroll Scrollzustand in Pixeln.
 */
void pui_scroll_clamp(PUIScroll *scroll);

/**
 * @brief Addiert eine Pixeldistanz und begrenzt anschließend den Scrolloffset.
 *
 * @param scroll Scrollzustand in Pixeln.
 * @param delta Additive Scrolldistanz in Pixeln.
 */
void pui_scroll_by(PUIScroll *scroll, float delta);

/**
 * @brief Berechnet das Scrollbar-Griffrechteck aus Inhalt, Viewport und Track.
 *
 * @param scroll Scrollzustand in Pixeln.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
PUIRect pui_scroll_thumb(const PUIScroll *scroll, PUIRect track, PUIAxis axis);

/**
 * @brief Verarbeitet Scrollbar-Ziehen und Track-Klicks und aktualisiert den Scrolloffset.
 *
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param track Rechteck der Scrollbarspur in Pixeln.
 * @param scroll Scrollzustand in Pixeln.
 * @param axis Achse für Ziehen oder Scrollen.
 */
bool pui_scrollbar(PUIContext *context, PUIId id, PUIRect track, PUIScroll *scroll, PUIAxis axis);

/**
 * @brief Validiert und kopiert eine begrenzte Liste eindeutiger, nichtleerer Tab-IDs.
 *
 * @return false bei ungültiger Anzahl, doppelten IDs oder fehlender Auswahl.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param items Zu kopierende Liste der Tab-Einträge; darf bei count=0 NULL sein.
 * @param count Anzahl zu kopierender Einträge, 0 bis PUI_MAX_TABS.
 * @param selected ID des anfangs ausgewählten Tabs; muss bei count=0 ebenfalls 0 sein.
 */
bool pui_tabs_init(PUITabs *tabs, const PUITab *items, int count, PUIId selected);

/**
 * @brief Setzt die individuelle Slotbreite eines Tabs.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param id Stabile, kontextweit eindeutige Widget-ID.
 * @param width Breite in den von der Funktion beschriebenen Einheiten.
 */
bool pui_tabs_set_width(PUITabs *tabs, PUIId id, float width);

/**
 * @brief Berechnet das sichtbare Tabrechteck ohne den konfigurierten Zwischenraum.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param bar Sichtbarer Bereich der Tabgruppe in Pixeln.
 * @param width Fallback-Slotbreite einschließlich Tababstand in Pixeln.
 * @param index Nullbasierter Index des gewünschten Eintrags.
 */
PUIRect pui_tab_rect(const PUITabs *tabs, PUIRect bar, float width, int index);

/**
 * @brief Scrollt eine Tabgruppe horizontal innerhalb ihrer Inhaltsgrenzen.
 *
 * @param tabs Tabzustand mit stabilen IDs.
 * @param viewport Sichtbarer Zeichenbereich bzw. seine Ausdehnung in Pixeln.
 * @param tab_width Fallback-Slotbreite einschließlich Abstand in Pixeln.
 * @param delta Additive Scrolldistanz in Pixeln.
 */
void pui_tabs_scroll(PUITabs *tabs, float viewport, float tab_width, float delta);
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
bool pui_tabs_update(PUIContext *context, PUITabs *tabs, PUIRect bar, float tab_width);

/** @brief Zustand eines einzeiligen UTF-8-Eingabefelds; der Textpuffer gehört dem Aufrufer. */
typedef struct {
    size_t cursor; ///< Cursorposition als Byteoffset an einer UTF-8-Grenze.
    size_t anchor; ///< Anfang der Auswahl als Byteoffset; cursor == anchor bedeutet keine Auswahl.
    float scroll;  ///< Horizontaler Darstellungsversatz in Pixeln.
} PUITextEdit;

/** @brief Renderer-unabhängige Bearbeitungstasten eines einzeiligen Felds. */
typedef enum {
    PUI_TEXT_LEFT,
    PUI_TEXT_RIGHT,
    PUI_TEXT_HOME,
    PUI_TEXT_END,
    PUI_TEXT_BACKSPACE,
    PUI_TEXT_DELETE,
    PUI_TEXT_SELECT_ALL
} PUITextKey;

/** @brief Initialisiert den Cursor am Textende und wählt optional den ganzen Text aus.
 * @param edit Besitzender Feldzustand.
 * @param text Nullterminierter UTF-8-Text; wird nicht verändert.
 * @param select_all Ob der Anfangswert zum direkten Ersetzen markiert wird.
 */
void pui_text_edit_init(PUITextEdit *edit, const char *text, bool select_all);
/** @brief Fügt UTF-8-Text am Cursor ein oder ersetzt die Auswahl atomar.
 * @param edit Feldzustand, dessen Cursor und Auswahl aktualisiert werden.
 * @param text Veränderbarer, nullterminierter Puffer mit mindestens capacity Bytes.
 * @param capacity Gesamtkapazität einschließlich Nullbyte.
 * @param inserted Geliehener UTF-8-Text; darf nicht mit text überlappen.
 * @return false bei Platzmangel, ungültigem UTF-8 oder Steuerzeichen; Text bleibt unverändert.
 */
bool pui_text_edit_insert(PUITextEdit *edit, char *text, size_t capacity, const char *inserted);
/** @brief Bewegt Cursor/Auswahl oder löscht ganze UTF-8-Zeichen.
 * @param edit Feldzustand.
 * @param text Veränderbarer, nullterminierter UTF-8-Puffer.
 * @param key Bearbeitungsaktion.
 * @param shift Ob Navigation die bestehende Auswahl erweitert.
 * @return true, wenn der Text durch Löschen verändert wurde.
 */
bool pui_text_edit_key(PUITextEdit *edit, char *text, PUITextKey key, bool shift);
/** @brief Setzt den Cursor auf eine nullbasierte Zeichenspalte, begrenzt auf das Textende.
 * @param edit Feldzustand.
 * @param text Nullterminierter UTF-8-Text.
 * @param column Zeichenspalte, unabhängig von der UTF-8-Bytelänge.
 * @param shift Ob die Auswahl erweitert wird.
 */
void pui_text_edit_click(PUITextEdit *edit, const char *text, size_t column, bool shift);
/** @brief Ermittelt die Zeichenspalte zu einem auf die Textlänge begrenzten Byteoffset.
 * @param text Nullterminierter UTF-8-Text.
 * @param offset Byteoffset; wird auf Textlänge und Zeichengrenze begrenzt.
 */
size_t pui_text_edit_column(const char *text, size_t offset);
/** @brief Select a word at a zero-based character column, preserving UTF-8 boundaries.
 * Groups ASCII letters/digits/underscore and non-ASCII codepoints as words; spaces and
 * ASCII punctuation form separate runs. At end-of-text selects the preceding run.
 */
void pui_text_edit_select_word(PUITextEdit *edit, const char *text, size_t column);

/** @brief Prüft die Fortsetzung einer Hierarchieebene in einer aufgeklappten Baum-Liste.
 * @param depths Geliehene Preorder-Liste nichtnegativer Einrückungstiefen.
 * @param count Anzahl sichtbarer (nicht eingeklappter) Einträge, einschließlich außerhalb des
 * Viewports.
 * @param row Nullbasierter aktueller Zeilenindex.
 * @param level Eigene oder übergeordnete Tiefe, deren nächstes Geschwister gesucht wird.
 * @return true bei einem folgenden Eintrag derselben Ebene vor dem Ende des Teilbaums.
 */
bool pui_tree_continues(const int *depths, int count, int row, int level);

/** @brief Zweispaltige Eigenschaftszeile ohne eigenen Zustand oder Rendering. */
typedef struct {
    PUIRect label; ///< Linke Spalte für die Bezeichnung.
    PUIRect value; ///< Rechte Spalte für Wert oder Eingabefeld.
} PUIPropertyRow;

/**
 * @brief Teilt eine Eigenschaftszeile in bündige Bezeichnungs- und Wertspalten.
 * @param area Zeilenbereich in Pixeln; negative Ausdehnungen werden auf 0 begrenzt.
 * @param label_fraction Anteil der linken Spalte an der Breite ohne Zwischenraum (0 bis 1).
 * @param gap Zwischenraum in Pixeln; wird auf die verfügbare Breite begrenzt.
 * @return Nicht überlappende Spalten innerhalb des begrenzten Zeilenbereichs.
 * Gleiche Breite, Anteil und Abstand ergeben über mehrere Zeilen dieselben Spaltenkanten.
 */
PUIPropertyRow pui_property_row(PUIRect area, float label_fraction, float gap);

/** @brief Geometrie eines teilbaren Bereichs; second_size ist die begrenzte zweite Paneelgröße. */
typedef struct {
    PUIRect first;     ///< Erstes Panel.
    PUIRect grip;      ///< Ziehbarer Zwischenraum.
    PUIRect second;    ///< Zweites Panel.
    float second_size; ///< Tatsächliche zweite Panelgröße in Pixeln.
} PUISplit;

/** @brief Teilt einen Bereich horizontal (X) oder vertikal (Y), auch bei zu kleinen Fenstern.
 * Mindestgrößen werden bei Platzmangel proportional reduziert; keine negative Geometrie.
 */
PUISplit pui_split_rects(PUIRect area, PUIAxis axis, float second_size, float minimum_first,
                         float minimum_second, float gap);

/** @} */
/** @brief Passt ein Bild proportional in einen Bereich mit sichtbarem Innenabstand ein.
 * @param bounds Verfügbarer Bereich in UI-Pixeln.
 * @param width Positive Quellbreite.
 * @param height Positive Quellhöhe.
 * @param padding Innenabstand je Seite; wird auf mindestens 2 Pixel begrenzt.
 * @return Zentrierter Bildbereich; leer bei ungültigen oder zu kleinen Abmessungen.
 */
PUIRect pui_image_fit(PUIRect bounds, float width, float height, float padding);

/** @file @brief SDL-free, validated YAML widget descriptions. */
// Includes composed editor layouts and generated color-picker input fields.
#define PUI_LAYOUT_NODES 256
#define PUI_LAYOUT_DEPTH 24

/** @brief Generic widget kinds; no application or document domains. */
/** @brief HSL lightness color: hue in degrees, saturation/lightness/alpha in [0,1]. */
typedef struct {
    float h, s, l, a;
} PUIHSLColor;

/** @brief Convert packed 0xRRGGBBAA to HSL; achromatic colors have hue zero. */
PUIHSLColor pui_color_to_hsl(uint32_t rgba);
/** @brief Convert HSL to 0xRRGGBBAA; finite values are clamped, hue wraps at 360 degrees. */
uint32_t pui_color_from_hsl(PUIHSLColor hsla);
/** @brief Parse #RGB, #RGBA, #RRGGBB or #RRGGBBAA; failure leaves the output unchanged. */
bool pui_color_parse_hex(const char *text, uint32_t *rgba);
/** @brief Write canonical #RRGGBBAA including terminator into a ten-byte buffer. */
void pui_color_format_hex(uint32_t rgba, char text[10]);

typedef enum {
    PUI_NODE_COLUMN,
    PUI_NODE_ROW,
    PUI_NODE_PANEL,
    PUI_NODE_TOOLBAR,
    PUI_NODE_BUTTON,
    PUI_NODE_LABEL,
    PUI_NODE_SEPARATOR,
    PUI_NODE_TABS,
    PUI_NODE_TAB,
    PUI_NODE_SECTION,
    PUI_NODE_INPUT,
    PUI_NODE_TREE,
    PUI_NODE_ITEM,
    PUI_NODE_SPLIT,
    PUI_NODE_SPACER,
    PUI_NODE_TOGGLE,
    PUI_NODE_CHECKBOX,
    PUI_NODE_BUTTON_GROUP,
    PUI_NODE_DROPDOWN,
    PUI_NODE_OPTION,
    PUI_NODE_POPUP_BUTTON,
    PUI_NODE_SLOT,
    PUI_NODE_CANVAS,
    PUI_NODE_COLOR_PICKER,
    PUI_NODE_RADIAL_MENU
} PUINodeType;

/** @brief Hit-test 2..8 radial sectors, clockwise from top; -1 for center, gaps or outside. */
int pui_radial_item(PUIRect bounds, int count, float x, float y);

/** @brief Geometry of a two/three-pane splitter; unused entries are zero. */
typedef struct {
    PUIRect panes[3], grips[2];
    int count;
} PUIRegions;

/** @brief Keep non-flexible preferred sizes; the flexible pane fills the remainder.
 * Constraints shrink proportionally when the area is too small. Inputs are not mutated.
 * Only the first count entries of preferred/minimum are used.
 * Returns count=0 for invalid count/flexible index or NULL arrays.
 */
PUIRegions pui_split_layout(PUIRect area, PUIAxis axis, int count, int flexible,
                            const float preferred[3], const float minimum[3], float gap);

/** @brief Preferred opening direction; flips to the opposite side when space is limited. */
typedef enum { PUI_POPUP_BELOW, PUI_POPUP_RIGHT } PUIPopupPlacement;

/** @brief Position a popup within bounds, flipping or clamping when necessary. */
PUIRect pui_popup_rect(PUIRect bounds, PUIRect anchor, float width, float height,
                       PUIPopupPlacement placement);

/** @brief Owned description; children form a linked list of indices (-1 ends it). */
typedef struct {
    PUINodeType type;
    PUIId id;
    char name[64], text[256], action[64], icon[128], image[512];
    float width, height, gap, padding, initial, minimum_first, minimum_second, image_padding;
    float initial_second; ///< Preferred middle size for three-pane splits.
    float initial_last;   ///< Preferred size of the final pane.
    float minimum_last;   ///< Minimum final-pane size; defaults to minimum_second for two panes.
    int flexible;         ///< Zero-based pane that absorbs resizing; split nodes only.
    bool enabled, open;
    bool generated_id; ///< ID was generated by the parser; use explicit IDs for persistent state.
    bool checked;   ///< Initial toggle/checkbox state, selected group button, or initially selected
                    ///< tab.
    bool draggable; ///< Empty space in this widget may move a borderless window.
    PUIAxis axis;
    char preview_action[64]; ///< Optional color-picker live preview/cancellation action.
    int color_mode;          ///< Color picker: 0=HSL, 1=RGBA.
    char value[128]; ///< Stable option value, initial dropdown value, or initial color-picker hex.
    float popup_width, popup_height;
    PUIPopupPlacement placement;
    int child, next;
} PUINode;

/** @brief Entire owned layout, allocated by the loader and released with pui_layout_free. */
typedef struct {
    char title[128], directory[1024];
    int width, height, count, root;
    uint32_t colors[7];
    PUINode nodes[PUI_LAYOUT_NODES];
} PUILayout;

/** @brief Load from a file; NULL on error with a path/line diagnostic in error. */
PUILayout *pui_layout_load(const char *path, char *error, size_t capacity);
/** @brief Parse UTF-8 YAML in memory; NULL on schema, duplicate-ID or resource-limit errors. */
PUILayout *pui_layout_parse(const char *yaml, size_t length, char *error, size_t capacity);
/** @brief Release a layout; NULL is accepted. Views must stop borrowing it first. */
void pui_layout_free(PUILayout *layout);
/** @brief Copy a layout and replace the named slot's content. NULL content empties the slot.
 * Both inputs remain caller-owned. Explicit IDs must be unique across the composed layout;
 * generated IDs are reassigned. Images retain their source file's directory.
 * Returns a new owned layout or NULL with an error; input layouts are never modified.
 */
PUILayout *pui_layout_compose(const PUILayout *base, const char *slot, const PUILayout *content,
                              char *error, size_t capacity);
/** @brief Append a tab with copied content to a named tabs node, without changing inputs.
 * Explicit content IDs are prefixed with "tab_id."; generated IDs are reassigned.
 * Enforces tab/node/depth limits and unique IDs. Caller owns the returned layout.
 */
PUILayout *pui_layout_add_tab(const PUILayout *base, const char *tabs, const char *tab_id,
                              const char *title, const PUILayout *content, char *error,
                              size_t capacity);

/* SDL support is enabled by the pengUI::sdl CMake target, not by the core target. */
#if defined(PUI_WITH_SDL) && PUI_WITH_SDL

/**
 * @file
 * @brief SDL-Darstellung, Schrift, Symbole und zur Laufzeit veränderbares UI-Theme.
 * @ingroup pui_api
 */

/** @addtogroup pui_api
 * @{
 */
#include <SDL3/SDL.h>
/* Editor theme: RGBA, separate from game artwork. */
/** @brief Aktueller Theme-Farbwert PUI_CANVAS als RGBA. */
#define PUI_CANVAS (pui_theme.canvas)
/** @brief Aktueller Theme-Farbwert PUI_PANEL als RGBA. */
#define PUI_PANEL (pui_theme.panel)
/** @brief Aktueller Theme-Farbwert PUI_SURFACE als RGBA. */
#define PUI_SURFACE (pui_theme.surface)
/** @brief Aktueller Theme-Farbwert PUI_BORDER als RGBA. */
#define PUI_BORDER (pui_theme.border)
/** @brief Aktueller Theme-Farbwert PUI_HOVER als RGBA. */
#define PUI_HOVER (pui_theme.hover)
/** @brief Aktueller Theme-Farbwert PUI_ACCENT als RGBA. */
#define PUI_ACCENT (pui_theme.accent)
/** @brief Aktueller Theme-Farbwert PUI_ACCENT_BG als RGBA. */
#define PUI_ACCENT_BG (pui_theme.accent)
/** @brief Aktueller Theme-Farbwert PUI_TEXT als RGBA. */
#define PUI_TEXT (pui_theme.text)
/** @brief Aktueller Theme-Farbwert PUI_MUTED als RGBA. */
#define PUI_MUTED (pui_theme.muted)
/** @brief Aktueller Theme-Farbwert PUI_DISABLED als RGBA. */
#define PUI_DISABLED (pui_theme.disabled)

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
void pui_draw_tree_guides(SDL_Renderer *renderer, const int *depths, int count, int row,
                          PUIRect area, float indent, float marker_center, float max_indent,
                          float branch_end, Uint32 rgba);

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
void pui_draw_text_input(SDL_Renderer *renderer, SDL_FRect rect, const char *text,
                         PUITextEdit *edit, bool focused, bool enabled, bool invalid,
                         bool cursor_visible);

/** @brief Interne Layout-/Darstellungsvorgabe PUI_FONT_WIDTH. */
#define PUI_FONT_WIDTH 8
/** @brief Interne Layout-/Darstellungsvorgabe PUI_FONT_HEIGHT. */
#define PUI_FONT_HEIGHT 16
/* UTF-8, Latin-1 glyphs; unsupported codepoints use ?. Texture cached per renderer. */
/**
 * @brief Misst UTF-8-Text mit der eingebetteten Schrift in Pixeln.
 *
 * @param text Nullterminierter UTF-8-Text.
 */
float pui_text_width(const char *text);

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
bool pui_text(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color);

/**
 * @brief Zeichnet Atlastext mit einem positiven, endlichen Größenfaktor.
 * @param renderer Geliehener SDL-Renderer.
 * @param x Horizontale Position in Bildschirm-Pixeln.
 * @param y Vertikale Position in Bildschirm-Pixeln.
 * @param text Nullterminierter UTF-8-Text.
 * @param color Gepacktes RGBA.
 * @param scale Größenfaktor; 1 entspricht pui_text, 0.75 einer kompakten Beschriftung.
 * @return false bei ungültigen Argumenten oder einem SDL-Renderfehler.
 */
bool pui_text_scaled(SDL_Renderer *renderer, float x, float y, const char *text, Uint32 color,
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
} PUITheme;

/** @brief Gemeinsame UI-Palette; über pui_set_theme aktualisieren. */
extern PUITheme pui_theme;

/**
 * @brief Kopiert ein Theme in den gemeinsam verwendeten UI-Farbzustand.
 *
 * @param theme Palette; Werte werden beim Setzen kopiert.
 */
void pui_set_theme(const PUITheme *theme);

/**
 * @brief Liefert die dunkelgraue Standardpalette mit gedämpftem orangefarbenem Akzent.
 */
PUITheme pui_dark_theme(void);

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
    ICON_COLLAPSE, ///< Fenster auf Titelleiste reduzieren.
    ICON_TABLER_BASE =
        0x10000 ///< Internal range for named Tabler icons; resolve names instead of storing values.
} PUIIcon;

/** @brief Resolve built-in names or bundled tabler:<name> icons; ICON_NONE if unknown. */
PUIIcon pui_icon_from_name(const char *name);

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
void pui_draw_button_themed(SDL_Renderer *renderer, const PUITheme *theme, SDL_FRect rect,
                            PUIIcon icon, const char *label, bool active, bool enabled, bool hover);

/**
 * @brief Füllt eine rechteckige Panelfläche mit einer RGBA-Farbe.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param rect Elementrechteck in UI-Pixeln.
 * @param color Gepackte Farbe im Format 0xRRGGBBAA.
 */
void pui_panel(SDL_Renderer *renderer, SDL_FRect rect, Uint32 color);

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
void pui_icon(SDL_Renderer *renderer, PUIIcon icon, float x, float y, float size, Uint32 color);

/** @brief Draw a checkbox and label. Interaction/state are supplied separately via pui_toggle. */
void pui_draw_checkbox(SDL_Renderer *renderer, SDL_FRect rect, const char *label, bool checked,
                       bool enabled, bool hover, bool focused);

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
void pui_draw_button(SDL_Renderer *renderer, SDL_FRect rect, PUIIcon icon, const char *label,
                     bool active, bool enabled, bool hover);

/**
 * @brief Berechnet die benötigte Tabbreite für Icon, Beschriftung und Innenabstände.
 *
 * @param icon Eingebautes Symbol oder ICON_NONE.
 * @param label Nullterminierter stabiler Name bzw. anzuzeigende Beschriftung.
 */
float pui_tab_content_width(PUIIcon icon, const char *label);

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
void pui_draw_tab(SDL_Renderer *renderer, SDL_FRect rect, PUIIcon icon, const char *label,
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
void pui_draw_scrollbar(SDL_Renderer *renderer, const PUIContext *context, PUIId id, PUIRect track,
                        const PUIScroll *scroll, PUIAxis axis);

/**
 * @brief Überträgt den aktuellen UI-Clip auf den SDL-Renderer.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param context Initialisierter UI-Kontext; muss während des Frames derselbe bleiben.
 */
void pui_apply_clip(SDL_Renderer *renderer, const PUIContext *context);
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
void pui_draw_image_button(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                           const char *label, float padding, bool active, bool enabled, bool hover,
                           bool pressed, bool focused);

/** @file @brief SDL presentation of generic YAML widgets. */
/** @brief Application callback. Both strings are borrowed for the duration of the call. */
typedef void (*PUIAction)(void *context, const char *action, const char *widget);
/** @brief Opaque view owns widget state and cached textures, borrows its layout and renderer. */
typedef struct PUIView PUIView;
/** @brief Application drawing callback inside a clipped generic canvas. No document domain
 * belongs to pengUI. The callback must not replace/free the active view while drawing.
 */
typedef void (*PUICanvasRenderer)(void *context, SDL_Renderer *renderer, PUIRect area,
                                  const char *widget);
void pui_view_set_canvas_renderer(PUIView *view, PUICanvasRenderer draw, void *context);
/** @brief True if an enabled, visible widget accepts input at the point (including popup scope). */
bool pui_view_hit(const PUIView *view, const char *widget, float x, float y);

#define PUI_TREE_MAX_CELLS 4

/** @brief Optional trailing tree-table icon button; callbacks receive the owning row ID. */
typedef struct {
    char icon[128], action[64];
    bool checked, enabled;
} PUITreeCell;

/** @brief One runtime tree row in depth-first order; root rows have depth 0.
 * IDs must be unique within the snapshot. Text and action strings are copied by the view.
 * Runtime rows do not consume layout nodes. At most PUI_LAYOUT_NODES rows per tree.
 */
typedef struct {
    char id[64], text[256], icon[128], action[64];
    int depth;
    bool enabled, open;
    int cell_count; ///< Optional trailing button columns, zero for an ordinary tree row.
    PUITreeCell cells[PUI_TREE_MAX_CELLS];
    bool selected; ///< Optional application-controlled selection highlight in addition to the
                   ///< primary row.
} PUITreeItem;

/** @brief Visible tree row under the pointer, or NULL; borrowed ID until the next layout/row edit.
 */
const char *pui_view_tree_item_at(PUIView *view, const char *tree, float x, float y);
/** @brief Replace runtime rows of a tree, retaining open state/selection by stable ID.
 * Call between frames. count=0 clears the tree; invalid input leaves the old data intact.
 */
bool pui_view_set_tree_items(PUIView *view, const char *tree, const PUITreeItem *items, int count);
/** @brief Select a runtime tree row by ID and open its ancestors; NULL clears without actions. */
bool pui_view_select_tree_item(PUIView *view, const char *tree, const char *item);
/** @brief Close all open popups between frames, committing focus through normal widget rules. */
void pui_view_close_popups(PUIView *view, PUIContext *context);
/** @brief Borrow the selected runtime row ID; NULL when no row is selected. */
const char *pui_view_selected_tree_item(const PUIView *view, const char *tree);
/** @brief Runtime row count; -1 for a missing tree or a tree using YAML children. */
int pui_view_tree_item_count(const PUIView *view, const char *tree);

/** @brief Create one independent view; layout/renderer must outlive it. NULL on failure. */
PUIView *pui_view_new(const PUILayout *layout, SDL_Renderer *renderer);
/** @brief Destroy before the renderer/layout; NULL is accepted. */
void pui_view_free(PUIView *view);
/** @brief Replace slot content transactionally between frames, never inside draw callbacks.
 * Copies content (NULL unmounts). Keeps state outside the replaced subtree for explicit IDs;
 * focus, capture and popups are reset. The view owns the composed copy, not content.
 * The view pointer remains stable. Failure leaves the original UI and input unchanged.
 */
bool pui_view_replace_slot(PUIView *view, PUIContext *context, const char *slot,
                           const PUILayout *content, char *error, size_t capacity);
/** @brief Append and select a tab between frames. Preserves existing explicit widget state
 * and tab order; resets focus, capture and popups. Failure leaves the view unchanged.
 * Content is copied; its explicit IDs are prefixed with "tab_id.".
 */
bool pui_view_add_tab(PUIView *view, PUIContext *context, const char *tabs, const char *tab_id,
                      const char *title, const PUILayout *content, char *error, size_t capacity);
/** @brief Selected tab's YAML ID, borrowed until the next layout replacement; NULL if invalid. */
const char *pui_view_selected_tab(const PUIView *view, const char *tabs);
/** @brief Number of tabs; zero for an invalid tabs ID or NULL view. */
int pui_view_tab_count(const PUIView *view, const char *tabs);
/** @brief Borrow the tab ID at its displayed zero-based position; NULL if out of range. */
const char *pui_view_tab_at(const PUIView *view, const char *tabs, int index);

/** @brief Requested cursor from the last frame; default for NULL views.
 * Applied automatically after drawing, with default-cursor fallback if unsupported by SDL.
 * Splitter drags keep their resize cursor until release, including outside the grip.
 */
SDL_SystemCursor pui_view_cursor(const PUIView *view);
/** @brief Apply the seven theme colors from YAML to the existing renderer theme. */
void pui_view_apply_theme(const PUILayout *layout);
/** @brief Open a YAML radial menu; nested menus add concentric outer rings.
 * Ancestor rings remain interactive. Escape/center closes the outermost ring first.
 * popup_width scales the compact rings (320 is the default scale). */
bool pui_view_open_radial(PUIView *view, const char *id, float x, float y);
/** @brief Return a visible radial item's center in view coordinates, including disabled items.
 * Returns false for closed menus or items in branches that are not currently visible. */
bool pui_view_radial_item_center(const PUIView *view, const char *id, float *x, float *y);
/** @brief Enable/disable a radial menu button at runtime; YAML enabled=false remains binding. */
bool pui_view_set_radial_enabled(PUIView *view, const char *item, bool enabled);
/** @brief Dismiss a radial menu without dispatching any action. */
void pui_view_close_radial(PUIView *view);

/** @brief Feed SDL input on the main thread; true when text editing or a popup consumed the event.
 */
bool pui_view_event(PUIView *view, PUIContext *pui, SDL_Window *window, const SDL_Event *event);
/** @brief Draw a frame; callbacks may change app state but must not free the active view/layout. */
void pui_view_draw(PUIView *view, PUIContext *pui, PUIRect area, PUIAction action, void *context);
/** @brief Read input, label or tab text by YAML id; NULL for other widgets. Borrowed until
 * mutation.
 */
const char *pui_view_text(const PUIView *view, const char *id);
/** @brief Read the current packed color, including an active preview; false for invalid IDs. */
bool pui_view_color(const PUIView *view, const char *id, uint32_t *rgba);
/** @brief Replace a picker's committed color and reset its fields without firing actions. */
bool pui_view_set_color(PUIView *view, const char *id, uint32_t rgba);
/** @brief True only during a color-slider gesture. */
bool pui_view_color_editing(const PUIView *view, const char *id);
/** @brief Set input, label or tab text; tab widths follow the new caption. */
bool pui_view_set_text(PUIView *view, const char *id, const char *text);
/** @brief Read a toggle, checkbox or grouped button. False for unchecked/invalid IDs. */
bool pui_view_checked(const PUIView *view, const char *id);
/** @brief Set checked state without dispatching an action. Selecting a group member clears
 * its siblings. Cannot clear the active group member or select a disabled member.
 * Returns false for invalid IDs/types or a disallowed update.
 */
bool pui_view_set_checked(PUIView *view, const char *id, bool checked);
/** @brief Selected dropdown value, borrowed until layout/view disposal; NULL for invalid IDs. */
const char *pui_view_value(const PUIView *view, const char *id);
/** @brief Select an enabled option by value without firing an action; false for invalid values. */
bool pui_view_set_value(PUIView *view, const char *id, const char *value);
/** @brief Load a bounded PNG image as a renderer-owned texture; caller must destroy it. */
SDL_Texture *pui_image_load(SDL_Renderer *renderer, const char *path);
/** @brief Load a bounded PNG from borrowed memory; returned texture belongs to its renderer. */
SDL_Texture *pui_image_load_memory(SDL_Renderer *renderer, const void *data, size_t length);

/** @file @brief Optional borderless SDL window controls, independent of application actions. */

/** @brief Stable callback context. Must outlive the attached window or be detached first. */
typedef struct {
    SDL_Window *window;    ///< Borrowed window.
    const PUIView *view;   ///< Borrowed view; update after YAML reload.
    const PUIContext *pui; ///< Borrowed interaction context.
} PUIWindow;

/** @brief Attach hit testing and remove native borders; retain native borders if unsupported. */
bool pui_window_attach(PUIWindow *chrome, SDL_Window *window, const PUIView *view,
                       const PUIContext *pui);
/** @brief Remove callback before releasing its borrowed view/context. */
void pui_window_detach(PUIWindow *chrome);
/** @brief Compute resize/drag regions using the last drawn YAML view; coordinates are window-local.
 */
SDL_HitTestResult pui_window_hit_test(const PUIWindow *chrome, int x, int y);
/** @brief Handle window.minimize/maximize/close. Close queues a close request for the application.
 */
bool pui_window_action(SDL_Window *window, const char *action);

#endif /* PUI_WITH_SDL */

#endif /* PUI_H */

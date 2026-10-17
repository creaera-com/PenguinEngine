/**
 * @file
 * @brief Wiederverwendbare UI-Eingabe, Layoutzustände und Widgetinteraktionen ohne SDL.
 * @ingroup ui_api
 */

/** @addtogroup ui_api
 * @{
 */
#ifndef PENGUIN_UI_H
#define PENGUIN_UI_H
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
#endif

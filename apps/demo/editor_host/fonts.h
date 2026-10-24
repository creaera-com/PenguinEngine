/** @file
 * @brief Installed TrueType/OpenType faces and rasterization; owned by the app. */
#ifndef PENGUI_APP_FONTS_H
#define PENGUI_APP_FONTS_H
#include <SDL3/SDL.h>

/** @brief Stable filesystem face identity; bundled face has an empty path. */
typedef struct {
    char path[512], family[128], style[128], label[256];
    unsigned index;
    bool bold, italic;
} PUIFontEntry;
typedef struct PUIFonts PUIFonts;
/** @brief Discover bundled and installed faces in native font directories; NULL on bundled failure.
 */
PUIFonts *pui_fonts_create(const char *resource_root);
/** @brief Release discovery entries and all lazily loaded font buffers. */
void pui_fonts_destroy(PUIFonts *fonts);
/** @brief Number of discovered faces; zero for NULL. */
int pui_fonts_count(const PUIFonts *fonts);
/** @brief Borrow an entry until catalog destruction; NULL for invalid indices. */
const PUIFontEntry *pui_fonts_entry(const PUIFonts *fonts, int index);
/** @brief Find exact saved identity, or bundled fallback if the face was removed. */
int pui_fonts_find(const PUIFonts *fonts, const char *path, unsigned face);
/** @brief Closest real face in the same family for requested weight/slant. */
int pui_fonts_style(const PUIFonts *fonts, int index, bool bold, bool italic);
/** @brief Advance in document pixels, including kerning. */
float pui_fonts_advance(PUIFonts *fonts, int index, unsigned code, unsigned next, float size);
/** @brief Baseline offset in document pixels for the selected face and size. */
float pui_fonts_ascent(PUIFonts *fonts, int index, float size);
/** @brief Rasterize a glyph at the requested pixel height; free with pui_fonts_free_bitmap. */
unsigned char *pui_fonts_bitmap(PUIFonts *fonts, int index, unsigned code, float size, int *width,
                                int *height, int *x, int *y);
/** @brief Release a rasterizer-owned bitmap; NULL is accepted. */
void pui_fonts_free_bitmap(unsigned char *bitmap);
#endif

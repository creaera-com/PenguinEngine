/**
 * @file
 * @brief SDL-Szenenrenderer und besitzender Tilesetcache.
 * @ingroup engine_api
 */

/** @addtogroup engine_api
 * @{
 */
#ifndef PENGUIN_RENDER_H
#define PENGUIN_RENDER_H
#include "penguin/engine.h"
#include <SDL3/SDL.h>

/** @brief Besitzender SDL-Tilesetcache; vor Zerstörung des Renderers freigeben. */
typedef struct {
    SDL_Texture *tileset; ///< Besitzende SDL-Tileset-Textur oder NULL.
    int width;            ///< Breite der geladenen Textur in Pixeln.
    int height;           ///< Höhe der geladenen Textur in Pixeln.
    char path[PE_PATH];   ///< Nullterminierter Datei- oder Projektpfad.
    int source_tile;      ///< Tilegröße der geladenen Bildquelle in Pixeln.
} PeAssets;

/**
 * @brief Lädt das Szenen-Tileset als SDL-Textur oder verwendet den passenden Cache weiter.
 *
 * Der Renderer muss während der Texturlebensdauer bestehen. Fehlertext gehört dem Aufrufer; Assets
 * vorher nullinitialisieren.
 *
 * @param assets Nullinitialisierter oder bereits gültiger, besitzender Tilesetcache.
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param root Projektwurzelpfad bzw. Aufruferpuffer für den ermittelten Pfad.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_assets_load(PeAssets *assets, SDL_Renderer *renderer, const char *root,
                    const PeScene *scene, char *error, size_t size);

/**
 * @brief Zerstört die Tileset-Textur und setzt den Asset-Cache zurück.
 *
 * @param assets Nullinitialisierter oder bereits gültiger, besitzender Tilesetcache.
 */
void pe_assets_free(PeAssets *assets);

/**
 * @brief Zeichnet sichtbare Tile-Ebenen und Entities sowie optionale Bearbeitungshilfen.
 *
 * @param renderer Geliehener SDL-Renderer; muss für die verwendeten Texturen gültig bleiben.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param assets Nullinitialisierter oder bereits gültiger, besitzender Tilesetcache.
 * @param camera Kamera mit gültigem positivem Zoom.
 * @param viewport Sichtbarer Zeichenbereich bzw. seine Ausdehnung in Pixeln.
 * @param grid Ob das Tile-Raster gezeichnet wird.
 * @param collision Ob das Kollisionsraster gezeichnet wird.
 */
void pe_render_scene(SDL_Renderer *renderer, const PeScene *scene, const PeAssets *assets,
                     const PeCamera *camera, SDL_FRect viewport, bool grid, bool collision);
/** @} */
#endif

/** @file
 * @brief Independent 2D drawing values, geometry and optional SDL3 backend. */
#ifndef PENGDRAW_H
#define PENGDRAW_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Rectangle in caller-defined units. Positive Y points down. */
typedef struct {
    float x, y, w, h;
} PDrawRect;

/** @brief Closed, line-segment contours and pure filled-area operations in C. */
#define PDRAW_PATH_POINTS 128
#define PDRAW_PATH_CONTOURS 32

typedef struct {
    float x, y;
} PDrawPoint;

/** @brief Nonzero winding: outer contours are positive, holes negative; ends are exclusive. */
typedef struct {
    unsigned count, contours;
    PDrawPoint points[PDRAW_PATH_POINTS];
    unsigned ends[PDRAW_PATH_CONTOURS];
} PDrawPath;

typedef enum {
    PDRAW_AREA_UNION,
    PDRAW_AREA_DIFFERENCE,
    PDRAW_AREA_INTERSECTION,
    PDRAW_AREA_XOR
} PDrawAreaOperation;

/** @brief Validate finite coordinates and closed contours with at least three vertices. */
bool pdraw_path_valid(const PDrawPath *path);
/** @brief Signed area of one contour; positive means filled outer boundary. */
float pdraw_path_contour_area(const PDrawPath *path, unsigned contour);
/** @brief Nonzero winding hit-test with half-open edges; holes remain empty. */
bool pdraw_path_contains(const PDrawPath *path, float x, float y);
/** @brief Intersect filled path with a box, excluding empty holes. */
bool pdraw_path_intersects(const PDrawPath *path, PDrawRect box);
/** @brief Bounds of a nonempty path. */
PDrawRect pdraw_path_bounds(const PDrawPath *path);
/** @brief Canonicalize two filled areas. Failure/complexity overflow leaves output untouched. */
bool pdraw_path_combine(const PDrawPath *a, const PDrawPath *b, PDrawAreaOperation operation,
                        PDrawPath *out);
/** @brief Extract connected components, retaining associated holes. Returns -1 on overflow. */
int pdraw_path_components(const PDrawPath *path, PDrawPath *out, int capacity);
/** @brief Build a solid contour band; placement -1=inside, 0=center, 1=outside. */
bool pdraw_path_outline(const PDrawPath *path, float width, int placement, PDrawPath *out);
/** @brief Emit non-overlapping triangles through a callback; no SDL dependency. */
bool pdraw_path_triangles(const PDrawPath *path, void (*triangle)(void *, const PDrawPoint[3]),
                          void *context);

/** @brief Editable cubic Bezier anchors and pure, bounded curve geometry. */
#define PDRAW_BEZIER_NODES 32

/** @brief Absolute anchor and incoming/outgoing control points in the same coordinate space. */
typedef struct {
    PDrawPoint point, incoming, outgoing;
} PDrawBezierNode;

/** @brief One open or closed contour. Shapes store normalized values; tools use parent pixels. */
typedef struct {
    unsigned count;
    unsigned closed; ///< Zero=open, one=closed; validated before reading checkpoint values.
    PDrawBezierNode nodes[PDRAW_BEZIER_NODES];
} PDrawBezier;

/** @brief Validate finite bounded coordinates and the fixed node capacity; empty is valid. */
bool pdraw_bezier_valid(const PDrawBezier *curve);
/** @brief Evaluate the cubic segment from node a to b at t in [0,1]. */
PDrawPoint pdraw_bezier_evaluate(PDrawBezierNode a, PDrawBezierNode b, float t);
/** @brief Exact cubic-extrema bounds; control handles themselves do not enlarge the shape. */
PDrawRect pdraw_bezier_bounds(const PDrawBezier *curve);
/** @brief Adaptive de Casteljau approximation, relaxing tolerance to respect PDRAW_PATH_POINTS.
 * Open results may have two points and are polylines, not filled PDrawPaths. */
bool pdraw_bezier_flatten(const PDrawBezier *curve, float tolerance, PDrawPath *out);
/** @brief Split one cubic at t without changing its shape; inserts one editable anchor.
 * Returns false for invalid segments, endpoints or full capacity; input stays unchanged. */
bool pdraw_bezier_insert(PDrawBezier *curve, unsigned segment, float t);

/** @brief Pure clockwise rectangle rotation and hit-test geometry. */
/** @brief Normalize finite degrees to [-180,180). */
float pdraw_angle_normalize(float degrees);
/** @brief Preserve rotation direction in [-360,360], including signed full turns. */
float pdraw_angle_turn(float degrees);
/** @brief Rotate a point about a pivot in screen/document coordinates. */
void pdraw_rotate_point(float *x, float *y, float cx, float cy, float degrees);
/** @brief Four clockwise corners, starting at the unrotated upper-left corner. */
void pdraw_rotated_corners(PDrawRect rect, float degrees, PDrawRect corners[4]);
/** @brief Axis-aligned bounds enclosing the rotated rectangle. */
PDrawRect pdraw_rotated_bounds(PDrawRect rect, float degrees);
/** @brief Exact oriented rectangle hit-test. */
bool pdraw_rotated_contains(PDrawRect rect, float degrees, float x, float y);
/** @brief Separating-axis test against an axis-aligned box. */
bool pdraw_rotated_intersects(PDrawRect rect, float degrees, PDrawRect box);

/** @brief Object-relative fill gradients, without rendering or UI resources. */
#define PDRAW_GRADIENT_STOPS 8u

/** @brief Solid uses the caller's fill color; reflected mirrors a linear ramp about its start. */
typedef enum {
    PDRAW_GRADIENT_SOLID,
    PDRAW_GRADIENT_LINEAR,
    PDRAW_GRADIENT_RADIAL,
    PDRAW_GRADIENT_CONICAL,
    PDRAW_GRADIENT_REFLECTED
} PDrawGradientKind;

/** @brief Position in [0,1] and straight 0xRRGGBBAA color. */
typedef struct {
    float position;
    uint32_t color;
} PDrawGradientStop;

/** @brief Endpoints in normalized, unrotated object coordinates. Outside endpoints are allowed. */
typedef struct {
    unsigned kind, count;
    float x1, y1, x2, y2;
    PDrawGradientStop stops[PDRAW_GRADIENT_STOPS];
} PDrawGradient;

/** @brief Validate endpoints, sorted stops and enum bounds, including retained disabled values. */
bool pdraw_gradient_valid(const PDrawGradient *gradient);
/** @brief Two-stop ramp, using the existing fill (orange when fully transparent) and white. */
PDrawGradient pdraw_gradient_default(uint32_t fill);
/** @brief Evaluate in unrotated object-local pixels with alpha-correct color interpolation. */
uint32_t pdraw_gradient_sample(const PDrawGradient *gradient, float x, float y, float width,
                               float height);
/** @brief Insert a sampled stop in the widest interval; return its index, or -1 at capacity. */
int pdraw_gradient_add_stop(PDrawGradient *gradient);
/** @brief Remove a stop while preserving at least two and endpoints at zero/one. */
bool pdraw_gradient_remove_stop(PDrawGradient *gradient, unsigned index);

/** @brief Stroke placement relative to the shape boundary. */
typedef enum {
    PDRAW_STROKE_CENTER,
    PDRAW_STROKE_INSIDE,
    PDRAW_STROKE_OUTSIDE
} PDrawStrokeAlignment;

/** @brief Straight RGBA color and width in drawing units. */
typedef struct {
    uint32_t color;
    float width;
    PDrawStrokeAlignment alignment;
} PDrawStroke;

/** @brief Borrowed-value drawing geometry, with no document identity or editor state.
 * Empty path/curve means rectangle; coordinates of paths/curves are normalized to rect.
 * ellipse selects analytic full-ellipse rendering; segments use the supplied curve.
 */
typedef struct {
    PDrawPath path;
    PDrawBezier curve;
    float corner_radius;
    PDrawStroke stroke;
    PDrawGradient gradient;
    bool ellipse;
} PDrawShape;

/** @brief Map local normalized geometry through rectangle size and clockwise rotation. */
PDrawPath pdraw_shape_path(const PDrawShape *shape, PDrawRect rect, float angle);
/** @brief Build a contour band, coarsening derived geometry if needed to respect capacity. */
bool pdraw_shape_outline(const PDrawShape *shape, PDrawRect rect, float angle, float width,
                         int placement, PDrawPath *out);

/** @brief Color alpha is opacity; blur is softness radius, spread expands the alpha silhouette. */
typedef struct {
    unsigned enabled;
    uint32_t color;
    float offset_x, offset_y, blur, spread;
} PDrawShadow;

/** @brief Validate finite, bounded settings, including disabled effects. */
bool pdraw_shadow_valid(const PDrawShadow *shadow);
/** @brief Subtle black shadow, offset 8 px, softness 12 px, opacity 35 percent. */
PDrawShadow pdraw_shadow_default(void);

/** @brief Pointer-free UTF-8 text and paragraph style values. */
#define PDRAW_TEXT_CAPACITY 2048

/** @brief Paragraph alignment. Justification excludes the final line of a paragraph. */
typedef enum {
    PDRAW_TEXT_LEFT,
    PDRAW_TEXT_CENTER,
    PDRAW_TEXT_RIGHT,
    PDRAW_TEXT_JUSTIFY
} PDrawTextAlignment;

/** @brief One uniformly styled, multi-column text frame; empty font key uses the caller's default
 * face. */
typedef struct {
    unsigned enabled, face, bold, italic, alignment, columns;
    char content[PDRAW_TEXT_CAPACITY], font[512];
    float size, line_height, letter_spacing, column_gap;
} PDrawText;

/** @brief Validate style bounds, string termination and UTF-8 including newlines. */
bool pdraw_text_valid(const PDrawText *text);

/** @brief Renderer-independent UTF-8 line breaking and column flow. */

/** @brief One codepoint's position and byte identity; coordinates are frame-local pixels. */
typedef struct {
    unsigned code;
    size_t byte, end;
    float x, y, advance;
} PDrawTextGlyph;

/** @brief Bounded text flow result; overflow marks content that cannot fit the frame. */
typedef struct {
    PDrawTextGlyph glyphs[PDRAW_TEXT_CAPACITY];
    int count;
    bool overflow;
    size_t visible_end;
    float end_x, end_y;
} PDrawTextLayout;

typedef float (*PDrawTextAdvance)(void *context, unsigned code, unsigned next, float size);
/** @brief Wrap words, split long words, align lines and flow into up to sixteen columns. */
void pdraw_text_layout(const PDrawText *text, float width, float height, PDrawTextAdvance advance,
                       void *context, PDrawTextLayout *out);
/** @brief Decode one validated UTF-8 codepoint and advance a byte offset. */
unsigned pdraw_text_codepoint(const char *text, size_t *offset);

#if defined(PDRAW_WITH_SDL) && PDRAW_WITH_SDL
#include <SDL3/SDL.h>
/** @brief Low-level shape functions respect caller clipping; outline is optional RGBA (0 disables).
 * Origin and zoom map drawing units to renderer coordinates. Textures remain caller-owned.
 */
void pdraw_path_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect, float angle,
                     SDL_FRect origin, float zoom, uint32_t fill, uint32_t outline);
void pdraw_curve_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect, float angle,
                      SDL_FRect origin, float zoom, uint32_t fill, uint32_t outline);
void pdraw_ellipse_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect,
                        float angle, SDL_FRect origin, float zoom, uint32_t fill, uint32_t outline);
void pdraw_stroke_draw(SDL_Renderer *renderer, PDrawRect rect, float angle, float zoom,
                       PDrawStroke stroke);
/** @brief Paint a closed gradient fill; false means unavailable and permits a solid fallback. */
bool pdraw_gradient_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect,
                         float angle, SDL_FRect origin, float zoom);
/** @brief Draw rectangle, path, curve or full ellipse with solid/gradient fill and stroke. */
void pdraw_shape_draw(SDL_Renderer *renderer, const PDrawShape *shape, PDrawRect rect, float angle,
                      SDL_FRect origin, float zoom, uint32_t fill);
/** @brief Render a borrowed image or pre-rasterized text texture; restore texture modulation. */
bool pdraw_texture_draw(SDL_Renderer *, SDL_Texture *, const SDL_FRect *, const SDL_FRect *,
                        double angle, SDL_FlipMode, uint32_t tint);

/** @brief Create a renderer preferring SDL GPU (Vulkan/Metal/D3D12), with effect shaders.
 * Honors SDL_RENDER_DRIVER; explicit gpu is strict, other values select the requested backend.
 * Without an explicit driver, unavailable GPU support falls back with a log warning.
 * NULL window creates an offscreen GPU renderer; bind a target before drawing.
 * Pair with pdraw_destroy_renderer(), after freeing caller-owned textures/caches.
 */
SDL_Renderer *pdraw_create_renderer(SDL_Window *window);
/** @brief Whether this renderer has the GPU effect pipeline (not just accelerated drawing). */
bool pdraw_renderer_gpu_effects(SDL_Renderer *renderer);

/** @brief Renderer-local effect diagnostics; CPU fallback renderers return zero counters. */
typedef struct {
    uint64_t shadow_draws, shadow_failures;
    uint64_t gradient_draws, gradient_failures;
    size_t texture_bytes;
} PDrawGPUStats;

/** @brief Query GPU effect counters without GPU synchronization or pixel downloads. */
PDrawGPUStats pdraw_renderer_gpu_stats(SDL_Renderer *renderer);
/** @brief Release renderer-owned effect resources, then destroy SDL renderer; NULL is safe. */
void pdraw_destroy_renderer(SDL_Renderer *renderer);

/** @brief Scratch texture ownership for bounded 2x supersampling; zero-initialize, destroy before
 * renderer. */
typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *target;
    int width, height;
    void *tiles; ///< Owned bounded replay cache; private to the SDL backend.
} PDrawRenderer;

/** @brief Replayable painting callback: origin and scale describe each supersampling tile.
 * Must not mutate the caller model. Native fallback receives origin (0,0), scale 1.
 */
typedef void (*PDrawPaint)(void *context, SDL_Renderer *, float origin_x, float origin_y,
                           float scale);
/** @brief Draw clipped bounds with bounded 2x supersampling; restore all renderer state.
 * NULL cache uses temporary resources; fallback supports renderers without target textures.
 */
void pdraw_render(PDrawRenderer *, SDL_Renderer *, SDL_FRect bounds, PDrawPaint, void *context);
/** @brief Reuse rendered tiles for a caller-supplied content revision (up to 64 MiB, 16 tiles).
 * Revision must change whenever callback output changes, including selection/animation.
 * Bounds, clipping, render target and renderer are also part of the cache identity.
 * A new revision repaints the existing tile texture; obsolete revisions are not retained.
 * NULL cache draws immediately. Destroy before the SDL renderer; no document data is owned.
 */
void pdraw_render_cached(PDrawRenderer *, SDL_Renderer *, SDL_FRect bounds, uint64_t revision,
                         PDrawPaint, void *context);
/** @brief Free scratch textures, safe repeatedly. */
void pdraw_renderer_destroy(PDrawRenderer *);

/** @brief Frame-local clip compositor; renderer is borrowed and must outlive its textures. */
typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *texture, *clipped;
    int width, height;
} PDrawMaskRenderer;

/** @brief Paint content only inside the mask, preserving holes and opacity. Initialize renderer.
 * Caller controls visibility; failed target operations hide content rather than expose unclipped
 * pixels.
 */
void pdraw_mask_render(PDrawMaskRenderer *, const PDrawShape *, PDrawRect, float, SDL_FRect, float,
                       void (*paint)(void *), void *context);
/** @brief Release frame-local masking textures. */
void pdraw_mask_renderer_destroy(PDrawMaskRenderer *);
/** @brief Paint content alpha translated by dx/dy in renderer pixels. */
typedef void (*PDrawShadowPaint)(void *, float dx, float dy);
/** @brief Composite a bounded softened alpha shadow; content is painted separately by caller. */
bool pdraw_shadow_draw(SDL_Renderer *, const PDrawShadow *, float zoom, PDrawRect bounds,
                       PDrawShadowPaint, void *);
#endif
#endif

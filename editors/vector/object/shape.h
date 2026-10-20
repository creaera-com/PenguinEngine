/** @file @brief Common vector shape values: geometry, style, transform and hierarchy identity. */
#ifndef PE_VECTOR_OBJECT_SHAPE_H
#define PE_VECTOR_OBJECT_SHAPE_H
#include "editors/vector/artboard/artboard.h"
#include "editors/vector/effects/shadow.h"
#include "editors/vector/geometry/bezier.h"
#include "editors/vector/geometry/path.h"
#include "editors/vector/gradient/state.h"
#include "editors/vector/image/state.h"
#include "editors/vector/repeat/state.h"
#include "editors/vector/text/state.h"

/** @brief Placement relative to the geometric shape contour. */
typedef enum {
    VECTOR_STROKE_CENTER,
    VECTOR_STROKE_INSIDE,
    VECTOR_STROKE_OUTSIDE
} VectorStrokeAlignment;

/** @brief Solid stroke in document pixels; width zero disables it. */
typedef struct {
    uint32_t color; ///< Packed 0xRRGGBBAA.
    float width;
    VectorStrokeAlignment alignment;
} VectorStroke;

/** @brief Curve identity; editing parametric shape nodes converts it to a free Bezier curve. */
typedef enum { VECTOR_CURVE_FREE, VECTOR_CURVE_ELLIPSE, VECTOR_CURVE_POLYGON } VectorCurveKind;

/** @brief Shape geometry is relative to parent_id; rect maps normalized path points and defines the
 * pivot. */
typedef struct {
    unsigned id, parent_id, flags;
    unsigned group_id;  ///< Zero for independent objects; group members share an artboard and ID.
    unsigned clip_mask; ///< One means this closed shape clips its group; never painted itself.
    uint32_t fill;      ///< Packed 0xRRGGBBAA; alpha is actual rendering opacity.
    VectorStroke stroke;
    VectorGradient gradient; ///< Closed-area fill, evaluated in object-local coordinates.
    VectorShadow shadow;     ///< Individual non-destructive drop shadow, below object content.
    VectorRepeat repeat; ///< Procedural grid on this source; other cells are not document objects.
    VectorImage image;   ///< Project-owned PNG/JPEG reference; empty source for vector shapes.
    VectorText text;     ///< Editable text frame; disabled for geometric shapes.
    unsigned curve_kind; ///< Validated VectorCurveKind; zero for non-curve shapes.
    VectorBezier curve; ///< Editable cubic controls; count zero selects rectangle/polygon geometry.
    VectorPath path;    ///< Empty for parametric rectangles; otherwise normalized closed contours.
    char name[64];
    PUIRect rect;
    float angle; ///< Clockwise degrees around the rectangle center, signed in [-360,360], including
                 ///< full turns.
} VectorShape;

/** @brief Map normalized geometry to parent-local space, including object rotation. */
VectorPath vector_shape_path(const VectorShape *shape, PUIRect rect, float angle);
/** @brief Build a closed contour's stroke, coarsening derived Bezier geometry to fit the path
 * budget. Editable control points remain unchanged. Returns false on geometry/complexity failure.
 */
bool vector_shape_outline(const VectorShape *shape, PUIRect rect, float angle, float width,
                          int placement, VectorPath *out);
/** @brief Filled-area hit test; holes allow selecting objects behind this shape. */
bool vector_shape_contains(const VectorShape *shape, float x, float y);
/** @brief Filled-area intersection with a parent-local box. */
bool vector_shape_intersects(const VectorShape *shape, PUIRect box);
#endif

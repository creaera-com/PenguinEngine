/** @file
 * @brief Persistent ellipse segment parameters, independent of UI and rendering. */
#ifndef PE_VECTOR_ELLIPSE_STATE_H
#define PE_VECTOR_ELLIPSE_STATE_H
#include <stdbool.h>

/** @brief How the angular contour is closed; zero preserves legacy full ellipses. */
typedef enum {
    VECTOR_ELLIPSE_FULL,
    VECTOR_ELLIPSE_ARC,
    VECTOR_ELLIPSE_PIE,
    VECTOR_ELLIPSE_CHORD
} VectorEllipseMode;

/** @brief Clockwise degrees from the right-hand point; end minus start is the signed sweep. */
typedef struct {
    unsigned mode;
    float start, end;
} VectorEllipse;

/** @brief Validate finite angles in [-360,360] and a nonzero sweep of at most one turn. */
bool vector_ellipse_parameters_valid(VectorEllipse value);
#endif

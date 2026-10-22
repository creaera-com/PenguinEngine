/** @file
 * @brief Common object flags for artboards and future layers, groups and shapes. */
#ifndef PE_VECTOR_OBJECT_STATE_H
#define PE_VECTOR_OBJECT_STATE_H

/** @brief Zero is visible and unlocked. Flags do not contain UI or selection state. */
typedef enum { VECTOR_OBJECT_HIDDEN = 1u, VECTOR_OBJECT_LOCKED = 2u } VectorObjectFlag;

#define VECTOR_OBJECT_STATE_MASK (VECTOR_OBJECT_HIDDEN | VECTOR_OBJECT_LOCKED)
#endif

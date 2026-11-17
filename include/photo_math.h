#ifndef PHOTO_MATH_H
#define PHOTO_MATH_H
#include <stddef.h>
#include <stdint.h>

#define PHOTO_MATH_REVISION "d8781df201e66561d63dcf04bfb1af6d0684fc95"
/* Explicit GLES provider, dedicated caller-owned current context, no CPU fallback. */
typedef struct photo_math photo_math;
typedef enum { PHOTO_OK, PHOTO_ARGUMENT, PHOTO_SPACE, PHOTO_CAPABILITY,
               PHOTO_CONTEXT, PHOTO_BACKEND } photo_status;
typedef enum { PHOTO_VECTOR, PHOTO_COVECTOR } photo_variance;
typedef struct {
    char representation[64];
    char space[64];
    char basis[64];
    char metric[64];
    uint32_t dimension;
    photo_variance variance;
    float values[32];
} photo_coordinates;
typedef struct { float axis_ratio; float residual; } photo_alignment_report;

/* Read generated programs from a provenance-verified shared operation bundle. */
photo_math *photo_math_open(const char *shared_root, char *error, size_t capacity);
photo_status photo_math_close(photo_math **engine);
photo_status photo_project32(photo_math *engine, const photo_coordinates *vector,
                            const photo_coordinates *covector, float *score);
photo_status photo_inspect_alignment8(photo_math *engine, const photo_coordinates *left,
                                     const photo_coordinates *right, photo_alignment_report *report);
/* Active SO(8) map H_second o H_first, in the SAME named Euclidean basis.
 * Both normals are nonzero covectors; argument is a vector or covector.
 * inverse is exactly 0 or 1. Covectors transform by R because R is orthogonal.
 * Offset-free linear reflecting planes only. Output is untouched on every error;
 * it may alias an input on success. The numerical operation lives upstream.
 */
photo_status photo_rotate8(photo_math *engine, const photo_coordinates *first_normal,
                          const photo_coordinates *second_normal, const photo_coordinates *argument,
                          int inverse, photo_coordinates *output);
const char *photo_status_text(photo_status status);
#endif

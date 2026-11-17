#ifndef PHOTO_MATH_H
#define PHOTO_MATH_H
#include <stddef.h>
#include <stdint.h>

/* This slice has a specific, explicitly selected GLES provider. The API names
 * operations, not a Householder implementation or a compulsory photo encoder.
 * A caller supplies a dedicated current GLES3 context and retains its ownership.
 * Calls and destruction must use that context. No implicit CPU fallback exists.
 */
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
    float values[32]; /* Explicit Float32 carrier, not an unlabelled embedding. */
} photo_coordinates;

typedef struct {
    float axis_ratio;
    float residual;
} photo_alignment_report;

/* Files are read from a caller-selected, revision-verified shared checkout.
 * Missing/invalid programs or unavailable float output are errors, not labels.
 */
photo_math *photo_math_open(const char *shared_root, char *error, size_t capacity);
photo_status photo_math_close(photo_math **engine);
photo_status photo_project32(photo_math *engine,
                            const photo_coordinates *vector,
                            const photo_coordinates *covector, float *score);
/* Existing shared shader returns diagnostics, NOT the full rotation or matrix. */
photo_status photo_inspect_alignment8(photo_math *engine,
                                    const photo_coordinates *left,
                                    const photo_coordinates *right,
                                    photo_alignment_report *report);
const char *photo_status_text(photo_status status);
#endif

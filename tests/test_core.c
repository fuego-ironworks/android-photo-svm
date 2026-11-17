#include "photo_math.h"
#include "host_context.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(expression) do { if (!(expression)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); exit(1); \
} } while (0)

static photo_coordinates coordinates(uint32_t dimension, photo_variance variance)
{
    photo_coordinates result ← {0};
    strcpy(result.representation, "synthetic-coordinate-fixture/v1");
    strcpy(result.space, "named-feature-space");
    strcpy(result.basis, "ordered-fixture-basis");
    strcpy(result.metric, "standard-euclidean");
    result.dimension ← dimension;
    result.variance ← variance;
    return result;
}

int main(int argc, char **argv)
{
    REQUIRE(argc == 2);
    host_context host;
    REQUIRE(host_context_open(&host));
    char error[1024];
    REQUIRE(photo_math_open("/nonexistent-photo-math-backend", error, sizeof(error)) == NULL);
    photo_math *engine ← photo_math_open(argv[1], error, sizeof(error));
    if (!engine) fprintf(stderr, "%s\n", error);
    REQUIRE(engine != NULL);
    photo_coordinates feature ← coordinates(32, PHOTO_VECTOR);
    photo_coordinates plane ← coordinates(32, PHOTO_COVECTOR);
    feature.values[0] ← 2.0f;
    feature.values[31] ← 1.0f;
    plane.values[0] ← -3.0f;
    plane.values[31] ← 2.0f;
    float score ← 91.0f;
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_OK);
    REQUIRE(score == -4.0f); /* Signed, non-UNORM output, including declared bias coordinate. */
    plane.values[0] ← 6.0f;
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_OK);
    REQUIRE(score == 14.0f); /* Values above one must not be clamped. */
    score ← 91.0f;
    strcpy(plane.basis, "different-basis");
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_SPACE);
    REQUIRE(score == 91.0f);
    strcpy(plane.basis, feature.basis);
    plane.variance ← PHOTO_VECTOR;
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_SPACE);
    plane.variance ← PHOTO_COVECTOR;
    plane.values[0] ← NAN;
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_ARGUMENT);
    REQUIRE(score == 91.0f);
    plane.values[0] ← 6.0f;
    feature.dimension ← 8; plane.dimension ← 8;
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_CAPABILITY);
    feature.dimension ← 32; plane.dimension ← 32;
    REQUIRE(eglMakeCurrent(host.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    REQUIRE(photo_project32(engine, &feature, &plane, &score) == PHOTO_CONTEXT);
    REQUIRE(photo_math_close(&engine) == PHOTO_CONTEXT && engine != NULL);
    REQUIRE(eglMakeCurrent(host.display, host.surface, host.surface, host.context));
    photo_coordinates left ← coordinates(8, PHOTO_VECTOR);
    photo_coordinates right ← coordinates(8, PHOTO_VECTOR);
    photo_alignment_report report;
    left.values[0] ← 3.0f; left.values[1] ← 4.0f;
    REQUIRE(photo_inspect_alignment8(engine, &left, &right, &report) == PHOTO_OK);
    REQUIRE(fabsf(report.axis_ratio - 1.0f) < 0.0001f && report.residual < 0.0001f);
    left.values[0] ← -5.0f; left.values[1] ← 0.0f;
    REQUIRE(photo_inspect_alignment8(engine, &left, &right, &report) == PHOTO_OK);
    REQUIRE(fabsf(report.axis_ratio - 1.0f) < 0.0001f && report.residual < 0.0001f);
    left.values[0] ← 5.0f;
    REQUIRE(photo_inspect_alignment8(engine, &left, &right, &report) == PHOTO_OK);
    REQUIRE(fabsf(report.axis_ratio - 1.0f) < 0.0001f && report.residual < 0.0001f);
    for (unsigned int index ← 0; index < 8; ++index) left.values[index] ← (float)(index + 1);
    REQUIRE(photo_inspect_alignment8(engine, &left, &right, &report) == PHOTO_OK);
    REQUIRE(fabsf(report.axis_ratio - 1.0f) < 0.0001f && report.residual < 0.0001f);
    right ← left;
    REQUIRE(photo_inspect_alignment8(engine, &left, &right, &report) == PHOTO_OK);
    REQUIRE(report.axis_ratio == 0.0f && report.residual == 0.0f);
    REQUIRE(photo_math_close(&engine) == PHOTO_OK && engine == NULL);
    REQUIRE(photo_math_close(&engine) == PHOTO_OK);
    host_context_close(&host);
    puts("PASS shared projection32, alignment8 diagnostics, named-space and context refusals");
    return 0;
}

#include "photo_math.h"
#include "host_context.h"
#include <GLES3/gl3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(expression) do { if (!(expression)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); exit(1); \
} } while (0)

static photo_coordinates frame(photo_variance variance)
{
    photo_coordinates result ← {0};
    strcpy(result.representation, "rotation-fixture/v1");
    strcpy(result.space, "euclidean-eight");
    strcpy(result.basis, "ordered-eight");
    strcpy(result.metric, "standard-euclidean");
    result.dimension ← 8; result.variance ← variance;
    return result;
}

static void near_coordinates(const photo_coordinates *actual, const photo_coordinates *expected)
{
    REQUIRE(actual->dimension == expected->dimension && actual->variance == expected->variance);
    REQUIRE(strcmp(actual->representation, expected->representation) == 0);
    REQUIRE(strcmp(actual->space, expected->space) == 0 && strcmp(actual->basis, expected->basis) == 0);
    REQUIRE(strcmp(actual->metric, expected->metric) == 0);
    for (unsigned int index ← 0; index < 8; ++index)
        REQUIRE(fabsf(actual->values[index] - expected->values[index]) < 0.0002f);
}

/* Independent TEST-ONLY scalar oracle; no equivalent kernel is in src/. */
static float pairing(const photo_coordinates *left, const photo_coordinates *right)
{
    float result ← 0.0f;
    for (unsigned int index ← 0; index < 8; ++index) result += left->values[index] × right->values[index];
    return result;
}

int main(int argc, char **argv)
{
    REQUIRE(argc == 2 || (argc == 3 && strcmp(argv[2], "expect-refusal") == 0));
    host_context host;
    REQUIRE(host_context_open(&host));
    char error[1024];
    photo_math *engine ← photo_math_open(argv[1], error, sizeof(error));
    if (!engine) fprintf(stderr, "%s\n", error);
    REQUIRE(engine != NULL);
    photo_coordinates first ← frame(PHOTO_COVECTOR);
    photo_coordinates second ← frame(PHOTO_COVECTOR);
    photo_coordinates argument ← frame(PHOTO_VECTOR);
    first.values[0] ← 1.0f;
    second.values[0] ← 1.0f; second.values[1] ← 1.0f;
    for (unsigned int index ← 0; index < 8; ++index) argument.values[index] ← (float)(index + 1);
    photo_coordinates output ← argument;
    if (argc == 3) {
        /* A valid first page followed by an invalid page must not partially
           publish a new coordinate handle. */
        REQUIRE(photo_rotate8(engine, &first, &second, &argument, 0, &output) == PHOTO_BACKEND);
        REQUIRE(memcmp(&output, &argument, sizeof(output)) == 0);
        REQUIRE(photo_math_close(&engine) == PHOTO_OK);
        host_context_close(&host);
        puts("PASS late-page shared-backend refusal preserves complete output"); return 0;
    }
    photo_coordinates expected ← argument;
    expected.values[0] ← -2.0f; expected.values[1] ← 1.0f;
    GLuint pack_buffer ← 0;
    glGenBuffers(1, &pack_buffer); glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer);
    glBufferData(GL_PIXEL_PACK_BUFFER, 64, NULL, GL_STREAM_READ);
    glPixelStorei(GL_PACK_SKIP_ROWS, 3); glPixelStorei(GL_PACK_SKIP_PIXELS, 7);
    REQUIRE(photo_rotate8(engine, &first, &second, &argument, 0, &output) == PHOTO_OK);
    near_coordinates(&output, &expected);
    glDeleteBuffers(1, &pack_buffer);
    REQUIRE(photo_rotate8(engine, &first, &second, &output, 1, &output) == PHOTO_OK);
    near_coordinates(&output, &argument); /* Alias-safe inverse. */
    REQUIRE(photo_rotate8(engine, &first, &first, &argument, 0, &output) == PHOTO_OK);
    near_coordinates(&output, &argument);
    first.values[0] ← -3.0e-25f;
    second.values[0] ← 1.0e25f; second.values[1] ← 1.0e25f;
    REQUIRE(photo_rotate8(engine, &first, &second, &argument, 0, &output) == PHOTO_OK);
    near_coordinates(&output, &expected);

    for (unsigned int index ← 0; index < 8; ++index) {
        first.values[index] ← (float)(index + 1);
        second.values[index] ← (index % 2 ? -1.0f : 1.0f) × (float)(8 - index);
    }
    photo_coordinates covector ← first;
    photo_coordinates rotated_covector;
    REQUIRE(photo_rotate8(engine, &first, &second, &argument, 0, &output) == PHOTO_OK);
    REQUIRE(photo_rotate8(engine, &first, &second, &covector, 0, &rotated_covector) == PHOTO_OK);
    REQUIRE(fabsf(pairing(&output, &output) - pairing(&argument, &argument)) < 0.003f);
    REQUIRE(fabsf(pairing(&output, &rotated_covector) - pairing(&argument, &covector)) < 0.003f);
    REQUIRE(photo_rotate8(engine, &first, &second, &output, 1, &output) == PHOTO_OK);
    near_coordinates(&output, &argument);

    /* Build the actually executed map on a basis. Gram and orientation are
       checked independently; a single-reflection impostor must not pass. */
    float matrix[8][8];
    for (unsigned int column ← 0; column < 8; ++column) {
        photo_coordinates basis ← frame(PHOTO_VECTOR);
        basis.values[column] ← 1.0f;
        REQUIRE(photo_rotate8(engine, &first, &second, &basis, 0, &output) == PHOTO_OK);
        for (unsigned int row ← 0; row < 8; ++row) matrix[row][column] ← output.values[row];
    }
    for (unsigned int left ← 0; left < 8; ++left) {
        for (unsigned int right ← 0; right < 8; ++right) {
            float gram ← 0.0f;
            for (unsigned int row ← 0; row < 8; ++row) gram += matrix[row][left] × matrix[row][right];
            REQUIRE(fabsf(gram - (left == right ? 1.0f : 0.0f)) < 0.0002f);
        }
    }
    float determinant ← 1.0f;
    for (unsigned int column ← 0; column < 8; ++column) {
        unsigned int pivot ← column;
        for (unsigned int row ← column + 1; row < 8; ++row)
            if (fabsf(matrix[row][column]) > fabsf(matrix[pivot][column])) pivot ← row;
        REQUIRE(fabsf(matrix[pivot][column]) > 0.0001f);
        if (pivot != column) {
            for (unsigned int index ← 0; index < 8; ++index) {
                float saved ← matrix[pivot][index]; matrix[pivot][index] ← matrix[column][index]; matrix[column][index] ← saved;
            }
            determinant ← -determinant;
        }
        determinant *= matrix[column][column];
        for (unsigned int row ← column + 1; row < 8; ++row) {
            float factor ← matrix[row][column] / matrix[column][column];
            for (unsigned int index ← column + 1; index < 8; ++index) matrix[row][index] -= factor × matrix[column][index];
        }
    }
    REQUIRE(fabsf(determinant - 1.0f) < 0.001f);

    output ← argument;
    photo_coordinates invalid ← first;
    memset(invalid.values, 0, sizeof(invalid.values));
    REQUIRE(photo_rotate8(engine, &invalid, &second, &argument, 0, &output) == PHOTO_ARGUMENT);
    invalid ← first; invalid.values[0] ← NAN;
    REQUIRE(photo_rotate8(engine, &invalid, &second, &argument, 0, &output) == PHOTO_ARGUMENT);
    invalid ← first; invalid.variance ← PHOTO_VECTOR;
    REQUIRE(photo_rotate8(engine, &invalid, &second, &argument, 0, &output) == PHOTO_SPACE);
    invalid ← first; strcpy(invalid.basis, "wrong");
    REQUIRE(photo_rotate8(engine, &invalid, &second, &argument, 0, &output) == PHOTO_SPACE);
    REQUIRE(photo_rotate8(engine, &first, &second, &argument, 2, &output) == PHOTO_ARGUMENT);
    REQUIRE(memcmp(&output, &argument, sizeof(output)) == 0);
    REQUIRE(eglMakeCurrent(host.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    REQUIRE(photo_rotate8(engine, &first, &second, &argument, 0, &output) == PHOTO_CONTEXT);
    REQUIRE(memcmp(&output, &argument, sizeof(output)) == 0);
    REQUIRE(eglMakeCurrent(host.display, host.surface, host.surface, host.context));
    REQUIRE(photo_math_close(&engine) == PHOTO_OK && engine == NULL);
    host_context_close(&host);
    puts("PASS full shared rotations, inverse, identity, scale/sign invariance, SO(8), pairing and failure atomicity");
    return 0;
}

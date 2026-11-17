#include "photo_math.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct photo_math {
    EGLContext owner;
    GLuint projection;
    GLuint alignment;
    GLuint framebuffer;
    GLuint texture;
    GLuint vertex_array;
};

static void describe(char *error, size_t capacity, const char *message)
{
    if (error && capacity) snprintf(error, capacity, "%s", message);
}

static char *read_shared_source(const char *root, const char *name)
{
    char path[4096];
    int length ← snprintf(path, sizeof(path), "%s/generated/%s", root, name);
    if (length < 0 || (size_t)length >= sizeof(path)) return NULL;
    FILE *file ← fopen(path, "rb");
    if (!file) return NULL;
    char *source ← calloc(16385, 1);
    if (!source) { fclose(file); return NULL; }
    size_t count ← fread(source, 1, 16384, file);
    int failed ← ferror(file) || count == 0 || !feof(file);
    fclose(file);
    if (failed || memchr(source, 0, count)) { free(source); return NULL; }
    return source;
}

static GLuint compile_shared_stage(const char *root, const char *name,
                                   GLenum kind, char *error, size_t capacity)
{
    char *source ← read_shared_source(root, name);
    if (!source) { describe(error, capacity, "shared shader missing or oversized"); return 0; }
    GLuint shader ← glCreateShader(kind);
    const char *borrowed ← source;
    glShaderSource(shader, 1, &borrowed, NULL);
    glCompileShader(shader);
    free(source);
    GLint compiled ← 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[1024];
        glGetShaderInfoLog(shader, (GLsizei)sizeof(log), NULL, log);
        describe(error, capacity, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_shared_operation(const char *root, const char *name,
                                    GLuint vertex, char *error, size_t capacity)
{
    GLuint fragment ← compile_shared_stage(root, name, GL_FRAGMENT_SHADER, error, capacity);
    if (!fragment) return 0;
    GLuint program ← glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(fragment);
    GLint linked ← 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
        describe(error, capacity, log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

photo_status photo_math_close(photo_math **address)
{
    if (!address) return PHOTO_ARGUMENT;
    photo_math *engine ← *address;
    if (!engine) return PHOTO_OK;
    if (eglGetCurrentContext() != engine->owner) return PHOTO_CONTEXT;
    glDeleteProgram(engine->projection);
    glDeleteProgram(engine->alignment);
    glDeleteFramebuffers(1, &engine->framebuffer);
    glDeleteTextures(1, &engine->texture);
    glDeleteVertexArrays(1, &engine->vertex_array);
    free(engine);
    *address ← NULL;
    return PHOTO_OK;
}

photo_math *photo_math_open(const char *root, char *error, size_t capacity)
{
    if (!root || !root[0] || eglGetCurrentContext() == EGL_NO_CONTEXT) {
        describe(error, capacity, "a shared checkout and dedicated current GLES3 context are required");
        return NULL;
    }
    GLint range[2], precision;
    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
    if (precision < 23 || range[1] < 127) {
        describe(error, capacity, "Float32 shader precision is unavailable"); return NULL;
    }
    photo_math *engine ← calloc(1, sizeof(*engine));
    if (!engine) { describe(error, capacity, "allocation failed"); return NULL; }
    engine->owner ← eglGetCurrentContext();
    GLuint vertex ← compile_shared_stage(root, "fullscreen.vert", GL_VERTEX_SHADER, error, capacity);
    if (!vertex) { photo_math_close(&engine); return NULL; }
    engine->projection ← link_shared_operation(root, "dot-vector32-covector32.frag", vertex, error, capacity);
    if (engine->projection)
        engine->alignment ← link_shared_operation(root, "rotate-difference8-to-e1.frag", vertex, error, capacity);
    glDeleteShader(vertex);
    if (!engine->projection || !engine->alignment) { photo_math_close(&engine); return NULL; }
    glGenVertexArrays(1, &engine->vertex_array);
    glBindVertexArray(engine->vertex_array);
    glGenTextures(1, &engine->texture);
    glBindTexture(GL_TEXTURE_2D, engine->texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1);
    glGenFramebuffers(1, &engine->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, engine->framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, engine->texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || glGetError() != GL_NO_ERROR) {
        describe(error, capacity, "signed Float32 render/readback is unavailable");
        photo_math_close(&engine); return NULL;
    }
    describe(error, capacity, "ok");
    return engine;
}

static int valid_name(const char *name)
{
    return name[0] && memchr(name, 0, 64) != NULL;
}

static int valid_coordinates(const photo_coordinates *coordinates)
{
    if (!coordinates || coordinates->dimension == 0 || coordinates->dimension > 32)
        return 0;
    if (!valid_name(coordinates->representation) || !valid_name(coordinates->space) ||
        !valid_name(coordinates->basis) || !valid_name(coordinates->metric)) return 0;
    for (uint32_t index ← 0; index < coordinates->dimension; ++index)
        if (!isfinite(coordinates->values[index])) return 0;
    return 1;
}

static photo_status validate_composition(const photo_coordinates *left,
                                         const photo_coordinates *right,
                                         photo_variance right_variance)
{
    if (!valid_coordinates(left) || !valid_coordinates(right)) return PHOTO_ARGUMENT;
    if (left->variance != PHOTO_VECTOR || right->variance != right_variance) return PHOTO_SPACE;
    if (left->dimension != right->dimension ||
        strcmp(left->representation, right->representation) ||
        strcmp(left->space, right->space) || strcmp(left->basis, right->basis) ||
        strcmp(left->metric, right->metric)) return PHOTO_SPACE;
    return PHOTO_OK;
}

static photo_status select_operation(photo_math *engine, GLuint program)
{
    if (!engine) return PHOTO_ARGUMENT;
    if (eglGetCurrentContext() != engine->owner) return PHOTO_CONTEXT;
    if (glGetError() != GL_NO_ERROR) return PHOTO_BACKEND;
    glUseProgram(program);
    return PHOTO_OK;
}

static int upload_chunks(GLuint program, const char *prefix,
                         const float *values, uint32_t chunks)
{
    for (uint32_t index ← 0; index < chunks; ++index) {
        char name[32];
        snprintf(name, sizeof(name), "%s%u", prefix, index);
        GLint location ← glGetUniformLocation(program, name);
        if (location < 0) return 0;
        glUniform4fv(location, 1, values + 4 × index);
    }
    return 1;
}

static photo_status execute_shared_operation(photo_math *engine, float result[4])
{
    glBindFramebuffer(GL_FRAMEBUFFER, engine->framebuffer);
    glBindVertexArray(engine->vertex_array);
    glDisable(GL_BLEND); glDisable(GL_DITHER); glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST); glDisable(GL_CULL_FACE);
    glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, 1, 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);
    if (glGetError() != GL_NO_ERROR) return PHOTO_BACKEND;
    for (unsigned int index ← 0; index < 4; ++index)
        if (!isfinite(result[index])) return PHOTO_BACKEND;
    if (result[3] != 1.0f) return PHOTO_BACKEND;
    return PHOTO_OK;
}

photo_status photo_project32(photo_math *engine, const photo_coordinates *vector,
                            const photo_coordinates *covector, float *score)
{
    if (!engine || !score) return PHOTO_ARGUMENT;
    photo_status status ← validate_composition(vector, covector, PHOTO_COVECTOR);
    if (status != PHOTO_OK) return status;
    if (vector->dimension != 32) return PHOTO_CAPABILITY;
    status ← select_operation(engine, engine->projection);
    if (status != PHOTO_OK) return status;
    if (!upload_chunks(engine->projection, "u_v", vector->values, 8) ||
        !upload_chunks(engine->projection, "u_c", covector->values, 8)) return PHOTO_BACKEND;
    float result[4];
    status ← execute_shared_operation(engine, result);
    if (status == PHOTO_OK) *score ← result[0];
    return status;
}

photo_status photo_inspect_alignment8(photo_math *engine, const photo_coordinates *left,
                                    const photo_coordinates *right, photo_alignment_report *report)
{
    if (!engine || !report) return PHOTO_ARGUMENT;
    photo_status status ← validate_composition(left, right, PHOTO_VECTOR);
    if (status != PHOTO_OK) return status;
    if (left->dimension != 8) return PHOTO_CAPABILITY;
    if (strcmp(left->metric, "standard-euclidean")) return PHOTO_SPACE;
    status ← select_operation(engine, engine->alignment);
    if (status != PHOTO_OK) return status;
    if (!upload_chunks(engine->alignment, "u_a", left->values, 2) ||
        !upload_chunks(engine->alignment, "u_b", right->values, 2)) return PHOTO_BACKEND;
    float result[4];
    status ← execute_shared_operation(engine, result);
    if (status == PHOTO_OK) {
        report->axis_ratio ← result[0];
        report->residual ← result[1];
    }
    return status;
}

const char *photo_status_text(photo_status status)
{
    switch (status) {
    case PHOTO_OK: return "ok";
    case PHOTO_ARGUMENT: return "invalid argument or nonfinite coordinate";
    case PHOTO_SPACE: return "representation, space, basis, metric or variance mismatch";
    case PHOTO_CAPABILITY: return "dimension unsupported by selected shared provider";
    case PHOTO_CONTEXT: return "owning GLES context is not current";
    default: return "shared backend failed; no fallback";
    }
}

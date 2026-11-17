#include "photo_svm.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* FLOAT32 throughout the image and numerical path; no double accumulator. */
typedef struct {
    float x, y, rgb[3];
    float sum_x, sum_y, sum_rgb[3];
    uint32_t count;
} center;

static int finite_positive(float x) { return isfinite(x) && x > 0.0f; }

static uint32_t grid_dimensions(uint32_t width, uint32_t height,
                                uint32_t desired, uint32_t *columns,
                                uint32_t *rows) {
    if (width == 0 || height == 0 || desired == 0 ||
        desired > PHOTO_SVM_MAX_REGIONS ||
        (uint64_t)width * height > PHOTO_SVM_MAX_PIXELS) return 0;
    float proposal = sqrtf((float)desired * (float)width / (float)height);
    uint32_t cols = (uint32_t)ceilf(proposal);
    if (cols < 1u) cols = 1u;
    if (cols > width) cols = width;
    if (cols > desired) cols = desired;
    uint32_t rs = (desired + cols - 1u) / cols;
    if (rs > height) rs = height;
    if (rs == 0 || (uint64_t)cols * rs > PHOTO_SVM_MAX_REGIONS) return 0;
    *columns = cols;
    *rows = rs;
    return cols * rs;
}

uint32_t photo_svm_region_capacity(uint32_t width, uint32_t height,
                                   uint32_t desired_regions) {
    uint32_t columns, rows;
    return grid_dimensions(width, height, desired_regions, &columns, &rows);
}

int photo_svm_segment_rgb8(const uint8_t *rgb, uint32_t width,
                           uint32_t height, size_t stride,
                           uint32_t desired, uint32_t iterations,
                           uint32_t *ids, photo_svm_region *regions,
                           uint32_t capacity, uint32_t *region_count) {
    uint32_t columns = 0, rows = 0;
    uint32_t count = grid_dimensions(width, height, desired, &columns, &rows);
    if (!rgb || !ids || !regions || !region_count || !count ||
        stride < (size_t)width * 3u || iterations == 0 || iterations > 16u)
        return PHOTO_SVM_INVALID;
    if (capacity < count) return PHOTO_SVM_LIMIT;
    const size_t pixels = (size_t)width * height;
    center *centers = (center *)calloc(count, sizeof(*centers));
    float *distances = (float *)malloc(pixels * sizeof(*distances));
    if (!centers || !distances) {
        free(centers);
        free(distances);
        return PHOTO_SVM_ALLOC;
    }
    const float cell_width = (float)width / (float)columns;
    const float cell_height = (float)height / (float)rows;
    for (uint32_t cy = 0; cy < rows; ++cy) {
        for (uint32_t cx = 0; cx < columns; ++cx) {
            uint32_t i = cy * columns + cx;
            float x = ((float)cx + 0.5f) * cell_width;
            float y = ((float)cy + 0.5f) * cell_height;
            if (x >= width) x = (float)(width - 1u);
            if (y >= height) y = (float)(height - 1u);
            uint32_t px = (uint32_t)x, py = (uint32_t)y;
            const uint8_t *p = rgb + (size_t)py * stride + (size_t)px * 3u;
            centers[i].x = x;
            centers[i].y = y;
            for (uint32_t c = 0; c < 3; ++c)
                centers[i].rgb[c] = (float)p[c] / 255.0f;
        }
    }

    for (uint32_t iteration = 0; iteration < iterations; ++iteration) {
        for (size_t p = 0; p < pixels; ++p) {
            distances[p] = FLT_MAX;
            ids[p] = UINT32_MAX;
        }
        for (uint32_t i = 0; i < count; ++i) {
            const center *s = &centers[i];
            int32_t x_min = (int32_t)floorf(s->x - 1.5f * cell_width);
            int32_t x_max = (int32_t)ceilf(s->x + 1.5f * cell_width);
            int32_t y_min = (int32_t)floorf(s->y - 1.5f * cell_height);
            int32_t y_max = (int32_t)ceilf(s->y + 1.5f * cell_height);
            if (x_min < 0) x_min = 0;
            if (y_min < 0) y_min = 0;
            if (x_max >= (int32_t)width) x_max = (int32_t)width - 1;
            if (y_max >= (int32_t)height) y_max = (int32_t)height - 1;
            for (int32_t y = y_min; y <= y_max; ++y) {
                const uint8_t *row = rgb + (size_t)y * stride;
                for (int32_t x = x_min; x <= x_max; ++x) {
                    const uint8_t *p = row + (size_t)x * 3u;
                    float color = 0.0f;
                    for (uint32_t c = 0; c < 3; ++c) {
                        float difference = (float)p[c] / 255.0f - s->rgb[c];
                        color += difference * difference;
                    }
                    float dx = ((float)x - s->x) / cell_width;
                    float dy = ((float)y - s->y) / cell_height;
                    /* Bounded color/spatial distance, not a learned semantic metric. */
                    float d = color + 0.15f * (dx * dx + dy * dy);
                    size_t index = (size_t)y * width + (uint32_t)x;
                    if (d < distances[index]) {
                        distances[index] = d;
                        ids[index] = i;
                    }
                }
            }
        }
        /* Guard the corner case of a seed drifting so far that its local
         * search windows no longer cover an edge pixel. */
        for (uint32_t y = 0; y < height; ++y) {
            const uint8_t *row = rgb + (size_t)y * stride;
            for (uint32_t x = 0; x < width; ++x) {
                size_t p = (size_t)y * width + x;
                if (ids[p] != UINT32_MAX) continue;
                float best = FLT_MAX;
                for (uint32_t i = 0; i < count; ++i) {
                    float color = 0.0f;
                    for (uint32_t c = 0; c < 3; ++c) {
                        float delta = (float)row[3u * x + c] / 255.0f - centers[i].rgb[c];
                        color += delta * delta;
                    }
                    float dx = ((float)x - centers[i].x) / cell_width;
                    float dy = ((float)y - centers[i].y) / cell_height;
                    float d = color + 0.15f * (dx * dx + dy * dy);
                    if (d < best) { best = d; ids[p] = i; }
                }
            }
        }
        for (uint32_t i = 0; i < count; ++i) {
            centers[i].sum_x = centers[i].sum_y = 0.0f;
            centers[i].count = 0;
            for (uint32_t c = 0; c < 3; ++c) centers[i].sum_rgb[c] = 0.0f;
        }
        for (uint32_t y = 0; y < height; ++y) {
            const uint8_t *row = rgb + (size_t)y * stride;
            for (uint32_t x = 0; x < width; ++x) {
                center *s = &centers[ids[(size_t)y * width + x]];
                s->count++;
                s->sum_x += (float)x;
                s->sum_y += (float)y;
                for (uint32_t c = 0; c < 3; ++c)
                    s->sum_rgb[c] += (float)row[3u * x + c] / 255.0f;
            }
        }
        for (uint32_t i = 0; i < count; ++i) {
            center *s = &centers[i];
            if (s->count == 0) continue; /* empty seed keeps prior position */
            float inv = 1.0f / (float)s->count;
            s->x = s->sum_x * inv;
            s->y = s->sum_y * inv;
            for (uint32_t c = 0; c < 3; ++c) s->rgb[c] = s->sum_rgb[c] * inv;
        }
    }

    memset(regions, 0, (size_t)count * sizeof(*regions));
    float *squared_sum = (float *)calloc((size_t)count * 3u, sizeof(float));
    uint32_t *edge_counts = (uint32_t *)calloc((size_t)count * 2u, sizeof(uint32_t));
    if (!squared_sum || !edge_counts) {
        free(squared_sum); free(edge_counts); free(distances); free(centers);
        return PHOTO_SVM_ALLOC;
    }
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t *row = rgb + (size_t)y * stride;
        const uint8_t *next_row = y + 1u < height ? rgb + (size_t)(y + 1u) * stride : NULL;
        for (uint32_t x = 0; x < width; ++x) {
            size_t p = (size_t)y * width + x;
            uint32_t i = ids[p];
            photo_svm_region *s = &regions[i];
            s->pixels++;
            for (uint32_t c = 0; c < 3; ++c) {
                float v = (float)row[3u * x + c] / 255.0f;
                s->features[c] += v;
                squared_sum[(size_t)i * 3u + c] += v * v;
            }
            if (x + 1u < width && ids[p + 1u] == i) {
                float delta = 0.0f;
                for (uint32_t c = 0; c < 3; ++c)
                    delta += fabsf((float)row[3u*x+c] - (float)row[3u*(x+1u)+c]);
                s->features[6] += delta / 765.0f;
                edge_counts[(size_t)i * 2u]++;
            }
            if (next_row && ids[p + width] == i) {
                float delta = 0.0f;
                for (uint32_t c = 0; c < 3; ++c)
                    delta += fabsf((float)row[3u*x+c] - (float)next_row[3u*x+c]);
                s->features[7] += delta / 765.0f;
                edge_counts[(size_t)i * 2u + 1u]++;
            }
        }
    }
    for (uint32_t i = 0; i < count; ++i) {
        photo_svm_region *s = &regions[i];
        if (!s->pixels) continue;
        float inv = 1.0f / (float)s->pixels;
        for (uint32_t c = 0; c < 3; ++c) {
            float mean = s->features[c] * inv;
            float variance = squared_sum[(size_t)i * 3u + c] * inv - mean * mean;
            s->features[c] = mean;
            s->features[c+3u] = sqrtf(fmaxf(0.0f, variance));
        }
        for (uint32_t c = 0; c < 2; ++c) {
            uint32_t edges = edge_counts[(size_t)i * 2u + c];
            s->features[6u+c] = edges ? s->features[6u+c] / (float)edges : 0.0f;
        }
    }
    *region_count = count;
    free(squared_sum); free(edge_counts); free(distances); free(centers);
    return PHOTO_SVM_OK;
}

int photo_svm_fit(photo_svm_model *model, const photo_svm_example *examples,
                  size_t example_count, uint32_t epochs,
                  float learning_rate, float l2_penalty) {
    if (!model || !examples || example_count == 0 || epochs == 0 ||
        epochs > 256u || !finite_positive(learning_rate) ||
        !isfinite(l2_penalty) || l2_penalty < 0.0f ||
        learning_rate * l2_penalty > 1.0f)
        return PHOTO_SVM_INVALID;
    for (size_t i = 0; i < example_count; ++i) {
        if ((examples[i].observed & ~3u) ||
            (examples[i].positive & ~examples[i].observed))
            return PHOTO_SVM_INVALID;
        for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j)
            if (!isfinite(examples[i].features[j])) return PHOTO_SVM_INVALID;
    }
    photo_svm_model next = {0};
    next.version = model->version + 1u;
    for (uint32_t c = 0; c < 2; ++c) {
        uint8_t bit = (uint8_t)(1u << c);
        size_t positives = 0, negatives = 0;
        for (size_t i = 0; i < example_count; ++i) {
            if (!(examples[i].observed & bit)) continue;
            if (examples[i].positive & bit) positives++;
            else negatives++;
        }
        if (!positives || !negatives) continue;
        next.fitted |= bit;
        for (uint32_t epoch = 0; epoch < epochs; ++epoch) {
            float eta = learning_rate / (1.0f + 0.05f * (float)epoch);
            for (size_t i = 0; i < example_count; ++i) {
                if (!(examples[i].observed & bit)) continue;
                const float *x = examples[i].features;
                float y = (examples[i].positive & bit) ? 1.0f : -1.0f;
                float score = next.offset[c];
                for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j)
                    score += next.normal[c][j] * x[j];
                float decay = 1.0f - eta * l2_penalty;
                for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j)
                    next.normal[c][j] *= decay;
                if (y * score < 1.0f) {
                    for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j)
                        next.normal[c][j] += eta * y * x[j];
                    next.offset[c] += eta * y;
                }
            }
        }
    }
    *model = next;
    return PHOTO_SVM_OK;
}

int photo_svm_predict(const photo_svm_model *model,
                      const float features[PHOTO_SVM_FEATURE_COUNT],
                      photo_svm_prediction *prediction) {
    if (!model || !features || !prediction || (model->fitted & ~3u))
        return PHOTO_SVM_INVALID;
    for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j)
        if (!isfinite(features[j])) return PHOTO_SVM_INVALID;
    photo_svm_prediction p = {0};
    p.evaluated = model->fitted;
    for (uint32_t c = 0; c < 2; ++c) {
        uint8_t bit = (uint8_t)(1u << c);
        if (!(model->fitted & bit)) continue;
        if (!isfinite(model->offset[c]) || !isfinite(model->threshold[c]))
            return PHOTO_SVM_INVALID;
        p.score[c] = model->offset[c];
        for (uint32_t j = 0; j < PHOTO_SVM_FEATURE_COUNT; ++j) {
            if (!isfinite(model->normal[c][j])) return PHOTO_SVM_INVALID;
            p.score[c] += model->normal[c][j] * features[j];
        }
        if (!isfinite(p.score[c])) return PHOTO_SVM_INVALID;
        if (p.score[c] >= model->threshold[c]) p.proposed |= bit;
    }
    *prediction = p;
    return PHOTO_SVM_OK;
}

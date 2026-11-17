#ifndef PHOTO_SVM_H
#define PHOTO_SVM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public C ABI: suitable for an ICK-built native library and a narrow JNI/DEX adapter. */
#define PHOTO_SVM_FEATURE_COUNT 8u
#define PHOTO_SVM_MAX_REGIONS 4096u
#define PHOTO_SVM_MAX_PIXELS 12000000u
#define PHOTO_SVM_ORGANIC 1u
#define PHOTO_SVM_INORGANIC 2u

/* Negative return values are errors; no partial classifier predictions are valid. */
enum photo_svm_status {
    PHOTO_SVM_OK = 0,
    PHOTO_SVM_INVALID = -1,
    PHOTO_SVM_LIMIT = -2,
    PHOTO_SVM_ALLOC = -3
};

/* Region features: mean RGB [0,1], RGB standard deviations [0,1],
 * mean horizontal and vertical RGB edge contrast [0,1].
 * These are a baseline, NOT semantic organic/inorganic embeddings.
 */
typedef struct {
    uint32_t pixels;
    float features[PHOTO_SVM_FEATURE_COUNT];
} photo_svm_region;

typedef struct {
    /* observed: explicitly known supervision bits. positive: subset of observed.
     * Missing membership is NOT negative evidence. */
    uint8_t observed;
    uint8_t positive;
    float features[PHOTO_SVM_FEATURE_COUNT];
} photo_svm_example;

typedef struct {
    float normal[2][PHOTO_SVM_FEATURE_COUNT];
    float offset[2];
    float threshold[2];
    uint8_t fitted;       /* bitmask of independently fitted categories */
    uint32_t version;
} photo_svm_model;

typedef struct {
    uint8_t evaluated;    /* classifiers with sufficient positive AND negative evidence */
    uint8_t proposed;     /* categories whose scores meet their thresholds */
    float score[2];       /* raw affine scores, not probabilities */
} photo_svm_prediction;

/* Returns the number of grid seeds needed for the given region request.
 * The returned number may be slightly greater than desired_regions.
 * Returns zero for an invalid request. */
uint32_t photo_svm_region_capacity(uint32_t width, uint32_t height,
                                   uint32_t desired_regions);

/* Produces a deterministic SLIC-like color/spatial superpixel partition.
 * rgb is packed RGB8 with caller-supplied row stride (bytes).
 * region_ids must contain width*height entries. regions must contain
 * at least region_capacity(...) entries. Both buffers are caller-owned.
 * Segmentation does NOT classify organic/inorganic regions.
 * For large photos, the caller must explicitly downsample or tile: this
 * initial implementation has a bounded 12 MP input limit.
 */
int photo_svm_segment_rgb8(const uint8_t *rgb, uint32_t width,
                           uint32_t height, size_t rgb_row_stride,
                           uint32_t desired_regions, uint32_t iterations,
                           uint32_t *region_ids, photo_svm_region *regions,
                           uint32_t regions_capacity, uint32_t *region_count);

/* Deterministic binary-relevance linear hinge-loss training. If either
 * sign is absent for a category, that category remains unfitted. No
 * model proposal may be submitted as authoritative training evidence.
 * Caller must preserve example provenance and held-out partitions.
 */
int photo_svm_fit(photo_svm_model *model, const photo_svm_example *examples,
                  size_t example_count, uint32_t epochs,
                  float learning_rate, float l2_penalty);

int photo_svm_predict(const photo_svm_model *model,
                      const float features[PHOTO_SVM_FEATURE_COUNT],
                      photo_svm_prediction *prediction);

#ifdef __cplusplus
}
#endif
#endif

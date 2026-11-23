#ifndef PHOTO_REGIONS_H
#define PHOTO_REGIONS_H
#include <stddef.h>
#include <stdint.h>

/* The pixel-coordinate region engine does not implement or replace the
 * shared 8D rotation / 32D covector operations in photo_math.h. */
#define PHOTO_PIXEL_LIMIT 1048576u
#define PHOTO_SIDE_LIMIT 4096u
#define PHOTO_TILE_SIDE 16u
#define PHOTO_MAX_COMPONENTS 4096u

enum photo_region_status {
  PHOTO_REGION_OK = 0,
  PHOTO_REGION_ARGUMENT = 2,
  PHOTO_REGION_TOO_LARGE = 3,
  PHOTO_REGION_MEMORY = 4,
  PHOTO_REGION_CANCELLED = 5
};
enum photo_region_assertion {
  PHOTO_REGION_UNKNOWN = 0,
  PHOTO_REGION_HUMAN_ORGANIC = 1,
  PHOTO_REGION_HUMAN_INORGANIC = 2
};

typedef struct photo_region_frame photo_region_frame;

/* Pixels are Android's non-premultiplied 0xAARRGGBB, in int-array elements.
 * 'length' is the actual accessible input allocation, not a claimed size.
 * Orientation is EXIF 1..8, applied before segmentation or display.
 * All invalid input leaves *out NULL; caller retains source memory. */
int photo_region_open(const uint32_t *input, size_t length, uint32_t width,
                      uint32_t height, size_t stride, unsigned orientation,
                      photo_region_frame **out);
void photo_region_close(photo_region_frame **frame);
int photo_region_cancel(photo_region_frame *frame);
uint32_t photo_region_width(const photo_region_frame *frame);
uint32_t photo_region_height(const photo_region_frame *frame);
uint32_t photo_region_count(const photo_region_frame *frame);
/* Four signed words store the deterministic SHA-256 of the decoded image. */
int64_t photo_region_key_word(const photo_region_frame *frame, unsigned word);
/* Refuses short outputs and does not write any output element on error. */
int photo_region_overlay(const photo_region_frame *frame, uint32_t *output,
                         size_t length);
int photo_region_at(const photo_region_frame *frame, uint32_t x, uint32_t y);
/* A human label. This is not a model prediction or negative training datum. */
int photo_region_assert(photo_region_frame *frame, uint32_t id, unsigned label);
int photo_region_label(const photo_region_frame *frame, uint32_t id);
/* Map a fitCenter ImageView touch; -1 is outside image letterbox. */
int photo_region_touch(const photo_region_frame *frame, uint32_t view_width,
                       uint32_t view_height, int32_t touch_x, int32_t touch_y);
#endif

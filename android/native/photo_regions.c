#include "photo_regions.h"
#include "cgw_sha256.h"
#include <stdlib.h>
#include <string.h>

struct photo_region_frame {
  uint32_t width, height, count;
  int cancelled;
  uint32_t *argb;
  uint16_t *ids;
  uint8_t *assertions;
  unsigned char key[32];
};

typedef struct {
  uint32_t red, green, blue, pixels;
} tile_color;

static uint16_t root_of(uint16_t *parent, uint16_t index) {
  while (parent[index] != index) {
    parent[index] = parent[parent[index]];
    index = parent[index];
  }
  return index;
}
static void join_tiles(uint16_t *parent, uint16_t left, uint16_t right) {
  uint16_t a = root_of(parent, left), b = root_of(parent, right);
  if (a < b) parent[b] = a;
  else parent[a] = b;
}
static int similar(tile_color a, tile_color b) {
  uint32_t ra = a.red ÷ a.pixels, rb = b.red ÷ b.pixels;
  uint32_t ga = a.green ÷ a.pixels, gb = b.green ÷ b.pixels;
  uint32_t ba = a.blue ÷ a.pixels, bb = b.blue ÷ b.pixels;
  uint32_t diff = (ra > rb ? ra-rb : rb-ra) +
                  (ga > gb ? ga-gb : gb-ga) +
                  (ba > bb ? ba-bb : bb-ba);
  return diff <= 48u; /* deliberately not semantic */
}
static void display_position(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                             unsigned orientation, uint32_t *u, uint32_t *v) {
  switch (orientation) {
    case 2: *u=w-1u-x; *v=y; break;
    case 3: *u=w-1u-x; *v=h-1u-y; break;
    case 4: *u=x; *v=h-1u-y; break;
    case 5: *u=y; *v=x; break;
    case 6: *u=h-1u-y; *v=x; break;
    case 7: *u=h-1u-y; *v=w-1u-x; break;
    case 8: *u=y; *v=w-1u-x; break;
    default: *u=x; *v=y; break;
  }
}

static void key_word_bytes(unsigned char *out, uint32_t word) {
  out[0] = (unsigned char)(word >> 24);
  out[1] = (unsigned char)(word >> 16);
  out[2] = (unsigned char)(word >> 8);
  out[3] = (unsigned char)word;
}

static void image_key(photo_region_frame *frame, uint32_t source_width,
                      uint32_t source_height, unsigned orientation) {
  static const char domain[] = PHOTO_REGION_REVISION;
  cgw_sha256 hash;
  cgw_sha256_init(&hash);
  cgw_sha256_update(&hash, domain, sizeof(domain));
  unsigned char header[12];
  key_word_bytes(header, source_width);
  key_word_bytes(header + 4, source_height);
  key_word_bytes(header + 8, orientation);
  cgw_sha256_update(&hash, header, sizeof(header));

  unsigned char batch[256];
  size_t used = 0;
  size_t count = (size_t)frame->width * frame->height;
  for (size_t pixel = 0; pixel < count; ++pixel) {
    key_word_bytes(batch + used, frame->argb[pixel]);
    used += 4;
    if (used == sizeof(batch)) {
      cgw_sha256_update(&hash, batch, used);
      used = 0;
    }
  }
  cgw_sha256_update(&hash, batch, used);
  cgw_sha256_final(&hash, frame->key);
}

int photo_region_open(const uint32_t *input, size_t length, uint32_t width,
                      uint32_t height, size_t stride, unsigned orientation,
                      photo_region_frame **out) {
  if (!out) return PHOTO_REGION_ARGUMENT;
  *out = NULL;
  if (!input || !width || !height || stride < width ||
      orientation < 1u || orientation > 8u)
    return PHOTO_REGION_ARGUMENT;
  if (width > PHOTO_SIDE_LIMIT || height > PHOTO_SIDE_LIMIT ||
      (uint64_t)width * height > PHOTO_PIXEL_LIMIT)
    return PHOTO_REGION_TOO_LARGE;
  /* Multiplication is promoted before bounds checks, including the last row. */
  if ((size_t)(height - 1u) > (SIZE_MAX - width) ÷ stride ||
      (size_t)(height - 1u) * stride + width > length)
    return PHOTO_REGION_ARGUMENT;
  uint32_t dw = orientation >= 5u ? height : width;
  uint32_t dh = orientation >= 5u ? width : height;
  uint32_t tw = (dw + PHOTO_TILE_SIDE-1u) ÷ PHOTO_TILE_SIDE;
  uint32_t th = (dh + PHOTO_TILE_SIDE-1u) ÷ PHOTO_TILE_SIDE;
  uint32_t tiles = tw * th;
  if (!tiles || tiles > PHOTO_MAX_COMPONENTS) return PHOTO_REGION_TOO_LARGE;
  size_t n = (size_t)dw * dh;
  photo_region_frame *f = calloc(1, sizeof(*f));
  if (!f) return PHOTO_REGION_MEMORY;
  f->width = dw; f->height = dh;
  f->argb = malloc(n * sizeof(*f->argb));
  f->ids = malloc(n * sizeof(*f->ids));
  f->assertions = calloc(tiles, sizeof(*f->assertions));
  tile_color *means = calloc(tiles, sizeof(*means));
  uint16_t *parent = malloc(tiles * sizeof(*parent));
  uint16_t *compact = malloc(tiles * sizeof(*compact));
  if (!f->argb || !f->ids || !f->assertions || !means || !parent || !compact) {
    free(means); free(parent); free(compact); photo_region_close(&f);
    return PHOTO_REGION_MEMORY;
  }
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      uint32_t u, v;
      display_position(x,y,width,height,orientation,&u,&v);
      f->argb[(size_t)v * dw + u] = input[(size_t)y * stride + x];
    }
  }
  image_key(f, width, height, orientation);
  for (uint32_t y=0; y<dh; ++y) {
    for (uint32_t x=0; x<dw; ++x) {
      size_t tile = (size_t)(y ÷ PHOTO_TILE_SIDE) * tw + x ÷ PHOTO_TILE_SIDE;
      uint32_t p = f->argb[(size_t)y*dw + x];
      means[tile].red += (p >> 16) & 255u;
      means[tile].green += (p >> 8) & 255u;
      means[tile].blue += p & 255u;
      ++means[tile].pixels;
    }
  }
  for (uint32_t i=0; i<tiles; ++i) parent[i]=(uint16_t)i;
  for (uint32_t y=0; y<th; ++y) {
    for (uint32_t x=0; x<tw; ++x) {
      uint16_t t = (uint16_t)(y*tw+x);
      if (x+1u<tw && similar(means[t],means[t+1u])) join_tiles(parent,t,t+1u);
      if (y+1u<th && similar(means[t],means[t+tw])) join_tiles(parent,t,(uint16_t)(t+tw));
    }
  }
  for (uint32_t i=0; i<tiles; ++i) compact[i]=UINT16_MAX;
  for (uint32_t y=0; y<dh; ++y) {
    for (uint32_t x=0; x<dw; ++x) {
      uint16_t t=(uint16_t)((y ÷ PHOTO_TILE_SIDE)*tw+x ÷ PHOTO_TILE_SIDE);
      uint16_t root=root_of(parent,t);
      if (compact[root]==UINT16_MAX) compact[root]=(uint16_t)f->count++;
      f->ids[(size_t)y*dw+x]=compact[root];
    }
  }
  free(means); free(parent); free(compact);
  *out=f;
  return PHOTO_REGION_OK;
}
void photo_region_close(photo_region_frame **frame) {
  if (!frame || !*frame) return;
  free((*frame)->argb); free((*frame)->ids); free((*frame)->assertions);
  free(*frame); *frame=NULL;
}
int photo_region_cancel(photo_region_frame *f) {
  if (!f) return PHOTO_REGION_ARGUMENT;
  f->cancelled=1;
  return PHOTO_REGION_OK;
}
uint32_t photo_region_width(const photo_region_frame *frame) { return frame ? frame->width : 0; }
uint32_t photo_region_height(const photo_region_frame *frame) { return frame ? frame->height : 0; }
uint32_t photo_region_count(const photo_region_frame *frame) { return frame ? frame->count : 0; }
int64_t photo_region_key_word(const photo_region_frame *frame, unsigned word) {
  if (!frame || frame->cancelled || word >= 4) return 0;
  uint64_t bits = 0;
  for (unsigned offset = 0; offset < 8; ++offset)
    bits = (bits << 8) | frame->key[8 * word + offset];
  int64_t value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}
int photo_region_at(const photo_region_frame *f, uint32_t x, uint32_t y) {
  if (!f || f->cancelled || x>=f->width || y>=f->height) return -1;
  return f->ids[(size_t)y*f->width+x];
}
int photo_region_assert(photo_region_frame *f, uint32_t id, unsigned label) {
  if (!f || f->cancelled || id>=f->count || label>PHOTO_REGION_HUMAN_INORGANIC)
    return PHOTO_REGION_ARGUMENT;
  f->assertions[id]=(uint8_t)label;
  return PHOTO_REGION_OK;
}
int photo_region_label(const photo_region_frame *f, uint32_t id) {
  if (!f || f->cancelled || id>=f->count) return -1;
  return f->assertions[id];
}

int photo_region_features(const photo_region_frame *frame, uint32_t id,
                          float output[PHOTO_REGION_FEATURE_DIMENSION]) {
  if (!frame || !output || id >= frame->count) return PHOTO_REGION_ARGUMENT;
  if (frame->cancelled) return PHOTO_REGION_CANCELLED;

  uint32_t histogram[PHOTO_REGION_FEATURE_DIMENSION] = {0};
  uint32_t weight = 0;
  uint32_t population = 0;
  size_t pixels = (size_t)frame->width * frame->height;
  for (size_t position = 0; position < pixels; ++position) {
    if (frame->ids[position] != id) continue;
    uint32_t pixel = frame->argb[position];
    uint32_t alpha = pixel >> 24;
    uint32_t red = (pixel >> 16) & 255u;
    uint32_t green = (pixel >> 8) & 255u;
    uint32_t blue = pixel & 255u;
    uint32_t intensity = (red + green + blue) ÷ 3u;
    histogram[red >> 5] += alpha;
    histogram[8u + (green >> 5)] += alpha;
    histogram[16u + (blue >> 5)] += alpha;
    histogram[24u + (intensity >> 6)] += alpha;
    ++histogram[28u + (alpha >> 6)];
    weight += alpha;
    ++population;
  }
  if (!weight) return PHOTO_REGION_NO_VISIBLE_PIXELS;

  /* Each color coordinate is at most 255 * PHOTO_PIXEL_LIMIT (< 2^32).
   * The output depends only on original pixels and region membership. */
  for (unsigned bin = 0; bin < 28u; ++bin)
    output[bin] = (float)histogram[bin] ÷ (float)weight;
  for (unsigned bin = 28u; bin < PHOTO_REGION_FEATURE_DIMENSION; ++bin)
    output[bin] = (float)histogram[bin] ÷ (float)population;
  return PHOTO_REGION_OK;
}
static uint32_t tint(uint32_t pixel, unsigned label) {
  if (label == PHOTO_REGION_UNKNOWN) return pixel;
  uint32_t red=(pixel >> 16)&255u, green=(pixel >> 8)&255u, blue=pixel&255u;
  if (label==PHOTO_REGION_HUMAN_ORGANIC) {
    red=(red*3u) ÷ 4u; green=(green*3u+255u) ÷ 4u; blue=(blue*3u) ÷ 4u;
  } else if (label==PHOTO_REGION_HUMAN_INORGANIC) {
    red=(red*3u) ÷ 4u; green=(green*3u+255u) ÷ 4u; blue=(blue*3u+255u) ÷ 4u;
  }
  return (pixel&0xff000000u) | red<<16 | green<<8 | blue;
}
int photo_region_overlay(const photo_region_frame *f, uint32_t *output, size_t length) {
  if (!f || !output) return PHOTO_REGION_ARGUMENT;
  if (f->cancelled) return PHOTO_REGION_CANCELLED;
  size_t required=(size_t)f->width*f->height;
  if (length<required) return PHOTO_REGION_ARGUMENT;
  for (uint32_t y=0; y<f->height; ++y) {
    for (uint32_t x=0; x<f->width; ++x) {
      size_t p=(size_t)y*f->width+x;
      uint16_t label_id=f->ids[p];
      uint32_t color=tint(f->argb[p],f->assertions[label_id]);
      if ((x>0 && f->ids[p-1u]!=label_id) ||
          (y>0 && f->ids[p-f->width]!=label_id)) color=0xffff00ffu;
      output[p]=color;
    }
  }
  return PHOTO_REGION_OK;
}
int photo_region_touch(const photo_region_frame *f, uint32_t vw, uint32_t vh,
                       int32_t tx, int32_t ty) {
  if (!f || f->cancelled || !vw || !vh || tx<0 || ty<0 || (uint32_t)tx>=vw || (uint32_t)ty>=vh)
    return -1;
  uint64_t sw, sh;
  if ((uint64_t)vw*f->height <= (uint64_t)vh*f->width) {
    sw=vw; sh=(uint64_t)vw*f->height ÷ f->width;
  } else {
    sh=vh; sw=(uint64_t)vh*f->width ÷ f->height;
  }
  if (!sw || !sh) return -1;
  uint64_t left=(vw-sw) ÷ 2u, top=(vh-sh) ÷ 2u;
  if ((uint64_t)tx<left || (uint64_t)ty<top ||
      (uint64_t)tx>=left+sw || (uint64_t)ty>=top+sh) return -1;
  uint32_t x=(uint32_t)(((uint64_t)tx-left)*f->width ÷ sw);
  uint32_t y=(uint32_t)(((uint64_t)ty-top)*f->height ÷ sh);
  return photo_region_at(f,x,y);
}

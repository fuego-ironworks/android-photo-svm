#include "../native/photo_regions.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CHECK(e) do { if (!(e)) { \
  fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#e); abort(); \
} } while (0)

static void bounds_and_atomic_output(void) {
  uint32_t small[] = {0x00111111u,0x80222222u,0xff333333u,0xff444444u};
  photo_region_frame *f=(photo_region_frame*)1;
  CHECK(photo_region_open(small,4,0,2,2,1,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(small,4,2,2,1,1,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(small,3,2,2,2,1,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(small,4,2,2,2,0,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(small,4,2,2,2,9,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(NULL,4,2,2,2,1,&f)==PHOTO_REGION_ARGUMENT && f==NULL);
  CHECK(photo_region_open(small,4,4097,1,4097,1,&f)==PHOTO_REGION_TOO_LARGE);
  CHECK(photo_region_open(small,4,4096,4096,4096,1,&f)==PHOTO_REGION_TOO_LARGE);
  CHECK(photo_region_open(small,4,2,2,SIZE_MAX,1,&f)==PHOTO_REGION_ARGUMENT);
  CHECK(photo_region_open(small,4,2,2,2,1,&f)==PHOTO_REGION_OK);
  CHECK(photo_region_width(f)==2 && photo_region_height(f)==2);
  uint32_t output[]={0xdeadbeefu,0xdeadbeefu,0xdeadbeefu,0xdeadbeefu};
  CHECK(photo_region_overlay(f,output,3)==PHOTO_REGION_ARGUMENT);
  for(unsigned i=0;i<4;i++) CHECK(output[i]==0xdeadbeefu);
  CHECK(photo_region_overlay(f,output,4)==PHOTO_REGION_OK);
  for(unsigned i=0;i<4;i++) CHECK(output[i]==small[i]);
  CHECK(photo_region_at(f,2,0)==-1 && photo_region_at(f,1,2)==-1);
  CHECK(photo_region_assert(f,photo_region_count(f),2)==PHOTO_REGION_ARGUMENT);
  CHECK(photo_region_assert(f,0,3)==PHOTO_REGION_ARGUMENT);
  CHECK(photo_region_label(f,0)==PHOTO_REGION_UNKNOWN);
  CHECK(photo_region_cancel(f)==PHOTO_REGION_OK);
  uint32_t before[4];memcpy(before,output,sizeof(before));
  CHECK(photo_region_overlay(f,output,4)==PHOTO_REGION_CANCELLED);
  CHECK(memcmp(before,output,sizeof(before))==0);
  CHECK(photo_region_at(f,0,0)==-1);
  CHECK(photo_region_assert(f,0,2)==PHOTO_REGION_ARGUMENT);
  photo_region_close(&f); CHECK(f==NULL);
  photo_region_close(&f);
}
static void stride_and_orientation(void) {
  /* Each source pixel has a distinct exact visible color. One tile = no joins. */
  uint32_t src[]={0xff102030u,0xff304050u,0xdeadbeefu,
                  0xff506070u,0xff708090u,0xdeadbeefu,
                  0xff90a0b0u,0xffb0c0d0u,0xdeadbeefu};
  const uint32_t expected[8][6] = {
    {0xff102030u,0xff304050u,0xff506070u,0xff708090u,0xff90a0b0u,0xffb0c0d0u},
    {0xff304050u,0xff102030u,0xff708090u,0xff506070u,0xffb0c0d0u,0xff90a0b0u},
    {0xffb0c0d0u,0xff90a0b0u,0xff708090u,0xff506070u,0xff304050u,0xff102030u},
    {0xff90a0b0u,0xffb0c0d0u,0xff506070u,0xff708090u,0xff102030u,0xff304050u},
    {0xff102030u,0xff506070u,0xff90a0b0u,0xff304050u,0xff708090u,0xffb0c0d0u},
    {0xff90a0b0u,0xff506070u,0xff102030u,0xffb0c0d0u,0xff708090u,0xff304050u},
    {0xffb0c0d0u,0xff708090u,0xff304050u,0xff90a0b0u,0xff506070u,0xff102030u},
    {0xff304050u,0xff708090u,0xffb0c0d0u,0xff102030u,0xff506070u,0xff90a0b0u}
  };
  for(unsigned orientation=1;orientation<=8;orientation++) {
    photo_region_frame *f=NULL;
    CHECK(photo_region_open(src,9,2,3,3,orientation,&f)==PHOTO_REGION_OK);
    CHECK(photo_region_width(f)==(orientation>=5?3u:2u));
    CHECK(photo_region_height(f)==(orientation>=5?2u:3u));
    uint32_t output[6];
    CHECK(photo_region_overlay(f,output,6)==PHOTO_REGION_OK);
    CHECK(memcmp(output,expected[orientation-1],sizeof(output))==0);
    CHECK(photo_region_count(f)==1);
    CHECK(photo_region_label(f,0)==PHOTO_REGION_UNKNOWN);
    if (orientation == 1) {
      const uint64_t expected_key[4] = {
        UINT64_C(0x55748fee702ceed3), UINT64_C(0xc90f558a04b2dd0b),
        UINT64_C(0xe25aa517008372b6), UINT64_C(0x6b90f10a185349de)
      };
      for (unsigned word = 0; word < 4; ++word)
        CHECK((uint64_t)photo_region_key_word(f, word) == expected_key[word]);
      /* Pixels outside the source stride never affect annotation identity. */
      src[2] ^= 1u;
      photo_region_frame *alternate = NULL;
      CHECK(photo_region_open(src,9,2,3,3,1,&alternate)==PHOTO_REGION_OK);
      CHECK(photo_region_key_word(alternate,0)==photo_region_key_word(f,0));
      photo_region_close(&alternate);
      src[0] ^= 1u;
      CHECK(photo_region_open(src,9,2,3,3,1,&alternate)==PHOTO_REGION_OK);
      CHECK(photo_region_key_word(alternate,0)!=photo_region_key_word(f,0));
      photo_region_close(&alternate);
      src[0] ^= 1u;
    } else {
      CHECK((uint64_t)photo_region_key_word(f,0)!=UINT64_C(0x55748fee702ceed3));
    }
    photo_region_close(&f);
  }
}
static void tiles_boundaries_and_human_assertion(void) {
  uint32_t src[32*16];
  for(unsigned y=0;y<16;y++) for(unsigned x=0;x<32;x++)
    src[y*32+x]=(x<16?0xff102030u:0xffd0e0f0u);
  photo_region_frame *f=NULL;
  CHECK(photo_region_open(src,32*16,32,16,32,1,&f)==PHOTO_REGION_OK);
  CHECK(photo_region_count(f)==2);
  CHECK(photo_region_at(f,0,15)==0 && photo_region_at(f,31,0)==1);
  uint32_t output[32*16];
  CHECK(photo_region_overlay(f,output,32*16)==PHOTO_REGION_OK);
  CHECK(output[15]==src[15]);
  CHECK(output[16]==0xffff00ffu);
  CHECK(output[17]==src[17]); /* unknown is not inorganic */
  CHECK(photo_region_assert(f,1,PHOTO_REGION_HUMAN_INORGANIC)==PHOTO_REGION_OK);
  CHECK(photo_region_overlay(f,output,32*16)==PHOTO_REGION_OK);
  CHECK(output[17]!=src[17] && output[17]!=0xffff00ffu);
  CHECK(photo_region_label(f,0)==PHOTO_REGION_UNKNOWN);
  CHECK(photo_region_label(f,1)==PHOTO_REGION_HUMAN_INORGANIC);
  CHECK(photo_region_assert(f,1,PHOTO_REGION_HUMAN_ORGANIC)==PHOTO_REGION_OK);
  CHECK(photo_region_overlay(f,output,32*16)==PHOTO_REGION_OK);
  CHECK(output[17]!=src[17] && output[17]!=0xffff00ffu);
  CHECK(photo_region_assert(f,1,PHOTO_REGION_UNKNOWN)==PHOTO_REGION_OK);
  CHECK(photo_region_overlay(f,output,32*16)==PHOTO_REGION_OK);
  CHECK(output[17]==src[17]);
  CHECK(photo_region_touch(f,320,160,159,80)==0);
  CHECK(photo_region_touch(f,320,160,160,80)==1);
  CHECK(photo_region_touch(f,320,320,160,0)==-1); /* top letterbox */
  CHECK(photo_region_touch(f,320,320,160,160)==1);
  CHECK(photo_region_touch(f,0,320,0,0)==-1);
  CHECK(photo_region_touch(f,320,320,-1,160)==-1);
  photo_region_close(&f);
}
static void large_boundaries_and_repeatability(void) {
  const unsigned w=1024,h=1024;
  uint32_t *image=malloc((size_t)w*h*sizeof(*image));
  uint32_t *a=malloc((size_t)w*h*sizeof(*a));
  uint32_t *b=malloc((size_t)w*h*sizeof(*b));
  CHECK(image && a && b);
  for(unsigned i=0;i<w*h;i++) image[i]=0xff000000u | (i&255u)<<16 | ((i>>2)&255u)<<8;
  photo_region_frame *f=NULL,*g=NULL;
  CHECK(photo_region_open(image,(size_t)w*h,w,h,w,1,&f)==PHOTO_REGION_OK);
  CHECK(photo_region_count(f)<=PHOTO_MAX_COMPONENTS);
  CHECK(photo_region_overlay(f,a,(size_t)w*h)==PHOTO_REGION_OK);
  CHECK(photo_region_open(image,(size_t)w*h,w,h,w,1,&g)==PHOTO_REGION_OK);
  CHECK(photo_region_overlay(g,b,(size_t)w*h)==PHOTO_REGION_OK);
  CHECK(photo_region_count(f)==photo_region_count(g));
  CHECK(memcmp(a,b,(size_t)w*h*sizeof(*a))==0);
  photo_region_close(&f);photo_region_close(&g);
  free(image); free(a); free(b);
}
int main(void) {
  bounds_and_atomic_output();
  stride_and_orientation();
  tiles_boundaries_and_human_assertion();
  large_boundaries_and_repeatability();
  puts("PASS photo regions: input limits, stride, EXIF transforms 1..8, deterministic tiles, unknown vs human assertions, cancellation, overlay, fitCenter touch");
  return 0;
}

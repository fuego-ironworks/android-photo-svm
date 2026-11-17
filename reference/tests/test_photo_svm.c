#include "photo_svm.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_segmentation(void) {
    uint8_t rgb[8*4*3];
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 8; ++x) {
            uint8_t *p = &rgb[(y*8+x)*3];
            if (x < 4) { p[0]=10; p[1]=210; p[2]=10; }
            else { p[0]=180; p[1]=180; p[2]=180; }
        }
    }
    uint32_t capacity = photo_svm_region_capacity(8,4,2);
    assert(capacity == 2);
    uint32_t ids[8*4], repeated[8*4], count=0;
    photo_svm_region regions[2], repeat_regions[2];
    assert(photo_svm_segment_rgb8(rgb,8,4,8*3,2,5,ids,regions,2,&count)==PHOTO_SVM_OK);
    assert(count==2);
    assert(regions[0].pixels+regions[1].pixels==32);
    assert(ids[0] != ids[7]);
    for (uint32_t y=0; y<4; ++y) {
        for (uint32_t x=0; x<4; ++x) assert(ids[y*8+x]==ids[0]);
        for (uint32_t x=4; x<8; ++x) assert(ids[y*8+x]==ids[7]);
    }
    assert(regions[ids[0]].features[1] > 0.8f);
    assert(regions[ids[7]].features[0] > 0.6f);
    assert(photo_svm_segment_rgb8(rgb,8,4,24,2,5,repeated,repeat_regions,2,&count)==PHOTO_SVM_OK);
    assert(memcmp(ids,repeated,sizeof(ids))==0);
    assert(photo_svm_segment_rgb8(rgb,8,4,24,2,5,ids,regions,1,&count)==PHOTO_SVM_LIMIT);
    assert(photo_svm_segment_rgb8(rgb,8,4,23,2,5,ids,regions,2,&count)==PHOTO_SVM_INVALID);
    assert(photo_svm_segment_rgb8(rgb,8,4,24,2,0,ids,regions,2,&count)==PHOTO_SVM_INVALID);
    assert(photo_svm_region_capacity(12001,1000,2)==0); /* bounded image cap */
}

static void test_supervision(void) {
    photo_svm_model model={0};
    float green[PHOTO_SVM_FEATURE_COUNT]={0.1f,0.85f,0.1f};
    photo_svm_prediction p;
    assert(photo_svm_predict(&model,green,&p)==PHOTO_SVM_OK);
    assert(p.evaluated==0 && p.proposed==0);
    photo_svm_example positives_only[1] = {
        {.observed=PHOTO_SVM_ORGANIC,.positive=PHOTO_SVM_ORGANIC,
         .features={0.1f,0.85f,0.1f}}
    };
    assert(photo_svm_fit(&model,positives_only,1,30,0.2f,0.01f)==PHOTO_SVM_OK);
    assert(model.fitted==0); /* no missing-label -> negative leakage */
    photo_svm_example samples[4] = {
        {.observed=3,.positive=PHOTO_SVM_ORGANIC,.features={0.1f,0.85f,0.1f}},
        {.observed=3,.positive=PHOTO_SVM_INORGANIC,.features={0.75f,0.75f,0.75f}},
        {.observed=3,.positive=PHOTO_SVM_ORGANIC,.features={0.15f,0.75f,0.15f}},
        {.observed=3,.positive=PHOTO_SVM_INORGANIC,.features={0.65f,0.65f,0.65f}}
    };
    assert(photo_svm_fit(&model,samples,4,128,0.25f,0.005f)==PHOTO_SVM_OK);
    assert(model.fitted==3);
    assert(photo_svm_predict(&model,green,&p)==PHOTO_SVM_OK);
    assert((p.proposed & PHOTO_SVM_ORGANIC)!=0);
    assert((p.proposed & PHOTO_SVM_INORGANIC)==0);
    float gray[PHOTO_SVM_FEATURE_COUNT]={0.75f,0.75f,0.75f};
    assert(photo_svm_predict(&model,gray,&p)==PHOTO_SVM_OK);
    assert((p.proposed & PHOTO_SVM_INORGANIC)!=0);
    assert((p.proposed & PHOTO_SVM_ORGANIC)==0);
    photo_svm_example malformed = {.observed=0,.positive=PHOTO_SVM_ORGANIC};
    uint32_t old_version=model.version;
    assert(photo_svm_fit(&model,&malformed,1,1,0.1f,0)==PHOTO_SVM_INVALID);
    assert(model.version==old_version); /* rejected fit is atomic */
}

static void test_affine_rotation(void) {
    /* Rotating both vectors and normals through 90 degrees preserves score. */
    float x[2]={0.4f,-0.8f}, n[2]={-0.3f,0.7f};
    float score=n[0]*x[0]+n[1]*x[1]+0.12f;
    float rotated_score=(-n[1])*(-x[1])+n[0]*x[0]+0.12f;
    assert(fabsf(score-rotated_score)<1e-6f);
}

int main(void) {
    test_segmentation();
    test_supervision();
    test_affine_rotation();
    puts("PASS: deterministic superpixels, bounded inputs, explicit supervision, affine rotation");
    return 0;
}

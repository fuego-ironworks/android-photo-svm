/* JNI adapts only the contracts already supported by android-NDK PR #47:
 * copied [I input -> opaque J handle, and J + separate [I output -> I status.
 * No Java references survive a synchronous call. No application SVM or
 * shader/rotation numerical kernel is duplicated here. */
#include "photo_regions.h"
#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define SLOT_COUNT 4u
#define MAX_TRANSFER (PHOTO_PIXEL_LIMIT + PHOTO_SIDE_LIMIT*64u)

typedef struct { photo_region_frame *frame; uint32_t generation; } slot;
static slot slots[SLOT_COUNT];
static pthread_mutex_t frames_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t next_generation = 1u;

/* A generation+slot token, never a process pointer. A released/stale token
 * cannot address a later frame in the same slot. */
static slot *lookup(jlong token) {
  if (token <= 0) return NULL;
  uint64_t generation = (uint64_t)token >> 8;
  if (!generation || generation > INT_MAX) return NULL;
  unsigned index = (unsigned)((uint64_t)token & 255u);
  if (!index || index > SLOT_COUNT) return NULL;
  slot *entry=&slots[index-1u];
  if (!entry->frame || entry->generation != generation) return NULL;
  return entry;
}

JNIEXPORT jlong JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_ingest(
    JNIEnv *env, jclass owner, jint width, jint height, jint stride,
    jint orientation, jintArray input) {
  (void)owner;
  if (!input || width <= 0 || height <= 0 || stride < width ||
      width > (jint)PHOTO_SIDE_LIMIT || height > (jint)PHOTO_SIDE_LIMIT ||
      orientation < 1 || orientation > 8 ||
      (uint64_t)width * (uint64_t)height > PHOTO_PIXEL_LIMIT) return 0;
  uint64_t needed=(uint64_t)(height-1) * (uint32_t)stride + (uint32_t)width;
  if (needed > MAX_TRANSFER || needed > INT_MAX) return 0;
  jsize actual=(*env)->GetArrayLength(env,input);
  if (actual < 0 || needed > (uint64_t)actual) return 0;
  uint32_t *copy=malloc((size_t)needed * sizeof(*copy));
  if (!copy) return 0;
  (*env)->GetIntArrayRegion(env,input,0,(jsize)needed,(jint*)copy);
  if ((*env)->ExceptionCheck(env)) { free(copy); return 0; }
  photo_region_frame *frame=NULL;
  int status=photo_region_open(copy,(size_t)needed,(uint32_t)width,(uint32_t)height,
                               (size_t)stride,(unsigned)orientation,&frame);
  free(copy);
  if (status != PHOTO_REGION_OK) return 0;
  jlong token=0;
  pthread_mutex_lock(&frames_lock);
  for (unsigned i=0; i<SLOT_COUNT; ++i) {
    if (!slots[i].frame && next_generation && next_generation <= INT_MAX) {
      slots[i].frame=frame;
      slots[i].generation=next_generation++;
      token=(jlong)(((uint64_t)slots[i].generation<<8)|(uint64_t)(i+1u));
      break;
    }
  }
  pthread_mutex_unlock(&frames_lock);
  if (!token) photo_region_close(&frame);
  return token;
}

JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_overlay(
    JNIEnv *env, jclass owner, jlong handle, jintArray output) {
  (void)owner;
  if (!output) return PHOTO_REGION_ARGUMENT;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  if (!entry) { pthread_mutex_unlock(&frames_lock); return PHOTO_REGION_ARGUMENT; }
  size_t count=(size_t)photo_region_width(entry->frame)*photo_region_height(entry->frame);
  if ((*env)->GetArrayLength(env,output) < (jsize)count) {
    pthread_mutex_unlock(&frames_lock); return PHOTO_REGION_ARGUMENT;
  }
  uint32_t *private_pixels=malloc(count*sizeof(*private_pixels));
  if (!private_pixels) { pthread_mutex_unlock(&frames_lock); return PHOTO_REGION_MEMORY; }
  int result=photo_region_overlay(entry->frame,private_pixels,count);
  if (result==PHOTO_REGION_OK) {
    (*env)->SetIntArrayRegion(env,output,0,(jsize)count,(const jint*)private_pixels);
  }
  free(private_pixels);
  pthread_mutex_unlock(&frames_lock);
  /* Java exception, if any, remains pending and is propagated by ART. */
  return result;
}

JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_release(
    JNIEnv *env, jclass owner, jlong handle) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  if (!entry) { pthread_mutex_unlock(&frames_lock); return PHOTO_REGION_ARGUMENT; }
  photo_region_frame *frame=entry->frame;
  entry->frame=NULL;
  pthread_mutex_unlock(&frames_lock);
  photo_region_close(&frame);
  return PHOTO_REGION_OK;
}
JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_cancel(
    JNIEnv *env, jclass owner, jlong handle) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  int result=entry ? photo_region_cancel(entry->frame) : PHOTO_REGION_ARGUMENT;
  pthread_mutex_unlock(&frames_lock);
  return result;
}
#define SCALAR_GETTER(METHOD,EXPRESSION) \
  JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_##METHOD( \
      JNIEnv *env, jclass owner, jlong handle) { \
    (void)env; (void)owner; \
    pthread_mutex_lock(&frames_lock); \
    slot *entry=lookup(handle); \
    jint result=entry ? (jint)(EXPRESSION) : -1; \
    pthread_mutex_unlock(&frames_lock); return result; \
  }
SCALAR_GETTER(displayWidth,photo_region_width(entry->frame))
SCALAR_GETTER(displayHeight,photo_region_height(entry->frame))
SCALAR_GETTER(regionCount,photo_region_count(entry->frame))
#undef SCALAR_GETTER

JNIEXPORT jlong JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(
    JNIEnv *env, jclass owner, jlong handle, jint word) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry = lookup(handle);
  jlong result = entry && word >= 0 ? photo_region_key_word(entry->frame, (unsigned)word) : 0;
  pthread_mutex_unlock(&frames_lock);
  return result;
}

JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_regionAt(
    JNIEnv *env, jclass owner, jlong handle, jint x, jint y) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  jint result=(entry && x>=0 && y>=0) ?
     photo_region_at(entry->frame,(uint32_t)x,(uint32_t)y) : -1;
  pthread_mutex_unlock(&frames_lock); return result;
}
JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_regionAtView(
    JNIEnv *env, jclass owner, jlong handle, jint vw, jint vh, jint x, jint y) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  jint result=(entry && vw>0 && vh>0) ?
    photo_region_touch(entry->frame,(uint32_t)vw,(uint32_t)vh,x,y) : -1;
  pthread_mutex_unlock(&frames_lock); return result;
}
JNIEXPORT jint JNICALL Java_org_isomorphisms_photosvm_PhotoActivity_assertRegion(
    JNIEnv *env, jclass owner, jlong handle, jint id, jint label) {
  (void)env; (void)owner;
  pthread_mutex_lock(&frames_lock);
  slot *entry=lookup(handle);
  jint result=(entry && id>=0 && label>=0) ?
    photo_region_assert(entry->frame,(uint32_t)id,(unsigned)label) : PHOTO_REGION_ARGUMENT;
  pthread_mutex_unlock(&frames_lock); return result;
}

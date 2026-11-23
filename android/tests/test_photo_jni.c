/* Host diagnostic of the C adapter's JNI array shapes and token registry.
 * Uses fake arrays and a real JDK JNI ABI header, NOT Android ART acceptance. */
#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x); abort(); } } while(0)
typedef struct { jsize len; jint *pixels; } fake_array;
static jboolean exception_pending = JNI_FALSE;
static jsize JNICALL fake_length(JNIEnv *env, jarray array) {
  (void)env; return ((fake_array *)(void *)array)->len;
}
static void JNICALL fake_read(JNIEnv *env, jintArray array, jsize start,
                              jsize count, jint *dest) {
  (void)env;
  fake_array *a=(fake_array *)(void *)array;
  CHECK(start>=0 && count>=0 && start<=a->len && count<=a->len-start);
  memcpy(dest,a->pixels+start,(size_t)count*sizeof(jint));
}
static void JNICALL fake_write(JNIEnv *env, jintArray array, jsize start,
                               jsize count, const jint *source) {
  (void)env;
  fake_array *a=(fake_array *)(void *)array;
  CHECK(start>=0 && count>=0 && start<=a->len && count<=a->len-start);
  memcpy(a->pixels+start,source,(size_t)count*sizeof(jint));
}
static jboolean JNICALL fake_exception(JNIEnv *env) { (void)env; return exception_pending; }
extern jlong Java_org_isomorphisms_photosvm_PhotoActivity_ingest(JNIEnv*,jclass,jint,jint,jint,jint,jintArray);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_overlay(JNIEnv*,jclass,jlong,jintArray);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_release(JNIEnv*,jclass,jlong);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_cancel(JNIEnv*,jclass,jlong);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_displayWidth(JNIEnv*,jclass,jlong);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_displayHeight(JNIEnv*,jclass,jlong);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_regionAt(JNIEnv*,jclass,jlong,jint,jint);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_regionAtView(JNIEnv*,jclass,jlong,jint,jint,jint,jint);
extern jint Java_org_isomorphisms_photosvm_PhotoActivity_assertRegion(JNIEnv*,jclass,jlong,jint,jint);
extern jlong Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(JNIEnv*,jclass,jlong,jint);
int main(void) {
  static const struct JNINativeInterface_ functions = {
    .GetArrayLength=fake_length,
    .GetIntArrayRegion=fake_read,
    .SetIntArrayRegion=fake_write,
    .ExceptionCheck=fake_exception
  };
  JNIEnv local=&functions;
  JNIEnv *env=&local;
  jint rgb[32*16],out[32*16];
  for(int y=0;y<16;y++) for(int x=0;x<32;x++) rgb[y*32+x]=(jint)(x<16?0xff101010u:0xffeeeeeeu);
  fake_array src={32*16,rgb}, dest={32*16,out}, short_dest={1,out};
  jintArray input=(jintArray)(void*)&src, output=(jintArray)(void*)&dest;
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,0,16,32,1,input)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,32,16,31,1,input)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,32,16,32,9,input)==0);
  jlong h=Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,32,16,32,1,input);
  CHECK(h>0);
  jlong h_key=Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(env,NULL,h,0);
  CHECK(h_key != 0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(env,NULL,h,4)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(env,NULL,h+(INT64_C(1)<<40),0)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_displayWidth(env,NULL,h)==32);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_displayHeight(env,NULL,h)==16);
  for(int i=0;i<32*16;i++) out[i]=(jint)0xdeadbeefu;
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_overlay(env,NULL,h,(jintArray)(void*)&short_dest)==2);
  for(int i=0;i<32*16;i++) CHECK((uint32_t)out[i]==0xdeadbeefu);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_overlay(env,NULL,h,output)==0);
  CHECK((uint32_t)out[0]==0xff101010u);
  CHECK((uint32_t)out[16]==0xffff00ffu);
  CHECK((uint32_t)out[17]==0xffeeeeeeu); /* unknown != inorganic */
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_regionAt(env,NULL,h,17,0)==1);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_regionAtView(env,NULL,h,320,320,0,0)==-1);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_assertRegion(env,NULL,h,1,2)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_overlay(env,NULL,h,output)==0);
  CHECK((uint32_t)out[17]!=0xffeeeeeeu);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_cancel(env,NULL,h)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(env,NULL,h,0)==0);
  for(int i=0;i<32*16;i++) out[i]=(jint)0xdeadbeefu;
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_overlay(env,NULL,h,output)==5);
  for(int i=0;i<32*16;i++) CHECK((uint32_t)out[i]==0xdeadbeefu);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_release(env,NULL,h)==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_release(env,NULL,h)==2);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_regionAt(env,NULL,h,0,0)==-1);
  jlong held[4];
  for(int i=0;i<4;i++) {
    held[i]=Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,32,16,32,1,input);
    CHECK(held[i]>0 && held[i]!=h);
    CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_imageKey(env,NULL,held[i],0)==h_key);
  }
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_ingest(env,NULL,32,16,32,1,input)==0);
  for(int i=0;i<4;i++) CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_release(env,NULL,held[i])==0);
  CHECK(Java_org_isomorphisms_photosvm_PhotoActivity_overlay(env,NULL,h,output)==2);
  puts("PASS JNI diagnostic: copied input, atomic output, human label, limits, cancellation, stale and exhausted tokens");
  return 0;
}

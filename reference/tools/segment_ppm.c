/* Host-only smoke tool. Accepts binary P6 PPM, not JPEG/PNG; Android adapter
 * will supply decoded pixel buffers directly to the library. */
#include "photo_svm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int next_token(FILE *f, char *out, size_t size) {
    int ch;
    do {
        ch = fgetc(f);
        if (ch == '#') { do { ch = fgetc(f); } while (ch != EOF && ch != '\n'); }
    } while (ch != EOF && isspace((unsigned char)ch));
    if (ch == EOF) return -1;
    size_t i = 0;
    do {
        if (i + 1 >= size) return -1;
        out[i++] = (char)ch;
        ch = fgetc(f);
    } while (ch != EOF && !isspace((unsigned char)ch));
    out[i] = '\0';
    return 0;
}
static int number(const char *s, unsigned *result) {
    char *end;
    unsigned long n = strtoul(s, &end, 10);
    if (*s == '\0' || *end || n > 0xffffffffu) return -1;
    *result = (unsigned)n;
    return 0;
}
int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "usage: segment_ppm input.ppm overlay.ppm [desired-regions]\n");
        return 2;
    }
    unsigned desired=128, width=0, height=0, max=0;
    if (argc == 4 && (number(argv[3], &desired) || desired == 0)) return 2;
    FILE *in=fopen(argv[1],"rb");
    if (!in) return 2;
    char token[64];
    if (next_token(in,token,sizeof(token)) || strcmp(token,"P6") ||
        next_token(in,token,sizeof(token)) || number(token,&width) ||
        next_token(in,token,sizeof(token)) || number(token,&height) ||
        next_token(in,token,sizeof(token)) || number(token,&max) || max!=255) {
        fprintf(stderr,"unsupported PPM (requires binary P6, 255 max)\n");
        fclose(in); return 2;
    }
    uint32_t capacity=photo_svm_region_capacity(width,height,desired);
    if (!capacity) { fclose(in); return 2; }
    size_t pixels=(size_t)width*height;
    uint8_t *rgb=(uint8_t*)malloc(pixels*3u);
    uint32_t *ids=(uint32_t*)malloc(pixels*sizeof(*ids));
    photo_svm_region *regions=(photo_svm_region*)malloc(capacity*sizeof(*regions));
    if (!rgb || !ids || !regions || fread(rgb,3u,pixels,in)!=pixels) {
        fprintf(stderr,"input truncated or allocation failure\n");
        free(rgb); free(ids); free(regions); fclose(in); return 2;
    }
    fclose(in);
    uint32_t count=0;
    if (photo_svm_segment_rgb8(rgb,width,height,(size_t)width*3u,
                              desired,5,ids,regions,capacity,&count)!=PHOTO_SVM_OK) {
        free(rgb); free(ids); free(regions); return 2;
    }
    FILE *out=fopen(argv[2],"wb");
    if (!out) { free(rgb); free(ids); free(regions); return 2; }
    fprintf(out,"P6\n%u %u\n255\n",width,height);
    for (unsigned y=0; y<height; ++y) {
        for (unsigned x=0; x<width; ++x) {
            size_t p=(size_t)y*width+x;
            uint8_t pixel[3]={rgb[3u*p],rgb[3u*p+1u],rgb[3u*p+2u]};
            if ((x>0 && ids[p]!=ids[p-1u]) ||
                (y>0 && ids[p]!=ids[p-width])) {
                pixel[0]=255; pixel[1]=0; pixel[2]=255;
            }
            if (fwrite(pixel,1,3,out)!=3) {
                fclose(out); free(rgb); free(ids); free(regions); return 2;
            }
        }
    }
    if (fclose(out)) { free(rgb); free(ids); free(regions); return 2; }
    fprintf(stderr,"segmented %ux%u pixels into %u color/spatial regions; NOT semantic labels\n",
            width,height,count);
    free(rgb); free(ids); free(regions);
    return 0;
}

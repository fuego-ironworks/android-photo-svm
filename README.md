# Android Photo SVM

Independent native photo segmentation and organic/inorganic region classification.
This repository owns the reusable library; IB integration is optional.

## Baseline (not a trained vision system)

The first slice implements deterministic RGB8 color/spatial superpixel
partitioning and two independently trained affine hinge-loss classifiers in a
small, portable C ABI. It does **not** yet infer meaningful organic/inorganic
semantics from arbitrary photos: there is no fitted visual encoder, accepted
real-photo dataset, DEX UI or shipping Android APK.

```sh
make CC=clang test demo-test
make CC=clang sanitize
make CC=clang build/segment_ppm
./build/segment_ppm input.ppm overlay.ppm 128
```

`segment_ppm` accepts binary P6 PPM images and paints magenta region edges.
It does not produce organic/inorganic labels. Applications must supply decoded
RGB buffers (e.g. from an Android image decoder) and carefully scoped,
explicit positive/negative training assertions. Unfitted classifiers abstain.

API: [`include/photo_svm.h`](include/photo_svm.h). Representation, ownership,
limits, and phased acceptance: [`docs/architecture.md`](docs/architecture.md).

The planned app uses **checked Idriç→DEX** and an **ICK-built native C library**
through the generic `isomorphisms/android-NDK` JNI adapter. Clang is only a host
smoke-check compiler, not a replacement for ICK. Validation on an actual MIRO
C67 and semantic mask quality on held-out photographs remain **NOT_RUN**.

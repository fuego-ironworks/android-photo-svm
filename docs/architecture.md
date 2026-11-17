# Photo SVM — boundary and acceptance

## Ownership

`android-photo-svm` owns reusable image-region segmentation, region features,
explicitly supervised photo/category classifiers, and a demo/test interface.
It does **not** depend on IB/Pensieve. Android-NDK owns generic DEX/JNI and APK
machinery; Idriç owns checked application semantics; ICK owns native lowering.

## Objects and classification

Image (RGB8 pixels, dimensions, stride) → SLIC-like color/spatial region IDs
(image-space coordinates) → feature vectors → independently learned affine
separators → positive/negative/unknown *proposals* → explicitly attributed
human corrections. A mask's pixel indices are not embedding dimensions.

The current baseline computes eight simple features per region. It is useful for
exercising a real inference interface but cannot distinguish living matter from
manufactured materials reliably on unconstrained photographs. No pretrained
vision encoder, semantic pixel mask, image dataset, or human-labeled corpus has
been included. An RGB-only model can exploit spurious color correlations.

Classification uses two independent planes, `normal·features + offset`, with a
separately stored threshold per category. `organic` and `inorganic` are bits, not
an exclusive argmax. Both, neither, or a partially evaluated classification are
valid. `fitted=0` is **unknown**, not inorganic. Training requires explicit
positive and explicit negative examples for each fitted category; unlabeled
records contribute to neither sign.

An orthogonal change of feature coordinates must rotate the normal by the same
transform to preserve affine scores. An orientation/coordinate transform does
not itself infer the correct classes. Future oblique CART nodes or voted SVM
ensembles would build on the affine primitives, not replace pixel segmentation.

## Memory / platform

The C ABI accepts predecoded RGB8 and caller-owned output arrays. Current bound:
12 million pixels, <=4096 requested regions, <=16 refinement iterations.
Scratch allocation is O(pixels + regions), including float32 pixel distances
and region IDs. Camera-size images above this bound require explicit tiling or
resizing: no hidden full-resolution assumption. The `segment_ppm` host utility
is deliberately limited to P6; JPEG/PNG decoding belongs to a platform adapter.

Android's checked Activity is intended to be compiled Idriç→DEX. A narrow
JNI interface would transfer pixel arrays to an ICK-built C implementation.
Neither an APK nor native ARM qualification is claimed by this first slice.
The `clang` smoke build is a portability/conformance test, not an ICK fallback.

## Progress gates

1. Native deterministic region + classifier API, host tests, sanitizers.
2. Real-photo fixture corpus with source identities, privacy-safe labelled
   regions, group-disjoint train/dev/held-out splits, measured region and mask
   scores (IoU, false positives, abstention, latency, RAM).
3. Source-pinned ICK ARM32/ARM64 build, JNI transport controls and checked DEX
   lifecycle around photo pick, orientation, view/overlay, corrections.
4. Installed API23/29/34/35 acceptance plus physical C67 measurements.

A test that segments two synthetic colors is evidence of spatial partitioning,
**not** proof of organic/inorganic recognition. Human corrections must be durable,
versioned, and never silently turned into general negative evidence.

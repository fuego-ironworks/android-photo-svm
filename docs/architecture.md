# Photo SVM — interface over the existing mathematics

## Architecture correction — 2026-11-17

The user's requirement is **functorial Icky C with modified iLua**, exposing the
existing rotation/hyperplane machinery to photographs. This repository is a
consumer and interface, not a second numerical-mathematics implementation.
This correction supersedes the earlier generic-C/Idric-application ownership
plan. Icky C is the source language/style; ICK is its compiler. Merely promising
to compile ordinary C with ICK later does not establish this implementation.

## Ownership

- Shared mathematical owners retain vector/covector spaces, metrics, rotations,
  reflections, transform composition, projections and classifier mathematics.
  Reuse their implementations and tests. Expose a missing callable boundary in
  its owner rather than copying the algorithm into this application.
- Functorial Icky C supplies a small photo-facing native interface: validated
  image/feature buffers, lifetimes, named compositions of shared operations,
  and the mapping of results back to photographs or selected regions.
- Modified iLua owns application composition: photo/region selection, commands,
  label/correction workflow, view selection and presentation requests. It calls
  the native interface; it does not contain per-pixel loops or another SVM.
- The existing Android-NDK Idric-to-DEX/JNI path is the Android platform adapter
  for Activity lifecycle, image access and native/view transport. It is not a
  reason to move the application's policy out of iLua or duplicate its state.
- IB/Pensieve is not a dependency. The native interface remains usable on Linux.

The intended composition, not an assertion of already exported function names:

```text
photo or selected region
  -> declared visual representation
  -> shared mathematical/classifier operations
  -> attributed category proposals
  -> iLua workflow and Android presentation
```

Whole-photo classification does not require pixel segmentation. Region/mask
support is a separate image-space adapter when needed. Image decoding, feature
representation and translating region results to pixels are photo-specific;
rotation and affine-plane arithmetic are not.

## Reuse map inspected on 2026-11-17

1. `isomorphisms/Idric`,
   `_/examples/unified-higher-mathematics/EuclideanGeometry.idric`, inspected blob
   `aa1d8fc41f173eabce8be21b27f5f530237c9216`: named Euclidean structures,
   `OrthogonalTransform`, `compose_orthogonal`, `two_reflections`, and
   `apply_orthogonal_exact`. This is an exact-sample semantic layer with a
   bounded family of generators, not a demonstrated arbitrary-dimensional
   floating-point C ABI.
2. `fuego-ironworks/idris-shader-backend`, retained revision
   `bdf7e48d0161965269354b0aa26a2befe8e16885`:
   `src/Example/RotateDifference8ToE1.idr` constructs an 8D proper alignment
   using two reflections; `src/Example/GivensRotate2ToE1.idr` separately exercises
   a genuine plane rotation. These are existing numerical source/probes to
   reuse or expose through their owner, not proof of a general photo backend.
   PR #31 is closed without merge; do not silently describe it as main.
3. `walnut-burgundy/coxeter` issue #5 and
   `walnut-burgundy/computer-science` issues #53/#54 retain the alignment and
   algorithm-selection requirements. Express the semantic operation and its
   invariants; do not force Householder, Givens, a dense matrix or a GPU at the
   application boundary merely because one implementation exists.
4. `dilapidated-shed/icurses` PR #5 at
   `e2c2df710955c99cb2ba3b9b0384a2b8b489e9dd` is the concrete Icky C/iLua ownership
   precedent. It pins modified `isomorphisms/lua` at
   `87306483cec50f8c750a22dda1d0742246fad756` and demonstrates Lua-owned interaction
   over native operations. Its shared-runtime admission blocker and physical
   C67 NOT_RUN status remain separate from those consumer observations.
5. `isomorphisms/android-NDK` owns generic DEX/JNI and APK machinery. Reuse the
   canonical implementation rather than creating another compiler or runtime.

The callable shared affine-learning interface and complete production
Icky/iLua/Android linkage are NOT_VERIFIED here. Missing exports are integration
work, not permission for an unnoticed local substitute. Refresh owner revisions
and runtime admission before selecting production dependencies.

## Mathematical and application boundaries

Preserve named spaces, dimensions, metric, orientation, composition order and
precision through the interface; do not flatten everything to unrelated float
arrays or impose the provisional eight-feature representation as the contract.

An affine classifier can be expressed as a shared projection/side-of-plane
operation. An explicit change of basis is one realization, not a compulsory
extra pass over every feature vector. Existing planning/lowering owns that
choice. A future oblique tree composes shared decisions; it does not introduce
another implementation of rotation or SVM arithmetic at each node.

Keep fitted model parameters, proposals and explicit human assertions distinct.
Unknown is not inorganic, unlabeled is not negative, and a mixed photograph may
support more than one category. Raw scores are not calibrated probabilities.
No rotation test alone establishes useful organic/inorganic photo labels.

## Current draft is a reference baseline, not the accepted architecture

At `b2c717523f5845b58c4a535d945ad8c994006bbb`, `src/photo_svm.c` implements local
RGB8 color/spatial superpixels, eight appearance features and its own affine
hinge-loss fit/predict loops. There is no iLua application or shared-geometry
call path. Preserve useful fixtures, but do not expand these local loops into
a parallel mathematics stack or present the draft as compliant integration.
The code has not yet been moved or rewired by this documentation correction.

Its existing C ABI accepts predecoded RGB8 and caller-owned output arrays,
with limits of 12 million pixels, 4096 requested regions and 16 iterations.
Those are baseline limits, not the new interface specification. The P6 tool is
a region-boundary demonstration, not JPEG/PNG or semantic recognition.
Historical Clang smoke/sanitizer tests do not qualify literal Icky C, ICK,
modified iLua, Android execution or real-photo accuracy. There is no production
GCC/Clang or stock-Lua fallback.

## Acceptance for the correction

1. Name and pin the actual shared implementations used, including their
   callable interfaces. Keep any necessary new general operation in its owner.
2. Demonstrate the call path from functorial Icky C through the shared backend;
   substitution/removal must fail the intended test rather than silently use
   the reference loops. Preserve composition and projection fixtures.
3. Qualify literal Icky source through ICK and the actual modified iLua runtime.
   Show that an iLua interaction-policy change changes behavior without
   rebuilding or modifying the numerical kernel.
4. Connect the existing DEX/JNI adapter and observe real selected image pixels,
   aligned results, bounded buffers, cancellation and lifecycle behavior.
5. Evaluate organic/inorganic proposals on explicitly labelled, source-group-
   disjoint real photographs. Keep photo quality, emulator execution and
   physical C67 memory/latency evidence separate.

PR #1 remains a draft until its architectural correction and relevant execution
are demonstrated. This document is not a claim that the integration is done.

# Ownership and current implementation boundary

Functorial Icky C with modified iLua sits above shared rotation and hyperplane mathematics. The initial independent ordinary-C classifier is retained only under `reference/`; neither the production library nor the iLua executable links its arithmetic or training loops.

## Current source

- `src/photo_math.c` owns validation, shared-program selection, bounded transport, execution, signed Float32 readback and resources. It does not implement projection, Householder/Givens arithmetic, an SVM trainer or a visual encoder.
- `include/photo_math.h` names representation, space, basis, metric, dimension and vector/covector variance. Matching dimensions alone do not authorize composition. Outputs remain unchanged on failure.
- `host/ilua_main.c` is a host-only lifecycle adapter and narrow binding. Coordinate userdata owns copied values. Exported component tables are copies. Lua's number representation is transport, not a second numerical implementation. The scripts are trusted; this is not a security sandbox.
- `app/geometry.ilua` owns ordered composition/inversion of shared operations. `compose(first, second)` applies first then second; the inverse applies the inverse operations in reverse order.
- `app/classify.ilua` owns model/proposal/assertion distinctions and thresholds. Unknown is not inorganic. Explicit corrections do not mutate old proposals. No durable event store or training update is implemented.

Whole-photo classification does not require segmentation. A visual representation, source-photo mapping, any region masks and human-labelled evaluation are still photo-specific work. Label meanings must come from the user's examples, not colour heuristics silently promoted to organic/inorganic truth.

## Reused mathematics

The draft depends on `fuego-ironworks/idris-shader-backend` PR #75, exact pin in `ci/dependencies.tsv`. The build invokes its registered checked shader compiler to produce a bounded operation bundle. Generated GLSL is not handwritten, copied into this repository or changed after emission. The candidate has moved away from the old retained `bdf7e48...` fragment snapshot; that historical boundary is preserved in this document's git history.

Three distinct operations are exposed:

1. Current upstream typed 32D vector/covector coordinate pairing, rendered into an unclamped signed Float32 framebuffer. A declared homogeneous coordinate can carry a bias; it is not inserted implicitly.
2. The unchanged legacy 8D alignment diagnostic, explicitly named `inspect_alignment8`. Its thresholded algorithm remains a diagnostic, not the new reusable transformation.
3. The new shared eight-dimensional two-reflection operation, returning all coordinates. Both nonzero normals are covectors in one standard Euclidean space. The result is an active SO(8) map in the same named basis. Reversing reflection order gives the inverse. Affine offsets, arbitrary metrics and implicit coordinate conversions are outside this operation.

The general mathematical lineage remains Idric's `_/examples/unified-higher-mathematics/EuclideanGeometry.idric`, including `compose_orthogonal`, `two_reflections` and `apply_orthogonal_exact`. The checked numerical shader module is a bounded lowering of that construction, not a claim that the exact semantic module is linked as a native float ABI. Givens and other planner/provider implementations keep their owners and are not claimed as linked here.

## Execution and ownership

The caller supplies a dedicated current GLES3 context and keeps context/display ownership. The library owns programs, framebuffer, texture and vertex array. All calls/destruction use the owning current context. Fixed-size readback resets pixel-pack state, checks all pages and publishes only a complete result. Missing kernels, invalid input, unsupported floating output and nonfinite results fail without local fallback. GLES is an explicit initial provider, not the definition of the mathematics.

The `dilapidated-shed/icurses` Icky/iLua boundary remains the precedent. Modified `isomorphisms/lua` is pinned at `87306483cec50f8c750a22dda1d0742246fad756`. Literal Icky source must be compiled by the owned ICK frontend; stock C/Lua is not a replacement. The inherited Clang reference job qualifies only preserved reference code.

Canonical DEX/JNI, Android lifecycle and packaging belong to `isomorphisms/android-NDK`. This host adapter is not a second Android runtime. Issue #2 retains the mobile acceptance work.

## Acceptance and limits

The workflow must generate the checked shared operations, validate their GLSL, execute native signed-pairing/diagnostic/full-rotation tests, reject corrupted/missing programs and preserve output on a later-page failure. Only then may it build and execute modified iLua, the symbolic-syntax suite, noncommuting composition/inverse tests and policy-only changes with unchanged native hashes. Source/dependency/compiler/generated-program and link receipts are retained.

Test source is not an execution result. New qualification remains unverified until the exact-head workflow completes. These tests concern synthetic coordinates and hosted software GLES, not organic/inorganic accuracy, visual encoding, masks, Android API compatibility, hardware-driver portability, C67 latency/memory, signing or release readiness. PR #1 stays draft. See `docs/rotation-interface.md` for the bounded API contract and negative controls.

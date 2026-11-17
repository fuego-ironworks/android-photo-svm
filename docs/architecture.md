# Ownership and implemented boundary

The user requires functorial Icky C with modified iLua above shared rotation and
hyperplane mathematics. This supersedes the initial independent C classifier.

## Current code

- `src/photo_math.c`: named compositions of validation, shared-program
  selection, bounded uniform transport, execution and signed Float32 readback.
  It does not implement projection/rotation arithmetic or train an SVM.
- `include/photo_math.h`: representation, named space, basis, metric, variance
  and explicit Float32 coordinates. Rank equality alone is not composability.
  Projection accepts a vector and matching covector; alignment requires two
  vectors and the declared standard Euclidean metric. Invalid requests leave
  caller outputs unchanged.
- `host/ilua_main.c`: the narrow native binding and a host-only lifecycle adapter.
  iLua coordinate handles own copied, bounded coordinates. Their contents are
  not exposed for mutation. Lua's number type is transport; the selected native
  representation is explicitly Float32. Other scalar profiles are refused.
- `app/classify.ilua`: model/proposal/assertion distinctions and threshold policy.
  Unknown is not inorganic. An explicit correction does not mutate a prior
  proposal. This slice has no durable event store or training update rule.
- `reference/`: the unchanged prior color/spatial partition, eight-feature
  extractor and local fit/predict loops. Tests remain available, but neither
  the library nor the iLua executable links this baseline. It is not a fallback.

Whole-photo classification does not require pixel segmentation. Feature-to-
pixel mapping, an actual visual representation and photo evaluation remain
photo-specific work; generic missing mathematical exports belong upstream.

## Actual reused source

The selected provider consumes these unchanged generated files from
`fuego-ironworks/idris-shader-backend` at
`bdf7e48d0161965269354b0aa26a2befe8e16885`:

- `generated/fullscreen.vert`, blob `a82ed3e72b842171a731d9c203f73a6989757c87`;
- `generated/dot-vector32-covector32.frag`, blob
  `5b891e5c67dcf49f9bf2d6b523208be0c30552d0`;
- `generated/rotate-difference8-to-e1.frag`, blob
  `49af343de226e7e1276a8b1d7110fc07d39698b3`.

This is a retained experimental revision (PR #31 closed without merging), not
an assertion that the feature is upstream main or a stable general library ABI.
The build verifies exact dependency commits, clean tracked source and these
blob identities, and records SHA-256 receipts. It does not rerun the Idriç
shader generator. Shader code is not copied or reimplemented in this repo.

The 8D source constructs an orientation-preserving alignment with two
reflections, but exports only diagnostics and uses an absolute 1e-6 threshold.
The interface faithfully exposes **inspect_alignment8**, not `rotate`, a general
matrix, or a proof of accuracy for degenerate/tiny/overflowing inputs. The 32D
contraction preserves negative and above-one scores via an RGBA32F framebuffer.
Nonfinite results and unavailable float capabilities are errors.

The caller supplies a **dedicated current GLES3 context**. The library owns only
its programs, framebuffer, texture and vertex array, not the context/display.
Calls are synchronous on the owning current context; wrong-context operations
and destruction are refused. The host adapter is not a replacement Android
Activity or a parallel DEX runtime. Mobile adaptation remains in issue #2.
GLES is an explicitly selected initial provider, not part of the mathematical
meaning or a requirement that future providers use the same factorization.

## Shared ownership still to connect

- `isomorphisms/Idric`,
  `_/examples/unified-higher-mathematics/EuclideanGeometry.idric`, inspected blob
  `aa1d8fc41f173eabce8be21b27f5f530237c9216`, supplies named spaces, exact-sample
  transforms, `compose_orthogonal`, `two_reflections`, `apply_orthogonal_exact`.
  It is not secretly reimplemented here or falsely claimed as a linked float ABI.
- The separate retained `GivensRotate2ToE1.idr` and Coxeter/computer-science
  alignment/planning work retain their owners. No Givens or general composition
  execution claim is made by this slice.
- `dilapidated-shed/icurses` PR #5 at
  `e2c2df710955c99cb2ba3b9b0384a2b8b489e9dd` supplies the Icky/iLua ownership and
  build precedent. Modified `isomorphisms/lua` is pinned at
  `87306483cec50f8c750a22dda1d0742246fad756`; runtime admission beyond this
  consumer's measured linkage is a separate obligation.
- `isomorphisms/android-NDK` owns canonical DEX/JNI, lifecycle and packaging.
  No Android implementation or physical C67 qualification is included here.

## Acceptance and limits

The workflow compiles **literal** Icky C through the pinned owned ICK frontend.
It first tests signed projection, bias-as-declared-homogeneous-coordinate,
alignment diagnostics, dimension/variance/basis/nonfinite/context refusals and
resource cleanup. A deliberately sign-reversed shared shader must fail the
actual score assertion; deleting a shared shader must prevent opening.

Only after that C checkpoint does it build the actual modified Lua runtime,
execute the upstream symbolic-syntax suite both standalone and embedded, and
run policy/opaque-handle tests. Editing only the iLua threshold must change the
proposal while hashes of the native executable/library stay unchanged. Link
receipts exclude dynamic stock Lua and the old local classifier.

These are synthetic-coordinate and hosted software-GLES tests, not training,
encoder quality, image masks, Android API compatibility, driver portability,
phone latency, memory pressure, or a release. Keep PR #1 draft. Future native
lowering, proper arbitrary transformation exports, affine fitting and tree
composition must retain upstream mathematical ownership and separate evidence.

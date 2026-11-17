# Android Photo SVM

A photo-facing **functorial Icky C interface with modified iLua policy**, consuming
shared mathematics. IB is not a dependency. The old independent classifier is
preserved under `reference/` and is not linked into the new library.

## Executable integration slice, not a finished photo classifier

`src/photo_math.c` binds the **unchanged, pinned generated shaders** owned by
`fuego-ironworks/idris-shader-backend`:

- a signed Float32 vector/covector contraction in dimension 32;
- the existing 8D difference-alignment diagnostic (axis ratio and residual).

There are no local dot-product, Householder, Givens or SVM-training loops in the
new library. The alignment diagnostic does **not** return a reusable rotation,
a matrix, transformed image coordinates, or a segmentation mask.

`app/classify.ilua` supplies policy over native operations: proposal threshold,
model provenance, unknown state and explicit human-assertion priority. This is
not a fitted model, durable correction store or calibrated probability system.
A homogeneous representation can carry an affine offset through the same
shared contraction, but that representation must be declared by its caller.

This first backend explicitly selects GLES3. It requires signed Float32 render
and readback support; it refuses missing kernels/context/capabilities rather
than substituting the reference classifier. It is not a decision that all photo
processing must use a GPU. No arbitrary-dimensional backend is claimed.

## Qualification

Check out the exact dependencies listed in `ci/dependencies.tsv`, then run:

```sh
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 \
  make test ICK=/absolute/path/to/qualified/ick \
  LUA_SOURCE=/path/to/modified-lua SHADER_SOURCE=/path/to/shared-shaders
```

The default build requires literal Icky source and the pinned modified iLua;
there is no GCC/Clang or stock-Lua fallback. Hosted acceptance first exercises
the C library, then builds/runs iLua, changes only the policy, verifies unchanged
native binary hashes, and rejects a substituted or missing shared kernel.

Tests use synthetic coordinates. **JPEG/PNG encoding, useful learned visual
features, fitted organic/inorganic models, masks, DEX/JNI, Android execution and
physical MIRO C67 testing remain unqualified.** Do not turn a hosted integration
PASS into a photo-quality or device claim. See `docs/architecture.md`.

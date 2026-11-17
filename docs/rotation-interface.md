# Shared rotations, not an application-local kernel

## Ownership and provenance

The source operation lives in `fuego-ironworks/idris-shader-backend` PR #75, pinned by `ci/dependencies.tsv`. `Shader.Orthogonal8` parameterizes the Householder operation from the existing alignment work; two ordered reflections implement an orientation-preserving Euclidean map. It follows Idric's `two_reflections`/`compose_orthogonal` semantics without claiming the exact symbolic module is linked. The unchanged legacy alignment diagnostic is still explicitly a diagnostic.

The build now compiles the owning checked source using the pinned Idric compiler API. It also uses upstream main's typed 32D coordinate pairing instead of the old retained untyped fragment. The shader compiler, source revision and source/generated hashes are retained. No generated GLSL is handwritten or patched in the candidate path. Mutated copies exist only in negative tests.

## API

`photo_rotate8(engine, first_normal, second_normal, argument, inverse, output)` evaluates `H_second(H_first(argument))`; `inverse=1` reverses those operations. Both normals are nonzero covectors with zero affine offsets. The argument may be a vector or a covector, in the same named representation, eight-dimensional space, ordered basis and standard Euclidean metric. A rotated covector is represented by `R n`, so rotating both sides preserves the coordinate pairing. Arbitrary metrics and silent basis conversion are refused.

The output keeps the argument's identity metadata and variance. Three signed Float32 pages carry every coordinate, with a validity lane checked on every page. Only a completely successful readback publishes the candidate; late failure leaves output untouched. Input/output aliasing is allowed. Dedicated-context pixel-pack and framebuffer state are set explicitly before fixed-size readback.

`app/geometry.ilua` exposes `two_reflections`, `apply`, `inverse` and `compose`. `compose(first, second)` applies first, then second; inverse composition reverses that order. Lua owns composition, never dot products or reflector arithmetic. Returned component tables are copies, not mutable native storage. This trusted scripting interface is not a Lua security sandbox.

## Tests and boundary

Native tests specify exact quarter-turn output including all untouched tail coordinates, equal-normal identity, inverse round-trip, normal sign/scaling invariance, norm and vector/covector pairing preservation, Gram identity and determinant +1. They also cover wrong basis/variance, nonfinite and zero normals, wrong context, aliases and poisoned pixel-pack state. Test-only scalar oracles are not linked into the library.

Modified-iLua tests exercise noncommuting rotations in two planes, inverse order, covector transport, immutable value copies, garbage collection and invalid plans. Negative controls change the actual shared score/rotation, invalidate a later output page and remove the rotation shader. They must either fail the expected numeric assertion or produce an explicitly verified refusal, not succeed with local math.

Source and tests are committed but runtime results require exact-head receipts. The provider is currently eight-dimensional standard Euclidean GLES; general-dimensional operations, other providers, affine fitting, visual encoders, durable corrections and phone integration remain separate work. Float32 overflow is a refusal, and denormal handling is hardware-dependent. No photo-quality result follows from these synthetic-coordinate tests.

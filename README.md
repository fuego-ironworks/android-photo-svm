# Android Photo SVM

A thin photo-oriented interface above shared mathematics: **modified iLua composition and policy → functorial Icky C validation/transport → checked shared operations**. No browser dependency, local SVM trainer, or duplicate application rotation kernel.

The current draft consumes `fuego-ironworks/idris-shader-backend` PR #75 at `d8781df201e66561d63dcf04bfb1af6d0684fc95`. The build compiles its checked source rather than importing the old abandoned generated-shader snapshot. Signed 32D coordinate pairing, the retained 8D alignment diagnostic, and new full 8D two-reflection rotations are separate operations. This is a draft dependency, not accepted upstream main.

`app/geometry.ilua` creates rotation plans from two reflecting hyperplane normals, composes plans in an explicit order and constructs their inverses. The numerical operations remain upstream. The Icky adapter only checks named spaces, transports Float32 values, executes the selected provider and owns resources. Coordinates retain vector/covector roles. This is an active rotation within one declared Euclidean basis, not an implicit change of basis or affine reflection.

`app/classify.ilua` owns simple threshold/proposal/correction policy. `app/decisions.ilua` adds bounded oblique decision trees over the existing 32D pairing, per-category abstention, source-bound proposals and exact-target correction priority. It is inference routing for supplied planes, not a CART/SVM trainer. The 8D rotation and 32D feature representations are not silently interchangeable.

Whole-photo classification does not require pixel segmentation. No semantic encoder, trained organic/inorganic quality, real photo masks, durable correction store, Android Activity, physical C67 result or APK is established.

## Qualification

Use the pinned owned ICK frontend and modified iLua in `ci/dependencies.tsv`. First bootstrap the pinned Idric compiler API and generate the shared bundle with `make -C .shader-source -f tools/photo-math.mk generate`, then run `make test ICK=/path/to/owned/ick`.

The workflow requires C execution before embedded iLua, full-coordinate/inverse/order/isometry tests, shader corruption and missing-shader refusals, failure-atomic readback, exact source/generated hashes and policy-only edits with unchanged native hashes. It also executes the oblique decision and scoped-assertion suite using the actual native pairing. Added tests are not PASS evidence until they execute at the exact head. The superseded ordinary-C implementation remains under `reference/`, excluded from this path.

See [architecture](docs/architecture.md), [rotation interface](docs/rotation-interface.md) and [decision interface](docs/decision-interface.md). Keep the PR draft until independent source, runtime and phone evidence supports promotion.

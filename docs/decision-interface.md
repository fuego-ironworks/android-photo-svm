# Oblique decision interface

`app/decisions.ilua` adds bounded decision-tree routing over the existing shared
32D vector/covector pairing. It adds no dot product, feature encoder, optimizer,
reflection kernel or alternative mathematical backend.

This is the inference/composition part of the proposed "CART with SVMs inside"
architecture, not a CART or SVM trainer. Callers supply explicit plane parameters
with model identity and revision. A node compares the shared pairing to its own
finite threshold; `score >= threshold` follows `yes`. A nonzero threshold describes
an affine boundary without requiring local numerical kernels. More complicated
boundaries come from routing, not from pretending a single linear score is nonlinear.

## API

`decisions.compile(specification)` snapshots a plain declarative tree. A model has
`id`, `revision`, `category` and `root`. Every node has a unique `id` and one kind:

- `plane`: native `covector`, finite `threshold`, `yes` node and `no` node.
- `leaf`: an explicit Boolean `included` value.
- `unknown`: a nonempty `reason`, never an implicit negative label.

A tree is limited to 127 nodes and 32 levels. Cycles, shared node tables, duplicate
IDs, unknown fields, nonfinite thresholds and metatables are rejected. The compiled
handle privately retains a snapshot and native coordinate handles. Altering the
input specification or a returned trace does not alter future decisions.

`decisions.propose(entry, category, plan, assertion)` returns a new record, without
changing the input, a previous proposal, a file or a canonical label. `entry` has:

- `target`: lowercase `image_sha256`, `region_id`, `region_revision`;
- `feature`: an immutable native coordinate handle, when available;
- `feature_revision`: the caller's feature/encoder revision, when available.

The caller must compute and verify the image digest and bind the region revision to
its exact mask/selection. This interface validates identity syntax and equality;
it does not inspect pixels, compute hashes or prove provenance. Whole-image callers
may use an explicitly declared whole-image region ID/revision.

Each successful proposal records category, target, feature/model/mathematics
revisions, the reached leaf and the ordered route with raw scores and thresholds.
Scores are not probabilities. A missing model, missing feature or explicit unknown
leaf abstains. Native failures propagate as errors, not unknowns or negative votes;
no partial route is published and earlier results remain unchanged.

Independent per-category plans permit overlapping organic and inorganic proposals.
There is no global winner or inference that "not organic" means "inorganic".
An explicit `human-assertion` must carry the exact target, category, event ID and
Boolean inclusion. It takes priority without requiring an available model/feature.
An assertion for another photo, region revision or category is rejected. This is a
scoped decision interface, not an authenticated user-input channel or durable store.

## Representation limits

Structural compilation does not numerically validate an SVM. The existing native
bridge validates dimensions, vector/covector roles and matching named spaces/bases
at every executed plane. An invalid unvisited plane is not numerically qualified by
compilation. Wrong representations on a later executed plane fail the entire call.

The available rotation operation remains 8D and pairing remains 32D. This interface
does not pad, truncate or silently pretend those are the same representation. The
integration suite explicitly rejects an 8D argument at a 32D decision. A coherent
encoder/transform contract must precede any real-photo quality claim.

## Acceptance

`tests/decisions.ilua` is executed by the real modified iLua host in `make test`.
It covers 48 checks: all four sectors of a two-plane nonconvex example, affine
thresholds and equality, snapshots, source/revision traces, abstention, overlapping
categories, scoped corrections, malformed/cyclic/oversized trees, native type/basis/
dimension refusals and failure after an earlier successful decision.

All features, labels and image identities in this suite are synthetic. These tests
do not establish visual quality, learned support vectors, an image decoder, masks,
a correction store, an Android APK or physical C67 acceptance.

The previous source head `710ed08886a29f0a3d485690678a97962c4e6dce` passed hosted
shared-source compilation, native ICK, modified iLua, rotations and shader-negative
controls in run `44155403185`. That result predates this decision layer and is not
its acceptance. A private local syntax-translated stock-Lua/fixed-score check passed
all 48 routing checks; it is only a control-flow reference, not candidate-runtime or
shared-shader evidence. Exact new-head hosted execution is required.

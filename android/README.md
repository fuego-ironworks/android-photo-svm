# Android photo regions

The photo picker opens a bounded JPEG or PNG, displays an oriented region
overlay, and accepts a human organic or inorganic assertion for a selected
region. Pick the same image again to recover the saved assertions. The direct
Idriç Activity uses Android framework decoding and a small ICK-built C library.

## Region identity

The C adapter computes SHA-256 over the `photo-regions-v1` domain, source
width/height/orientation, and the oriented ARGB pixels in row order. Integers
use big-endian bytes and input stride padding is excluded. An annotation
preference key combines this digest with the deterministic region ID. A region
algorithm change requires a new domain before it can assign different IDs to
the same image. Source URLs, pathnames and photo bytes are absent from saved
assertions. The app replays only explicit assertions for the reopened image.

The input limit is 4,096 pixels on either side and 1,048,576 total pixels. The
native frame uses 16-pixel tiles, adjacent color similarity, and a bounded
number of regions and live handles. UI coordinates map back into its pixel
grid. Unknown pixels retain their source color, and an explicit assertion tints
the selected region. The region engine supplies masks and assertion provenance.

The 8D rotations and 32D vector–covector pairing stay with the shared
mathematics. Applying a fitted semantic model to descriptors from these
regions is a separate integration step after the label path runs on Android.

## Qualification

The native fixture covers all eight orientation transforms, pixel bounds,
padded strides, stable annotation identity, a changed-pixel negative control,
and human assertions. The host JNI fixture checks synchronous array copies,
failure without partial output, and stale or forged handles. Synthetic EXIF
fixture `tests/fixtures/exif-6.jpg` is 320×160 in storage and should display
160×320, dark above light.

The platform test matrix and receipt fields are in
[Android acceptance](tests/ANDROID-ACCEPTANCE.md). Device acceptance is pending.
See [dependency revisions](ci/dependencies.tsv) for the checked frontend,
native compiler, and SHA-256 provenance.

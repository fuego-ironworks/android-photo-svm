# Region features for supplied planes

The host iLua adapter can construct a region frame directly from decoded pixels.
`photo_native.regions(width, height, orientation, rgba)` accepts exactly four
bytes per pixel in R, G, B, A order, with the existing region bounds. The
native frame applies the EXIF orientation. `photo_native.region_at(frame, x, y)`
selects a region by oriented pixel position. Region IDs and positions start at zero.
`photo_native.region_entry(frame, id)` supplies the existing decision interface
with the frame's content-derived SHA-256, region revision and ID, plus an
immutable native Float32 feature vector. A fully transparent region supplies only
the target, causing a compiled plan to abstain.

The first carrier is `rgba-histogram-v1`, in space `photo-region-color` with basis
`rgba-histogram-rgb8-intensity4-alpha4` and metric `standard-euclidean`. Its basis order is
eight red bins, eight green, eight blue, four mean-channel intensity bins, and
four alpha bins. Each color/intensity histogram weights input pixels by their
alpha byte and divides by the region's total alpha. The alpha histogram uses
unweighted pixel counts. Original oriented pixels and region IDs supply the
counts. Assertion tints and edge highlights never enter the feature carrier.

A supplied plane must declare the same representation, space, basis and metric,
and covector variance. iLua invokes the existing shared 32-dimensional pairing
before applying the affine threshold. The fixture checks a transparent region,
a same-size wrong basis, a changed decoded image, and two correctly oriented
regions with independently predictable signed scores. It uses synthetic colors
to verify the arithmetic and source binding. Semantic organic/inorganic learning
requires annotated photos. The DEX UI still supplies and replays human labels.

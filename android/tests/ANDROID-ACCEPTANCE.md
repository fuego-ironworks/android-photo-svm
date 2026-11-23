# Installed Android acceptance

Record the app commit, DEX inputs, ICK source revision, APK hash, ABI, API,
build fingerprint, and stdout or screenshots. Run on API 23, 29, 34 and 35,
then on the MIRO C67.

1. Launch the APK and use the document picker to open `fixtures/exif-6.jpg`.
   The raw 320×160 image displays as a 160×320 portrait, with the light region
   below the dark region. The magenta boundary follows the visible color edge.
2. Select the light region and mark it Organic. The green shading stays within
   that region. Tap the viewport letterbox and verify the selection does not
   move. Repeat in landscape.
3. Open the picker and cancel. Verify the visible photo and selected region
   stay in place. Force stop the app, relaunch, and pick the same fixture.
   Verify the saved assertion reappears. Open a different fixture and verify
   that it starts without an assertion.
4. Try a missing provider stream, truncated JPEG and an image over the size
   limit. Verify a visible error and successful subsequent selection.
5. Measure input staging, orientation/decode, native segmentation, overlay
   latency and peak RSS on the exact C67 artifact. Use actual photographs,
   including a JPEG whose EXIF orientation is separate from its pixel order.

The organic/inorganic quality gate requires people to label independently
held-out photos and a fitted model to make region proposals. Use the whole photo
or its capture group as the evaluation unit.

#!/bin/sh
set -eu
bundle=$1
out=$2
mkdir -p "$out/mutant"
cp -R "$bundle/generated" "$out/mutant/"
# Mutations affect only copied GENERATED output used by negative tests.
sed '/^  _idris_fragColor =/a\  _idris_fragColor.x = -_idris_fragColor.x;' "$bundle/generated/coordinate-pairing32-grayscale.frag" > "$out/mutant/generated/coordinate-pairing32-grayscale.frag"
! cmp -s "$bundle/generated/coordinate-pairing32-grayscale.frag" "$out/mutant/generated/coordinate-pairing32-grayscale.frag"
glslangValidator -S frag "$out/mutant/generated/coordinate-pairing32-grayscale.frag"
if "$out/test_core" "$out/mutant" > "$out/wrong-shared-projection.log" 2>&1; then exit 1; fi
grep -F 'score == -4.0f' "$out/wrong-shared-projection.log"
cp "$bundle/generated/coordinate-pairing32-grayscale.frag" "$out/mutant/generated/"
sed '/^  _idris_fragColor =/a\  _idris_fragColor.x = -_idris_fragColor.x;' "$bundle/generated/apply-two-reflections8.frag" > "$out/mutant/generated/apply-two-reflections8.frag"
! cmp -s "$bundle/generated/apply-two-reflections8.frag" "$out/mutant/generated/apply-two-reflections8.frag"
glslangValidator -S frag "$out/mutant/generated/apply-two-reflections8.frag"
if "$out/test_rotation" "$out/mutant" > "$out/wrong-shared-rotation.log" 2>&1; then exit 1; fi
grep -F 'actual->values[index] - expected->values[index]' "$out/wrong-shared-rotation.log"
sed '/^  _idris_fragColor =/a\  if (u_page > 0.5) _idris_fragColor.w = 0.0;' "$bundle/generated/apply-two-reflections8.frag" > "$out/mutant/generated/apply-two-reflections8.frag"
glslangValidator -S frag "$out/mutant/generated/apply-two-reflections8.frag"
"$out/test_rotation" "$out/mutant" expect-refusal > "$out/late-page-refusal.log" 2>&1
cat "$out/late-page-refusal.log"
rm "$out/mutant/generated/apply-two-reflections8.frag"
if "$out/test_core" "$out/mutant" > "$out/missing-shared-backend.log" 2>&1; then exit 1; fi
grep -F 'shared shader missing or oversized' "$out/missing-shared-backend.log"
test -z "$(nm "$out/libphoto_math.a" | grep -E 'photo_svm|psvm_|svm_fit|svm_predict' || true)"

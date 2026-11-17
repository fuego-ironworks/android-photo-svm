#!/bin/sh
set -eu
exe=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
# A 4x2 PPM with green left / gray right, generated without a decoder.
{
  printf 'P6\n4 2\n255\n'
  printf '\010\330\010\010\330\010\262\262\262\262\262\262'
  printf '\010\330\010\010\330\010\262\262\262\262\262\262'
} > "$tmp/input.ppm"
"$exe" "$tmp/input.ppm" "$tmp/output.ppm" 2 2> "$tmp/report"
grep -q '2 color/spatial regions; NOT semantic labels' "$tmp/report"
[ "$(wc -c < "$tmp/output.ppm")" -eq 35 ]
# Check that an actual edge overlay was produced, not a file copy.
! cmp -s "$tmp/input.ppm" "$tmp/output.ppm"
echo 'PASS: PPM image-to-region overlay'

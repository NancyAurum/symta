#!/usr/bin/env bash
# UIM widget regression harness.
#
# Each test case is dispatched via `--case=NAME` in tests/uim/src/go.s.
# We run the linked go.exe headlessly with `--screenshot=PATH
# --screenshot-frame=N`, then byte-compare the PNG to the golden in
# baselines/. PNG output from the gfx plugin is deterministic (libpng
# with PNG_TRANSFORM_IDENTITY, no tIME chunk, no zlib timestamp), so
# byte-equality is sufficient — no pixel-tolerance heuristic needed.
#
# Test list and per-case window dimensions are kept in $CASES below.
# Resolution coverage: we run the gallery case at the VESA-era 640x480
# minimum and at 800x600. The other cases use whatever size the source
# in src/tc_*.s declares.
#
# Usage:
#   ./tests/uim/run.sh                # run all cases
#   ./tests/uim/run.sh gallery640     # one case
#   ./tests/uim/run.sh --update       # rewrite goldens from current run
#   ./tests/uim/run.sh --capture      # alias for --update (semantic)

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

BUILD="$HERE"
ACTUAL="$HERE/actual"
EXPECT="$HERE/baselines"

UPDATE=0
GLOB=""
for arg in "$@"; do
  case "$arg" in
    --update|--capture) UPDATE=1 ;;
    *) GLOB="$arg" ;;
  esac
done

mkdir -p "$ACTUAL" "$EXPECT"

# Stage the one application-level asset that isn't auto-staged:
# the pic/test/ subset, a couple of example pictograms tc_windows.s
# references for its `pic 'test/hourglass'` decoration. Everything
# else (pic/ui/, ttf/, SDL DLLs) is materialised automatically:
#   * uimgen   writes pic/ui/*.svg on first import of `uim`
#   * ffi_begin (ttf) stages symta/ttf/inter.ttf into ./ttf/
#   * ffi_begin (ui)  stages symta/sdl/*.dll next to go.exe
ASSET_SRC="$ROOT/examples/16-ui"
[ -d "$BUILD/pic/test" ] || { mkdir -p "$BUILD/pic"
                              cp -r "$ASSET_SRC/pic/test" "$BUILD/pic/test"; }

# Build the harness once; uses cached sbc on subsequent runs.
( cd "$BUILD" && "$SYMTA" . ) >"$BUILD/build.log" 2>&1 || {
  echo "UIM harness build failed:"
  tail -20 "$BUILD/build.log" | sed 's/^/         /'
  exit 1
}

# Per-case: name, screenshot frame (after which UIM exits), W, H.
# 60 frames = ~1 second @ 60fps; enough for any auto-layout to settle.
CASES=(
  "buttons    60 640 480"
  "inputs     60 640 480"
  "lists      60 640 480"
  "gallery640 60 640 480"
  "gallery800 60 800 600"
  "isometric  60 640 480"
  "synthetic  60 640 480"
  "windows    60 640 480"
)

passed=0
failed=0
new=0
fail_names=""

for spec in "${CASES[@]}"; do
  set -- $spec
  name=$1; frame=$2; w=$3; h=$4
  case "$GLOB" in
    "") ;;
    *) case "$name" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac

  out="$ACTUAL/$name.png"
  golden="$EXPECT/$name.png"
  rm -f "$out"

  GO="$BUILD/go.exe"
  [ -x "$GO" ] || GO="$BUILD/go"
  ( cd "$BUILD" && "$GO" \
      --case=$name --w=$w --h=$h \
      --screenshot=actual/$name.png --screenshot-frame=$frame \
  ) >"$ACTUAL/$name.log" 2>&1

  if [ ! -f "$out" ]; then
    failed=$((failed+1))
    fail_names="$fail_names $name"
    echo "  RUN-FAIL $name"
    tail -10 "$ACTUAL/$name.log" | sed 's/^/         /'
    continue
  fi

  if [ ! -f "$golden" ]; then
    if [ $UPDATE -eq 1 ]; then
      cp "$out" "$golden"
      echo "  NEW    $name"
      new=$((new+1))
    else
      echo "  ?      $name   (no golden; run with --update to capture)"
      new=$((new+1))
    fi
    continue
  fi

  # Compare by DECODED PIXELS, not by raw PNG bytes. libpng's filter /
  # zlib-chunk choices vary harmlessly between platforms (Win64
  # vs Linux can produce a ~5-10 B PNG header delta even with
  # identical image content), and we don't own that representation.
  # The harness's --pngcmp mode loads both files through the gfx
  # plugin and walks them pixel-by-pixel, printing `MATCH` when the
  # decoded RGB(A) buffers are identical.
  cmp_result=$( cd "$BUILD" && "$GO" --pngcmp-a="$out" --pngcmp-b="$golden" 2>&1 )
  if printf '%s' "$cmp_result" | grep -q '^MATCH'; then
    passed=$((passed+1))
  else
    if [ $UPDATE -eq 1 ]; then
      cp "$out" "$golden"
      echo "  UPDATE $name"
    else
      failed=$((failed+1))
      fail_names="$fail_names $name"
      sa=$(wc -c < "$out" | tr -d ' ')
      se=$(wc -c < "$golden" | tr -d ' ')
      reason=$(printf '%s' "$cmp_result" | head -1)
      echo "  FAIL   $name   (actual=${sa}B golden=${se}B; $reason; see $out)"
    fi
  fi
done

echo
echo "Summary: $passed passed, $failed failed, $new new"
if [ $failed -gt 0 ]; then
  echo "Failed:$fail_names"
  exit 1
fi
exit 0

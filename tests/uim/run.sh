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

# Stage runtime assets (pic / ttf / SDL DLLs) from the 16-ui example
# so we don't duplicate ~4MB of binaries inside the tests tree. The
# examples are checked-in; the test dir is not.
ASSET_SRC="$ROOT/examples/16-ui"
for d in pic ttf; do
  [ -d "$BUILD/$d" ] || cp -r "$ASSET_SRC/$d" "$BUILD/$d"
done
for f in "$ASSET_SRC"/*.dll; do
  base=$(basename "$f")
  [ -f "$BUILD/$base" ] || cp "$f" "$BUILD/$base"
done

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

  ( cd "$BUILD" && ./go.exe \
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

  if cmp -s "$out" "$golden"; then
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
      echo "  FAIL   $name   (actual=${sa}B golden=${se}B; see $out)"
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

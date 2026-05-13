#!/usr/bin/env bash
# Headless gfx-FFI golden-image suite driver.
#
# Same convention as the other symta/tests/*/run.sh scripts: builds
# the harness once, runs it, exits nonzero if any case fails. The
# harness itself (src/harness.s + src/cases.s) does the actual PNG
# diffing against golden/. See ./README.md for the full story.
#
# Usage:
#   ./tests/gfx/run.sh             # run all cases
#   ./tests/gfx/run.sh --update    # promote out/*.png into golden/
#                                  # (after visually inspecting them)

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

UPDATE=0
for arg in "$@"; do
  case "$arg" in
    --update) UPDATE=1 ;;
  esac
done

# Build the harness; on a hot cache this is a no-op.
( cd "$HERE" && "$SYMTA" . ) >"$HERE/build.log" 2>&1 || {
  echo "gfx harness build failed:"
  tail -20 "$HERE/build.log" | sed 's/^/         /'
  exit 1
}

mkdir -p "$HERE/out"

# The harness exits non-zero on any test failure; passthrough.
( cd "$HERE" && ./go.exe )
rc=$?

if [ $UPDATE -eq 1 ] && [ $rc -ne 0 ]; then
  echo
  echo "Promoting out/*.png -> golden/ (visual inspection skipped)"
  cp -f "$HERE/out/"*.png "$HERE/golden/"
fi

exit $rc

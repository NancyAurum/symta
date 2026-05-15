#!/usr/bin/env bash
# Weak hashtable throughput benchmark.
#
# Builds benchmark/wh/src/ (a tiny harness that imports bn_wh) and
# runs the binary.  Output is plain `name: N: elapsed s, ns/op`
# lines so a human can diff revisions.
#
# Usage:
#   ./benchmark/wh/run.sh             # build + run
#   ./benchmark/wh/run.sh --save FILE # tee raw output for diff

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

SAVE=""
while [ $# -gt 0 ]; do
  case "$1" in
    --save) SAVE="$2"; shift 2 ;;
    *) echo "unknown arg: $1"; exit 1 ;;
  esac
done

( cd "$HERE" && "$SYMTA" . ) >"$HERE/build.log" 2>&1 || {
  echo "wh-bench build failed:"
  tail -20 "$HERE/build.log" | sed 's/^/         /'
  exit 1
}

if [ -n "$SAVE" ]; then
  "$HERE/go.exe" | tee "$SAVE"
else
  "$HERE/go.exe"
fi

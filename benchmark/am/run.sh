#!/usr/bin/env bash
# Adaptive-map benchmark runner.
#
# Builds the harness once, runs each benchmark group (`bn_int`,
# `bn_text`, `bn_generic`, `bn_bitmap`, `bn_promote`, `bn_iter`),
# captures the raw output, and prints a flat ns/op table at the
# end for easy comparison across revisions.
#
# Usage:
#   ./symta/benchmark/am/run.sh              # all groups
#   ./symta/benchmark/am/run.sh int          # one group
#   ./symta/benchmark/am/run.sh --save FILE  # write the raw output
#                                              to FILE for later diff
#
# The output is *not* a regression test: variance is non-trivial
# and the benchmarks aren't pinned to specific timings. Use the
# numbers as a reference point when changing the adaptive map's
# implementation. Commit baseline.txt alongside any change that's
# expected to move the numbers.

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

CASE=""
SAVE=""
while [ $# -gt 0 ]; do
  case "$1" in
    --save)  shift; SAVE="$1" ;;
    --help|-h)
      sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) CASE="$1" ;;
  esac
  shift
done

# Build the harness once.
( cd "$HERE" && "$SYMTA" . ) >"$HERE/build.log" 2>&1 || {
  echo "AM-bench harness build failed:"
  tail -30 "$HERE/build.log" | sed 's/^/         /'
  exit 1
}

GO="$HERE/go.exe"
[ -x "$GO" ] || GO="$HERE/go"

# Run the benchmarks. If --save, also tee to the save file.
run_one() {
  local case_arg="$1"
  if [ -n "$case_arg" ]; then
    (cd "$HERE" && "$GO" --case="$case_arg" 2>&1)
  else
    (cd "$HERE" && "$GO" 2>&1)
  fi
}

RAW=$(run_one "$CASE")

if [ -n "$SAVE" ]; then
  printf '%s\n' "$RAW" > "$SAVE"
  echo "saved raw output to $SAVE"
fi

# Pretty print: line up the ns/op column for human eyeballing.
printf '%s\n' "$RAW" | awk '
  /^\[bn_/  { group = substr($0, 2, length($0)-2); print ""; print group ":"; next }
  /^#/      { print "  " $0; next }
  /ns\/op/  {
    # Lines look like:  "<label> <N>: <elapsed> s, <ns_per_op> ns/op"
    # or:               "<label> <N>: <elapsed> s, <ns_per_op> ns/entry"
    if (match($0, /^([^ ]+)[[:space:]]+([0-9]+):[[:space:]]+([0-9.]+) s, ([0-9]+) ns\/op/, m)) {
      printf "  %-10s N=%-8d  %s s   %6d ns/op\n", m[1], m[2], m[3], m[4]
    } else if (match($0, /^([^ ]+)[[:space:]]+([0-9]+):[[:space:]]+([0-9.]+) s, ([0-9]+) ns\/entry/, m)) {
      printf "  %-10s N=%-8d  %s s   %6d ns/entry\n", m[1], m[2], m[3], m[4]
    } else {
      print "  " $0
    }
    next
  }
  /ns\/entry/ {
    # Promote+rounds lines:
    # "<label> pop=<P> rounds=<R>: <elapsed> s, <ns_per_op> ns/entry"
    print "  " $0
    next
  }
  /^[[:space:]]*\(/ { print "  " $0; next }
  /^$/      { next }
  { print "  ?? " $0 }
'

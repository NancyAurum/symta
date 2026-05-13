#!/usr/bin/env bash
# Runtime / interpreter benchmark runner.
#
# Builds the harness once, runs each benchmark group (`bn_loop`,
# `bn_arith`, `bn_branch`, `bn_call`, `bn_mcall`, `bn_list`),
# captures the raw output, and prints a flat ns/op table at the
# end for easy comparison across revisions.
#
# Usage:
#   ./symta/benchmark/rt/run.sh              # all groups
#   ./symta/benchmark/rt/run.sh arith        # one group
#   ./symta/benchmark/rt/run.sh --save FILE  # write the raw output
#                                              to FILE for later diff
#
# The output is *not* a regression test: variance is non-trivial
# and the benchmarks aren't pinned to specific timings. Use the
# numbers as a reference point when changing the interpreter
# dispatch or any opcode handler. Commit baseline.txt alongside
# any change that's expected to move the numbers.

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
  echo "RT-bench harness build failed:"
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
    if (match($0, /^([^ ]+)[[:space:]]+([0-9]+):[[:space:]]+([0-9.]+) s, ([0-9]+) ns\/op/, m)) {
      printf "  %-10s N=%-10d  %s s   %6d ns/op\n", m[1], m[2], m[3], m[4]
    } else {
      print "  " $0
    }
    next
  }
  /^[[:space:]]*\(/ { print "  " $0; next }
  /^$/      { next }
  { print "  ?? " $0 }
'

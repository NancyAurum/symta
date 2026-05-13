#!/usr/bin/env bash
# FFI regression suite.
#
# Builds the small in-tree test library (cffilib/) into a shared
# object, stages it as `ffi/cffi.ffi` next to the test project,
# then runs each Symta test case via the `--case=NAME` dispatcher
# in src/go.s. Output is compared to a golden file in expected/
# after normalising platform-dependent noise.
#
# Each case prints PASS / FAIL lines; the golden is a flat list
# of PASSes. A FAIL line in actual output makes the diff fail
# loudly.
#
# Usage:
#   ./tests/ffi/run.sh                # run all
#   ./tests/ffi/run.sh sanity         # just the basename-matching case
#   ./tests/ffi/run.sh --update       # rewrite goldens from actual output

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

UPDATE=0
GLOB=""
for arg in "$@"; do
  case "$arg" in
    --update|--capture) UPDATE=1 ;;
    *) GLOB="$arg" ;;
  esac
done

# Detect platform — pick the right cffilib makefile + .ffi name.
UNAME=$(uname -s 2>/dev/null || echo Windows_NT)
case "$UNAME" in
  *MINGW*|*MSYS*|*CYGWIN*|Windows_NT) MK=Makefile.w64 ;;
  Darwin)                              MK=Makefile.osx ;;
  *)                                   MK=Makefile     ;;
esac

# Build cffilib if needed. We rebuild unconditionally; the makefile
# itself is mtime-aware so a hot rebuild costs nothing.
echo "[cffilib] $MK"
( cd "$HERE/cffilib" && make -s -f "$MK" ) >"$HERE/cffilib.log" 2>&1 || {
  echo "cffilib build failed:"
  tail -20 "$HERE/cffilib.log" | sed 's/^/         /'
  exit 1
}

# Build the Symta test harness. Stages cffi.ffi into the project's
# ffi/ dir so `ffi_begin local cffi` picks it up.
mkdir -p "$HERE/ffi"
cp -f "$HERE/cffilib/lib/cffi.ffi" "$HERE/ffi/cffi.ffi"

# Build the harness once. The compiler caches per-module sbc so
# rebuilds are fast.
( cd "$HERE" && "$SYMTA" . ) >"$HERE/build.log" 2>&1 || {
  echo "FFI harness build failed:"
  tail -30 "$HERE/build.log" | sed 's/^/         /'
  exit 1
}

# Test cases, in order. Add new cases by appending the basename.
CASES=(
  sanity
  int_roundtrip
  uint32
  float
  double
  ptr
  string
  arith
  arity_i32
  arity_f64
  interleave
  buffer
  str_ops
  float_edge
  stress
  libc
  zlib
)

mkdir -p "$HERE/actual" "$HERE/expected"

passed=0
failed=0
new=0
fail_names=""

# Each case's expected output is a list of PASS lines, one per
# assertion in the case file. We capture actual output, strip
# CR (Windows), and compare. The contract: any FAIL line in actual
# output is a regression.

for name in "${CASES[@]}"; do
  case "$GLOB" in
    "") ;;
    *) case "$name" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac

  out="$HERE/actual/$name.out"
  golden="$HERE/expected/$name.out"

  # On Windows the compiled output is `go.exe`; on Linux/macOS it's
  # just `go`. Both produced by the same `symta .` invocation above.
  GO="$HERE/go.exe"
  [ -x "$GO" ] || GO="$HERE/go"
  raw=$( cd "$HERE" && "$GO" --case=$name 2>&1 )
  printf '%s\n' "$raw" | tr -d '\r' > "$out"

  # Fast pre-check: any FAIL in actual = test failed.
  if grep -q '^FAIL' "$out"; then
    failed=$((failed+1))
    fail_names="$fail_names $name"
    echo "  FAIL   $name"
    grep '^FAIL' "$out" | head -3 | sed 's/^/         /'
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
      echo "  DIFF   $name"
      diff "$golden" "$out" | head -10 | sed 's/^/         /'
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

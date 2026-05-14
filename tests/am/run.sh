#!/usr/bin/env bash
# Adaptive map regression suite.
#
# Each test case is a Symta module that exports `tc_<name>` and
# prints `PASS <label>` / `FAIL <label>: ...` lines. The harness in
# src/go.s dispatches via `--case=<name>`. We diff the captured
# output against expected/<name>.out after normalising the few
# environment-specific bits (CRLF, paths in any stack traces that
# leak through).
#
# Usage:
#   ./tests/am/run.sh            # run all
#   ./tests/am/run.sh modes      # just the basename-matching case
#   ./tests/am/run.sh --update   # rewrite goldens from actual output

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

# Build the harness once. The compiler caches per-module sbc so
# rebuilds after this are fast.
( cd "$HERE" && "$SYMTA" . ) >"$HERE/build.log" 2>&1 || {
  echo "AM harness build failed:"
  tail -30 "$HERE/build.log" | sed 's/^/         /'
  exit 1
}

# Add new cases by appending the basename here. Each case file
# should be src/tc_<name>.s and export tc_<name>.
CASES=(
  empty
  bitmap0
  bitmap1
  int
  text
  generic
  promote
  gid
  mixed
  iteration
  stress
  void
)

mkdir -p "$HERE/actual" "$HERE/expected"

passed=0
failed=0
new=0
fail_names=""

normalize() {
  tr -d '\r' \
    | sed -e 's|[A-Z]:/[Uu]sers/[^/]*/[^,]*/symta/\{1,\}|REPO/|g' \
          -e 's|/home/[^/]*/[^,]*/symta/\{1,\}|REPO/|g' \
          -e 's|/Users/[^/]*/[^,]*/symta/\{1,\}|REPO/|g'
}

for name in "${CASES[@]}"; do
  case "$GLOB" in
    "") ;;
    *) case "$name" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac

  out="$HERE/actual/$name.out"
  golden="$HERE/expected/$name.out"

  GO="$HERE/go.exe"
  [ -x "$GO" ] || GO="$HERE/go"
  raw=$( cd "$HERE" && "$GO" --case=$name 2>&1 )
  printf '%s\n' "$raw" | normalize > "$out"

  # Refuse to write a golden that contains FAIL / UNHANDLED ERROR
  # / segfault: those are always test failures, never expected output.
  # (See FFI-3 -- six error-ing FFI goldens got accidentally captured
  # because --update wrote them without checking.)
  taint=""
  if grep -q '^FAIL' "$out"; then
    taint="FAIL"
  elif grep -q '^UNHANDLED ERROR' "$out"; then
    taint="UNHANDLED ERROR"
  elif grep -q 'segfault at' "$out"; then
    taint="segfault"
  fi
  if [ -n "$taint" ]; then
    failed=$((failed+1))
    fail_names="$fail_names $name"
    echo "  FAIL   $name   ($taint in output -- refusing to update)"
    grep -E '^FAIL|^UNHANDLED ERROR|segfault at' "$out" | head -3 \
      | sed 's/^/         /'
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

#!/usr/bin/env bash
# Macro / DSL regression tests.
#
# Each file in tests-macros/cases/ exercises one macro family --
# `case`, `{}`, `if/when`, lambdas, etc. -- across every documented
# variant. Output is captured and diffed against the golden in
# tests-macros/expected/.
#
# Where tests-runtime/ covers full-program behavior across a small
# gallery and tests-compiler/ pins bytecode determinism, this suite
# pins *macro semantics*: if a future compiler change breaks a `case`
# pattern variant or a `{}` extractor without breaking parsing, this
# is the suite that catches it.
#
# Usage:
#   ./tests-macros/run.sh            # run all
#   ./tests-macros/run.sh case       # run just cases/case.s
#   ./tests-macros/run.sh --update   # rewrite goldens from current output

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"
CASES="$HERE/cases"
EXPECT="$HERE/expected"

GLOB="${1:-}"
UPDATE=0
case "$GLOB" in
  --update) UPDATE=1; GLOB="" ;;
esac

passed=0
failed=0
new=0
fail_names=""

mkdir -p "$EXPECT"

normalize() {
  tr -d '\r' \
    | sed -e 's/\bgid=[0-9]\{6,\}/gid=XXXXX/g' \
          -e 's/object=[0-9a-fA-F]\{4,\}/object=XXXXX/'
}

for f in "$CASES"/*.s; do
  base=$(basename "$f" .s)
  case "$GLOB" in
    "") ;;
    *) case "$base" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac

  expected="$EXPECT/$base.out"
  raw=$("$SYMTA" -f "$f" 2>&1)
  actual=$(printf '%s\n' "$raw" | normalize)

  if [ ! -f "$expected" ]; then
    if [ $UPDATE -eq 1 ]; then
      printf '%s\n' "$actual" > "$expected"
      echo "  NEW    $base"
      new=$((new+1))
    else
      echo "  ?      $base   (no golden; run with --update to capture)"
      new=$((new+1))
    fi
    continue
  fi

  golden=$(cat "$expected" | normalize)
  if [ "$actual" = "$golden" ]; then
    passed=$((passed+1))
  else
    if [ $UPDATE -eq 1 ]; then
      printf '%s\n' "$actual" > "$expected"
      echo "  UPDATE $base"
    else
      failed=$((failed+1))
      fail_names="$fail_names $base"
      echo "  FAIL   $base"
      tmp_g=".macros-golden-$$"
      tmp_a=".macros-actual-$$"
      printf '%s\n' "$golden" > "$tmp_g"
      printf '%s\n' "$actual" > "$tmp_a"
      diff "$tmp_g" "$tmp_a" | head -20 | sed 's/^/         /'
      rm -f "$tmp_g" "$tmp_a"
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

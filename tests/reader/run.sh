#!/usr/bin/env bash
# Reader (parser) regression tests.
#
# Each file in tests-reader/cases/ exercises one parser feature --
# precedence-ladder pairings, if/then/else shapes, postfix
# invocation, lambda-arrow, offside rule, splice handling, etc.
#
# Tests run BEFORE the macros suite. A parse-shape bug shows up
# here first, well before macroexpand has a chance to fail in some
# downstream way that's harder to attribute.
#
# Usage:
#   ./tests-reader/run.sh            # run all
#   ./tests-reader/run.sh precedence # run just cases/precedence.s
#   ./tests-reader/run.sh --update   # rewrite goldens

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"
CASES="$HERE/cases"
EXPECT="$HERE/expected"

UPDATE=0
GLOB=""
for arg in "$@"; do
  case "$arg" in
    --update) UPDATE=1 ;;
    *) GLOB="$arg" ;;
  esac
done

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
      tmp_g=".reader-golden-$$"
      tmp_a=".reader-actual-$$"
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

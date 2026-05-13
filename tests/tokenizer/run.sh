#!/usr/bin/env bash
# Tokenizer regression tests.
#
# Each file in tests-tokenizer/cases/ exercises one tokenizer
# feature -- number forms, symbol shapes, operator disambiguation,
# string splicing, comments, source-position tracking, etc.
#
# Tests run BEFORE the reader suite so that a tokenize-level bug
# doesn't surface as a confusing parse error. Same execution
# model as tests-runtime/run.sh: each case prints its findings,
# diff against a golden in tests-tokenizer/expected/.
#
# Usage:
#   ./tests-tokenizer/run.sh            # run all
#   ./tests-tokenizer/run.sh numbers    # run just cases/numbers.s
#   ./tests-tokenizer/run.sh --update   # rewrite goldens

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
  # Same shape as tests/runtime/run.sh's normalize. Strips
  # platform-specific path prefixes (Win C:/Users/foo/..., Linux
  # /home/bar/...) from embedded source-position metadata so stack
  # traces compare equal across operating systems.
  tr -d '\r' \
    | sed -e 's/\bgid=[0-9]\{6,\}/gid=XXXXX/g' \
          -e 's/object=[0-9a-fA-F]\{4,\}/object=XXXXX/' \
          -e 's|[A-Z]:/[Uu]sers/[^/]*/[^,]*/symta/|REPO/|g' \
          -e 's|/home/[^/]*/[^,]*/symta/|REPO/|g' \
          -e 's|/Users/[^/]*/[^,]*/symta/|REPO/|g'
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
      tmp_g=".tokenizer-golden-$$"
      tmp_a=".tokenizer-actual-$$"
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

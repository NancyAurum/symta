#!/usr/bin/env bash
# Weak hashtable test runner.
#
# Runs tests/wh/wh-test.s and diffs the output against the committed
# golden in tests/wh/expected.out.  Exits non-zero on any FAIL line
# in the harness OR any diff vs the golden.
#
# Usage:
#   ./tests/wh/run.sh             # run + diff
#   ./tests/wh/run.sh --update    # capture current output as golden

set -u
cd "$(dirname "$0")/../.."

SYMTA=./symta.exe
[ -x "$SYMTA" ] || SYMTA=./symta
EXPECT=tests/wh/expected.out
UPDATE=0
case "${1:-}" in --update) UPDATE=1 ;; esac

actual=$(timeout 10 "$SYMTA" -f tests/wh/wh-test.s 2>&1 | tr -d '\r')

# Refuse to capture a golden that contains UNHANDLED ERROR / segfault.
taint=""
case "$actual" in
  *"UNHANDLED ERROR"*) taint="UNHANDLED ERROR" ;;
  *"segfault at"*)     taint="segfault" ;;
  *"  FAIL  "*)        taint="FAIL line in harness output" ;;
esac

if [ -n "$taint" ]; then
  printf '%s\n' "$actual"
  echo
  echo "wh tests: REFUSING to update -- $taint"
  exit 1
fi

if [ $UPDATE -eq 1 ]; then
  printf '%s\n' "$actual" > "$EXPECT"
  echo "wh tests: golden updated."
  exit 0
fi

if [ ! -f "$EXPECT" ]; then
  echo "wh tests: no golden -- run with --update to capture"
  exit 1
fi

golden=$(cat "$EXPECT")

if [ "$actual" = "$golden" ]; then
  passed=$(printf '%s\n' "$actual" | grep -c '^  ok ' || true)
  echo "Summary: $passed passed, 0 failed, 0 new"
  exit 0
fi

echo "wh tests: output differs from golden"
tmp_g=".wh-golden-$$"
tmp_a=".wh-actual-$$"
printf '%s\n' "$golden" > "$tmp_g"
printf '%s\n' "$actual" > "$tmp_a"
diff "$tmp_g" "$tmp_a" | head -30 | sed 's/^/         /'
rm -f "$tmp_g" "$tmp_a"
exit 1

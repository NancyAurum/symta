#!/usr/bin/env bash
# REPL regression test runner.
#
# Each test case is a `cases/<name>.in` file -- one REPL line per
# input line, ending with `exit` (or `quit`).  The driver pipes it
# into `./symta.exe` and diffs the captured output against
# `expected/<name>.out`.
#
# Usage:
#   ./tests/repl/run.sh            # run all
#   ./tests/repl/run.sh 02         # run cases whose basename starts with 02
#   ./tests/repl/run.sh --update   # rewrite goldens from current output
#
# Why pipe stdin instead of using `expect`:
#   - We don't actually need pseudo-tty behaviour -- the REPL reads
#     get_line() from stdin and writes its prompt to stdout, so a
#     plain stdin pipe captures everything we care about and works
#     identically on Windows/Linux/macOS.
#
# Normalization:
#   - Strip CR (CRLF -> LF).
#   - Redact the version line, since `Symta v[rt_get version]` will
#     drift on every release bump and we don't want every test to
#     break in lockstep.
#   - Redact `object=` hex addresses (nondeterministic).

set -u
cd "$(dirname "$0")/../.."

SYMTA=./symta.exe
[ -x "$SYMTA" ] || SYMTA=./symta
CASES=tests/repl/cases
EXPECT=tests/repl/expected
GLOB="${1:-}"
UPDATE=0
case "$GLOB" in
  --update) UPDATE=1; GLOB="" ;;
esac

mkdir -p "$EXPECT"

passed=0
failed=0
new=0
fail_names=""

normalize() {
  tr -d '\r' \
    | sed -e 's/^Symta v.*$/Symta vXXX/' \
          -e 's/object=[0-9a-fA-F]\{4,\}/object=XXXXX/g'
}

for f in "$CASES"/*.in; do
  [ -f "$f" ] || continue
  base=$(basename "$f" .in)
  case "$GLOB" in
    "") ;;
    *) case "$base" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac
  expected="$EXPECT/$base.out"

  raw=$(timeout 10 "$SYMTA" < "$f" 2>&1)
  actual=$(printf '%s\n' "$raw" | normalize)

  taint=""
  case "$actual" in
    *"UNHANDLED ERROR"*) taint="UNHANDLED ERROR" ;;
    *"segfault at"*)     taint="segfault" ;;
  esac

  if [ ! -f "$expected" ]; then
    if [ $UPDATE -eq 1 ] && [ -z "$taint" ]; then
      printf '%s\n' "$actual" > "$expected"
      echo "  NEW    $base"
      new=$((new+1))
    elif [ -n "$taint" ]; then
      failed=$((failed+1))
      fail_names="$fail_names $base"
      echo "  FAIL   $base   ($taint in output -- refusing to update)"
    else
      echo "  ?      $base   (no golden; run with --update to capture)"
      new=$((new+1))
    fi
    continue
  fi

  golden=$(cat "$expected" | normalize)
  if [ "$actual" = "$golden" ] && [ -z "$taint" ]; then
    passed=$((passed+1))
  else
    if [ $UPDATE -eq 1 ] && [ -z "$taint" ]; then
      printf '%s\n' "$actual" > "$expected"
      echo "  UPDATE $base"
    elif [ -n "$taint" ]; then
      failed=$((failed+1))
      fail_names="$fail_names $base"
      echo "  FAIL   $base   ($taint in output -- refusing to update)"
    else
      failed=$((failed+1))
      fail_names="$fail_names $base"
      echo "  FAIL   $base"
      tmp_g=".repl-golden-$$"
      tmp_a=".repl-actual-$$"
      printf '%s\n' "$golden" > "$tmp_g"
      printf '%s\n' "$actual" > "$tmp_a"
      diff "$tmp_g" "$tmp_a" | head -10 | sed 's/^/         /'
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

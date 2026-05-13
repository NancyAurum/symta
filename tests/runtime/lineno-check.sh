#!/usr/bin/env bash
# Line-number regression test.
#
# Before the row-init fix in tokenize.c, the bare `-f` path produced
# error messages and stack traces with line numbers off by one (row was
# 0-indexed internally but printed verbatim, so foo on physical line N
# was tagged as `N-1`). This test pins the 1-indexed behavior so we
# notice any future drift.
#
# Exit non-zero if any case fails.

set -u
cd "$(dirname "$0")/../.."

SYMTA=./symta.exe
[ -x "$SYMTA" ] || SYMTA=./symta

tmp=tests/runtime/.lineno-check.s
pass=0
fail=0

check() {
  src="$1"
  want_pat="$2"
  label="$3"
  printf '%s\n' "$src" > "$tmp"
  out=$("$SYMTA" -f "$tmp" 2>&1)
  if echo "$out" | grep -q "$want_pat"; then
    pass=$((pass+1))
  else
    fail=$((fail+1))
    echo "  FAIL $label"
    echo "    expected pattern: $want_pat"
    echo "    actual first line: $(echo "$out" | head -1)"
  fi
}

# Case 1: undef on physical line 1 -> row should be 1, not 0.
check '| foo_undefined' ':1,2: undefined variable .foo_undefined.' \
      'line-1 error tagged as line 1'

# Case 2: undef on physical line 3 (skipping a comment + blank).
check '// comment

| foo_undefined' ':3,2: undefined variable .foo_undefined.' \
      'line-3 error tagged as line 3'

# Case 3: undef on physical line 5 in a body with leading blanks.
check '



| foo_undefined' ':5,2: undefined variable .foo_undefined.' \
      'line-5 error tagged as line 5'

# Case 4: function metadata points at the `=` of the definition.
# A `bad` call deep inside a 20-line body should still report the
# function frame at the definition line, not somewhere inside.
check '// pad 1
// pad 2
// pad 3
// pad 4
// pad 5
//
big_fn X =
  Y X + 1
  Z X + 2
  if X > 100
    then say "big"
    else bad "from line 14"
  // line 15 (after error path)
  // line 16
  // line 17

big_fn 0' 'big_fn:7,9' \
      'function frame meta points at `=` line, not body or trailing lines'

# Case 5: nested function call through case-dispatch.
check '// pad 1
// pad 2
//
classify V =
  case V:
    1 = "one"
    Else = bad "from case Else line 8"

classify 2' 'classify:4,11' \
      'function with case-dispatch body still reports definition line'

rm -f "$tmp"

echo "Summary: $pass passed, $fail failed"
[ $fail -eq 0 ]

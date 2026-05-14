#!/usr/bin/env bash
# Runtime / compiler regression test runner.
#
# For every Symta single-file example, compile + run it and compare
# the output against a golden file in symta/examples/.expected/.
# Exit non-zero if any test fails.
#
# Usage:
#   ./tests-runtime/run.sh            # run all
#   ./tests-runtime/run.sh 18         # run just the example whose
#                                       basename starts with 18
#   ./tests-runtime/run.sh --update   # rewrite the goldens from
#                                       current output (use with care)

set -u
cd "$(dirname "$0")/../.."

SYMTA=./symta.exe
[ -x "$SYMTA" ] || SYMTA=./symta
EXPECT=examples/.expected
GLOB="${1:-}"
UPDATE=0
case "$GLOB" in
  --update) UPDATE=1; GLOB="" ;;
esac

passed=0
failed=0
new=0
fail_names=""

for f in examples/*.s; do
  base=$(basename "$f" .s)
  case "$GLOB" in
    "") ;;
    *) case "$base" in "$GLOB"*) ;; *) continue ;; esac ;;
  esac
  expected="$EXPECT/$base.out"

  # Some examples take longer; bump timeout for known-slow ones.
  case "$base" in
    11-parser|14-quirks|24-runtime|25-lexmacro) tmout=15 ;;
    *) tmout=5 ;;
  esac

  # Normalize for diff: strip CR (LF/CRLF agnostic) and redact
  # known nondeterministic patterns (memory addresses, heap stats,
  # gids embedded as 'gid=<digits>'). If a test legitimately needs
  # a deterministic number, it can opt out of redaction by emitting
  # the number in a different format.
  normalize() {
    # The trailing `symta/+` in the path-stripping regexes is to
    # collapse `<repo>/symta/` AND `<repo>/symta//` (the Win build
    # sometimes leaves doubled slashes when concatenating paths) to
    # a single `REPO/` so the goldens compare equal regardless of
    # which build inserted the extra slash.
    tr -d '\r' \
      | sed -e 's/\bgid=[0-9]\{6,\}/gid=XXXXX/g' \
            -e 's/^heap used: [0-9]\{1,\}+[0-9]\{1,\}$/heap used: XXX+XXX/' \
            -e 's/^object=[0-9a-fA-F]\{4,\}/object=XXXXX/' \
            -e 's|[A-Z]:/[Uu]sers/[^/]*/[^,]*/symta/\{1,\}|REPO/|g' \
            -e 's|/home/[^/]*/[^,]*/symta/\{1,\}|REPO/|g' \
            -e 's|/Users/[^/]*/[^,]*/symta/\{1,\}|REPO/|g'
  }

  # Some examples have benign nondeterminism (hash iteration order,
  # equal-key sort ties). Retry up to 3 times -- pass if any run
  # matches the golden.
  golden=$(cat "$expected" 2>/dev/null | normalize)
  actual=""
  for _try in 1 2 3; do
    raw=$(timeout "$tmout" "$SYMTA" -f "$f" 2>&1)
    rc=$?
    actual=$(printf '%s\n' "$raw" | normalize)
    [ "$actual" = "$golden" ] && break
  done

  # Refuse to write a golden that contains an unhandled error or a
  # segfault: those are always test failures, never expected output.
  # (See FFI-3 -- six error-ing goldens got accidentally captured
  # because --update wrote them without checking.)
  taint=""
  case "$actual" in
    *"UNHANDLED ERROR"*) taint="UNHANDLED ERROR" ;;
    *"segfault at"*)     taint="segfault" ;;
  esac

  if [ ! -f "$expected" ]; then
    if [ $UPDATE -eq 1 ] && [ -z "$taint" ]; then
      mkdir -p "$EXPECT"
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
    # quiet success
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
      # Use repo-local temp files because busybox-bash lacks process
      # substitution and w64devkit has no /tmp.
      tmp_g=".runtime-golden-$$"
      tmp_a=".runtime-actual-$$"
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

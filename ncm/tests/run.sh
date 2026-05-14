#!/usr/bin/env bash
# Standalone NCM regression suite.
#
# Builds ../ncm{,.exe} if it's stale, then runs each case in
# `cases/` through it, comparing the output to `expected/<case>.out`.
# This is a layer below tests/runtime/25-lexmacro.s -- it catches
# NCM bugs without dragging in the rest of Symta, so failures
# point straight at runtime/ncm.h-derived (symta/ncm/src/ncm.h)
# code.
#
# Usage:
#   bash symta/ncm/tests/run.sh             # run all
#   bash symta/ncm/tests/run.sh shifts      # run just `shifts.c`
#   bash symta/ncm/tests/run.sh --update    # refresh expected/ from
#                                             current output
#
# Output is normalised:
#   * `#line N "PATH"` directives have their PATH replaced by the
#     bare basename, so the goldens survive being run from any cwd
#     or by a maintainer with a different repo path.

set -u
HERE=$(cd "$(dirname "$0")" && pwd)
NCMROOT=$(cd "$HERE/.." && pwd)
NCM="$NCMROOT/ncm.exe"
[ -x "$NCM" ] || NCM="$NCMROOT/ncm"

# Build if missing.
if [ ! -x "$NCM" ]; then
  ( cd "$NCMROOT" && bash build.sh ) >"$HERE/build.log" 2>&1 || {
    echo "ncm build failed:"
    tail -20 "$HERE/build.log" | sed 's/^/         /'
    exit 1
  }
  [ -x "$NCMROOT/ncm.exe" ] && NCM="$NCMROOT/ncm.exe" || NCM="$NCMROOT/ncm"
fi

UPDATE=0
GLOB=""
for arg in "$@"; do
  case "$arg" in
    --update|--capture) UPDATE=1 ;;
    *) GLOB="$arg" ;;
  esac
done

mkdir -p "$HERE/actual" "$HERE/expected"

# Normalise: replace absolute or repo-relative paths in
# `#line N "..."` with the bare filename, so goldens are portable.
normalize() {
  sed -E 's|^#line ([0-9]+) ".*[/\\]([^/\\]*)"|#line \1 "\2"|'
}

passed=0
failed=0
new=0
fail_names=""

for case_file in "$HERE/cases"/*.c; do
  base=$(basename "$case_file" .c)
  case "$GLOB" in
    "") ;;
    *) [ "$base" = "$GLOB" ] || continue ;;
  esac

  out="$HERE/actual/$base.out"
  golden="$HERE/expected/$base.out"

  # NCM's CLI: ncm <infile> <outfile>.  It writes to a real file
  # path (not /dev/stdout), so we use a per-case temp under
  # `actual/` and then normalise into `out`.  No case uses #:include
  # today; add `i=<dir>` here if a future case needs it.
  tmp="$HERE/actual/$base.raw"
  "$NCM" "$case_file" "$tmp" 2> "$HERE/actual/$base.err" || true
  cat "$tmp" 2>/dev/null | normalize > "$out"
  # Append any stderr (which is where NCM Fatal lines land) so the
  # taint check below catches them.
  if [ -s "$HERE/actual/$base.err" ]; then
    cat "$HERE/actual/$base.err" >> "$out"
  fi
  rm -f "$tmp" "$HERE/actual/$base.err"

  # Refuse to write a golden that contains "Fatal:" -- those are
  # NCM-level errors, not legitimate output.
  if grep -q '^.*Fatal:' "$out"; then
    failed=$((failed+1))
    fail_names="$fail_names $base"
    echo "  FAIL   $base   (Fatal in output -- refusing to update)"
    grep '^.*Fatal:' "$out" | head -3 | sed 's/^/         /'
    continue
  fi

  if [ ! -f "$golden" ]; then
    if [ $UPDATE -eq 1 ]; then
      cp "$out" "$golden"
      echo "  NEW    $base"
      new=$((new+1))
    else
      echo "  ?      $base   (no golden; run with --update to capture)"
      new=$((new+1))
    fi
    continue
  fi

  if cmp -s "$out" "$golden"; then
    passed=$((passed+1))
  else
    if [ $UPDATE -eq 1 ]; then
      cp "$out" "$golden"
      echo "  UPDATE $base"
    else
      failed=$((failed+1))
      fail_names="$fail_names $base"
      echo "  DIFF   $base"
      diff -u "$golden" "$out" | head -15 | sed 's/^/         /'
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

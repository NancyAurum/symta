#!/usr/bin/env bash
# benchmark/game/run.sh -- compilation-speed benchmark.
#
# Compiles the Spell of Mastery game (../game/, ~22k lines of
# Symta across ~125 modules) and prints cold + warm timings.
# Cold = wiped sbc cache, full recompile.  Warm = re-run on top
# of the cold result; should be a near-no-op (just checks
# mtimes and exits).
#
# Use as a regression check after any compiler or runtime change:
# the cold timing reflects compiler/macroexpander throughput, the
# warm timing reflects sbc-load + dep-walk overhead.  Variance is
# noticeable on Windows (Defender, cold filesystem); commit a
# baseline next to substantial changes and compare bands, not
# point values.
#
# Usage:
#   ./benchmark/game/run.sh              # cold + warm
#   ./benchmark/game/run.sh --warm-only  # skip the cold reset
#   ./benchmark/game/run.sh --save FILE  # tee raw output for diff

set -u

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
GAME=$(cd "$ROOT/../game" && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"

if [ ! -d "$GAME/src" ]; then
  echo "benchmark/game: $GAME has no src/ -- is this a SoM checkout?"
  exit 1
fi
if [ ! -x "$SYMTA" ]; then
  echo "benchmark/game: $SYMTA not found"
  exit 1
fi

WARM_ONLY=0
SAVE=""
while [ $# -gt 0 ]; do
  case "$1" in
    --warm-only) WARM_ONLY=1 ;;
    --save) shift; SAVE="$1" ;;
  esac
  shift
done

emit() {
  if [ -n "$SAVE" ]; then echo "$@" | tee -a "$SAVE"
  else echo "$@"
  fi
}

if [ -n "$SAVE" ]; then : > "$SAVE"; fi

LOC=$(wc -l "$GAME"/src/*.s 2>/dev/null | tail -1 | awk '{print $1}')
emit "game compilation benchmark: $LOC source lines"
emit "  symta: $SYMTA"
emit "  game:  $GAME"

if [ $WARM_ONLY -eq 0 ]; then
  # Cold: wipe the sbc cache and time a full rebuild.
  rm -rf "$GAME/sbc" "$GAME/cache"
  t0=$(date +%s.%N)
  out=$("$SYMTA" "$GAME/" 2>&1)
  rc=$?
  t1=$(date +%s.%N)
  modules=$(echo "$out" | grep -c "^compiling ")
  cold=$(awk -v a="$t1" -v b="$t0" 'BEGIN{printf "%.2f", a-b}')
  emit ""
  emit "[cold]  $cold s   $modules modules   rc=$rc"
  if [ $rc -ne 0 ]; then
    emit "  (cold build failed -- last 5 lines:)"
    echo "$out" | tail -5 | sed 's/^/      /'
  fi
fi

# Warm: re-run on top of cold/existing cache.
t0=$(date +%s.%N)
out=$("$SYMTA" "$GAME/" 2>&1)
rc=$?
t1=$(date +%s.%N)
modules=$(echo "$out" | grep -c "^compiling ")
warm=$(awk -v a="$t1" -v b="$t0" 'BEGIN{printf "%.3f", a-b}')
emit ""
emit "[warm]  $warm s   $modules modules recompiled   rc=$rc"

emit ""

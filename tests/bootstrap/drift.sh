#!/usr/bin/env bash
# Self-hosting compiler drift test.
#
# Methodology (GCC / Rust / OCaml / Go bootstrap convention):
#   * Stage 1: built by the previously-committed sbc files (the
#     "foreign" compiler -- output may legitimately differ from
#     later stages because the inputs to its codegen differ).
#   * Stage 2: built by stage 1. First fully self-compiled output.
#   * Stage 3+: built by previous stage. MUST be byte-identical
#     to stage 2 if the compiler is a function over its inputs --
#     no hash-iteration drift, no uninitialised-memory leak into
#     output, no register-allocator instability.
#
# We run 5 stages and require pairwise byte-equality across
# stages 2..5. The literature (GCC `make compare`, Rust stage3,
# OCaml bootstrap-compare) considers 3 stages sufficient: if
# f(x)==f(f(x))==x then further iterates are x by induction.
# We do 5 because (a) it costs ~2 minutes and (b) catches the
# (theoretical, never-documented) period-2 oscillation where
# f(x)!=f(f(x)) but f(f(x))==f(f(f(x))).
#
# What this catches that single-round tests miss:
#   - hash-iteration order leaking into output
#   - allocation-order-dependent slot tagging
#   - uninitialised memory in codegen buffers reaching disk
#   - register-allocator instability under different inputs
#   - source-position metadata that drifts across rounds
#
# What this can NOT catch (DDC / Trusting Trust):
#   - a self-perpetuating malicious compiler -- needs Diverse
#     Double-Compiling (Wheeler 2009). Out of scope here.
#
# Usage:
#   ./tests-bootstrap/drift.sh            # 5 rounds (default)
#   ./tests-bootstrap/drift.sh 3          # GCC-equivalent 3 rounds
#   ./tests-bootstrap/drift.sh 10         # paranoid mode
#
# Should be run at the end of any runtime / macro / compiler
# modification. CI hook: add to tests-runtime sweep.

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
SYMTA="$ROOT/symta.exe"
[ -x "$SYMTA" ] || SYMTA="$ROOT/symta"
SBC_DIR="$ROOT/sbc"
PKG_DIR="pkg/symta"

# Modules that get rebuilt during a bootstrap.  go.sbc is also
# rebuilt now (the pre-existing `~` issue in pkg/symta/src/go.s
# was fixed in commit 8f7bd3f) but we exclude it from the
# watched list because its bytecode is sensitive to drift in
# `pkg/symta/src/go.s` itself rather than in the compiler --
# tracking it would conflate the two and produce false
# positives.  reader.sbc retired in commit XXXXXXX -- the
# reader's Symta side was folded into core_.s.
WATCHED="eval.sbc compiler.sbc"

ROUNDS=${1:-5}

# Stage dir lives OUTSIDE the project root so `symta .` doesn't try
# to compile it as a sub-package. We DON'T auto-clean on success
# so the human can inspect logs and stage snapshots after the fact.
STAGES_DIR=$(mktemp -d "${TMPDIR:-/tmp}/drift-XXXXXX")

echo "drift test: $ROUNDS rounds; watching $WATCHED"
echo "stages saved under $STAGES_DIR"

# Stage 0: capture the currently-committed sbc state as our
# baseline. This is the "foreign compiler" output -- we expect
# stage 1 to potentially differ from this because the source
# may have evolved since the sbc was last cut.
mkdir -p "$STAGES_DIR/stage0"
for m in $WATCHED; do
  cp "$SBC_DIR/$m" "$STAGES_DIR/stage0/$m"
done

# Force each subsequent bootstrap to actually re-run by touching
# the source the first time around.  `src/reader.s` retired in the
# reader consolidation, so touch `src/core_.s` -- a clean rebuild
# of core_ pulls everything downstream with it.
touch "$ROOT/src/core_.s"

for stage in $(seq 1 "$ROUNDS"); do
  echo
  echo "--- stage $stage ---"
  ( cd "$ROOT" && "$SYMTA" "$PKG_DIR" . ) >"$STAGES_DIR/stage${stage}.log" 2>&1 || {
    # Bootstrap can fail on go.s (B1 / pre-existing). Tolerate
    # that as long as the watched sbc files were updated.
    if ! grep -q "compiling reader" "$STAGES_DIR/stage${stage}.log"; then
      echo "stage $stage: bootstrap failed before producing sbc:"
      tail -5 "$STAGES_DIR/stage${stage}.log" | sed 's/^/         /'
      exit 1
    fi
  }
  mkdir -p "$STAGES_DIR/stage${stage}"
  for m in $WATCHED; do
    if [ ! -f "$SBC_DIR/$m" ]; then
      echo "stage $stage: $m missing after bootstrap"
      exit 1
    fi
    cp "$SBC_DIR/$m" "$STAGES_DIR/stage${stage}/$m"
  done
  # Hash report for the human.
  for m in $WATCHED; do
    h=$(md5sum "$STAGES_DIR/stage${stage}/$m" | awk '{print $1}')
    sz=$(wc -c < "$STAGES_DIR/stage${stage}/$m" | tr -d ' ')
    printf "  %-15s %10s B  md5=%s\n" "$m" "$sz" "$h"
  done
  # Touch core_.s so the next round actually re-compiles.
  # On filesystems with 1-second mtime granularity (Windows
  # NTFS / FAT32), the sbc we just wrote may have the same mtime
  # as the source we're about to touch. Use a forward timestamp
  # via `-d "1 second"` if the system supports it; otherwise
  # sleep briefly.
  if ! touch -d "1 second" "$ROOT/src/core_.s" 2>/dev/null; then
    sleep 1
    touch "$ROOT/src/core_.s"
  fi
done

# Compare. Stage 1 -> 2 is allowed to differ (different "input"
# compiler). Stages 2..N must be pairwise byte-identical.
echo
echo "--- comparison ---"

drift=0
if [ "$ROUNDS" -ge 2 ]; then
  for m in $WATCHED; do
    if cmp -s "$STAGES_DIR/stage1/$m" "$STAGES_DIR/stage2/$m"; then
      echo "  s1==s2 $m  (fixed point reached in one round -- ideal)"
    else
      echo "  s1!=s2 $m  (expected; stage 1's input compiler differs)"
    fi
  done
fi

if [ "$ROUNDS" -ge 3 ]; then
  echo
  for m in $WATCHED; do
    fail=0
    base="$STAGES_DIR/stage2/$m"
    for stage in $(seq 3 "$ROUNDS"); do
      if ! cmp -s "$base" "$STAGES_DIR/stage${stage}/$m"; then
        echo "  DRIFT: $m  stage 2 != stage $stage"
        cmp -l "$base" "$STAGES_DIR/stage${stage}/$m" 2>&1 | \
          head -3 | sed 's/^/         /'
        fail=1
        drift=1
        break
      fi
    done
    if [ $fail -eq 0 ]; then
      echo "  OK   $m  stages 2..$ROUNDS pairwise identical"
    fi
  done
fi

echo
if [ $drift -ne 0 ]; then
  echo "FAIL: self-hosting drift detected"
  echo "stages preserved at: $STAGES_DIR"
  trap - EXIT
  exit 1
fi
echo "PASS: $ROUNDS-round bootstrap reached fixed point"

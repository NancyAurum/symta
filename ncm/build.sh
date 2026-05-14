#!/bin/sh
# Builds the standalone NCM driver (./ncm or ./ncm.exe).
#
# After the May 2026 consolidation, the canonical NCM source-of-truth
# is symta/ncm/src/ncm.h and the stb_ds.h dependency is shared with
# the rest of symta at ../runtime/stb_ds.h.
#
# `ncu_opts.h` uses strdup / strndup; the helper provides its own
# strndup behind `#ifdef WINDOWS` because mingw's libc doesn't
# export it.  On Linux we need `-D_GNU_SOURCE` to expose glibc's
# strndup.  We pick the right flag from `uname`.
set -e
mkdir -p lib/

case "$(uname -s 2>/dev/null || echo)" in
  MINGW*|MSYS*|CYGWIN*|Windows_NT) PLAT_FLAGS="-DWINDOWS" ;;
  *)                               PLAT_FLAGS="-D_GNU_SOURCE" ;;
esac

cc -std=c99 --pedantic -O2 $PLAT_FLAGS -I../runtime src/main.c -o ./ncm

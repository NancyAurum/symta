/*  symta/runtime/ncm.h
 *
 *  Forwarder.  The canonical NCM source-of-truth is under
 *  ../ncm/src/ncm.h, where it stands alone with its own build
 *  (`make` in symta/ncm/) and regression suite (`symta/ncm/tests/`).
 *  Symta's runtime just folds it in here so `#include "ncm.h"` at
 *  the bltin.c call site keeps working.
 *
 *  Why two locations?  Until May 2026 we had two divergent NCM
 *  copies (`./ncm/` standalone + `symta/runtime/ncm.h` Symta-side).
 *  Fixes landed in one but not the other, so bugs survived in
 *  binaries that the other half's tests had already caught.  The
 *  consolidation pinned the canonical source under `symta/ncm/` so
 *  there's exactly one file to maintain.
 *
 *  Don't add code here.  Edits go in ../ncm/src/ncm.h with a
 *  regression case in symta/ncm/tests/.
 */
#include "../ncm/src/ncm.h"

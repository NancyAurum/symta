/* See meta_table.h for the design rationale.
 *
 * Note: the GC integration lives in gc.c (function `gc_meta_table`),
 * because the `GC_REC` and `GC_REDIR` macros it needs are file-scoped
 * to that translation unit.  This file exposes the table via the
 * `meta_*` accessors and the `meta_table_for_gc_` weak symbol so gc.c
 * can manipulate the same pointer. */

#include "common.h"
#include "dh.h"
#include "meta_table.h"

/* The single global parser-meta table.  Stored as `void *` in the
 * header for ABI cleanliness; cast to `dh_t *` here for actual use. */
void *meta_table_g = 0;
#define TBL ((dh_t *)meta_table_g)

void meta_set_(dyn obj, dyn src) {
  /* Immediates have no GC identity.  The caller (the Symta-side
   * `meta` function) handles them via the legacy wrapper type
   * once Phase 3 lands; for now we silently ignore. */
  if (IMMEDIATE(obj)) return;
  if (!meta_table_g) meta_table_g = dhAlloc();
  dh_t *t = TBL;
  dhSet(&t, obj, src);
  meta_table_g = t;
}

dyn meta_get_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return No;
  dyn v = dhGet(TBL, obj);
  /* dhGet returns NIL on miss; map that to No. */
  return (v == NIL) ? No : v;
}

int meta_has_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return 0;
  return dhGet(TBL, obj) != NIL;
}

void meta_del_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return;
  dh_t *t = TBL;
  dhDel(&t, obj);
  meta_table_g = t;
}

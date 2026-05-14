/* See meta_table.h for the design rationale.
 *
 * Note: the GC integration lives in gc.c (function `gc_meta_table`),
 * because the `GC_REC` and `GC_REDIR` macros it needs are file-scoped
 * to that translation unit.  This file exposes the table via the
 * `meta_*` accessors and the `meta_table_g` symbol so gc.c can reach
 * the same pointer.
 *
 * Backing store: `ih_t` (runtime/ih.h) -- a `dyn`-keyed map with
 * identity hashing on the raw 64-bit dyn bits.  We deliberately
 * DON'T use `dh_t`: that map hashes keys via `MCALL m_hash`, which
 * for lists recurses into element hashes -- and `list.hash` blows
 * up the moment any element is a `float` (no `.hash` defined on
 * that type).  Identity-hashed keys also match the intended
 * "weakly held by GC identity" semantics: two AST nodes with the
 * same shape but built from different parses must have separate
 * meta entries, because they have separate source positions. */

#include "common.h"
#include "ih.h"
#include "meta_table.h"

/* The single global parser-meta table.  Stored as `void *` in the
 * header for ABI cleanliness; cast to `ih_t *` here for actual use. */
void *meta_table_g = 0;
#define TBL ((ih_t *)meta_table_g)

void meta_set_(dyn obj, dyn src) {
  /* Immediates have no GC identity.  The caller (the Symta-side
   * `meta` function) handles them via the legacy wrapper type
   * once Phase 3 lands; for now we silently ignore. */
  if (IMMEDIATE(obj)) return;
  if (!meta_table_g) meta_table_g = ihAlloc();
  ih_t *t = TBL;
  ihSet(&t, obj, src);
  meta_table_g = t;
}

dyn meta_get_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return No;
  dyn v = ihGet(TBL, obj);
  /* ihGet returns NIL on miss; map that to No. */
  return (v == NIL) ? No : v;
}

int meta_has_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return 0;
  return ihGet(TBL, obj) != NIL;
}

void meta_del_(dyn obj) {
  if (!meta_table_g || IMMEDIATE(obj)) return;
  ih_t *t = TBL;
  ihDel(&t, obj);
  meta_table_g = t;
}

/* Weak hashtable from heap-object dyn keys -> arbitrary dyn values.
 *
 * Single global instance.  Originally built for the parser to attach
 * source-position triples to AST nodes (Phase 1-5 of the May 2026
 * reader consolidation); that use case ultimately switched to a
 * wrapper-meta type allocated in C (see commit 2129767 / docs/
 * reader-consolidation.md) because the per-AST-node hashtable lookup
 * tanked cold compile time.  The table API is kept around as the
 * subject under investigation for the slowdown: see
 * tests/runtime/wh-test.s for the GC-invariant suite and
 * benchmark/wh/ for adversarial-key timing.
 *
 * Keys are weakly held: during GC, entries whose key isn't otherwise
 * reachable get dropped automatically.  The value side is strongly
 * held: as long as a key survives, its associated value is kept
 * alive (and forwarded across GCs).
 *
 * Immediates (int / fixtext / No / T_TAG / ...) have no GC identity,
 * so `meta_set_` is a no-op on them.  The caller (Phase 3+ Symta
 * `meta` function) handles immediates via the legacy wrapper type. */

#ifndef META_TABLE_H
#define META_TABLE_H

#include "symta.h"

void meta_set_(dyn obj, dyn src);
dyn  meta_get_(dyn obj);
int  meta_has_(dyn obj);
void meta_del_(dyn obj);
void meta_clear_(void);          /* wipe all entries (tests / reset) */
uint64_t meta_n_(void);          /* live entry count */

/* The single backing dh_t.  Forward-declared via `void *` so this
 * header doesn't have to drag in the dh.h template machinery; gc.c
 * (which already pulls in dh.h via am.h) casts back to `dh_t**` /
 * `dh_t*` as needed.  Don't touch from anywhere else; use the
 * meta_* accessors above. */
extern void *meta_table_g;

#endif

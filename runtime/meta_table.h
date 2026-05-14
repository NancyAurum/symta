/* Weak hashtable from heap-object dyn keys -> arbitrary dyn values.
 *
 * The parser uses one global instance of this to attach source-position
 * meta (a [row col file] triple) to each AST node without wrapping the
 * node in a `meta` struct.  Keys are weakly held: during GC, entries
 * whose key isn't otherwise reachable get dropped automatically.
 *
 * The value side is strongly held: as long as a key survives, its
 * associated value is kept alive (and forwarded across GCs).
 *
 * Immediates (int / fixtext / No / T_TAG / ...) have no GC identity,
 * so `meta_set_` is a no-op on them.  The caller (Phase 3+ Symta
 * `meta` function) handles immediates via the legacy wrapper type.
 *
 * Single global instance for Phase 1+2.  A first-class
 * user-instantiable weak-hashtable type can come later if other
 * subsystems want it; right now only the parser needs this. */

#ifndef META_TABLE_H
#define META_TABLE_H

#include "symta.h"

void meta_set_(dyn obj, dyn src);
dyn  meta_get_(dyn obj);
int  meta_has_(dyn obj);
void meta_del_(dyn obj);

/* The single backing dh_t.  Forward-declared via `void *` so this
 * header doesn't have to drag in the dh.h template machinery; gc.c
 * (which already pulls in dh.h via am.h) casts back to `dh_t**` /
 * `dh_t*` as needed.  Don't touch from anywhere else; use the
 * meta_* accessors above. */
extern void *meta_table_g;

#endif

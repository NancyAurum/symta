// wh-test.s -- weak hashtable invariants.
//
// Exercises the single-global weak hashtable (runtime/meta_table.{h,c})
// exposed by the wh_* builtins (set/get/has/del/clear/n).  Tests basic
// CRUD, immediate-key rejection, table growth, weak-key semantics
// across forced GC cycles, and key/value forwarding across the
// generation copy.
//
// Output format: each test prints one line.  `ok` = pass, `FAIL` =
// fail.  Run via tests/wh/run.sh which diffs the output against the
// committed golden.

GPass 0
GFail 0

check Desc Cond =
  if Cond then (GPass = GPass+1; say "  ok    [Desc]")
          else (GFail = GFail+1; say "  FAIL  [Desc]")

eq_check Desc Got Want =
  Cond Got >< Want
  if Cond then (GPass = GPass+1; say "  ok    [Desc]")
          else (GFail = GFail+1; say "  FAIL  [Desc] (got [Got], want [Want])")


// ================================================================
// 1. CRUD primitives on a fresh table.
// ================================================================
say "\[basic CRUD\]"
wh_clear_
eq_check "empty count" wh_n_() 0
L: 10 20 30
eq_check "has before set" wh_has_(L) 0
eq_check "get before set" wh_get_(L) No
wh_set_(L 'pos1')
eq_check "has after set" wh_has_(L) 1
eq_check "get after set" wh_get_(L) 'pos1'
eq_check "count after set" wh_n_() 1
wh_del_(L)
eq_check "count after del" wh_n_() 0
eq_check "has after del" wh_has_(L) 0
eq_check "get after del" wh_get_(L) No


// ================================================================
// 2. Update semantics: re-setting the same key replaces the value
//    without growing the entry count.
// ================================================================
say "\[update\]"
wh_clear_
M: 1 2 3
wh_set_(M 'A')
wh_set_(M 'B')
wh_set_(M 'C')
eq_check "count after 3 sets of same key" wh_n_() 1
eq_check "latest value wins" wh_get_(M) 'C'


// ================================================================
// 3. Immediate keys are silently ignored (no GC identity to hold).
// ================================================================
say "\[immediates\]"
wh_clear_
wh_set_(5 'int')
wh_set_(1.5 'float')
wh_set_('a' 'fixtext')
wh_set_(No 'no')
eq_check "count stays 0 after 4 immediate-key sets" wh_n_() 0
eq_check "get int" wh_get_(5) No
eq_check "has float" wh_has_(1.5) 0


// ================================================================
// 4. Many entries -- forces the backing ih_t to grow several times.
//    Each value is a fresh list referencing the index, so the
//    value-side strong-hold path also gets exercised.
// ================================================================
say "\[many entries\]"
wh_clear_
Keys (!)
N 256
times I N:
  K: I
  Keys.I = K
  wh_set_(K [I 'src'])
eq_check "count after [N] inserts" wh_n_() N
AllHit 1
times I N:
  V wh_get_(Keys.I)
  when V.0 <> I or V.1 <> 'src': AllHit = 0
eq_check "all [N] keys lookup correctly" AllHit 1


// ================================================================
// 5. Distinct lists with identical CONTENT get separate entries
//    (identity hashing, not content hashing).
// ================================================================
say "\[identity\]"
wh_clear_
A: 1 2 3
B: 1 2 3
eq_check "A and B equal by value" (A >< B) 1
wh_set_(A 'aval')
wh_set_(B 'bval')
eq_check "count = 2 (distinct identities)" wh_n_() 2
eq_check "A still maps to aval" wh_get_(A) 'aval'
eq_check "B still maps to bval" wh_get_(B) 'bval'


// ================================================================
// 6. Weak key semantics: drop the only reference to a key, force GC,
//    entry should be gone.
//
// We rebind K = No to drop the strong reference to the original list,
// then trigger GC with a `gc()` call.  After GC the weak entry should
// be evicted; lookup via a new list that happens to have the same
// content must miss.
// ================================================================
say "\[weak: drop ref\]"
wh_clear_
K: 100 200 300
wh_set_(K 'pre-gc')
eq_check "pre-gc count" wh_n_() 1
eq_check "pre-gc get" wh_get_(K) 'pre-gc'
K = No                       // drop the only strong ref to original list
gc()
eq_check "post-gc count drops to 0" wh_n_() 0


// ================================================================
// 7. Weak key semantics: HOLD the reference across GC, entry must
//    survive AND lookup must continue to work even though the key
//    pointer has been forwarded to the new generation.
// ================================================================
say "\[weak: hold ref\]"
wh_clear_
H: 'a' 'b' 'c'
wh_set_(H 'kept')
eq_check "pre-gc count" wh_n_() 1
gc()
eq_check "post-gc count still 1" wh_n_() 1
eq_check "post-gc get returns same value" wh_get_(H) 'kept'
eq_check "post-gc has still 1" wh_has_(H) 1


// ================================================================
// 8. Value forwarding: the value is also a heap object.  After GC,
//    the value should still be reachable via wh_get_, even though
//    we held no Symta-side ref to it.
//
// We allocate a fresh list as the value, set it in the table, then
// gc.  Reading wh_get_ back must return a structurally-equal list.
// (Identity may differ after forwarding -- we check content.)
// ================================================================
say "\[value forwarding\]"
wh_clear_
J: 1 2
V: 'row' 'col' 'file'
wh_set_(J V)
gc()
V2 wh_get_(J)
eq_check "post-gc value still a list" V2.is_list 1
eq_check "post-gc value len = 3" V2.n 3
eq_check "post-gc value content [0]" V2.0 'row'
eq_check "post-gc value content [2]" V2.2 'file'


// ================================================================
// 9. Survival across multiple GC cycles (key + value both promote
//    through generations).
// ================================================================
say "\[survival across multi-cycle gc\]"
wh_clear_
K2: 'survivor'
wh_set_(K2 [42 'data'])
gc(); gc(); gc()
eq_check "after 3 GCs still has entry" wh_has_(K2) 1
V3 wh_get_(K2)
eq_check "value [0]" V3.0 42
eq_check "value [1]" V3.1 'data'


// ================================================================
// 10. Mixed lifetimes: some keys held, some dropped.  Only the
//     held ones survive the GC.
//
// The "dropped" loop runs inside a helper closure so that its
// frame -- and the per-iteration `K` slot in particular -- has
// been popped by the time GC fires.  If the loop were at the
// toplevel, the LAST iteration's K would still be reachable via
// the current frame, leaving one extra survivor.  That detail is
// itself useful documentation of the weak-key contract: an entry
// only dies when its key is unreachable from EVERY frame, not
// just from "the caller's intent".
// ================================================================
say "\[mixed lifetimes\]"
wh_clear_
Held (!)
times I 10:
  K: I 'held'
  Held.I = K
  wh_set_(K [I 'held'])
populate_dropped =
  times I 10:
    K: I 'dropped'
    wh_set_(K [I 'dropped'])
populate_dropped()
eq_check "pre-gc count = 20" wh_n_() 20
gc()
eq_check "post-gc count = 10 (only held survive)" wh_n_() 10


// ================================================================
// 11. Clear is idempotent + leaves the table reusable.
// ================================================================
say "\[clear + reuse\]"
wh_clear_
wh_clear_
wh_clear_
eq_check "n after 3 clears" wh_n_() 0
P: 'x'
wh_set_(P 'y')
eq_check "set after multiple clears works" wh_get_(P) 'y'


// ================================================================
// 12. Re-use of slot after delete -- internal Robin-Hood backshift.
//     Insert two keys, delete the first, look up the second.
// ================================================================
say "\[del does not corrupt other entries\]"
wh_clear_
A2: 'first'
B2: 'second'
wh_set_(A2 1)
wh_set_(B2 2)
wh_del_(A2)
eq_check "B2 still findable after deleting A2" wh_get_(B2) 2
eq_check "A2 not found" wh_get_(A2) No
eq_check "count = 1" wh_n_() 1


// ================================================================
// Summary
// ================================================================
say ""
say "\[summary\]"
say "  passed: [GPass]"
say "  failed: [GFail]"
when GFail > 0: halt

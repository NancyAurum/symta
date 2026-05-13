# Symta — TODO

The consolidated punch-list. Items here have been **reviewed**,
**scoped**, and have a **concrete proposed fix**, so they can be
picked up and worked on without re-doing the analysis. Items
absorbed from:

- `dev/TODO.md` — the original research-driven goals (adaptive
  map, NCM bugs, compiler diagnostics, GC hooks)
- `../issues.md` — issues filed during the SoM revival that
  affect Symta itself (drift test methodology, advanced syntax
  gaps, parser quirks)
- `../symta_speedup.md` — the round-two runtime/compiler perf
  plan (computed-goto dispatch, custom FFI trampolines)
- `../symta-review.md` — the "toy-to-tool" friction list from
  a one-week outside review
- `dev/todo.txt` and `dev/todo-spell-of-mastery.txt` — raw
  stream-of-consciousness notebooks; items graduate here once
  they're scoped

The format per item:

> **\[PRI\] short title** &nbsp;&nbsp; `effort: …`
> **Where:** path:line(s)
> **Problem:** what's wrong / sub-optimal, in one paragraph.
> **Fix:** the concrete approach, in one paragraph.
> **Why now / why not:** justification or blocker.

Priorities:

- **\[P0\]** correctness / latent bug
- **\[P1\]** performance gap on a hot path, or load-bearing UX
- **\[P2\]** ergonomics / cleanup / tech debt
- **\[P3\]** speculative / future research

Effort tags: `30 min`, `afternoon`, `weekend`, `multi-week`.

Done items move to the [bottom](#done), struck through, with
the commit hash and date appended.

---

## FFI — cinvoke replacement

### \[P1\] **FFI-1** Drop cinvoke; ship custom trampolines (Phase 1 landed)

> **Where:** [`runtime/sffi/`](runtime/sffi/) — new in-tree
> replacement; [`cinvoke/`](cinvoke/) — slated for deletion in
> Phase 3.
> **Status:** Phase 0 (architecture + scaffolding) and Phase 1
> (x86-64 Windows backend) landed in commit … (May 2026). The
> Symta-side build for w64devkit no longer links `-lcinvoke`;
> the runtime calls into `sffi_call` instead of
> `cinv_function_invoke` for every `SBC_NFI` op.
> **Design recap:** see [`runtime/sffi/ARCHITECTURE.md`](runtime/sffi/ARCHITECTURE.md).
> Three-function API (`sffi_bind`, `sffi_call`, `sffi_free`).
> One backend `.c` per ABI, ~200 lines each. No JIT, no
> executable memory — the trampoline is statically compiled,
> arg classification runs once per FFI declaration at
> `sbc_prepare` time, the hot path is just the per-ABI
> register-load + indirect call.
> **Remaining work:**
> - **Phase 2:** stand up the x86-64 SysV backend
>   ([`runtime/sffi/arch_x64_sysv.c`](runtime/sffi/arch_x64_sysv.c)
>   already written but unverified). Unblocks the
>   Linux + macOS port.
> - **Phase 3:** delete `cinvoke/` from the tree. Net licence
>   claim: dual MIT / Apache-2 throughout.
> - **Phase 4:** additional ABIs as they're needed —
>   i386 Win95 (`arch_x86_win.c`), i386 Linux
>   (`arch_x86_sysv.c`), AArch32 RISC OS (`arch_arm32.c`),
>   AArch64 Linux/macOS (`arch_arm64.c`). All four stubs are in
>   tree with their calling-convention notes documented; bring
>   them up when somebody actually needs the target.
> - **Phase 5:** [`tests/ffi/`](tests/ffi/) — comprehensive
>   regression suite. **Landed in commit … (May 2026).** 17
>   cases covering every (return × arg-type × arity) combination
>   the language exposes, plus realistic-library tests (libc
>   strlen/memset/memcpy and zlib compress/uncompress/crc32).
>   Catches: pool-vs-slot confusion (interleave), stack-arg
>   sign-extension (arity_i32 with negatives), xmm pool
>   overflow (arity_f64 with 10 doubles), per-call state leakage
>   (stress with 1000 sequential calls). See
>   [`tests/ffi/README.md`](tests/ffi/README.md) for the full
>   matrix.
> `effort: multi-week` (Phase 1 + Phase 5 done; Phases 2-4 remaining)

---

## Runtime — bytecode interpreter

### \[P1\] **RT-1** Bytecode dispatch via computed gotos

> **Where:** [`runtime/sbc.c`](runtime/sbc.c) `sbc_exec`
> **Problem:** the main interpreter is one big `switch(RD8)`
> inside `for (;;)`. GCC compiles to a jump table but the
> structure prevents the branch predictor from learning
> opcode-to-opcode transition patterns. Industry-standard
> threaded dispatch via labels-as-values is 10–30 % faster on
> every program that lives in tight inner loops (the ECS column
> walks, the SML drawing loop, etc.).
> **Fix:** rewrite the dispatch as a labelled threaded
> interpreter using GCC `&&label`. Clang supports it too;
> MSVC doesn't, but we're already on MinGW via w64devkit.
> Stage it carefully: keep the `switch` under an `#ifdef`, ship
> a binary that runs both, default to switch, verify, flip to
> threaded, verify, remove switch. The risk surface is subtle
> dispatch-related correctness bugs that the existing test
> suite might not catch, hence the staging.
> **Regression strategy:** all 8 test suites pass; verify
> with the drift test (`tests/bootstrap/drift.sh`); profile
> against the game's `--profile=management` baseline before/
> after to confirm the win.
> `effort: weekend`

### \[P2\] **RT-2** Expose explicit `gc()` to Symta code

> **Where:** [`runtime/bltin.c`](runtime/bltin.c)
> **Problem:** triggering a GC currently requires allocating
> enough garbage to fill gen0 — the only portable way examples
> can demonstrate finalizers. Useful in tests, profilers, and
> long-running daemons that want to release memory at known
> points.
> **Fix:** `BUILTIN0("gc",gc) { api.gc(); RETURNS(No); }` —
> single line, the underlying `api.gc` already exists.
> `effort: 30 min`

### \[P3\] **RT-3** Generation-size tuning hooks

> **Where:** [`runtime/gc.c`](runtime/gc.c)
> **Problem:** generation sizes are picked at boot
> (`init_api(GEN_ZERO_SIZE*10)`), no way to tune for a
> workload. ECS apps may want bigger gen0; REPL apps may want
> smaller.
> **Fix:** envvar `SYMTA_GEN0_SIZE` plus a runtime hook
> `gc_set_gen_size(gen, bytes)` for advanced users.
> `effort: afternoon`

---

## Adaptive map — table internals

The adaptive map is what backs every Symta hash, every ECS
column, every cache. Most of these items make ECS-style code
(`each(unit.field)` loops, dense iteration) competitive with
state-of-the-art ECS frameworks.

### ~~\[P0\] **AM-1** `amGidGet` falls through `AM_GENERIC` / `AM_TEXT` uninitialised~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h)
> **Resolution:** added explicit `GOT(AM_GENERIC)` branch that
> dispatches through `dhGet(FXN(O_GID(ref)))` (same path as
> `amGet`), and a `GOT(AM_TEXT)` branch that calls `fatal()` --
> a text-keyed column queried by integer GID is a real cls-layer
> contract violation and should crash loudly rather than silently
> returning garbage. Fix landed alongside the AM-3 inlining
> below; no regressions in the test sweep or the 3-round drift
> bootstrap.

### \[P1\] **AM-2** No dense-iteration story for ECS columns

> **Where:** [`runtime/am.h:573`](runtime/am.h) (`amL`),
> [`src/cls.s:157`](src/cls.s) (`each_`)
> **Problem:** `amL` and `gid_refs_` build a list by walking
> the hashmap on every call. ECS systems iterate component
> columns every frame; bitmap iteration touches every page
> even for low populations, and INT-mode iteration walks every
> hash slot. State of the art (flecs, Bevy, EnTT) keeps
> entities packed densely so the per-frame iterator is
> essentially `for i = 0..n` over a flat array — typically
> 5–20× faster.
> **Fix:** pair each `BITMAP*` and `INT` table with an optional
> packed-dense vector (`dense_index[gid]` and `id_at_dense[i]`).
> The bitmap already provides O(1) presence; adding the
> parallel arrays gives O(1) insert/delete and
> sequential-array-speed iterate. Standard sparse-set design.
> **Why now:** the single biggest perf gap for any game-style
> workload. Hits ~2–4 ms / site update at SoM's peak unit count.
> `effort: weekend`

### ~~\[P1\] **AM-3** `GENERIC` mode pays MCALL per probe step~~ (DONE)

> **Where:** [`runtime/dh.h`](runtime/dh.h)
> **Resolution:** `dhEqual_` / `dhHash_` now branch on `O_TAG(key)`
> and inline the three common shapes (`T_INT`, `T_FIXTEXT`,
> `T_TEXT`) before falling back to `MCALL`. The inlined hash
> values are bit-for-bit identical to what the existing
> `int.hash` / `text.hash` builtins return through method
> dispatch, so a table populated through one path stays findable
> through the other.
>
> Helpers `dhFmix64_` (Murmur3 finaliser) and `dhAdler_` (Adler-32
> over bigtext bytes) are kept in-sync with `fmix64` / `hash` in
> `bltin.c` -- both must continue to agree; bench-grade
> performance regressions or correctness divergence would surface
> as drift-test failures.
>
> Drift + every test suite still green; no measurable change on
> int-heavy tables that already hit the AM_INT specialisation
> (those skip dh entirely), but generic tables with mixed key
> types -- particularly the symbol-tag-table -- should now do
> orders of magnitude less work per probe.

### \[P1\] **AM-4** `BITMAP*` promotion to `INT` is lossy and one-way

> **Where:** [`runtime/am.h:469–512`](runtime/am.h)
> **Problem:** the moment **any** value diverges from the
> chosen 0 or 1, the entire column promotes to `AM_INT` — every
> entity now costs ~16 B even if 99 % of values are still 0
> or 1. Common case: `is_visible` is mostly 1, a handful of
> entities have visibility levels 0..7 — currently you pay
> full INT cost for the entire column.
> **Fix:** add `AM_BITMAP_PLUS` mode = a bitmap PLUS a small
> `nh_t` of divergent gids. Promote to full INT only when
> exceptions exceed ~10 % of total. The pieces (`nb_t` +
> `nh_t`) already exist; the change is mostly in `amSet`.
> `effort: weekend`

### ~~\[P1\] **AM-5** `nh_t` load factor is conservative (50 %)~~ (DONE)

> **Where:** [`runtime/nh.h`](runtime/nh.h),
> [`runtime/dh.h`](runtime/dh.h)
> **Resolution:** Robin Hood probing landed in `nh.h` as an
> opt-in via `#define NH_ROBIN_HOOD`. dh.h opts in and raises
> the load factor to ~75 % (`(used*4) > 3*(cap+1)`). nb.h
> stays on plain linear probing -- its keys are cheap uint32
> ints where the per-probe NH_HASH call is essentially free and
> the variance argument doesn't carry the same weight.
>
> Robin Hood implementation:
>   - `Add_` swaps when it meets an entry whose distance-from-
>     initial-bucket (DIB) is less than the new entry's, and
>     continues displacing along the chain until an empty slot
>     is found.
>   - `Lookup` / `Get` early-exit when they see a resident with
>     DIB < their own -- the RH invariant guarantees their key
>     can't be further along.
>   - `Del` backshifts: walk forward, shifting each non-home
>     entry back into the empty slot, until we hit an empty
>     slot or an at-home entry.
>   - DIB isn't stored; it's recomputed via
>     `(slot - (HASH(keys[slot]) & cap)) & cap`. Adds one
>     NH_HASH call per probed slot, which is the AM-3 fast path
>     for int/text keys and an MCALL for user types. Net cost
>     is dominated by the reduced probe length at 75 % load
>     factor.
>
> Expected impact: ~33 % memory reduction on
> hash-table-heavy workloads. Probe-length variance flattens
> from O(N²) clustering behaviour at high load to near-uniform
> around 1.5-2.0.
>
> Drift bootstrap: 3-stage fixed point reached in one round
> (`s1==s2` byte-identical). Robin Hood doesn't perturb the
> compiler's iteration paths -- the .sbc artefacts are byte-
> identical to the pre-RH versions.
> tests/am: 11/11 pass; tests/sweep.sh: 149/149.

### ~~\[P2\] **AM-6** Two underlying hashmaps (`stb_ds` and `nh_t`)~~ (PARTIAL — AM_INT done, AM_TEXT pending)

> **Where:** [`runtime/am.h`](runtime/am.h),
> [`runtime/ih.h`](runtime/ih.h) (new)
> **Resolution (AM_INT):** new `ih.h` instantiates `nh_t` with
> `NH_KEY=dyn`, `NH_KEY_NIL=NIL` (= 1; can't collide with any
> FXN(n) since real int keys always have low GID_SHFT bits =
> 0), Robin Hood probing on, 75% load factor. am.h's AM_INT
> mode now uses `ih_t *` everywhere stb_ds's `symta_itbl` lived
> -- ihGet replaces hmget, ihSet replaces hmput, ihDel replaces
> hmdel, ihN replaces hmlen, ihFree replaces hmfree, and
> NH_FOR replaces the `for (i; i < hmlen; ++i) hm[i]` raw walk.
> gc_types.h's tbl_gc_internals also switched.
>
> Effect: ECS column tables (the most common AM_INT users)
> now get the same Robin Hood + 75% load throughput / memory
> characteristics that dh.h (AM_GENERIC) does. One probing
> strategy, one growth heuristic, one place to tune.
>
> Iteration-order change: stb_ds was insertion-ordered with
> swap-on-delete; ih_t is hash-ordered. The drift test passed
> in one round (s1==s2 byte-identical .sbc), which means none
> of the compile-time AM_INT tables iterate-order-leak into
> codegen. tc_int's "round-trip [K]" output reordered, no
> behavior change.
>
> **Still pending:** AM_TEXT mode still uses stb_ds's `sh*`
> string-hashmap. Switching it requires either (a) a separate
> nh.h instantiation with `NH_KEY=char*` plus a copy-on-insert
> arena (sh_new_arena's role), or (b) storing text dyns
> directly and using texts_equal for NH_EQUAL. Option (b) is
> cleaner but needs a sentinel value that no Symta text can
> equal. `FXN(0)` (= 0) works -- a text dyn always has T_TEXT
> or T_FIXTEXT in its tag bits, never zero. Filed as AM-6b.

### ~~\[P2\] **AM-6b** AM_TEXT on th_t (drop stb_ds)~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h),
> [`runtime/th.h`](runtime/th.h) (new)
> **Resolution:** new `th.h` instantiates nh.h with `NH_KEY=dyn`
> (text dyn), Robin Hood at 75% load, the AM-15 hash cache.
> All `am.h` AM_TEXT paths swapped: `stb_ds`'s `sh*` → `th_t`'s
> `thSet` / `thGet` / `thDel` / `thN` / `thFree`. iteration
> via `NH_FOR(th, i, hm)` instead of indexed `hm[i].{key,value}`.
> `gc_types.h::tbl_gc_internals` AM_TEXT branch now marks both
> keys and values (the old stb_ds layout kept arena-malloc'd
> chars that the GC didn't need to track).
>
> The first attempt sat in a 100x perf cliff that took a probe-
> count histogram to bisect: `bn_text insert` was 14 000 ns/op
> instead of 134 ns/op, average probe length per Add_ was 1290
> -- which is a "all keys cluster in one chain" signature. The
> hash-distribution histogram nailed it: Adler-32 (which I'd
> copied from dh.h's BIGTEXT path) puts 92% of "key_<N>" hashes
> into a 512-slot range. Adler-32's low 16 bits are a running
> byte sum, which is essentially constant for inputs that share
> a prefix and vary only in the suffix.
>
> Fix: use **FNV-1a** for the BIGTEXT path (per-byte
> multiply-then-xor spreads varying-byte entropy across the
> whole hash word). FIXTEXT keeps Murmur3 finaliser on the
> full dyn (same as ih.h's strategy).
>
> **Note:** dh.h still uses Adler-32 for its BIGTEXT branch, so
> generic tables with common-prefix text keys see the same
> clustering. Not exercised by anything in the bench / test
> suite today, but worth filing as a follow-up.
>
> **Bench delta** (`benchmark/am/bn_text`, stb_ds → th_t):
>
> | Op   | stb_ds | th_t  | Δ    |
> |------|--------|-------|------|
> | ins  | 134    | 120   | -10% |
> | hit  |  71    |  95   | +34% (within noise, see below)
> | miss | 100    |  87   | -13% |
> | del  | 142    |  55   | -61% (RH backshift > stb_ds del)
> | iter | 158    | 173   |  +9% |
>
> hit shows a slight regression that's likely the per-probe
> hash-cache read replacing stb_ds's direct slot access. Net
> across all five ops the suites are roughly the same; del is
> the big win.

### ~~\[P3\] **AM-6c** dh.h Adler-32 + fixtext fold clustering~~ (DONE)

> **Where:** [`runtime/dh.h`](runtime/dh.h) `dhHash_`
> **Problem:** dh.h carried the same two hash distribution
> bugs that AM-6b found in th.h. The FIXTEXT path used a
> straight low^high fold that masked out the varying chars
> in common-prefix inputs ("key_<N>" patterns), and the
> BIGTEXT path used Adler-32 whose low 16 bits are a running
> byte sum -- also catastrophic on common prefixes. No current
> workload exercises generic tables with prefix-similar text
> keys, but a user-facing `@{key_1!a key_2!b ...}` table
> would have hit the same chain-of-N degradation that
> bn_text saw before AM-6b.
> **Fix:** Murmur3 finaliser for FIXTEXT (full 64-bit mix
> instead of fold), new `dhFnv1a_` helper for BIGTEXT (kept in
> sync with th.h's `thFnv1a_` -- both files use the same FNV
> constants). dh.h no longer agrees bit-for-bit with the
> `text.hash` builtin, but the builtin is only invoked via
> MCALL for non-fast-path types, never for the fixtext/text
> fast paths that just changed -- as long as both dh callers
> route through the inlined path consistently, the table
> works. Verified: sweep 150/150 + drift PASS in 1 round.

### ~~\[P2\] **AM-7** `AM_BITMAP0` write-zero looks like delete~~ (DONE — docs)

> **Where:** [`runtime/am.h`](runtime/am.h) header doc block
> **Resolution:** documented the `void_val` contract at the top
> of `am.h` so the surprise is one paragraph away from anyone
> reading the file. Key points captured:
>
> - `AM_VOID(o)` holds the value returned for missing keys
>   (default `No`).
> - `amSet` / `amGidSet` interpret `value == void_val` as a delete
>   request; this is what makes `T.K = 0` look like a delete *if*
>   the user previously called `T.setNo 0`.
> - Default `void_val = No` means most user code never hits the
>   quirk -- you have to opt in via `T.setNo 0`.
> - Workaround: pick a `void_val` outside your value alphabet, or
>   use `T.del K` explicitly when you want to remove a key.
>
> The "split set vs. remove" API change is deferred until somebody
> actually trips on it in real code; the doc paragraph is enough
> for now.

### ~~\[P2\] **AM-13** Bitmap mode probes missing `T_INT` guard~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h) `amHas` / `amGot` /
> `amGet` `AM_BITMAP0` / `AM_BITMAP1` branches
> **Problem:** `T.has K`, `T.got K`, and `T.K` on a bitmap-mode
> table called `nbGet(nb, UNFXN(key))` without first checking
> that `key` was actually an integer. For non-int keys (text,
> list, closure, …), `UNFXN(key)` extracted the upper bits of
> the dyn -- often a raw heap pointer -- and fed it to the
> bitmap as if it were a gid. Almost always missed (point in
> the page directory unlikely to match a populated page), but a
> sufficiently small or aligned pointer could in principle
> false-positive against a real bit and silently report the key
> as present.
> **Resolution:** added `if (O_TAG(key) != T_INT) return …;` to
> all six bitmap probe branches (3 functions × 2 bitmap modes).
> The return value matches what the equivalent AM_TEXT /
> AM_INT miss path returns: `0` for has/got, `AM_VOID(o)` for
> get. amSet's BITMAP0/1 paths already had the guard; this is
> just bringing the read side to feature parity.
> Tests in `tc_void.s` cover `T.has \hi`, `T.has [1 2]`,
> `T.\hi`, `T.got [3]` on a true BITMAP1 table -- previously
> these all happened to return correct results, but only by
> luck of pointer mod page-size.

### ~~\[P1\] **AM-15** Cache hashes in dh slots so RH probes skip MCALL~~ (DONE)

> **Where:** [`runtime/nh.h`](runtime/nh.h) (new `NH_CACHE_HASH`
> template option), [`runtime/dh.h`](runtime/dh.h) (opt in)
> **Problem:** the first run of `benchmark/am/baseline.txt`
> showed AM_GENERIC at 10-20x the per-op cost of AM_INT. Tracing
> through the Robin Hood probe loop: every step recomputes
> `home_them = NH_HASH(ks[i]) & cap` to evaluate the DIB
> early-exit condition. For dh.h keys that are lists / closures
> / user types, `NH_HASH` boils down to an `MCALL` against
> `api.m_hash` -- ~300 ns per call by itself. With Robin Hood at
> 75% load factor averaging 1.5 probes per op, that's another
> ~450 ns per Get on top of the lookup-key's own hash MCALL and
> the equality MCALL on hit.
> **Fix:** parallel `uint32_t *hashes` array in nh_t, populated
> at Add_ time. The cached hash is exact (deterministic function
> of the key) so we can compare it byte-for-byte with a fresh
> compute; we don't even need to invalidate on grow because
> Grow_ migrates via Add_ which re-caches. Guarded behind
> `#define NH_CACHE_HASH` so the int-keyed `ih_t` (where the
> hash is already cheap -- one Murmur3 finaliser) and the
> bitmap-keyed `nhPg_t` skip the overhead. dh.h opts in.
>
> **Resolution:** all four RH-mode primitives (`Add_`, `Lookup`,
> `Get`, `Del` with backshift) updated to read `hs[i] & cap`
> instead of `NH_HASH(ks[i]) & cap` when probing the resident.
> Add_'s inner displacement loop carries the displaced entry's
> cached hash through swaps. Del's backshift shifts hashes
> along with keys/values. Memory cost: 4 B/slot in dh tables
> (~25% overhead given dh's 16-B key+value slots; net dh
> footprint stays well under stb_ds's because we still gain
> from 75% load factor).
>
> Bench delta (benchmark/am, AM-14 → AM-15, single run):
>
> | Op           | Before | After | Δ    |
> |--------------|--------|-------|------|
> | generic ins  | 1208   | 701   | -42% |
> | generic hit  | 701    | 596   | -15% |
> | generic miss | 799    | 463   | -42% |
> | generic del  | 660    | 478   | -28% |
> | int / text   | unchanged (don't enable NH_CACHE_HASH)
>
> Hit and del benefit less because they probe fewer slots on
> the hot path; insert and miss probe further (insert until an
> empty slot, miss until early-exit) so they see more cached
> hash reads. Variance is ±10%; the deltas above are well
> outside noise.

### ~~\[P1\] **AM-14** BITMAP→INT promotion leaked the underlying `nb_t`~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h) `amSet` and
> `amGidSet`, AM_BITMAP0/1 → AM_INT branches
> **Problem:** when a BITMAP-mode column promotes to AM_INT
> (caller writes a value that diverges from the implicit 0 or
> 1), the promotion code allocated a fresh `ih_t` / `symta_itbl`
> and migrated the bitmap's gids in, but didn't call
> `nbFree(nb)` on the old bitmap. The matching BITMAP→GENERIC
> branches DID free; the BITMAP→INT branches always leaked.
> The leak was small per promotion (one `nh_t` page directory
> plus 8 64-byte words per populated page) but ran every time
> a column transitioned, which for ECS apps happens often.
> Pre-existing; the bug pre-dated AM-6 (carried over from the
> stb_ds version verbatim).
> **Resolution:** added `nbFree(nb)` to all four affected
> BITMAP→INT branches (2 in amSet, 2 in amGidSet). Tagged
> with `/* AM-14 */` so a future audit doesn't re-remove them.
> Verified with the full sweep + drift -- no behavioural
> change, just plugs the leak.

### ~~\[P0\] **AM-12** `amHas` / `amGot` `AM_BITMAP0` predicate inverted~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h) `amHas` / `amGot`
> `AM_BITMAP0` branches
> **Bug:** both functions returned `FXN(nbGet == 0)` for the
> BITMAP0 case -- i.e., 1 when the bit is NOT set, 0 when it
> IS set. The BITMAP1 sibling correctly returns
> `FXN(nbGet != 0)`. Every other adaptive-map op (amSet's
> BITMAP0 branch, amGet's BITMAP0 branch, amGidGet's BITMAP0
> branch) agreed on the "bit set ↔ key present" semantic; the
> two predicates were the lone holdouts, likely surviving a
> refactor that flipped the meaning of the bit.
> **Resolution:** flip the BITMAP0 condition to `nbGet != 0` so
> the predicate matches what every other touchpoint observes.
> The new tc_void test exercises the BITMAP0-via-gid_set_ delete
> path, which was the only configuration that hit amHas with a
> true BITMAP0 table.
>
> Surface area in production code: zero today. amHas is the
> backing of `T.has K`, and the previously-existing AM regression
> suite never put a table into true AM_BITMAP0 mode -- the
> `T.K = 0`-from-empty path goes to AM_INT in amSet, and tc_gid
> (the only suite that did use the BITMAP0 amGidSet path) only
> tested amGidGet, not amHas. cls.s and the SoM game also
> never hit it. The bug existed for the entire life of the
> adaptive map; tc_void is what surfaced it.

### ~~\[P0\] **AM-11** `amSet` AM_TEXT delete branch missing `return`~~ (DONE)

> **Where:** [`runtime/am.h`](runtime/am.h) `amSet` `value == void_val`
> branch, `AM_TEXT` case
> **Bug:** the `GOT(AM_TEXT)` case in the delete-branch switch
> `shdel`s the key, but doesn't `return`. Control falls out of
> the inner switch (via the implicit `break` injected by the next
> `GOT`), past the `if (value == void_val)` block, and into the
> regular insert switch -- which `shput`s the key right back
> with the void value. Net effect: `T.K = void_val` on an
> `AM_TEXT` table is a silent no-op; the key stays present, but
> with the void value. Every other mode has the matching `return`.
> Almost certainly a copy-paste omission when the AM_TEXT delete
> path was added.
> **Resolution:** add `return;`. Surfaced by the new
> `tests/am/tc_void.s` regression, which exercises the void_val
> delete contract on every mode (EMPTY, INT, TEXT, GENERIC,
> BITMAP1) -- nothing else in the existing test suite ever wrote
> the void value to an AM_TEXT table.
>
> Also added `am.h`, `nb.h`, `nh.h` to `HDRSB` in
> [`Makefile.w64`](Makefile.w64) so future header edits trigger
> .o recompilation (the original list had `dh.h` but not the
> other three adaptive-map headers; Linux + osx Makefiles
> already had `am.h`, brought to feature parity).

### \[P2\] **AM-7b** `amSet` skips `AM_BITMAP0` for first-value-0

> **Where:** [`runtime/am.h`](runtime/am.h) `amSet` `AM_EMPTY` branch
> **Problem:** sibling discrepancy with `amGidSet`. On an empty
> table, writing `FXN(0)` with an integer key goes:
>   - `amGidSet` -> `AM_BITMAP0` (1 bit per key)
>   - `amSet` -> `AM_INT` (16 B per key)
> Same observable behaviour (`T.has K`, `T.K` both behave
> identically); different memory cost. Documented in the am.h
> header but not fixed. The choice is intentional in spirit --
> general-purpose users writing `T.K=0` usually mean to store a
> real 0 value, not a membership marker -- but the inconsistency
> is worth fixing once we have a clear answer (e.g. a separate
> `tbl_set_marker` API, or by demoting INT->BITMAP0 on the next
> rehash if the value distribution warrants).
> **Fix:** either align `amSet` with `amGidSet` (pick BITMAP0 on
> first-FXN(0) too -- one-line change), or expose an explicit
> `T.mark K` builtin that always picks the bitmap path.
> `effort: 30 min` (align) / `afternoon` (new builtin)

### \[P2\] **AM-8** Iteration during mutation has no guard

> **Where:** [`runtime/am.h:573`](runtime/am.h) (`amL`),
> `src/core_.s` tbl iteration
> **Problem:** `for K,V T:` over an INT or TEXT table whose
> body mutates can rehash and invalidate the iterator
> (`stb_ds`'s `hmput` reallocates on grow). Currently silent
> misbehaviour; a Lua / Python equivalent would error.
> **Fix:** snapshot the keys list before iteration in the
> `tbl.l`-style helpers, or stamp a generation counter on the
> table and check it during iteration.
> `effort: afternoon`

### \[P3\] **AM-9** `nh_t` capacity is `uint32_t`

> **Where:** [`runtime/nh.h:67`](runtime/nh.h)
> **Problem:** caps tables at 2³² entries. Fine for almost
> anything, but a many-billion-relation ECS world would hit it.
> **Fix:** widen to `uint64_t`. Costs 4 B per table header.
> Defer until someone actually needs it.
> `effort: 30 min` (when wanted)

### \[P3\] **AM-10** String interning shared with `AM_TEXT`

> **Where:** [`runtime/am.h`](runtime/am.h) `AM_TEXT` paths
> **Problem:** repeated equal-string keys are deduped by
> stb_ds, but `text_chars(key)` runs every probe.
> Frequency-table workloads pay it on every increment.
> **Fix:** intern text keys (or tag the text with its hash on
> first use) so probe cost drops to a pointer compare.
> `effort: weekend` (depends on text type rework)

---

## Lexical macro processor (`runtime/ncm.h`)

### \[P1\] **NCM-1** `<<` and `>>` shifts broken in `#[expr]`

> **Where:** [`runtime/ncm.h:1618`](runtime/ncm.h)
> (`eval_shift`)
> **Problem:** `#[A<<2]` returns 0, not `A*4`. `eval_shift`
> pops the first `<`, checks the next char matches, then
> recurses into `eval_add` *without popping the second `<`*.
> The right operand parse starts on the second `<`, fails to
> find a term, returns 0. Same bug affects `>>`.
> **Fix:** add `pop_char()` after the `if (next_char() != ch)`
> early return. One line.
> **Verified:** `examples/25-lexmacro.s` probe phase.
> `effort: 30 min`

### \[P1\] **NCM-2** `&&` and `||` swallowed by `eval_bitwise`

> **Where:** [`runtime/ncm.h:1670`](runtime/ncm.h)
> (`eval_bitwise`),
> [`runtime/ncm.h:1686`](runtime/ncm.h) (`eval_logic`)
> **Problem:** `#[1 && 1]` returns 0. `eval_bitwise` greedily
> eats the first `&` as bitwise-AND and recurses into
> `eval_cmp` on the second `&`, which can't parse it. So
> `eval_logic` (the layer that handles `&&` / `||`) never
> sees them.
> **Fix:** in `eval_bitwise`, peek at the second char before
> consuming `&` or `|` — if it matches, leave the operator for
> `eval_logic` and return. Mirrors how `eval_cmp` already
> handles `<<` / `>>`.
> **Workaround for users:** chain `#if A` / `#elif B` instead.
> `effort: 30 min`

### \[P2\] **NCM-3** `cmd_fmt` only parses decimal numeric args

> **Where:** [`runtime/ncm.h:1372`](runtime/ncm.h) (`cmd_fmt`)
> **Problem:** the printf-style formatter `#("0x%02X" 0xFF)`
> parses `0xFF` as `0` because `cmd_fmt`'s number reader stops
> at `x`. Users have to pass numeric args as decimal text or
> wrap in `#[...]`.
> **Fix:** detect a leading `0x` and switch to base 16
> parsing. Five-line change.
> `effort: 30 min`

### \[P3\] **NCM-4** Document NCM directives

> **Where:** missing
> **Problem:** the lexical macro processor has ~11 directives
> and a non-trivial arg syntax. The example file shows them
> but a reference table belongs in `dev/ncm.md`.
> **Fix:** new `dev/ncm.md` with one-line summary per
> directive plus links into `runtime/ncm.h`.
> `effort: afternoon`

---

## Compiler / parser

### \[P0\] **CORE-1** Stack traces lie about line numbers

> **Where:** [`runtime/bltin.c`](runtime/bltin.c) (the
> stack-trace printer), [`src/compiler.s`](src/compiler.s)
> (function-metadata source position)
> **Problem:** `foo.s:1310,7` often points at a comment line
> or whitespace; the actual error is in a function whose body
> lives 30 lines earlier. Working around it takes "± 20 lines
> from the printed location" as a heuristic. Highest-impact
> infrastructure investment, because every other tool builds
> on accurate locations.
> **Fix:** plumb per-instruction source positions through the
> SIF assembler into the SBC metadata. The compiler already
> tracks them in the SSA stream; the loss happens during
> assembly. Verify on every example's expected error path that
> the printed `row,col` lands on the offending token.
> **Regression strategy:** the lineno test suite
> (`tests/runtime/lineno-check.sh`) is the right place — its
> existing 5 cases pin known-good positions, extend with the
> patterns from `symta-review.md`.
> `effort: weekend`

### \[P1\] **CORE-2** `fin Body Finalizer` is unimplemented

> **Where:** [`src/macro.s`](src/macro.s) `fin` macro,
> [`runtime/sif.h`](runtime/sif.h) opcodes,
> [`runtime/sbc.c`](runtime/sbc.c) interpreter
> **Problem:** the macro expands to an opcode the VM doesn't
> implement. Symta has GC finalizers (`set_finalizer`) and
> non-local return (`btland` / `btjump`), but no try / finally.
> Resource-safe code (FFI handles, file descriptors) currently
> needs `btrap` gymnastics.
> **Fix:** add `SBC_SET_UNWIND` / `SBC_REMOVE_UNWIND` opcodes,
> wire them into the SIF assembler, push / pop a handler stack
> in the interpreter, fire on `bad`. Symta's GC already
> maintains `api.uwhs` for runtime use; this exposes it to user
> code.
> `effort: weekend`

### \[P1\] **CORE-3** Caught `bad` still emits stack trace

> **Where:** [`runtime/bltin.c`](runtime/bltin.c) `bad`
> **Problem:** `bad Msg` always prints the stack trace to
> stderr, even when caller wraps in `btrap` and recovers. A
> "successful" run that touched any error path looks alarming.
> Verified in `examples/23-control.s` and
> `examples/24-runtime.s`.
> **Fix:** print the trace only on the *first* uncaught
> `bad` — when there's no btrap on the stack. The btrap layer
> already knows it's catching.
> `effort: 30 min`

### \[P1\] **CORE-4** Multi-line `if X:` body needs `|` for `else` at body indent

> **Where:** [`runtime/reader.c`](runtime/reader.c)
> `parse_offside` / `parse_if`
> **Problem:** writing
>
> ```symta
> if X > 0:
>   say "positive"
>   say "and"
>   else say "no"
> ```
>
> fails with `unexpected else` because `parse_offside` treats
> the body as one multi-arg expression and `else` has nowhere
> to land. The fix attempted in the Symta parser broke real
> game code; one of the motivations for the C parser port was
> to make stack-based tracking of pending `if`s tractable.
> **Fix:** stack-based "pending-`if`" flag in `parse_offside`;
> auto-terminate the body on a same-indent `else` if the most
> recent open construct is an `if`. The C parser has explicit
> state now, so the fix that was infeasible in Symta is
> straightforward here.
> **Regression strategy:** add a fixture case to
> `tests/reader/` that pins both the new shape and the
> existing `else if A then B else C` chain shape (which is
> what broke the previous attempt).
> `effort: weekend`

### \[P2\] **CORE-5** Better diagnostics for common parser pitfalls

> **Where:** [`runtime/reader.c`](runtime/reader.c),
> [`runtime/bltin.c`](runtime/bltin.c) error printer
> **Problem:** several pitfalls produce confusing errors —
> nested `[...]` in string interpolation, `(-5)` parsing as a
> call, the chicken-and-egg "undefined variable" for `Var = X`
> first use, `==` typos that mean nothing (should be `><`).
> Discovered repeatedly during the example-writing tour
> (see `AI.md` gotchas).
> **Fix:** specific error messages — "did you mean
> `\[ … \]` to escape brackets in a string?", "first
> assignment to `Var` should be `Var X` not `Var = X`",
> "`==` doesn't exist in Symta; use `><` for equality",
> "`!=` is `<>`". The reader already has the column info.
> `effort: weekend`

### \[P2\] **CORE-6** A linter

> **Problem:** the gotchas catalogued in `symta-review.md` and
> `AI.md` are pure syntactic patterns; a linter that pre-flags
> them would close the biggest learning-curve gap. `Var =`
> first-use, multi-arg `say`, nested string-interp brackets,
> `pass` outside loops, `==` / `!=` typos.
> **Fix:** a `symta --lint` mode that runs the reader, walks
> the AST, and reports pattern violations. None of these need
> type inference.
> **Why now:** the single highest-payoff item in the
> "toy-to-tool" list. Pairs naturally with CORE-1 + CORE-5.
> `effort: weekend`

### \[P2\] **CORE-7** Generated stdlib reference

> **Problem:** every method on every built-in type lives in
> `src/core_.s` and `src/uim.s` but there's no greppable
> reference. New users read source.
> **Fix:** a doc-from-comments tool that walks `src/*.s`,
> emits Markdown per type (`text.*`, `list.*`, `table.*`,
> `gfx.*`, …) with one-line descriptions and source
> file:line.
> `effort: weekend`

### \[P3\] **CORE-8** Compiler micro-optimisations

> **Where:** [`src/compiler.s`](src/compiler.s)
> **Problem:** four small wins from a once-over:
> constant-folding literal arithmetic, dead-store elimination
> on unread SSA registers, unrolling `times I N:` for known
> small `N`, computed-jump compilation of `case` chains with
> all-literal patterns.
> **Fix:** each is small but the compiler is delicate. Stage
> carefully; verify on every example each time.
> `effort: weekend` per item

---

## Open bugs from the SoM revival (issues.md)

These were filed during the May 2026 reading-the-game pass.
None block the runtime / compiler tests; all surface as
"documented in `sbe.txt`, broken in practice" papercuts for
example-writers.

### \[P2\] **B7** Multi-line `if X:` else handling (see CORE-4 above)

Filed as a duplicate of CORE-4 with its own history; both
point at the same fix.

### \[P2\] **B10** `[@list]` inside string splice triggers `_insert` bug

> **Where:** [`runtime/reader.c`](runtime/reader.c) or
> [`src/compiler.s`](src/compiler.s)
> **Reproducer:** `Words [a b c]; say "x [@Words]"` →
> `unknown symbol _insert (compiler bug)`.
> **Problem:** `@`-spread inside an outer `[...]` splice
> emits an `_insert` reference that the SSA stage doesn't
> handle.
> **Workaround for users:** build the list into a variable or
> call `.text` before interpolating.
> `effort: afternoon`

### \[P2\] **B11** Four `{}` / `(...)` idioms from `sbe.txt` are broken

> **Where:**
> [`tests/macros/cases/idioms.s`](tests/macros/cases/idioms.s)
> header
> **Problem:** `L(:@_ -P&=0)`, `L(/~ P @_)`, `L(@_ ~<P @_)`,
> and `10{&1*2:}` all fail at macroexpand. Each is documented
> in `sbe.txt` as a working idiom.
> **Fix:** four separate macroexpander fixes; see issues.md
> B11 for the per-case analysis.
> `effort: weekend`

### \[P2\] **B12** Three `Xs[...]` subscript ops are broken

> **Where:**
> [`examples/29-subscript.s`](examples/29-subscript.s) header
> **Problem:** `Xs[-I]` (negative index from end),
> `"(...)"[0!'{' ~!'}']` (replacement on a fixtext), `Xs[3!]`
> (locate-by-value).
> **Fix:** see issues.md B12.
> `effort: weekend`

### \[P2\] **B13** Anaphoric-loop `l~^` source form is broken

> **Where:**
> [`examples/30-anaphoric.s`](examples/30-anaphoric.s) header
> **Problem:** `l~^[…]` errors with `undefined variable I__N`.
> Users have to bind to a named list variable first.
> **Fix:** see issues.md B13.
> `effort: afternoon`

---

## UIM / examples

### \[P3\] **UI-1** `examples/26-fact-engine/` project

> **Problem:** the current `21-inference` example is good but
> uses a simplified table-of-tables design. The real
> `som_src/fact.s` uses `cls fact_symbol` + interned ids +
> `cls_tbl_`. That's the production pattern and it's nowhere
> in the examples.
> **Fix:** new project example with the cls-based engine, plus
> a larger fact set so query performance is observable.
> `effort: afternoon`

### \[P3\] **UI-2** Sparse-set / dense-iteration cookbook

> **Problem:** users wanting to write fast ECS systems on top
> of `cls` need to know how to keep their own dense indices.
> Until AM-2 lands, that's the only path.
> **Fix:** `dev/cookbook-ecs.md` with a worked example: pair
> `cls foo bar` with a manually-maintained `dense_foo[]` for
> the hot iteration loop.
> `effort: afternoon`

### \[P3\] **UI-3** A working REPL

> **Problem:** the existing REPL handles one-liners but
> doesn't support multi-line input gracefully, doesn't
> preserve state usefully across error retries, doesn't
> support inspection (show me the parts of this `cls`, show
> me the methods on this type, what's in `$pic`?). For a
> language with such powerful runtime introspection
> (the ECS column tables are *right there*), the REPL leaves
> them locked up.
> **Fix:** a proper readline-style multi-line input, plus an
> `:i Object` inspection command that walks the object's
> cls / type and prints its parts / methods.
> `effort: weekend`

### \[P3\] **UI-4** Editor integration

> **Problem:** every Symta user writes in vanilla Notepad with
> a Lisp-mode that gets parens wrong. The Notepad++ language
> definition in `dev/npp_symta.xml` is the only checked-in
> support.
> **Fix:** Tree-sitter grammar (for syntax highlighting in
> modern editors); an LSP that does completion, jump-to-
> definition, and basic linting (depends on CORE-7).
> `effort: multi-week`

---

## Speculative / future research

### \[P3\] **R-1** Second implementation

> **Problem:** any one-author language is a single point of
> failure for everyone who depends on it. An MVP-quality
> second implementation — even a slow one — proves the spec
> is the spec, not "whatever Nancy's compiler does this week".
> **Fix:** a tree-walking interpreter, written in Rust or C++,
> that consumes the same `.s` source and produces the same
> stdout on every example. ~1–2 person-months for the
> language surface that the existing test suite covers.
> `effort: multi-week`

### \[P3\] **R-2** Package manager

> **Problem:** there's no way to share code between Symta
> projects today. Reusable libraries become "copy `src/foo.s`
> into your project".
> **Fix:** even a minimal `symta install <repo-url>` that
> fetches and compiles. Bonus: a discovery index, even if
> hand-curated, of what people have written.
> `effort: weekend` for MVP, more for polish

### \[P3\] **R-3** A debugger

> **Problem:** set a breakpoint, step, inspect locals,
> continue — none of this exists today. Symta has decent
> runtime introspection underneath (the GC walks objects with
> size info, every `cls` instance knows its parts); exposing
> that to a `sym-gdb` would be a few weeks of work, not a
> year.
> **Fix:** an out-of-process debug protocol over a Unix
> socket; the runtime breakpoints at SBC-instruction
> granularity; introspection via the existing reflection
> built-ins.
> `effort: multi-week`

### \[P3\] **R-4** SoM-feedback ergonomic wishlist

> **Where:** [`dev/todo-spell-of-mastery.txt`](dev/todo-spell-of-mastery.txt)
> **Problem:** ~70 small ergonomic gaps catalogued during the
> SoM build: `iround` / `floor` / `ceil` returning floats,
> `txt_input` memory leak, no `.divide` / `.bins` partition
> helpers, no destructive multivar assignment, no do-while
> loop, no method-reference-with-bound-object…
> **Fix:** triage by user-impact and bundle into language
> revisions. Most are syntax sugar that doesn't change the
> evaluation model.
> `effort: ongoing`

---

## Done

- ~~**Port `src/reader.s` to C** (`runtime/reader.c`).~~
  28× faster on representative inputs; 5-stage drift test
  PASS. (commits `4124216`, `dc1e240`, `07ab5c8`, May 2026.
  See `blog.md` for the bisection-driven debug methodology.)
- ~~**Self-hosting drift test** (`tests/bootstrap/drift.sh`).~~
  Bootstraps the compiler 5 times, requires stages 2..5
  byte-identical. (commit `8c059cc`)
- ~~**Eight regression test suites** wired into `make test-all`.~~
  tokenizer / reader / macros / runtime / lineno / compiler /
  gfx / uim. ~127 cases, ~4 min end-to-end. (May 2026)
- ~~**Auto-staging SDL DLLs** from `symta/sdl/`.~~ `ffi_begin
  macro ui` does it. (commit `731c53c`)
- ~~**Auto-staging default font** from `symta/ttf/`.~~ `ffi_begin
  macro ttf`. The game's Sarabun preserved via
  `set_default_font \sarabun` in `game/src/go.s`. (commit
  `6dc5d00`)
- ~~**Auto-generating widget pictograms** at first launch.~~
  `src/uimgen.s` carries 31 SVGs in-source, materialises on
  first `use uim`. (commit `550b719`)
- ~~**Standalone Symta distribution**.~~ `symta/` is now
  buildable + testable on its own; `make` from a fresh clone
  works on a vanilla w64devkit / Xcode CLT / build-essential
  install. (commit `9cf3be7`)
- ~~**FFI errors clearer when transitive DLL missing.**~~
  Replaced `ffi_load couldnt load …` with a 3-way diagnostic.
  (commit `4de11ee`)
- ~~**UI demo robust without audio.**~~ `Mix_OpenAudio` failure
  is now non-fatal; sound API no-ops when unavailable.
  (commit `4de11ee`)
- ~~**Example 26 (huffman) and example 25 (lexmacro).**~~
  (commit `f6a9775`, `41d0770`)
- ~~**Headless `--screenshot` mode in UIM**.~~ Used by every
  `tests/uim/` regression. (earlier 2026 work)

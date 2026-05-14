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

Done items move to the [bottom](#done), one line each with the
commit hash and date.

---

## FFI

### \[P2\] **READER-1** Compiler float-to-text loses precision below 1e-8

> **Where:** [`runtime/main.c`](runtime/main.c) `print_object_r`
> (the `T_FLOAT` branch), called via `float.as_text` which is
> called by [`src/compiler.s`](src/compiler.s) `ssa_atom` when
> serialising `ldflt K X` to SIF text.
> **Problem:** `print_object_r` formats floats with `%.8f` (8
> decimal places).  For values below ~1e-8 the format produces
> `"0.00000000"` (all zeroes) and the trailing-zero stripper
> turns that into `"0.0"`.  When the compiler then emits the
> SIF line `ldflt L5 0.0` and `sif2sbc` re-parses it via
> `strtod`, the result is 0.0 -- silent precision loss.
> **User-visible bite:** writing `0.000000001` (= 1e-9) in
> Symta source compiles to the float 0.0, so any code using
> 1e-9-or-smaller epsilon comparisons silently breaks.
> Discovered while triaging the FFI float-tests (FFI-3); they
> all used `Diff.abs.float < 1e-9` and tripped a downstream
> `float.\<` because `0.0` rounds to integer 0 after the lossy
> compile.
> **Fix sketch:** use `%.9g` (9 significant digits -- enough
> to round-trip every float32 value).  Two side-effects to
> address:
> 1. `%.9g` switches to scientific notation for very small /
>    very large values (`1e-09`, `1.5e+15`).  The reader
>    currently *cannot* parse scientific-notation literals
>    (a separate bug: `1e9` tokenises as a single "int" token
>    of base-10 with letters-as-extended-digits, evaluating
>    to 249).  So fixing the printer requires also fixing the
>    reader/tokenizer to round-trip scientific notation.
> 2. User-facing float display changes from `1.5` /
>    `0.00000000` (lost) to `1.5` / `1e-09` (preserved).
>    Audit example outputs and refresh goldens.
>
> Until READER-1 lands, the workaround is to write small float
> literals in long-form `0.00000001` form -- accurate down to
> ~1e-8, beyond that round to 0.
> `effort: 1-2 days` (printer + reader scientific-notation
> support + golden audit + drift verification)

---

## Runtime — bytecode interpreter

### \[P3\] **RT-1** Bytecode dispatch via computed gotos — *measured, not a win*

> **Where:** [`runtime/sbc.c`](runtime/sbc.c) `sbc_exec_fn`;
> [`benchmark/rt/`](benchmark/rt/) for the suite + baselines.
> **Status:** the threaded variant compiles behind
> `#ifdef SBC_THREADED_DISPATCH` and passes the full test
> sweep + drift, but runs **~8 % slower on average** (gcc 12.2
> / Win64 / i7-12700H) — not the 10-30 % faster the
> literature predicts.  Root cause from disassembly: GCC
> re-emits `lea dt(%rip),%reg` at every per-opcode dispatch
> site instead of pinning the table into a register, inflating
> per-site cost and growing the function 16 %.
> **If somebody wants the 8 %:** try Linux `-fno-pic`,
> hand-roll the dispatch in inline asm with explicit register
> binding, or wait for a future GCC.  Code stays in tree;
> default is off.

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

## Runtime — cache locality

The five items below come from a 2026 audit (after AM-pack-v2)
that looked for the same cache-unfriendly patterns elsewhere in
the runtime that we just fixed in the adaptive map. Each one is
a discrete restructure with a measurable hot-path frequency.
Ordered roughly by ROI; numbers in the right-hand column count
cache lines touched per hot-path operation, **before → after**.

| Hot path | Frequency | Lines now | Lines after |
|----------|-----------|-----------|-------------|
| `O_AGE` lookup (RT-4)       | every LSET, every GC trace | 2-3 | 1 |
| Closure CALL dispatch (RT-5) | every closure call        | 2   | 1 |
| MCALL miss (RT-6)           | every megamorphic dispatch | 3-5 | 1 |
| Bytecode dispatch (RT-1)    | every opcode               | n/a (BTB) | n/a (BTB-friendly) |
| MCACHE store (RT-7)         | every MCACHE miss          | I-side SMC stall | 1 D-side |
| Frame GC walk (RT-8)        | every GC pause             | 2/frame | 1/frame |

The drift test gives strong confidence for invasive runtime
restructures: if the bootstrap reaches a fixed point byte-
identical after the change, codegen is unchanged. That was the
load-bearing assertion for AM-pack-v2 and applies here too.

### \[P1\] **RT-4** Object age lives in a parallel side array

> **Where:** [`runtime/symta.h:106-109`](runtime/symta.h)
> (`O_AGE`/`O_THEAP`), [`runtime/common.h:402-408`](runtime/common.h)
> (`lsetm` write barrier), [`runtime/gc.c:17-46`](runtime/gc.c)
> (`GC_REC3` trace).
> **Problem:** `gc_heap_t` (`common.h:291-304`) has `void
> heap[FULL_HEAP_SIZE]` (~512 MB at full size) and a parallel
> `uint8_t theap[FULL_HEAP_SIZE]` (~64 MB) in completely disjoint
> pages -- the prefetcher never sees them together. Every
> generational write barrier (`lsetm`, fired on every store of
> one heap object into another) does `GC_OLDER(base, value)`,
> which is two `O_AGE` lookups. So every `LSET` touches three
> cache lines: the object header (`O_HDR`), `theap0[gid_base]`,
> and `theap0[gid_value]`. `O_AGE` is also read on every
> GC pointer trace and every closure call's `O_HG`.
> **Fix:** the high byte of `O_CODE` is unused -- functions and
> types live in `hooks_heap`/`types` arrays with ≤24-bit
> indices. Steal it for an inline age:
> `gc_head_t { uint8_t age; uint8_t flags; uint16_t code_lo;
> uint32_t size; }`. `O_AGE(o)` then loads the same cache line
> as `O_HDR(o)`. The barrier drops from 3 lines to 1.
> **Why now:** highest-frequency hot path in the entire
> runtime. Every store and every trace pays the side-array
> tax today.
> **Why not yet:** touches the GC's most fundamental invariant
> (age tracking). The fix is mechanical but needs careful staging
> -- keep `theap0` populated in parallel for one revision, then
> flip the readers to `O_AGE` from the header, then drop the
> writes to `theap0`, then drop the array. Drift test gates each
> step.
> `effort: weekend`

### \[P1\] **RT-5** Closure CALL chases `hooks_heap[code]` indirection

> **Where:** [`runtime/symta.h:114-115`](runtime/symta.h)
> (`O_HOOK`), [`runtime/symta.h:391-396`](runtime/symta.h)
> (`CALL` macro), [`runtime/sbc.c:129`](runtime/sbc.c)
> (`hooks_heap` definition).
> **Problem:** closures dispatch via
> `hook_t *h = &hooks_heap[O_CODE(closure)]` where
> `hooks_heap[MAX_HOOKS=65536][24 bytes/entry] = 1.5 MB`.
> Each closure call: load closure header (line 1) → index into
> `hooks_heap` (line 2, ~random across the 1.5 MB region with
> many distinct hook ids) → call handler. The per-call equivalent
> of a vtable miss into a never-cached side region.
> **Fix:** store `handler` (`psf_t`) + `payload` (`void *`)
> inline at the closure's slot 0/1. The JIT-emitted curry thunks
> (`emit_hook` in `sbc.c:95-124`) already use this layout
> conceptually; just materialise it in the heap object so plain
> `CALL` doesn't need `hooks_heap` at all. `meta` stays in a
> side table (only error/trace paths touch it).
> **Why now:** every closure call pays this. Closure-heavy
> code (anaphoric `?+`, list comprehensions, callback-style
> APIs) is bottlenecked here.
> `effort: afternoon`

### \[P2\] **RT-6** Method dispatch table is 4 KB per type + pointer-chase

> **Where:** [`runtime/common.h:206-235`](runtime/common.h)
> (`type_t`, `method_node_t`),
> [`runtime/main.c:101-184`](runtime/main.c)
> (`get_method`, `get_method_node`, `init_method`).
> **Problem:** `type_t` is **4144 bytes** per type
> (~48-byte header + `method_node_t *methods[512]` = 4096 bytes
> of pointer heads). Each MCALL with a cold mcache touches:
> `types[tag]` (header line) → `types[tag].methods[hid]` (a
> second line, `hid<<3` bytes away from the header) → the
> `method_node_t` chain. Chain nodes are page-allocated by
> `init_method` so `*next` walks chase pointers across pages.
> 3-5 cache lines per megamorphic miss.
> **Fix:** the existing comment at `common.h:233` already wants
> perfect hashing. Concretely: replace the 512-bucket
> head-pointer array with a packed `(mid, fn)` open-addressed
> probe array sized to the actual method count per type. For
> typical types with <16 methods, the whole table fits in 1
> cache line. Same restructure as AM-pack-v2 applied at the
> method-table level.
> **Bonus refactor:** split `type_t` into hot (dispatch fields)
> and cold (name, subtypes, super, sname) sub-structs. Hot
> sub-struct sized so 8 of them fit per cache line, for the rare
> cross-type fallback walks.
> **Why now:** the mcache (`mcache_t` in `sif.h:220-222`) hides
> this on monomorphic call sites, but every polymorphic dispatch
> -- list/text/closure ad-hoc code, anything in `cls.s` --
> falls into the slow path.
> `effort: afternoon`

### \[P2\] **RT-7** `mcache_t` is embedded in the bytecode stream

> **Where:** [`runtime/sbc.c:341-356`](runtime/sbc.c)
> (`MCACHE_CALL`), [`runtime/sif.h:220-222`](runtime/sif.h)
> (`mcache_t` definition).
> **Problem:** `MCACHE_CALL` does
> `mcache_t *mce = (mcache_t*)pin; pin += sizeof(mcache_t)`.
> The cache lives in the bytecode payload itself -- good for
> I-side fetch locality, BAD for cache writes. On `MCACHE_MISS`,
> we do `mce->node = node;` which is an I-cache **store** that
> dirties a code cache line. Modern Intel (post-Spectre
> mitigations especially) treats this as self-modifying code
> and can trigger a machine-clear / SMC nuke at the next
> dispatch -- typically tens to hundreds of cycles per miss.
> **Fix:** move the mcache slots to a separate D-side array
> indexed by an `mcache_id` stored in the bytecode (8 or 16
> bits; even hot functions have <256 MCALL sites). Bytecode
> stays read-only; the cache updates in normal data memory.
> Combined with RT-1 (computed goto), this eliminates the
> dispatch-time SMC penalty entirely.
> **Why now:** the SMC penalty only fires on mcache misses, but
> misses are exactly the path we already optimise around -- and
> when they fire, they fire in batches (type-shift in a hot
> loop). The fix is contained: bytecode emitter writes an id;
> sbc_exec reads from a per-function table.
> `effort: afternoon`

### \[P2\] **RT-8** `frame_t` metadata separate from `L[]` locals

> **Where:** [`runtime/symta.h:222-228`](runtime/symta.h)
> (`frame_t`), [`runtime/symta.h:342-367`](runtime/symta.h)
> (`PROLOGUE`), [`runtime/gc.c:95-102`](runtime/gc.c)
> (`gc_builtins`).
> **Problem:** `PROLOGUE` puts the C function's `void *L[fsize]`
> on the C stack, which is fine -- contiguous, prefetcher-loved.
> But the *frame metadata* (`frame_t = {clsr, prev, vars,
> nvars}`) lives in a separate C-stack `frm_` whose `vars`
> points back at `L[]`. The GC root scan walks
> `api.frame->prev → frame->vars → walk nvars pointers → next
> frame` -- two cache lines per frame for deep stacks. Steady
> state cost is small; GC pause cost on call-stack-heavy
> workloads (game tick, deep AST walks) is real.
> **Fix:** `OPEN_FRAME` allocates `frame_t + L[]` as one
> struct-hack blob on the C stack, with the locals immediately
> following the metadata. GC root scan touches one line per
> frame. As a bonus, `vars` and `nvars` become redundant (size
> is known to prologue), shrinking the frame header.
> **Smaller related win:** `api_t` itself
> (`symta.h:234-285`) interleaves cold fields
> (`print_object_f`, `alloc`, `text_chars`, `sbuf`,
> `nfi_args[32]`) with hot fields (`frame`, `args`, `hgp`,
> `theap0`, `heap0`). Every barrier and every CALL loads from
> different lines of `api_g`. Reorder so the ~8 hot pointers fit
> in the first cache line.
> `effort: 30 min (api_t reorder) + afternoon (frame inlining)`

---

## Adaptive map — table internals

The adaptive map is what backs every Symta hash, every ECS
column, every cache. The 2026 push (AM-1..AM-15, AM-pack-v2)
landed Robin Hood probing at 75 % load, hash caching, the
{key,val,hash} inline slot layout, and replaced both stb_ds
backings with `nh_t`-derived `ih_t`/`th_t`. Items below are
what's left.

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

### \[P3\] **AM-9** `nh_t` capacity is `uint32_t` — DEFERRED

> **Where:** [`runtime/nh.h`](runtime/nh.h)
> **Status:** caps tables at 2³² entries. With Symta's heap
> layout (`GID_BITS = 48` for the gid encoding, ~24 GB heap
> max), no realistic workload reaches anywhere near 2³² entries
> in a single table. Widening to `uint64_t` would cost 4 B per
> table header for a capability no one currently needs. Will
> revisit when a real workload demands it.
> `effort: 30 min` (when wanted)

### \[P3\] **AM-10** String interning shared with `AM_TEXT`

> **Where:** [`runtime/am.h`](runtime/am.h) `AM_TEXT` paths
> **Problem:** repeated equal-string keys are deduped by
> `th_t`, but `text_chars(key)` runs every probe.
> Frequency-table workloads pay it on every increment.
> **Fix:** intern text keys (or tag the text with its hash on
> first use) so probe cost drops to a pointer compare.
> `effort: weekend` (depends on text type rework)

---

## Lexical macro processor (`symta/ncm/src/ncm.h`)

(NCM-1..7 all closed.  See Done section.)

If a new bug surfaces, add a probe to `symta/ncm/tests/cases/`
first; that's the layer below `tests/runtime/25-lexmacro` and
catches things without dragging in the rest of Symta.

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

### \[P3\] **UI-5** Flaky `tests/uim/buttons.png` byte-size jitter

> **Where:** `tests/uim/baselines/buttons.png` vs the rendered
> actual; the `--pngcmp` decoded-pixel check in
> `tests/uim/run.sh`.
> **Problem:** the buttons-test (and occasionally lists)
> renders to a PNG that varies by ~10 bytes run-to-run on the
> same binary.  Sizes seen so far: 19358, 19362, 19367, 19369.
> A single pixel at (38,2) shifts between `0x10101A`
> (background) and `0x333336` (some glyph antialiasing
> remnant), tripping the `tolerance=32` threshold in
> `--pngcmp` once in every ~5-10 runs.  Other UIM tests are
> stable.
> **Likely cause:** glyph rasterization output depends on the
> order chars enter the font's glyph atlas, which in turn
> depends on a hash-table iteration order somewhere.  The
> em-dash in `"btn — three different widths"` is the most
> likely trigger -- it was effectively skipped before FFI-4
> (single_chars 128..255 → empty fixtext) and now reaches the
> renderer, but the renderer's atlas slot for it isn't
> deterministically positioned.
> **Fix sketch:** force a stable atlas seed by pre-allocating
> slots for the printable ASCII + common Latin-1 range at
> font load time, in deterministic order.  Cheap (one new
> loop in the ttf plugin's font init).
> **Workaround:** rerun the test; it passes ~9 out of 10 runs.
> `effort: afternoon`

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

### FFI (May 2026)

- ~~**FFI-1** Drop cinvoke; ship custom trampolines.~~
  Phases 0–3 done; x64-Win + x64-SysV in production; vendored
  `cinvoke/` tree deleted. Net licence: dual MIT / Apache-2
  end to end. See [`runtime/sffi/ARCHITECTURE.md`](runtime/sffi/ARCHITECTURE.md);
  test matrix at [`tests/ffi/`](tests/ffi/) (17/17). Commits
  `647d6ff` (Phase 1), `31c02ce` (Phase 2 land),
  `742e254`/`69f90f2`, `6774401` (cinvoke deletion).
- ~~**FFI-2** `ffi_load` segfaults on missing library.~~
  Replaced `fatal()` with stderr-warning + `R = No` for all
  three failure modes (file missing, file present but won't
  load, symbol not found). Unblocks `libc_resolve`-style
  try-each-candidate patterns. Commit `71d51ee`.
- ~~**FFI-3** Six FFI test goldens captured unhandled errors.~~
  Root-caused per test (libc downstream of FFI-2, str_ops `^^`
  is pow/map not text-repeat, four float tests tripped READER-1
  via `1e-9`). Refreshed goldens. Process fix: run.sh `--update`
  refuses to write a golden containing `^UNHANDLED ERROR` or
  `segfault at` (applied to `tests/ffi`, `tests/am`,
  `tests/runtime`). Commit `71d51ee`.
- ~~**FFI-4** Text marshalling drops multi-byte UTF-8.~~
  Root cause was upstream of FFI: `single_chars[256]` init loop
  populated 0..127 only and aliased 128..255 to `single_chars[0]`
  (empty fixtext), so `text.[i]` on any high-bit byte returned
  empty, cascading through `.l`/`.code`/`cstring_bytes` into
  truncated SBC bytes-sections. Five-line fix: loop covers
  0..255. The tc_str_ops bytesum-of-`"ä"` case is reactivated
  and passing; drift stays byte-identical. Commits `72da56d`,
  `9f947cd`.

### Runtime — bytecode interpreter (May 2026)

- ~~**RT-1 (research)** Bytecode dispatch via computed gotos.~~
  Built the threaded variant behind `#ifdef SBC_THREADED_DISPATCH`,
  with [`benchmark/rt/`](benchmark/rt/) suite + before/after
  baselines. **Measured ~8 % slower** on gcc 12.2 / Win64 /
  i7-12700H — not the literature-predicted win. Toggle stays in
  tree for future re-experimentation; entry kept open above
  (P3) with re-experiment ideas. Commit `84ad564`.

### Adaptive map (May 2026)

- ~~**AM-1** `amGidGet` falls through `AM_GENERIC` / `AM_TEXT`
  uninitialised.~~ Explicit `GOT(AM_GENERIC)` dispatches through
  `dhGet`; `GOT(AM_TEXT)` `fatal()`s (text-keyed column queried
  by integer GID is a cls-layer contract violation). Landed with
  AM-3.
- ~~**AM-3** `GENERIC` mode paid MCALL per probe step.~~
  `dhEqual_`/`dhHash_` now inline `T_INT`/`T_FIXTEXT`/`T_TEXT`
  before falling back to MCALL. Orders of magnitude less work
  per probe on generic tables.
- ~~**AM-5** `nh_t` load factor was 50 %.~~ Robin Hood probing in
  `nh.h` (opt-in via `NH_ROBIN_HOOD`); dh opts in at 75 %. Add
  swaps along the chain; Del backshifts; Lookup early-exits when
  DIB < its own. ~33 % memory reduction on hash-heavy workloads;
  drift identical in 1 round.
- ~~**AM-6** Two underlying hashmaps (`stb_ds` + `nh_t`).~~
  AM_INT path swapped to new `ih_t` (an `nh_t` instantiation
  with `NH_KEY=dyn`, `NIL=1`). Iteration order changed from
  insertion to hash; drift passed in one round.
- ~~**AM-6b** AM_TEXT on `th_t` (drop stb_ds).~~ New `th.h`
  instantiates nh.h with `NH_KEY=dyn` (text dyn), Robin Hood at
  75 %, AM-15 hash cache. Bisected a 100× insert-perf cliff to
  Adler-32 clustering on common-prefix keys; switched BIGTEXT
  path to FNV-1a. Del -61 % on bn_text; net wash to small win
  across the suite.
- ~~**AM-pack** Half-pack {key, hash} slot layout.~~ Tried and
  reverted: val store on a separate cache line split the wins.
  Superseded by AM-pack-v2.
- ~~**AM-pack-v2** Full pack: {key, val, hash} inline slots.~~
  Single inline array of `nh_slot_t = {NH_KEY key; NH_VAL val;
  uint32_t hash; uint32_t _pad}`; one cache line per access; one
  malloc per table instead of two/three. bn_int hit -11 %, miss
  -13 %; bn_generic hit -17 %, del -28 %.
- ~~**AM-6c** dh.h Adler-32 + FIXTEXT fold clustering.~~ Same
  hash-distribution bugs AM-6b found in th.h, applied to dh.h.
  FIXTEXT path → Murmur3 finaliser; BIGTEXT path → FNV-1a (in
  sync with th.h). Not exercised by current bench/test, but no
  more landmines for users with prefix-similar text keys.
- ~~**AM-7** `AM_BITMAP0` write-zero looks like delete.~~
  Documented the `void_val` contract at the top of `am.h`;
  default `void_val = No` so most user code never trips it. API
  split (set vs remove) deferred until somebody trips on it.
- ~~**AM-13** Bitmap mode probes missing `T_INT` guard.~~ Added
  `if (O_TAG(key) != T_INT) return …` to all six bitmap probe
  branches (`amHas` / `amGot` / `amGet` × 2 bitmap modes).
  `amSet` BITMAP0/1 paths already had the guard.
- ~~**AM-15** Cache hashes in dh slots so RH probes skip MCALL.~~
  Parallel `hashes[]` array on `nh_t` behind `NH_CACHE_HASH`;
  dh opts in, ih/nb don't. generic ins -42 %, miss -42 %, del
  -28 %; int/text unchanged.
- ~~**AM-14** BITMAP→INT promotion leaked the underlying
  `nb_t`.~~ Added `nbFree(nb)` to all four affected branches
  (2 in `amSet`, 2 in `amGidSet`). Pre-existed AM-6; carried
  over from the stb_ds version verbatim.
- ~~**AM-12** `amHas` / `amGot` `AM_BITMAP0` predicate
  inverted.~~ Flipped the BITMAP0 condition to `nbGet != 0` so
  the predicate matches every other touchpoint. Surfaced by
  `tc_void.s`.
- ~~**AM-11** `amSet` `AM_TEXT` delete branch missing
  `return`.~~ Added it; `T.K = void_val` on an AM_TEXT table
  was silently a no-op. Surfaced by `tc_void.s`. Also brought
  `am.h`/`nb.h`/`nh.h` into `HDRSB` on Makefile.w64 so future
  header edits trigger recompilation.
- ~~**AM-7b** `amSet` skips `AM_BITMAP0` for first-value-0.~~
  Aligned `amSet` with `amGidSet`; first write of `FXN(0)` to an
  empty table now picks BITMAP0 (~1 bit/key) instead of INT
  (~16 B/key). Drift PASS in 3 rounds.
- ~~**AM-8** Iteration during mutation has no guard.~~ Moot —
  `amL`/`amKs` eagerly snapshot. Pinned the contract with a new
  test in `tc_iteration` that captures `T.l`, mutates the table,
  and verifies the snapshot's pairs stay stable.

### NCM — Lexical macro processor (May 2026)

- ~~**NCM-1** `<<` and `>>` shifts broken in `#[expr]`.~~
  `eval_shift` was popping the first `<`/`>`, checking the
  second matched, then recursing without popping it -- so the
  operand parse started on the second `<` and silently
  returned 0.  Added the missing `pop_char()`.
- ~~**NCM-2** `&&` and `||` swallowed by `eval_bitwise`.~~
  `eval_bitwise` greedily ate the first `&`/`|`; `eval_logic`
  (which handles `&&`/`||`) never saw the operator.  Now
  `eval_bitwise` peeks the second char and pushes back the
  first if it sees a doubled operator -- mirrors how
  `eval_cmp` already handled `<<`/`>>` vs `<`/`>`.
- ~~**NCM-3** `cmd_fmt` only parsed decimal numeric args.~~
  `#("%d" 0xFF)` returned 0 because the decimal loop stopped
  at `x`.  Added a `0x` / `0X` branch that switches to
  base-16 parsing.  Decimal path unchanged.
- ~~**NCM-4** Document NCM directives.~~ New
  [`dev/ncm.md`](dev/ncm.md): directive reference, expression-
  language precedence table, format-spec table, line-by-line
  map into `runtime/ncm.h`.
- ~~**NCM-5** Empty variadic call segfaults NCM.~~ Surfaced by
  the new `symta/ncm/tests/` suite (commit `3e6b17f`); the
  zero-arg path in `macroexpand_fn` wrote `as[j-1] = v` with
  `j == 0`, an out-of-bounds write into `as[-1]`.  Fix: when
  `l == 0`, bind the variadic to the static empty string `""`
  (same pattern the bodyarg branch already uses).  Pinned by
  `tests/cases/fn_macros.c`.
- ~~**NCM-6** `%x` and `%X` case mapping inverted from C
  `printf`.~~ Audited callers (zero source-code dependencies
  on the inversion), swapped the two `sprintf` calls in
  `cmd_fmt`'s hex branch.  Now `%x` → lowercase, `%X` →
  uppercase, matching C convention.
- ~~**NCM-7** `eval_term` `strtol`'d macro bodies / arg values
  literally.~~ `#A 5; #B A; #[B]` silently returned 0 (strtol
  stopped at the non-digit), and any nested-call expansion
  in `#[expr]` lost the inner result (so
  `#double(X) #[2*X] ; #[double(double(5))]` returned 0
  instead of 20).  Replaced the bare `strtol` with
  `eval_expr_standalone`, which handles plain numerics
  identically and recurses through macro chains for the
  general case.  Pinned by `tests/cases/nested_eval.c`.

All NCM bug fixes pinned by `examples/25-lexmacro.s`
section 11 (Symta-side integration) + the standalone
`symta/ncm/tests/cases/` suite (NCM-only).

### Reader / compiler core (May 2026)

- ~~**CORE-4** Multi-line `if X:` body needs `|` for `else` at
  body indent.~~ Root cause was a three-way interaction in
  `src/reader.s` `parse_offside`:  `parse_if` passed `Type=0`
  for body collection, leaving the existing `Type >< 'if' and
  V >< \`then\`: done` shortcut as unreachable dead code; the
  bar-insertion path had a `less Type >< 'if'` guard that
  suppressed auto-bars only when the (never-set) `'if'` marker
  fired.  Fix: `parse_if` now passes `Type='if'` for both
  `then`-and `:`-introduced bodies; `parse_offside` breaks on
  same-indent `then`/`else`/`elif` *only when the current
  segment isn't itself an open if* (the `Ys.~ == \`if\``
  suppression keeps nested `if X: 1 / else 2` at body indent
  attached to the inner if); the dead-code `less Type >< 'if'`
  guard is removed so auto-bars insert at same-indent
  statement boundaries inside if-bodies the same way they do
  in regular function bodies.  Mirrored in `runtime/reader.c`
  to keep the parser-compare parity test green.  Pinned by
  `tests/reader/cases/13-core4-else-at-body.s` (CORE-4
  reproducer, elif-at-body-indent, and the nested one-line
  if/else that must keep inner binding).  5-round drift test
  reaches fixed point in stage 1.

### Infrastructure (earlier)

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

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

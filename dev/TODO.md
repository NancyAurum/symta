# dev/TODO.md — research-driven development goals

A living, structured punch-list for Symta. Unlike `dev/todo.txt`
(which is a stream-of-consciousness notebook), this file holds
items that have been **reviewed**, **scoped**, and have a
**concrete proposed fix**, so they can be picked up and worked on
without needing to re-do the analysis.

Format per item:

> **\[PRI\] short title** &nbsp;&nbsp; `effort: …`
> **Where:** path:line(s)
> **Problem:** what's wrong / sub-optimal, in one paragraph.
> **Fix:** the concrete approach, in one paragraph.
> **Why now / why not:** justification or blocker.

Priorities:

- **\[P0\]** correctness / latent bug
- **\[P1\]** performance gap on the hot path
- **\[P2\]** ergonomics / cleanup / tech debt
- **\[P3\]** speculative / future research

Effort tags: `30 min`, `afternoon`, `weekend`, `multi-week`.

Done items move to the bottom, struck through, with the commit
hash and date appended.

---

## Adaptive Map (`runtime/am.h`, `runtime/nh.h`, `runtime/nb.h`, `runtime/dh.h`)

### \[P0\] `amGidGet` falls through `AM_GENERIC` / `AM_TEXT` uninitialized
> **Where:** [runtime/am.h:64](runtime/am.h)
> **Problem:** the `CASE` only handles `EMPTY`, `INT`, `BITMAP0`,
> `BITMAP1`. For `GENERIC` and `TEXT`, `r` is returned uninitialized
> (latent UB). The cls layer presumably guarantees integer-keyed
> tables here, but the contract isn't enforced.
> **Fix:** add explicit branches that either dispatch correctly
> (TEXT can resolve, GENERIC can call `dhGet` with the ref as key)
> or `_fatal` so the violated assumption surfaces immediately.
> `effort: 30 min`

### \[P1\] No dense-iteration story for ECS columns
> **Where:** [runtime/am.h:573 (`amL`)](runtime/am.h),
> [src/cls.s:157 (`each_`)](src/cls.s)
> **Problem:** `amL` and `gid_refs_` build a list by walking the
> hashmap on every call. ECS systems iterate component columns
> every frame; bitmap iteration touches every page even for low
> populations, and INT-mode iteration walks every hash slot. State
> of the art (flecs, Bevy, EnTT) keeps entities packed densely so
> the per-frame iterator is essentially `for i = 0..n` over a flat
> array — typically 5–20× faster.
> **Fix:** pair each `BITMAP*` and `INT` table with an optional
> packed-dense vector (`dense_index[gid]` and `id_at_dense[i]`).
> The bitmap already provides O(1) presence; adding the parallel
> arrays gives O(1) insert/delete and sequential-array-speed
> iterate. Standard sparse-set design.
> **Why now:** this is the single biggest performance gap for any
> game-style workload.
> `effort: weekend`

### \[P1\] `GENERIC` mode pays MCALL per probe step
> **Where:** [runtime/dh.h](runtime/dh.h) (`dhEqual_`, `dhHash_`)
> **Problem:** every probe (including misses, including during
> grow) does a full Symta method dispatch — build arglist, vtable
> lookup, return through unbox. A million-key generic map crawls.
> Lua / Python / Ruby all inline int/text/symbol keys and only
> fall through to method dispatch for genuine user types.
> **Fix:** in `dhEqual_` / `dhHash_`, branch on `O_TAG(key)` first
> and inline the int and text cases. Most "generic" tables in real
> code are still 90%+ int or text mixed with one stray symbol that
> forced GENERIC mode.
> `effort: afternoon`

### \[P1\] `BITMAP*` promotion to `INT` is lossy and one-way
> **Where:** [runtime/am.h:469–512](runtime/am.h)
> **Problem:** the moment **any** value diverges from the chosen
> 0 or 1, the entire column promotes to `AM_INT` — every entity
> now costs ~16 B even if 99% of values are still 0 or 1. Common
> case: `is_visible` is mostly 1, a handful of entities have
> visibility levels 0..7 — currently you pay full INT cost for
> the entire column.
> **Fix:** add `AM_BITMAP_PLUS` mode = a bitmap PLUS a small
> `nh_t` of divergent gids. Promote to full INT only when
> exceptions exceed some fraction (~10%) of total. The pieces
> (`nb_t` + `nh_t`) already exist; the change is mostly in
> `amSet`.
> `effort: weekend`

### \[P1\] `nh_t` load factor is conservative (50%)
> **Where:** [runtime/nh.h:85](runtime/nh.h)
> **Problem:** `NH_NEEDS_GROW(nh)` triggers at half-full. Linear
> probing without tombstones makes 50% defensible, but for ECS
> tables of 100k–1M entries you're allocating roughly 2× the
> memory you actually need.
> **Fix:** switch to Robin Hood probing or double hashing —
> either buys 75–80% load factor with similar lookup latency.
> Robin Hood is ~30 lines of additional bookkeeping; the existing
> linear probe stays correct as a fallback.
> `effort: afternoon`

### \[P2\] Two underlying hashmaps (`stb_ds` and `nh_t`) duplicates tuning surface
> **Where:** [runtime/am.h](runtime/am.h) uses `stb_ds` for
> `AM_INT`/`AM_TEXT`, `nh_t` for `AM_GENERIC`
> **Problem:** two probing strategies, two memory layouts, two
> places to tune load factor. Performance work has to be done
> twice. stb_ds is also harder to specialize for our cases.
> **Fix:** standardize on `nh_t`. It's already templated for typed
> keys via macro instantiation. Drop stb_ds usage from am.h
> entirely.
> `effort: afternoon`

### \[P2\] `AM_BITMAP0` write-zero looks like delete
> **Where:** [runtime/am.h:163–211](runtime/am.h)
> **Problem:** writing `0` to a `BITMAP0` table compares
> `value == void_val` (also `0`), takes the delete branch, and
> `nbDel`s the key. So `T.K = 0` on a bitmap-0 table makes
> `T.has K` return false — even though logically the user just
> wrote a value. Consistent with how AM_VOID works elsewhere, but
> surprising and undocumented.
> **Fix:** at minimum, document the contract. Better: split
> "set value" from "remove key" so the behaviour is explicit.
> `effort: 30 min` (docs) / `afternoon` (split API)

### \[P2\] Iteration during mutation has no guard
> **Where:** [runtime/am.h:573 (`amL`)](runtime/am.h),
> [src/core_.s tbl iteration](src/core_.s)
> **Problem:** `for K,V T:` over an INT or TEXT table that the
> body mutates can rehash and invalidate the iterator
> (`stb_ds`'s `hmput` reallocates on grow). Currently silent
> misbehaviour; a Lua/Python equivalent would error.
> **Fix:** snapshot the keys list before iteration in the
> `tbl.l`-style helpers, or stamp a generation counter on the
> table and check it during iteration.
> `effort: afternoon`

### \[P3\] `nh_t` capacity is `uint32_t`
> **Where:** [runtime/nh.h:67](runtime/nh.h)
> **Problem:** caps tables at 2³² entries. Fine for almost
> anything, but a many-billion-relation ECS world (think
> simulation-scale) would hit this.
> **Fix:** widen to `uint64_t`. Costs 4 B per table header. Defer
> until someone actually needs it.
> `effort: 30 min` (when wanted)

### \[P3\] String interning shared with `AM_TEXT`
> **Where:** [runtime/am.h `AM_TEXT` paths](runtime/am.h)
> **Problem:** repeated equal-string keys are deduped by stb_ds,
> but `text_chars(key)` runs every probe. Frequency-table
> workloads pay it on every increment.
> **Fix:** intern text keys (or tag the text with its hash on
> first use) so probe cost drops to a pointer compare.
> `effort: weekend` (depends on text type rework)

---

## Lexical macro processor (`runtime/ncm.h`)

### \[P1\] `<<` and `>>` shifts are broken in `#[expr]`
> **Where:** [runtime/ncm.h:1618 (`eval_shift`)](runtime/ncm.h)
> **Problem:** `#[A<<2]` returns 0, not `A*4`. `eval_shift` pops
> the first `<`, then checks the next char matches, then recurses
> into `eval_add` *without popping the second `<`*. The right
> operand parse starts on the second `<`, fails to find a term,
> returns 0. Same bug affects `>>`.
> **Fix:** add `pop_char()` after the `if (next_char() != ch)`
> early return. One line.
> **Verified:** examples/25-lexmacro.s probe phase, also visible
> with `symta -e 'say (ncm_process_ "x" "#[16<<2]" [])'`.
> `effort: 30 min`

### \[P1\] `&&` and `||` swallowed by `eval_bitwise`
> **Where:** [runtime/ncm.h:1670 (`eval_bitwise`)](runtime/ncm.h),
> [runtime/ncm.h:1686 (`eval_logic`)](runtime/ncm.h)
> **Problem:** `#[1 && 1]` returns 0. `eval_bitwise` greedily eats
> the first `&` as bitwise-AND and recurses into `eval_cmp` on the
> second `&`, which can't parse it. So `eval_logic` (the layer
> that handles `&&`/`||`) never sees them.
> **Fix:** in `eval_bitwise`, peek at the second char before
> consuming `&` or `|` — if it matches, leave the operator for
> `eval_logic` and return. Mirrors how `eval_cmp` already handles
> the `<<` / `>>` case (`if (ch == '<' && ch2 == '<') goto end;`).
> **Workaround for users:** chain `#if A` / `#elif B` instead of
> `#if A && B`. Done in examples/25-lexmacro.s.
> `effort: 30 min`

### \[P2\] `cmd_fmt` only parses decimal numeric args
> **Where:** [runtime/ncm.h:1372 (`cmd_fmt`)](runtime/ncm.h)
> **Problem:** the printf-style formatter `#("0x%02X" 0xFF)`
> parses the arg `0xFF` as `0` because `cmd_fmt`'s number reader
> (`for (; isdigit(*q); q++)`) stops at `x`. So users have to
> pass numeric args as decimal text or wrap in `#[...]`.
> **Fix:** detect a leading `0x` and switch to base 16 parsing.
> Five-line change.
> **Workaround for users:** use `#("0x%02X" #[0xFF])` — `#[expr]`
> evaluates first and yields a decimal which `cmd_fmt` parses
> correctly.
> `effort: 30 min`

### \[P3\] Document NCM directives
> **Where:** missing
> **Problem:** the lexical macro processor has at least 11
> directives (`#NAME`, `#name(args)`, `#:include`, `#[expr]`,
> `#<sym>`, `#(...)`, `#if/#elif/#else/#endif`, `#undef`,
> `#once`, `#multi`, `##`, `#:#`, `#{...#}`) and a non-trivial
> arg syntax (`name=Default`, `[Variadic]`, `{Body}`). The
> example file shows them but a reference table belongs in
> `dev/ncm.md` so users know what's available without reading
> the C source.
> **Fix:** new `dev/ncm.md` with a one-line summary per directive
> and links into `runtime/ncm.h` for details.
> `effort: afternoon`

---

## Compiler (`src/compiler.s`, `src/macro.s`, `src/reader.s`)

### \[P2\] Better diagnostics for the common parser pitfalls
> **Where:** [src/reader.s](src/reader.s)
> **Problem:** several pitfalls produce confusing errors —
> nested `[...]` in string interpolation, `(-5)` parsing as a
> call, unescaped `{` confusing column counters, the chicken-and-
> egg "undefined variable" for `Var = X` first use. The example-
> writing tour discovered these one by one (see AI.md gotchas).
> **Fix:** add specific error messages for each — "did you mean
> `\[ ... \]` to escape brackets in a string?", "first assignment
> to `Var` should be `Var X` not `Var = X`", etc. Lex/parse
> already has the column info.
> `effort: weekend`

### \[P3\] `_setjmp` / `_longjmp` exist; `set_unwind_handler` doesn't
> **Where:** [src/macro.s `fin` macro](src/macro.s),
> [runtime/sif.h opcodes](runtime/sif.h)
> **Problem:** the `fin Body Finalizer` macro expands to an
> opcode the VM doesn't implement. Symta has GC finalizers
> (`set_finalizer`) and non-local return (`btland`/`btjump`), but
> no try/finally. Useful for FFI cleanup that must run regardless
> of error path.
> **Fix:** add `SBC_SET_UNWIND` / `SBC_REMOVE_UNWIND` opcodes,
> wire them into the SIF assembler, push/pop a handler stack in
> the interpreter, fire on `bad`. Symta's GC already maintains
> `api.uwhs` for runtime use; this exposes it to user code.
> `effort: weekend`

---

## Examples & Documentation

### \[P2\] Stack trace from caught `bad` is noisy
> **Where:** [runtime/bltin.c (`bad`)](runtime/bltin.c)
> **Problem:** `bad Msg` always prints the stack trace to stderr,
> even when caller wraps in `btrap` and recovers. So a
> "successful" run that touched any error path looks alarming.
> Verified in examples 23-control and 24-runtime — the bterror
> is correctly returned, but stderr is noisy.
> **Fix:** print the trace only on the *first* uncaught `bad`,
> i.e. when there's no btrap on the stack. The btrap layer
> already knows it's catching.
> `effort: 30 min`

### \[P3\] Add an `examples/26-fact-engine/` project
> **Problem:** the current 21-inference is good but uses a
> simplified table-of-tables design. The real `som_src/fact.s`
> uses `cls fact_symbol` + interned ids + `cls_tbl_`. That's the
> production pattern and it's nowhere in the examples.
> **Fix:** new project example with the cls-based engine, plus a
> larger fact set so query performance is observable.
> `effort: afternoon`

### \[P3\] Sparse-set / dense-iteration cookbook entry
> **Problem:** users wanting to write fast ECS systems on top of
> `cls` need to know how to keep their own dense indices. Until
> P1-#1 above lands, that's the only path.
> **Fix:** `dev/cookbook-ecs.md` with a worked example: pair
> `cls foo bar` with a manually-maintained `dense_foo[]` for the
> hot iteration loop.
> `effort: afternoon`

---

## Runtime / GC

### \[P3\] Expose explicit `gc()` to Symta code
> **Where:** [runtime/bltin.c](runtime/bltin.c)
> **Problem:** triggering a GC currently requires allocating
> enough garbage to fill gen0 — the only portable way examples
> can demonstrate finalizers. Useful in tests, profilers, and
> long-running daemons that want to release memory at known
> points.
> **Fix:** `BUILTIN0("gc",gc) { api.gc(); RETURNS(No); }` —
> single line, the underlying `api.gc` already exists.
> `effort: 30 min`

### \[P3\] Minor-only GC trigger / generation tuning hooks
> **Where:** [runtime/gc.c](runtime/gc.c)
> **Problem:** generation sizes are picked at boot
> (`init_api(GEN_ZERO_SIZE*10)`), no way to tune for a workload.
> ECS apps may want bigger gen0; REPL apps may want smaller.
> **Fix:** envvar `SYMTA_GEN0_SIZE` plus a runtime hook
> `gc_set_gen_size(gen, bytes)` for advanced users.
> `effort: afternoon`

---

## Done

<!--
- ~~\[P0\] FFI errors are cryptic when transitive DLLs missing~~
  (commit `4de11ee`, 2026-05-05)
- ~~\[P0\] UI demo fails on machines with no audio device~~
  (commit `f6a9775`, 2026-05-05)
- ~~Add 25 progressively-richer examples covering the language surface~~
  (commits `9654f8a`..`f6a9775`, 2026-05-05)
- ~~Polish 21-inference; add 22-tags, 23-control, 24-runtime~~
  (commit `f6a9775`, 2026-05-05)
-->

- ~~**\[P0\] FFI errors cryptic when a transitive DLL is missing.**~~
  Replaced ``ffi_load couldnt load `ui/show_gfx` `` with a 3-way
  diagnostic. (commit `4de11ee`)
- ~~**\[P0\] UI demo fails on machines with no audio device.**~~
  Made `Mix_OpenAudio` failure non-fatal; sound API no-ops when
  unavailable. (commit `4de11ee`)
- ~~**\[P0\] `_list_ has no method is_token` in `text.sexp`.**~~
  Added missing `_.is_token_ = 0` fallback in `src/sexp.s`.
  (commit `4de11ee`)
- ~~**Examples 17–24** covering bits, tables, strings, custom
  generic types, Prolog-style inference, tag intrinsics, non-local
  return, GC + reflection.~~ (commit `f6a9775`)
- ~~**`AI.md` cheat sheet** including 20 gotchas distilled from
  writing the example set.~~ (commit `61f2c31`)
- ~~**`dev/TODO.md`** -- this file. Structured punch-list seeded
  from the Adaptive Map review.~~ (commit `4ef26fa`)
- ~~**Example 25-lexmacro** demonstrating the NCM lexical macro
  processor. Also rewrote ncm.h's "CONFIDENTIAL / DON'T DISTRIBUTE"
  banner that contradicted the dual MIT/Apache-2.0 LICENSE.~~
  (commit `41d0770`)

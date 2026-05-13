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

### \[P1\] **FFI-1** Drop cinvoke; ship custom x86-64 trampolines

> **Where:** [`cinvoke/`](cinvoke/), [`runtime/bltin.c`](runtime/bltin.c)
> (`_ffi_call`), [`src/macro.s`](src/macro.s) (`ffi_begin`)
> **Problem:** `cinvoke/` is the only third-party component that
> isn't dual-MIT/Apache (modified BSD-3 with a non-endorsement
> clause). Legally compatible, but it forces every redistributor
> to retain cinvoke's notice and makes Symta's own
> licence-purity claim ("dual MIT / Apache-2 throughout")
> technically false. Performance side: cinvoke is general-purpose
> (supports runtime-dynamic signatures, struct-by-value, …),
> and Symta uses a strict subset (`int`, `u4`, `s4`, `ptr`,
> `float`, `double`, `text`, `void`). Cinvoke costs ~50–100 ns
> per call; with ~50 K gfx calls per frame in the Management
> window that's ~2.5–5 ms / frame in trampolines alone.
> **Fix:** hand-coded inline-asm trampoline tailored to Symta's
> signatures, for two ABIs: x86-64 Windows (rcx/rdx/r8/r9
> register passing) and x86-64 SysV (rdi/rsi/rdx/rcx/r8/r9). The
> Symta side knows the signature at FFI declaration time, so the
> trampoline just does per-argument register/stack loads from a
> `dyn` array, indirect call, and return-value marshalling.
> ~300 lines of inline asm per ABI plus a dispatch table.
> Estimated 30–50 % faster per call than cinvoke. Implement
> Windows x64 first (developer's platform); keep cinvoke as a
> `--use-cinvoke` build flag for the SysV path until that's
> tested too. Net: `cinvoke/` directory deleted,
> `runtime/Makefile.w64` no longer links `-lcinvoke`,
> `LICENSE` claim becomes literally true.
> **Regression strategy:** new `tests/ffi/` project exercising
> every FFI type combo (return × argument × variadic-ish patterns
> the gfx setters use). Run all 29 gfx tests through the new
> trampoline. The `--profile=management` harness should produce
> identical counts (blits, get_pic), faster timings.
> `effort: multi-week`

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

### \[P0\] **AM-1** `amGidGet` falls through `AM_GENERIC` / `AM_TEXT` uninitialised

> **Where:** [`runtime/am.h:64`](runtime/am.h)
> **Problem:** the `CASE` only handles `EMPTY`, `INT`, `BITMAP0`,
> `BITMAP1`. For `GENERIC` and `TEXT`, `r` is returned
> uninitialised (latent UB). The cls layer presumably guarantees
> integer-keyed tables here, but the contract isn't enforced.
> **Fix:** add explicit branches that either dispatch correctly
> (TEXT can resolve, GENERIC can call `dhGet` with the ref as
> key) or `_fatal` so the violated assumption surfaces
> immediately.
> `effort: 30 min`

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

### \[P1\] **AM-3** `GENERIC` mode pays MCALL per probe step

> **Where:** [`runtime/dh.h`](runtime/dh.h)
> (`dhEqual_`, `dhHash_`)
> **Problem:** every probe (including misses, including during
> grow) does a full Symta method dispatch — build arglist,
> vtable lookup, return through unbox. A million-key generic
> map crawls. Lua / Python / Ruby all inline int/text/symbol
> keys and fall through to method dispatch only for genuine
> user types.
> **Fix:** in `dhEqual_` / `dhHash_`, branch on `O_TAG(key)`
> first and inline the int and text cases. Most "generic"
> tables in real code are still 90 %+ int or text mixed with
> one stray symbol that forced GENERIC mode.
> `effort: afternoon`

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

### \[P1\] **AM-5** `nh_t` load factor is conservative (50 %)

> **Where:** [`runtime/nh.h:85`](runtime/nh.h)
> **Problem:** `NH_NEEDS_GROW(nh)` triggers at half-full.
> Linear probing without tombstones makes 50 % defensible, but
> for ECS tables of 100k–1M entries you're allocating ~2× the
> memory you actually need.
> **Fix:** switch to Robin Hood probing or double hashing —
> either buys 75–80 % load factor with similar lookup latency.
> Robin Hood is ~30 lines of additional bookkeeping; the
> existing linear probe stays correct as a fallback.
> `effort: afternoon`

### \[P2\] **AM-6** Two underlying hashmaps (`stb_ds` and `nh_t`)

> **Where:** [`runtime/am.h`](runtime/am.h) uses `stb_ds` for
> `AM_INT` / `AM_TEXT`, `nh_t` for `AM_GENERIC`
> **Problem:** two probing strategies, two memory layouts,
> two places to tune load factor. Performance work has to be
> done twice. stb_ds is also harder to specialise for our
> cases.
> **Fix:** standardise on `nh_t`. It's already templated for
> typed keys via macro instantiation. Drop stb_ds usage from
> am.h entirely.
> `effort: afternoon`

### \[P2\] **AM-7** `AM_BITMAP0` write-zero looks like delete

> **Where:** [`runtime/am.h:163–211`](runtime/am.h)
> **Problem:** writing `0` to a `BITMAP0` table compares
> `value == void_val` (also `0`), takes the delete branch, and
> `nbDel`s the key. So `T.K = 0` on a bitmap-0 table makes
> `T.has K` return false — even though logically the user just
> wrote a value. Consistent with how AM_VOID works elsewhere,
> but surprising and undocumented.
> **Fix:** at minimum, document the contract. Better: split
> "set value" from "remove key" so the behaviour is explicit.
> `effort: 30 min` (docs) / `afternoon` (split API)

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

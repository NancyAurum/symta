# Symta — TODO

The consolidated punch-list.  Items here have been **reviewed**,
**scoped**, and have a **concrete proposed fix**, so they can be
picked up and worked on without re-doing the analysis.

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

Recent compile-time numbers for context (game cold, 21 k LOC,
i7-12700H / w64devkit):

| stage | cold | comment |
|---|---|---|
| pre-consolidation (`0b9e2c4`) | 51-54 s | baseline |
| after reader-consolidation (`2129767`) | 38-40 s | C reader wins ~25 % |
| after `_.><`/`_.<>` C builtin (`a341309`) | 20-23 s | dispatch chain win |

The remaining items chain together as

```
  OP-2, OP-3, OP-4         (small wins, no schema change)
    ↓
  RT-4, RT-6               (cache locality, no schema change)
    ↓
  TYPE-1, TYPE-2, TYPE-3   (parser + registry + inference, no
                            codegen change)
    ↓
  TYPE-4                   (unboxed locals + opcodes -- the
                            biggest single schema move)
    ↓
  TYPE-5, TYPE-6, TYPE-7   (each shippable on its own)
    ↓
  NATIVE-PRE               (freeze schema)
    ↓
  NATIVE-1, NATIVE-2       (x86 + ARM AOT in SBC)
    ↓
  NATIVE-3                 (optional JIT layer on top)
```

The OP-* and RT-* items are sub-10 s territory (the engine
runs faster on the same opcodes).  TYPE-4 + NATIVE land near
C99 latency on numeric loops.  See
[`docs/type-system.md`](docs/type-system.md) for the type
system design.

---

## FFI / reader

### \[P2\] **READER-1** Compiler float-to-text loses precision below 1e-8

> **Where:** [`runtime/main.c`](runtime/main.c) `print_object_r`
> (the `T_FLOAT` branch), called via `float.as_text` which is
> called by [`src/compiler.s`](src/compiler.s) `ssa_atom` when
> serialising `ldflt K X` to SIF text.
> **Problem:** `print_object_r` formats floats with `%.8f` (8
> decimal places).  For values below ~1e-8 the format produces
> `"0.00000000"` (all zeroes) and the trailing-zero stripper
> turns that into `"0.0"`.  When the compiler then emits
> `ldflt L5 0.0` and `sif2sbc` re-parses it via `strtod`, the
> result is `0.0` -- silent precision loss.
> **Fix sketch:** use `%.9g` (9 significant digits — enough to
> round-trip every float32 value).  Two side-effects to address:
> 1. `%.9g` switches to scientific notation for very small /
>    very large values (`1e-09`, `1.5e+15`).  The reader
>    currently **cannot** parse scientific-notation literals
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
> `effort: 1-2 days`

---

## Runtime — opcode inlining (OP-*)

The pattern that paid off most for cold compile so far: a Symta
operator defined in `core_.s` whose body chains two or more MCALLs
gets replaced with a direct C builtin handler, registered against
`T_OBJECT` post init_subtypes.  Type-specific overrides
(`int.><`, `text.><`, …) still win via METHOD_FN dispatch
ordering; only the heap-type fallback path is shortcutted.

Each instance saves ~150-200 ns per call.  Multiplied by the
~10⁷-10⁸ call sites a 21 k LOC game compile hits, single-digit
percentage of cold compile time per fix.

### \[P1\] **OP-2** Compile-time peephole: `X <> No` → `SBC_GOT`, `X >< No` → `SBC_NO`

> **Where:** [`src/macro.s`](src/macro.s) lines 619-638 (the
> existing `><` / `<>` peephole), [`src/compiler.s`](src/compiler.s)
> ssa_form (the `_no` / `_got` cases at lines 577-579).
> **Problem:** the existing peephole emits a direct `_same`/`_vary`
> form only when the LEFT operand is a literal `int` or `fixkw`.
> A common pattern in the compiler / macroexpander is `Var <> No`
> (or `Var >< No`) where the left is a runtime value -- those still
> compile to a full MCALL into `_.<>`/`_.><` (50 ns now after
> commit `a341309`, was 250 ns before).  The runtime already has
> SBC_GOT and SBC_NO opcodes (~5 ns each) and they have the exact
> same semantics: `got X` ≡ `X <> No`, `no X` ≡ `X >< No`.
> **Fix:** extend the macro at `src/macro.s:619-638` to also detect
> "right operand is the literal `No`" and emit `_got A` / `_no A`
> forms.  Right-operand-literal detection is one extra `case`
> branch.  The SSA layer already lowers `_got`/`_no` to single
> opcodes.
> **Win:** 50 ns → 5 ns per call (10×) on `<> No` / `>< No` sites.
> Empirically the compiler itself uses these patterns less than my
> initial estimate (it prefers `got X`), but the user-visible
> idioms in macro.s and uim.s do hit them.  Even a 1-2 % cold-
> compile shave is cheap to claim with a one-line macro change.
> `effort: 30 min` (with re-bench)

### \[P2\] **OP-3** `_.<<` / `_.>` / `_.>>` follow the `_.<>` precedent

> **Where:** [`src/core_.s`](src/core_.s) lines 49-51.
> **Problem:** these three operators are still defined Symta-side
> as `not B < Me` / `B < Me` / `not Me < B`, each chaining a
> `<` MCALL plus a `not` opcode.  For int/float operands the
> direct type-specific `<<` / `>>` builtins win first, but for
> any other heap type (list, tag, view, cons, …) we fall back
> through these chains.  The same recipe that worked for `_.<>`
> /`_.><` would work here.
> **Fix:** register `_.<<` / `_.>` / `_.>>` as C builtins on
> `T_OBJECT` post `init_subtypes`, doing `a <= b` / `b < a` /
> `a >= b` on the raw dyn bits.  Reuse the `add_method`
> redefinition-allow special case that already covers
> `api.m_equal` / `api.m_ne_` (extend to `m_lte_`/`m_gt_`/`m_gte_`).
> Delete the Symta-side defs.
> **Caveat:** the raw-dyn comparison semantics are well-defined
> for these only when the types match; mixed-tag comparisons get
> nonsensical results (T_NO < T_LIST is meaningless).  The current
> Symta defs propagate the comparison into `<` which DOES go
> through type-specific dispatch.  If we replace with raw-dyn,
> we lose that.  In practice these operators are rarely called
> with mismatched heap types, but check with a probe before
> deleting -- some Symta code might rely on it.
> **Expected win:** smaller than OP-2; these are less hot.
> `effort: afternoon` (with the cross-type-comparison audit)

### \[P2\] **OP-4** `meta.__` forwarding is a triple-MCALL per call on wrapped AST nodes

> **Where:** [`src/core_.s`](src/core_.s) lines 819-822.
> **Problem:** every method call on a meta-wrapped AST node goes
>
>   1. MCALL `.foo` on meta → no method, falls to `meta.__`
>   2. `meta.__` body runs: `Args.0 = $object_; Args.apply_method(Method)`
>   3. MCALL `apply_method` on Args -> b_list_apply_method builtin
>   4. Builtin then dispatches to the actual `.foo` on the unwrapped object
>
> So `(meta_wrapped_list).head` is ~3 MCALLs instead of 1.  The
> compiler and macroexpander hit this on every AST node that
> carries source-position meta, which is most of them.
> **Fix sketch:** a C-side fast path for `_.__` (the universal
> sink) that recognises the meta-forwarding pattern and inlines
> it: read `O_TAG(receiver)`, if it's T_meta, swap Args.0 to the
> object_ slot and re-issue the dispatch in one shot.  Or
> structurally: split the meta wrapper into a tag-only object
> (no Symta type) and a side-table for the meta value, so method
> dispatch on the AST node never goes through a wrapper at all.
> **Why now:** likely the next big win in the same vein as
> OP-1 (which was the `_.>< / _.<>` consolidation in commit
> `a341309`).  Worth bench-comparing the two approaches
> structurally before picking one.
> `effort: weekend` (design + bench)

### \[P3\] **OP-5** Method dispatch via type-tag jump table

> **Where:** [`runtime/sbc.c`](runtime/sbc.c) `MCACHE_CALL`,
> [`src/compiler.s`](src/compiler.s) `ssa_apply_method`.
> **Problem:** MCALL on `_.foo` defined for several types (e.g.
> `is_text`, `is_keyword`, `is_int` predicates) dispatches via
> the mcache.  If the call site is monomorphic the mcache hits;
> if polymorphic (the body sees lists / tags / closures / texts)
> it thrashes.  Each miss costs ~50 ns.
> **Fix:** compiler peephole that recognises `case X is_T: ...`
> patterns and emits a tag-test opcode (new SBC_TAG_EQ
> [tag, src, branch_dst]) rather than an MCALL.  Each
> `case X [Sym @Rest]:` head test could similarly become
> SBC_FXNTAG + SBC_IMMEQ + jump (already possible with existing
> opcodes; just isn't peepholed today).
> `effort: weekend`

---

## Runtime — bytecode interpreter (RT-*)

### \[P3\] **RT-1** Bytecode dispatch via computed gotos — *measured, not a win*

> **Status:** the threaded variant compiles behind
> `#ifdef SBC_THREADED_DISPATCH` and passes the full test sweep +
> drift, but runs **~8 % slower on average** (gcc 12.2 / Win64 /
> i7-12700H) -- not the 10-30 % faster the literature predicts.
> Root cause from disassembly: GCC re-emits `lea dt(%rip),%reg`
> at every per-opcode dispatch site instead of pinning the table
> into a register, inflating per-site cost and growing the
> function 16 %.  Code stays in tree; default is off.
> **If somebody wants the 8 %:** try Linux `-fno-pic`, hand-roll
> the dispatch in inline asm with explicit register binding,
> or wait for a future GCC.

---

## Runtime — cache locality (RT-4, RT-6)

### \[P1\] **RT-4** Object age lives in a parallel side array

> **Where:** [`runtime/symta.h:106-109`](runtime/symta.h)
> (`O_AGE`/`O_THEAP`), [`runtime/common.h:402-408`](runtime/common.h)
> (`lsetm` write barrier), [`runtime/gc.c:17-46`](runtime/gc.c)
> (`GC_REC3` trace).
> **Problem:** `gc_heap_t` (`common.h:291-304`) has `void
> heap[FULL_HEAP_SIZE]` (~512 MB at full size) and a parallel
> `uint8_t theap[FULL_HEAP_SIZE]` (~64 MB) in completely disjoint
> pages -- the prefetcher never sees them together.  Every
> generational write barrier (`lsetm`, fired on every store of
> one heap object into another) does `GC_OLDER(base, value)`,
> which is two `O_AGE` lookups.  So every `LSET` touches three
> cache lines: the object header (`O_HDR`), `theap0[gid_base]`,
> and `theap0[gid_value]`.  `O_AGE` is also read on every GC
> pointer trace and every closure call's `O_HG`.
> **Fix:** the high byte of `O_CODE` is unused -- functions and
> types live in `hooks_heap`/`types` arrays with ≤24-bit indices.
> Steal it for an inline age:
> `gc_head_t { uint8_t age; uint8_t flags; uint16_t code_lo;
> uint32_t size; }`.  `O_AGE(o)` then loads the same cache line
> as `O_HDR(o)`.  The barrier drops from 3 lines to 1.
> **Gotcha found in a May 2026 prototype (reverted before
> commit):** `GC_REDIR(o,p)` writes the redirect pointer `p`
> across the entire 8-byte `gc_head_t`, which obliterates any
> "age" nibble parked inside the header.  After a relocation,
> an inline read of `O_AGE` therefore returns whatever bits of
> the redirect pointer happen to occupy that slot.  So the move
> can't just be "put age inline" -- the MOVED flag still has to
> live somewhere that survives the pointer write.  Either keep
> `theap0[gid-1] == GC_MOVED` purely as a relocation marker
> (no longer storing age), or use a tagged-redirect-pointer
> scheme where one of the low alignment bits of `p` doubles as
> a MOVED flag.  Picking between them is what makes this a
> weekend job, not an afternoon.
> `effort: weekend`

### \[P2\] **RT-6** Method dispatch table is 4 KB per type + pointer-chase

> **Where:** [`runtime/common.h:206-235`](runtime/common.h)
> (`type_t`, `method_node_t`),
> [`runtime/main.c:101-184`](runtime/main.c) (`get_method`,
> `get_method_node`, `init_method`).
> **Problem:** `type_t` is **4144 bytes** per type (~48-byte
> header + `method_node_t *methods[512]` = 4096 bytes of
> pointer heads).  Each MCALL with a cold mcache touches
> `types[tag]` (header line) → `types[tag].methods[hid]` (a
> second line, `hid<<3` bytes away from the header) → the
> `method_node_t` chain.  Chain nodes are page-allocated by
> `init_method` so `*next` walks chase pointers across pages.
> 3-5 cache lines per megamorphic miss.
> **Fix:** the existing comment at `common.h:233` already wants
> perfect hashing.  Concretely: replace the 512-bucket
> head-pointer array with a packed `(mid, fn)` open-addressed
> probe array sized to the actual method count per type.  For
> typical types with <16 methods, the whole table fits in 1
> cache line.  Same restructure as AM-pack-v2 applied at the
> method-table level.
> **Why now:** the mcache hides this on monomorphic call sites,
> but every polymorphic dispatch -- list/text/closure ad-hoc
> code, anything in `cls.s` -- falls into the slow path.
> `effort: afternoon`

---

## Adaptive map — table internals

The adaptive map is what backs every Symta hash, every ECS column,
every cache.  The 2026 push (AM-1..AM-15, AM-pack-v2) landed Robin
Hood probing at 75 % load, hash caching, the `{key,val,hash}`
inline slot layout, and replaced both stb_ds backings with
`nh_t`-derived `ih_t`/`th_t`.  Items below are what's left.

### \[P1\] **AM-2** No dense-iteration story for ECS columns

> **Where:** [`runtime/am.h:573`](runtime/am.h) (`amL`),
> [`src/cls.s:157`](src/cls.s) (`each_`).
> **Problem:** `amL` and `gid_refs_` build a list by walking the
> hashmap on every call.  ECS systems iterate component columns
> every frame; bitmap iteration touches every page even for low
> populations, and INT-mode iteration walks every hash slot.
> State of the art (flecs, Bevy, EnTT) keeps entities packed
> densely so the per-frame iterator is essentially `for i = 0..n`
> over a flat array — typically 5-20× faster.
> **Fix:** pair each `BITMAP*` and `INT` table with an optional
> packed-dense vector (`dense_index[gid]` and `id_at_dense[i]`).
> The bitmap already provides O(1) presence; adding the parallel
> arrays gives O(1) insert/delete and sequential-array-speed
> iterate.  Standard sparse-set design.
> `effort: weekend`

### \[P1\] **AM-4** `BITMAP*` promotion to `INT` is lossy and one-way

> **Where:** [`runtime/am.h:469-512`](runtime/am.h).
> **Problem:** the moment **any** value diverges from the chosen
> 0 or 1, the entire column promotes to `AM_INT` — every entity
> now costs ~16 B even if 99 % of values are still 0 or 1.
> Common case: `is_visible` is mostly 1, a handful of entities
> have visibility levels 0..7 — currently you pay full INT cost
> for the entire column.
> **Fix:** add `AM_BITMAP_PLUS` mode = a bitmap PLUS a small
> `nh_t` of divergent gids.  Promote to full INT only when
> exceptions exceed ~10 % of total.  The pieces (`nb_t` + `nh_t`)
> already exist; the change is mostly in `amSet`.
> `effort: weekend`

### \[P3\] **AM-9** `nh_t` capacity is `uint32_t` — DEFERRED

> Caps tables at 2³² entries.  With Symta's heap layout
> (`GID_BITS = 48`, ~24 GB heap max), no realistic workload
> reaches anywhere near 2³² entries in a single table.
> Widening to `uint64_t` would cost 4 B per table header for a
> capability no one currently needs.  Will revisit when a real
> workload demands it.
> `effort: 30 min` (when wanted)

### \[P3\] **AM-10** String interning shared with `AM_TEXT`

> **Where:** [`runtime/am.h`](runtime/am.h) `AM_TEXT` paths.
> **Problem:** repeated equal-string keys are deduped by `th_t`,
> but `text_chars(key)` runs every probe.  Frequency-table
> workloads pay it on every increment.
> **Fix:** intern text keys (or tag the text with its hash on
> first use) so probe cost drops to a pointer compare.
> `effort: weekend`

---

## Compiler / parser

### \[P2\] **CORE-5** Better diagnostics for common parser pitfalls

> **Where:** [`runtime/reader.c`](runtime/reader.c),
> [`runtime/bltin.c`](runtime/bltin.c) error printer.
> **Problem:** several pitfalls produce confusing errors --
> nested `[...]` in string interpolation, `(-5)` parsing as a
> call, the chicken-and-egg "undefined variable" for `Var = X`
> first use, `==` typos that mean nothing (should be `><`).
> **Fix:** specific error messages -- "did you mean `\[ … \]` to
> escape brackets in a string?", "first assignment to `Var`
> should be `Var X` not `Var = X`", "`==` doesn't exist in
> Symta; use `><` for equality", "`!=` is `<>`".  The reader
> already has the column info.
> `effort: weekend`

### \[P2\] **CORE-6** A linter

> **Problem:** the gotchas catalogued in `symta-review.md` and
> `AI.md` are pure syntactic patterns; a linter that pre-flags
> them would close the biggest learning-curve gap.  `Var =`
> first-use, multi-arg `say`, nested string-interp brackets,
> `pass` outside loops, `==` / `!=` typos.
> **Fix:** a `symta --lint` mode that runs the reader, walks
> the AST, and reports pattern violations.  None of these need
> type inference.
> `effort: weekend`

### \[P2\] **CORE-7** Generated stdlib reference

> **Problem:** every method on every built-in type lives in
> `src/core_.s` and `src/uim.s` but there's no greppable
> reference.  New users read source.
> **Fix:** a doc-from-comments tool that walks `src/*.s`, emits
> Markdown per type (`text.*`, `list.*`, `table.*`, `gfx.*`, …)
> with one-line descriptions and source file:line.
> `effort: weekend`

### \[P3\] **CORE-8** Compiler micro-optimisations

> **Where:** [`src/compiler.s`](src/compiler.s).
>
> 1. ~~**Constant-folding literal arithmetic.**~~ Landed at
>    commit `3fbdac9`.
>
> 2. **Dead-store elimination on unread SSA registers.**  Open.
>    Requires a real liveness pass; the compiler doesn't track
>    it today.  Effort: weekend.
>
> 3. **Unrolling `times I N:` for known small `N`.**  Open.
>    Macro-level transform in `src/macro.s`; needs to be careful
>    about `pass` / `done` body labels and break/continue.
>    Effort: weekend.
>
> 4. **Computed-jump compilation of `case` chains with all-
>    literal patterns.**  Open.  Requires a new SBC opcode
>    (jump table) and an emission path in the compiler.
>    Effort: weekend.
>
> **Gotcha for one-sided identity folds** (`X+0 → X`, `X*1 → X`,
> etc.): these look safe but break the existing `[_add 0 No]`
> behavior because the SBC `fxnadd` opcode is bit-level int
> arithmetic with non-trivial output when one operand is the
> T_NO tag.  Examples/14-quirks.s pins this.

---

## Type system (TYPE-*) — prerequisite for serious NATIVE speedup

See [`docs/type-system.md`](docs/type-system.md) for the full
design exploration.  TL;DR: a gradual, layered type system in
the mould of TypeScript + F\* + Common Lisp declarations.
Three goals -- optimisation, correctness defence beyond runtime
tag tests, and unboxed primitives that don't fit in a 64-bit
tagged dyn (`I64`, `F64`, SIMD vectors).

Without TYPE-4 the NATIVE roadmap only buys a 1.5-2× win on
MCALL-heavy code (saving dispatch overhead).  *With* TYPE-4's
unboxed locals + arithmetic opcodes, NATIVE buys 5-20× on
numeric loops -- the actual "near C99 latency" claim.

### \[P1\] **TYPE-1** Declaration syntax (parser only)

> **Effort:** weekend.
> **Where:** [`runtime/reader.c`](runtime/reader.c) +
> [`src/macro.s`](src/macro.s).
> **Adds:** `Var:Type` in arg lists, `Fn args -> RetType`,
> `type alias Name = ...`.  No runtime / compile-time check at
> this stage -- types live in the AST and are ignored by
> macroexpand.  Required substrate; nothing else lands without
> this.

### \[P1\] **TYPE-2** Built-in type registry + runtime introspection

> **Effort:** afternoon.
> Define `Int`, `I64`, `U64`, `F32`, `F64`, `Bool`, `Text`,
> `Tag`, `Byte`, `Dyn`, `List<_>`, `Fn<_,_>` as first-class
> runtime type objects.  `typeof X` and `X :> T` queries.
> Foundation for inference and reflection.

### \[P1\] **TYPE-3** Local Hindley-Milner inference for typed regions

> **Effort:** 1-2 weeks.
> HM-style inference within function bodies when some args are
> typed; propagation through `=` bindings, `case` patterns, and
> method calls (using the dispatch table to narrow result types).
> Flow narrowing in `case` arms.  Compile-time errors for
> mismatches in fully-typed regions; silent dyn-coerce in
> partial regions.

### \[P1\] **TYPE-4** Unboxed primitive locals + arithmetic opcodes

> **Effort:** 3-4 weeks.
> The big lever.  Extend `frame_t` with `I[]` (int64) and `F[]`
> (float64) lanes alongside the existing `L[]` (dyn) lane.
> Compiler tracks each local's lane.  New SBC opcodes
> `SBC_I64ADD`, `SBC_F64ADD`, `SBC_BOX_I64`, `SBC_UNBOX_I64`,
> etc.  Auto-box at the typed→untyped boundary, auto-unbox the
> other way.
> **Schema change:** this is the biggest single bytecode-format
> move.  Must land BEFORE `NATIVE-PRE` freezes the ABI.
> **Payoff in isolation:** ~5-10× on arithmetic-heavy loops
> even without NATIVE, because the interpreter dispatch can
> skip MCALL on typed ops.

### \[P2\] **TYPE-5** Typed arrays + tensor zero-copy

> **Effort:** 2-3 weeks.
> New `Array<T>` built-in, layout-compatible with C arrays.
> Bounds-checked subscripts (elidable per TYPE-6).  FFI / GPU
> kernels receive `&array[0]` and `n` directly -- no marshalling.
> Existing `bytes` type subsumed as `Array<Byte>`.  This is what
> makes "say tensors onto GPU" practical without round-tripping
> every element through a dyn.

### \[P2\] **TYPE-6** Refinement types + bounds-check elision

> **Effort:** 4-8 weeks.
> Predicate-attached types: `Int{Me > 0}`, `Index<n>`.  Most
> checks run at runtime initially; cheap compile-time cases
> (constant predicates, simple range narrowing) elide.  Catches
> off-by-one bugs at compile time, eliminates ~10-20 % of
> runtime checks on array-heavy code.

### \[P2\] **TYPE-7** Generic + sum types

> **Effort:** 2-3 weeks each.
> `type Pair<A B>: ...` (generics, compile-time substitution),
> `type Color: Rgb(r:Byte g:Byte b:Byte) | Hsv(...)` (tagged
> unions with exhaustive pattern matching checks).

### \[P3\] **TYPE-8** Type classes — *deferred*

> Likely unnecessary given Symta's existing multi-dispatch via
> `cls`.  Revisit when a concrete use case demands ad-hoc
> polymorphism the current dispatch can't express.

### \[P3\] **TYPE-9** Effect system — *research*

> Track allocation, GC-triggering, error-throwing, mutability
> in the type.  Useful for proving hot loops are alloc-free.
> No path to landing yet; speculative.

---

## Native code generation (NATIVE-*) — the big multiplier

The five-year bet, gated behind TYPE-4 + the
bytecode-stability promise.

The vision: each `.sbc` file carries one or more **native
sections** alongside its bytecode.  At load time the runtime
picks the section that matches the host CPU (x86-64 or ARM64),
applies relocations, and dispatches functions straight to native
code with no interpretation overhead.  The bytecode section
stays as the universal fallback for unrecognised architectures
and for debugging.

Why this is the next-biggest lever: today every Symta-level op
goes through an opcode dispatch (~3 ns inner loop) plus the
work of the opcode itself (~20-50 ns for simple ones, ~100 ns
for MCALL).  A native compile in-lines the dispatch entirely
and exposes the actual work to gcc-level register allocation
and the host's branch predictor.  Conservatively 2-5× on
compute-heavy code; the interpreter dispatch and MCALL
indirection vanish for monomorphic call sites.

CPU landscape over the project's relevant horizon: x86-64
(Intel + AMD, with the recent x86 ecosystem alliance) and
ARM64 (Apple Silicon, AWS Graviton, Windows on ARM).  Shipping
both sections in the same SBC covers >99 % of dev machines
indefinitely.

### \[P1\] **NATIVE-PRE** Bytecode + ABI stability promise

> **Where:** [`runtime/sif.h`](runtime/sif.h) opcode enum,
> [`runtime/symta.h`](runtime/symta.h) ABI (`api_t`, calling
> conventions, `frame_t` layout, `gc_head_t` layout).
> **Problem:** the SBC layout has been moving steadily (CORE-1
> v2 lineno side table, RT-7 mcache D-side, RT-8b frame-and-locals
> colocation, SBC_MAGIC + SBC_REVISION 1).  None of this matters
> as long as bytecode is the only consumer -- the runtime
> regenerates SBCs on every source change.  Native code can't:
> a 100 KB AOT-compiled function blob can't be regenerated at
> load time, so the ABI it was compiled against has to still
> exist when it runs.
> **Fix:** freeze the opcode set, `api_t` layout, calling
> convention, frame layout (including TYPE-4's `I[]` and `F[]`
> lanes), and `gc_head_t` shape under `SBC_REVISION = 2` once
> the remaining OP-*, RT-*, and TYPE-1..4 items have landed.  Document each opcode with its inputs,
> outputs, side effects, and exception behaviour.  Document
> the runtime services native code can call into: `gc_alloc`,
> MCACHE miss handler, type-system queries, FFI marshalling.
> Bump `SBC_REVISION` only when the freeze breaks; old SBCs
> get a clean "recompile against revision N" error.
> **Why now / why not:** the bytecode is too in-flux to commit
> a native compiler to it yet.  Targets RT-4, RT-6, OP-2, OP-3,
> OP-4 should land first; THEN freeze.  Until then, native
> compilation is risky -- every bytecode change invalidates
> every native section.
> `effort: weekend` (spec writing + audit) + indefinite
> stability commitment

### \[P3\] **NATIVE-1** x86-64 AOT code generator

> **Where:** new directory `runtime/native/x64/`,
> [`src/compiler.s`](src/compiler.s) post-SSA emission stage,
> [`runtime/sbc.c`](runtime/sbc.c) load path.
> **Approach:** add a code generator that consumes the same SSA
> representation the existing bytecode emitter uses and writes
> raw x86-64 machine bytes into a new `nat_x64` section of the
> SBC.  Function entry is a `Closure(handler=&native_disp,
> payload=&nat_blob[offset])`-style descriptor; the dispatch
> handler is one indirect jump.
> **Code generation choice:** three options.
>
> 1. **Hand-rolled emitter** -- a few hundred lines of C that
>    knows how to encode the ~30 hot opcodes (FXNADD, MCALL,
>    JMP, BZ, MOVE, LIST, LGET, …).  No external dependency.
>    Manageable.  Cold opcodes can fall back to a "call into
>    the interpreter for this op" trampoline.
> 2. **DynASM** (LuaJIT's macro-assembler).  Battle-tested,
>    cross-arch (x86 + ARM with shared macros), small.
>    Adds a build-time Lua dependency.
> 3. **MIR** (V. Makarov's minimal JIT toolchain).  Real
>    register allocator and basic optimisations.  Self-contained
>    C source.  Larger up-front integration work.
>
> The project ethos points at option 1 (no external deps, stays
> in tree) with the understanding that option 3 is the
> long-term destination if/when scalar perf matters more than
> code size.
> **Calling convention:** stick to System V on Linux/macOS,
> Win64 on Windows.  Symta dyns fit naturally in `rdi`/`rsi`
> argument registers.  Stack frame layout matches RT-8b's
> co-allocated `frame_t + locals` block.
> **GC integration:** native code maintains the same `api.frame`
> chain it does in the interpreter (one store per CALL); the
> existing GC root walker keeps working.  Native code calling
> into the runtime (alloc, MCALL miss) goes through the same
> entry points the bytecode interpreter uses.
> **Expected gains:** 2-5× on compute-bound code.  Mixed code
> dominated by MCALL stays at ~1.5× because the dispatch
> already routes through cache-friendly mcache.
> `effort: multi-week` (4-6 weeks for the MVP, plus
> bench/maintenance)

### \[P3\] **NATIVE-2** ARM64 AOT code generator

> **Where:** new directory `runtime/native/arm64/`, same
> integration points as NATIVE-1.
> **Approach:** same as NATIVE-1 but emitting AArch64.  Most of
> the integration code (section format, dispatch wrapper, GC
> hooks) is shared with NATIVE-1; only the opcode-emission
> tables differ.  If DynASM is the chosen route (option 2 in
> NATIVE-1), one .dasc source covers both archs.
> **Why both:** shipping only x86 would leave Apple Silicon /
> Graviton / Windows-on-ARM developers on the interpreter.
> The user-facing perf story should not depend on the host's
> ISA.
> `effort: multi-week` (concurrent with NATIVE-1 if DynASM,
> sequential if hand-rolled)

### \[P3\] **NATIVE-3** JIT fallback (alternative to AOT)

> **Variation on NATIVE-1/2.**  Instead of baking native code
> into the SBC at compile time, compile lazily at first call:
> the closure's `handler` field initially points at a "compile
> me" trampoline that emits native code, patches the closure,
> and resumes.  Lower SBC file size; higher startup latency;
> simpler distribution (one SBC works on any arch, no need to
> regenerate for new platforms).
> **Why we might prefer AOT instead:** the cold compile of a
> 21 k LOC project is dominated by the compiler itself, which
> would benefit from being native-compiled.  AOT means the
> compiler's SBC ships with native sections built in; JIT
> means cold compile pays the JIT-warmup tax too.
> **Decision:** revisit after NATIVE-PRE and NATIVE-1 land.
> The two aren't mutually exclusive -- JIT for everything
> with no native section, AOT for shipped libraries.

---

## Open bugs from the SoM revival

These were filed during the May 2026 reading-the-game pass.
None block the runtime / compiler tests; all surface as
"documented in `sbe.txt`, broken in practice" papercuts for
example-writers.

### \[P2\] **B10** `[@list]` inside string splice triggers `_insert` bug

> **Where:** [`runtime/reader.c`](runtime/reader.c) or
> [`src/compiler.s`](src/compiler.s).
> **Reproducer:** `Words [a b c]; say "x [@Words]"` →
> `unknown symbol _insert (compiler bug)`.
> **Workaround for users:** build the list into a variable or
> call `.text` before interpolating.
> `effort: afternoon`

### \[P2\] **B11** Four `{}` / `(...)` idioms from `sbe.txt` are broken

> **Where:**
> [`tests/macros/cases/idioms.s`](tests/macros/cases/idioms.s)
> header.
> **Problem:** `L(:@_ -P&=0)`, `L(/~ P @_)`, `L(@_ ~<P @_)`,
> and `10{&1*2:}` all fail at macroexpand.  Each is documented
> in `sbe.txt` as a working idiom.
> `effort: weekend`

### \[P2\] **B12** Three `Xs[...]` subscript ops are broken

> **Where:** [`examples/29-subscript.s`](examples/29-subscript.s)
> header.
> **Problem:** `Xs[-I]` (negative index from end),
> `"(...)"[0!'{' ~!'}']` (replacement on a fixtext), `Xs[3!]`
> (locate-by-value).
> `effort: weekend`

### \[P2\] **B13** Anaphoric-loop `l~^` source form is broken

> **Where:** [`examples/30-anaphoric.s`](examples/30-anaphoric.s)
> header.
> **Problem:** `l~^[…]` errors with `undefined variable I__N`.
> Users have to bind to a named list variable first.
> `effort: afternoon`

---

## UIM / examples

### \[P3\] **UI-1** `examples/26-fact-engine/` project

> **Problem:** the current `21-inference` example uses a
> simplified table-of-tables design.  The real `som_src/fact.s`
> uses `cls fact_symbol` + interned ids + `cls_tbl_`.
> **Fix:** new project example with the cls-based engine.
> `effort: afternoon`

### \[P3\] **UI-2** Sparse-set / dense-iteration cookbook

> **Problem:** users wanting to write fast ECS systems on top
> of `cls` need to know how to keep their own dense indices.
> Until AM-2 lands, that's the only path.
> **Fix:** `dev/cookbook-ecs.md` with a worked example.
> `effort: afternoon`

### \[P3\] **UI-3** REPL polish

> **Status:** the basic REPL works again as of commits
> `03dc45e` + `57f94d4`.  Multi-line input, history, and
> introspection commands (`:i Object`, `:methods Type`) are
> not yet implemented.
> **Fix:** proper readline-style multi-line input, plus an
> `:i Object` inspection command.
> `effort: weekend`

### \[P3\] **UI-4** Editor integration

> **Problem:** every Symta user writes in vanilla Notepad with
> a Lisp-mode that gets parens wrong.  The Notepad++ language
> definition in `dev/npp_symta.xml` is the only checked-in
> support.
> **Fix:** Tree-sitter grammar; LSP for completion / jump-to-def
> / basic linting (depends on CORE-7).
> `effort: multi-week`

### \[P3\] **UI-5** Flaky `tests/uim/buttons.png` byte-size jitter

> **Where:** `tests/uim/baselines/buttons.png` vs the rendered
> actual.
> **Problem:** the buttons-test renders to a PNG that varies by
> ~10 bytes run-to-run on the same binary.  A single pixel at
> (38,2) shifts between background and glyph-antialiasing
> remnant.  Likely cause: glyph atlas slot order depends on
> hash-table iteration order somewhere.
> **Fix sketch:** pre-allocate atlas slots for printable ASCII
> + common Latin-1 in deterministic order at font load time.
> **Workaround:** rerun the test; passes ~9 / 10 runs.
> `effort: afternoon`

---

## Speculative / future research

### \[P3\] **R-1** Second implementation

> A tree-walking interpreter in Rust / C++ that consumes the
> same `.s` source and produces the same stdout on every example.
> Proves the spec is the spec, not "whatever Nancy's compiler
> does this week".  ~1-2 person-months.
> `effort: multi-week`

### \[P3\] **R-2** Package manager

> Minimal `symta install <repo-url>` that fetches and compiles.
> `effort: weekend` for MVP

### \[P3\] **R-3** A debugger

> Out-of-process debug protocol over a Unix socket; runtime
> breakpoints at SBC-instruction granularity; introspection via
> the existing reflection built-ins.
> `effort: multi-week`

### \[P3\] **R-4** SoM-feedback ergonomic wishlist

> **Where:** [`dev/todo-spell-of-mastery.txt`](dev/todo-spell-of-mastery.txt).
> ~70 small ergonomic gaps catalogued during the SoM build.
> Most are syntax sugar that doesn't change the evaluation model.
> `effort: ongoing`

---

<a name="done"></a>
## Done (recent — kept as one-liners for findability)

- **OP-1** `_.><` / `_.<>` as direct C-builtins on T_OBJECT —
  `a341309` (May 2026).  Cold compile 38-40 s → 20-23 s.
- **Reader consolidation** — `2129767` (May 2026).  C reader is
  production; `meta` allocated in C via `intern("meta")` +
  `OBJECT`; ~448 lines deleted from `core_.s`.  Game cold
  51-54 s → 38-40 s.  See [docs/reader-consolidation.md].
- **READER-2 / READER-3** — closed alongside reader-consolidation
  (May 2026).
- **CORE-8 #1** constant-folding literal fxn arithmetic —
  `3fbdac9` (May 2026).
- **CORE-4** else/elif at body indent works in the C reader —
  `929c52e` (May 2026).
- **RT-5** closure CALL inlined `(handler, payload)` — `f4e73ff`
  (May 2026).
- **RT-7** mcache moved from bytecode stream to D-side array —
  `a304504` (May 2026).
- **RT-8a/b/c** `api_t` hot-line packing + `frame_t + locals`
  co-allocation — `ffe3053` … `14115ee` (May 2026).
- **SBC magic + revision header** — `b4222ba` (May 2026).
- **NCM-1..7** — see prior versions of TODO.md.
- **Bootstrap REPL fix + REPL test suite** — `03dc45e`,
  `57f94d4` (May 2026).
- **FFI make.exe locator on Windows** — `8f34041` (May 2026).
- **Weak hashtable + GC-invariant test suite + adversarial
  bench** — `8f8ff63` (May 2026).  Now kept as a tested
  primitive for any identity-keyed metadata use case.

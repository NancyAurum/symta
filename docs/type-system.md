# Symta type system — design exploration

A research doc for the TYPE-* roadmap.  Filed before any code lands
so the dependency graph (TYPE → NATIVE → JIT) and the design
choices are in writing for review.

## What we're solving

Three goals, ordered by leverage:

1. **Optimisation** — let the compiler / native backend emit
   unboxed machine code where types are known.  Today every
   `Int + Int` is an MCALL into `b_int_add` (~20 ns).  When the
   compiler knows both operands are `int64`, it emits a single
   `add` instruction (~0.3 ns).  100× per-op.  Aggregated across
   compile + GPU/tensor + game loop, this is the path to C99
   latency.

2. **Correctness defence beyond runtime tag tests** — catch
   off-by-one, null deref, exhaustiveness failures at compile
   time, not just at runtime.  Refinement types + exhaustive
   pattern matching let a 10 k LOC project survive a refactor
   without manually re-running every code path.

3. **Unboxed primitives that don't fit in a tagged 64-bit dyn**
   — `int64`, `float64`, and SIMD vectors.  Symta's current
   tag layout reserves 16 bits for the tag, leaving 48 bits of
   payload (47 signed) for immediates.  Native-machine 64-bit
   values need either heap boxing (slow) or a separate
   unboxed-locals lane.

## Existing Symta machinery

What we already have to build on:

- **`type Foo X Y: object_!X y!Y`** — product types with named
  fields, plus auto-generated predicates (`Foo.is_foo = 1`,
  `_.is_foo = 0`) and field accessors.  This is the user-facing
  "make a new type" syntax today.
- **Method dispatch** — single dispatch on receiver type, with
  per-call-site mcache (RT-7).  Already fast for monomorphic
  call sites.
- **`cls`** — ECS-style component / entity classes built on
  adaptive maps.
- **Runtime reflection** — `typename X`, `methods_ X`, `imm_ X`,
  the runtime knows what's heap and what's immediate.
- **Tag space** — 16 tag bits, ~22 types used, ~10 free.  Room
  for a few new immediate types.
- **`type Foo.~`** — `~` flag for "no inheritance from `_`", used
  by `meta`.  The forwarding pattern is well-established.
- **`btrap` / `bterror`** — error values that aren't exceptions;
  the compiler / runtime already has structured "value or error"
  results.
- **`#ALT_NO`** — alternate `No` values that share the same
  immediate-tag but distinguish themselves by gid.  Could be a
  template for "tagged variants".

The pieces are already most of what a gradual type system needs.
What's missing is the *declaration syntax*, the *inference
machinery*, and the *unboxed-locals lane*.

## What to borrow from each tradition

| System | What's useful for Symta | What to skip |
|---|---|---|
| **Common Lisp** | `declare type` / `the` annotations as optional hints the compiler USES; types as runtime values (subtype-of, type-of); user-defined `satisfies` predicates as types | Implementation-defined "may or may not check" semantics -- we want a precise contract: declarations are unchecked optimisation hints, separate from refinement claims |
| **TypeScript** | Gradual typing: typed and untyped code coexist; structural typing for record-like values; flow-sensitive narrowing in `case`/`if`; union (`A \| B`) and intersection (`A & B`) types | The `any` escape hatch erodes guarantees -- Symta should have a single "Dyn" universe instead of `any`; conditional types are too far out for v1 |
| **Haskell** | Hindley-Milner inference for fully-typed regions; algebraic data types (sum types); type classes for ad-hoc polymorphism; purity as a tracked effect | Whole-program purity discipline; HKT; the IO monad ceremony.  Symta is procedural; mutation is the norm |
| **ML / OCaml** | HM inference scaled to a non-pure language; pattern matching with exhaustiveness checks; module signatures for capability hiding | Per-file module ceremony; .mli files; functors (Symta already has macros for that level of abstraction) |
| **Coq / Lean / Agda** | Refinement types as a separate layer above the optimisation type system; dependent types for proving (only where you opt in); Curry-Howard for proof terms | Mandatory totality / mandatory termination; tactic languages; extraction.  These need a research project on their own; v1 doesn't need them |
| **F\*** | The exact "refinement + effect" layering we want: a base ML-style core, opt-in refinements above, opt-in effects orthogonal | The whole compiler infrastructure |
| **LuaJIT** | The unboxed-numeric-locals pattern via NaN-tagging; tracing JIT integration | NaN-tagging is a major rewrite of the runtime dyn representation -- Symta would have to give up its current tag scheme.  Keep the existing tagged dyn for boxed values, add a parallel unboxed lane |
| **Java's JVM** | Local variable typing in bytecode (lvars are typed `int`, `long`, `float`, `double`, `Object`); separate stacks/cells per type | The verifier's "every basic block must have a coherent stack type" rigidity -- we want flow-sensitive inference, not type-checking-as-verification |

The mix that fits Symta best: **gradual** (TypeScript), **HM
inference in typed regions** (Haskell / ML), **declarations as
optimisation hints** (Common Lisp), **refinements as a separate
opt-in layer** (F* / Liquid Haskell).

## Proposed Symta type system

### Layer 0: the universal type `Dyn`

Every untyped local, parameter, return, and field has type `Dyn`
implicitly.  Today's code stays valid -- no change.  `Dyn` =
"any 64-bit dyn value, including immediates and heap pointers".

### Layer 1: primitive scalar types

Built into the language, recognised by the compiler:

| Type | Storage | Source syntax |
|---|---|---|
| `Int` | 47-bit signed (existing immediate) | `42` |
| `I64` | 64-bit native, unboxed in locals | `42i64` |
| `U64` | 64-bit unsigned | `42u64` |
| `F32` | float32 (existing immediate) | `3.14f32` or `3.14` (default) |
| `F64` | float64, unboxed in locals | `3.14f64` or `3.14d` |
| `Bool` | 0 / 1, can live in a tag bit | `1` / `0` |
| `Text` | existing T_TEXT / T_FIXTEXT | `"foo"` |
| `Tag` | existing T_TAG immediate | `\foo` / `'foo` |
| `Byte` | 8-bit, unboxed in locals | `42b` |

The unboxed ones (`I64`, `U64`, `F64`, `Bool`, `Byte`) require a
*typed locals lane* -- see Layer 4.

### Layer 2: composite types (declared)

```
type Point: x:F64 y:F64
type Pair<A B>: first:A second:B
type Color: Rgb(r:Byte g:Byte b:Byte) | Hsv(h:F32 s:F32 v:F32)
```

- **Records** (Symta's existing `type T: ...` covers this).
- **Generics** (`<A B>`) -- new.  Compile-time substitution like
  C++ templates / OCaml polymorphism.
- **Sum types** (`|`) -- new.  Stored as a tagged union; the
  compiler emits the tag check on each pattern match arm.
- **Type aliases**: `type Vec3 = (F64 F64 F64)`.

### Layer 3: function signatures

Optional, but informative:

```
add_f64 X:F64 Y:F64 -> F64 = X + Y
distance P:Point Q:Point -> F64 = ((P.x - Q.x) ^^ 2 + (P.y - Q.y) ^^ 2).sqrt
```

When all args + return are typed, the compiler emits unboxed
machine code.  When *some* are typed, the compiler still gets to
specialise / unbox those.  Untyped fallback is the current MCALL
path.

Multi-dispatch by signature (Common Lisp / Julia style):

```
add X:Int Y:Int -> Int = ...           // specific
add X:F64 Y:F64 -> F64 = ...           // specific
add X:Dyn Y:Dyn -> Dyn = ...           // fallback
```

The compiler picks the most-specific match at the call site (or
falls back to runtime dispatch if call-site types aren't known).

### Layer 4: typed locals + unboxed arithmetic

The big optimisation lever.

Today every Symta local is a `void*`-sized slot holding a dyn.
For unboxed values we need extra slots:

```
typed_frame_t {
  frame_t header;       // existing
  dyn       L[n_dyn];   // existing dyn locals
  uint64_t  I[n_i64];   // unboxed integer locals
  double    F[n_f64];   // unboxed double locals
}
```

The compiler tracks each local's static type and emits opcodes
operating on the right lane:

- `SBC_I64ADD dst src1 src2` -- reads `I[src1]`, `I[src2]`,
  writes `I[dst]`.  Single x86 ADD.
- `SBC_F64ADD dst src1 src2` -- F-lane.  Single x86 ADDSD.
- `SBC_BOX_I64 dst src` -- box `I[src]` into a dyn at `L[dst]`.
  Allocates a wrapper (small heap obj with tag T_I64_BOX).
- `SBC_UNBOX_I64 dst src` -- the inverse.  Type-checks at
  runtime if the dyn's tag doesn't match `T_I64_BOX`.

The compiler inserts box/unbox at the boundary between typed and
untyped code:

```
F X:I64 -> I64 = X + 1   // unboxed throughout
Y F(42i64)               // result is I64 in caller's I-lane
say Y                    // say takes Dyn -- box at call site
```

For loops over typed arrays, the inner loop becomes 1-2
instructions per element instead of 50-100.

### Layer 5: typed arrays + tensor support

Building on Layer 4:

```
type Array<T> = ...     // generic flat array
F64Buf = Array<F64>     // alias
type Tensor<T Shape>: data:Array<T> shape:Shape
```

- Element-type-parametric arrays.  Storage is unboxed: a
  `Array<F64>` is just N×8 bytes contiguous (plus a header).
- Shape is a compile-time-known tuple of dimensions for tensor
  types; the index calculation is constant-folded.
- Zero-copy interop with FFI / GPU: an `Array<F32>` is layout-
  compatible with a C `float*`, so passing one to a CUDA /
  Metal / OpenGL kernel is just a pointer pass.
- For Symta-side access we still go through bounds-checked
  accessors.  The compiler emits the bounds check (or omits it
  when the index type is provably in-range, see Layer 6).

This is what makes "say tensors onto GPU" practical without
copying every element through a dyn.

### Layer 6: refinement types (opt-in correctness)

```
type PosInt = Int{Me > 0}
type Index<n> = Int{0 <= Me < n}
F Idx:Index<10> -> ... = Xs.Idx     // bounds check elided
```

- Refinements are predicate-decorated base types.
- Compiler checks call sites where it can (constant inputs,
  simple range narrowing); falls back to runtime check
  otherwise.
- More ambitious refinements (interval analysis, SMT-style
  reasoning) come later.

Most refinements I've seen carry their weight via:
- Array index bounds (eliminate the runtime bounds check)
- "non-null" (eliminate the No check)
- Range constraints on numeric inputs

These three alone shave ~10-20 % from real-world code.

### Layer 7: effects (much later, maybe never)

Track in the type whether a function:
- May allocate
- May call user code (and thus trigger GC)
- May throw `bterror`
- May read / write specific globals
- Has a deterministic result given inputs

Useful for proving code paths can't trigger GC inside hot
loops, or that two functions can be reordered.  Pure research
project; not on the critical path for native speedup.

## Implementation phases

Roughly the dependency order.  Each phase delivers value on its
own; the project doesn't need to ship all of them to be worth it.

### TYPE-1: Declaration syntax (parser only, no checks)

> **Effort:** weekend.  **Payoff:** zero functional change;
> required substrate for everything else.

- Extend `src/macro.s` / `runtime/reader.c` to parse:
  - `Var : Type` in argument lists
  - `Fn args -> RetType = body`
  - `type alias Name = ...`
- Types are stored in the AST but ignored by macroexpand /
  compiler initially.
- Adds no runtime / compile-time enforcement.  Code that types
  itself still runs identically to untyped code.

### TYPE-2: Built-in type registry

> **Effort:** afternoon.  **Payoff:** types are first-class
> values queryable at runtime.

- Define `Int`, `I64`, `U64`, `F32`, `F64`, `Bool`, `Text`,
  `Tag`, `Byte`, `Dyn`, `List<_>`, `Fn<_,_>`, etc. as runtime
  type objects.
- `typeof X` returns the type object for X.
- `X :> T` returns 1 if X is compatible with T.
- Foundation for inference + reflection layers.

### TYPE-3: Local inference for declared functions

> **Effort:** 1-2 weeks.  **Payoff:** the compiler knows what it's
> dealing with; sets up everything below.

- Hindley-Milner-style inference within a function body when
  some args are typed.
- Propagate types through `=` bindings, `case` patterns, method
  calls (using the dispatch table to narrow result types).
- Flow narrowing in `case` arms.
- Errors at compile time for type mismatches in fully-typed
  regions; warnings (or silent dyn coerce) in partially-typed.

### TYPE-4: Unboxed primitive locals + opcodes

> **Effort:** 3-4 weeks.  **Payoff:** ~5-10× on
> arithmetic-heavy loops.  The "tensor" + "JIT-friendly" lever.

- Extend `frame_t` with `I[]` and `F[]` lanes.
- Compiler tracks per-local lane.
- New SBC opcodes for unboxed arithmetic / load / store.
- Auto-box at calls into untyped functions.
- Auto-unbox at returns from typed functions called from typed
  code.

### TYPE-5: Typed arrays + tensor zero-copy

> **Effort:** 2-3 weeks.  **Payoff:** GPU /
> tensor libraries become practical.  Big-data ergonomics.

- New built-in `Array<T>` type, layout-compatible with C arrays.
- Bounds-checked subscripts (elidable per TYPE-6).
- FFI integration: pass `&array[0]` and length to C / GPU
  kernels.
- Existing `bytes` type can probably be subsumed as
  `Array<Byte>`.

### TYPE-6: Refinement types + bounds-check elision

> **Effort:** 4-8 weeks.  **Payoff:** correctness + ~10-20 % on
> array-heavy code.

- Predicate-attached types parsed but most checks deferred to
  runtime initially.
- Cheap compile-time cases (constant predicates, simple range
  narrowing) elided.
- More ambitious analysis (SMT, interval propagation) future
  work.

### TYPE-7: Generic types + sum types

> **Effort:** 2-3 weeks each.  **Payoff:** algebraic data
> modelling that scales to a 100 k LOC project.

- `<T>` parameter syntax for type declarations.
- Sum types `A | B` with auto-generated tag.
- Pattern matching exhaustiveness check.
- (Symta already has multi-dispatch on type; this builds on
  that.)

### TYPE-8: Type classes (or skip)

> **Effort:** unknown.  **Payoff:** ad-hoc polymorphism.
> May be unnecessary given Symta's existing dispatch story.

Defer until we see a concrete use case that the existing
`cls` / multi-dispatch can't handle.

### TYPE-9: Effects (research)

> **Effort:** indefinite.  **Payoff:** correctness, optimisation
> reordering safety.

Speculative.

## What this unlocks for NATIVE

The NATIVE roadmap (x86 + ARM AOT sections in SBC) depends on
TYPE-1 → TYPE-4 because:

- **NATIVE-1 emits code per opcode.**  An untyped `Int + Int`
  is an MCALL into `b_int_add`; native code can't do better than
  invoke the same handler.  A typed `I64 + I64` is a single
  `add` instruction.  Order-of-magnitude difference.
- **GC root scanning needs lane-aware frames.**  When a frame
  has `L[]` plus `I[]` plus `F[]`, the GC walker only scans
  `L[]`.  This is well-trodden ground (JVM, .NET).
- **The unbox boundary IS the FFI boundary.**  Native code
  calling into C / GPU kernels can pass unboxed `Array<F64>`
  pointers directly.  No marshalling.

Without TYPE-1 to TYPE-4, NATIVE-1 lands a 1.5-2× win on
MCALL-heavy code (saving the dispatch cost).  *With* the type
system, it's 5-20× on numerics.  That's where "near C99
latency" claims become real.

## Runtime reflection for JIT unboxing

The user asked specifically about this.  Plan:

- Each SBC function gets a **type section** alongside its
  bytecode + lineno table (CORE-1 v2 pattern).
- Layout: for each local index, a small enum value:
  `LANE_DYN = 0, LANE_I64 = 1, LANE_F64 = 2, LANE_BOOL = 3,
  ...`
- Function signature: input lanes + return lane.
- JIT reads this at compile-to-native time to decide which
  registers to use, which arithmetic instructions to emit, etc.
- For dynamically-typed code, all lanes are `LANE_DYN` --
  same as today.

The type section is purely descriptive; the bytecode interpreter
ignores it (uses the existing `L[]`-only model).  Only the JIT /
AOT native compiler reads it.

## Boxed forms for raw types

Same answer as `meta`: a small heap struct that holds the
unboxed value, has type tag `T_I64_BOX` / `T_F64_BOX` / etc.,
and can be passed into untyped code.  Boxes are auto-allocated
at the typed→untyped boundary (compiler insertion) or explicitly
via `box X`.

Boxed forms live in `Array<Dyn>` and hash tables that don't
know they're holding doubles.  Unbox at the boundary back into
typed code.

This is what Java does with `Integer` vs `int`, and what C# does
with `System.Int32` boxing.  Well-understood pattern.

## Open questions

These need answers before TYPE-3 lands:

1. **Syntax for type parameters.**  `<A B>` clashes with
   comparison.  `[A B]` clashes with lists.  `{A B}` clashes
   with tables.  Most likely answer: `<<A B>>` or `:[A B]` or
   borrow Haskell's `forall A B.` keyword.
2. **Subtyping rule.**  Is `Int <: Dyn` automatic?  (Yes.)  Is
   `Array<Int> <: Array<Dyn>`?  (Probably not -- variance is a
   minefield.)  Is `Pair<A B> <: Pair<C D>` when `A <: C` and
   `B <: D`?  Pick one rule, document it, stick to it.
3. **How does multi-dispatch interact with type classes?**
   They're the same thing solved differently.  Pick one;
   Symta's existing multi-dispatch is probably the answer,
   with type classes deferred.
4. **Refinement decidability.**  Refinement predicates are
   arbitrary Symta code (`Int{Me > 0}`).  The compiler can't
   evaluate arbitrary code at compile time.  Either restrict
   refinement predicates to a decidable fragment, or use them
   only at runtime by default.
5. **Bytecode revision bump.**  TYPE-4 changes `frame_t`
   layout.  Must land BEFORE NATIVE-PRE freezes the ABI for
   native sections.

## Sequencing vs NATIVE / JIT

Final dependency chain:

```
  OP-2, OP-3, OP-4 (small wins, no schema change)
    ↓
  RT-4, RT-6 (cache locality, big wins, no schema change)
    ↓
  TYPE-1, TYPE-2 (parser + registry, no codegen change)
    ↓
  TYPE-3 (inference -- compiler tracks types but doesn't yet emit
          different code)
    ↓
  TYPE-4 (unboxed locals -- bytecode changes; biggest single
          schema move)
    ↓
  TYPE-5, TYPE-6, TYPE-7 (each shippable on its own)
    ↓
  NATIVE-PRE  (freeze schema)
    ↓
  NATIVE-1, NATIVE-2  (x86 + ARM AOT)
    ↓
  NATIVE-3  (optional JIT layer on top)
```

Calendar time for the whole stack from where we are today:
- OP-* + RT-* perf work: ~2-3 months
- TYPE-1 through TYPE-4: ~3-4 months
- TYPE-5 through TYPE-7: ~2-3 months
- NATIVE-PRE: weekend (when the time comes)
- NATIVE-1: 4-6 weeks for x86 MVP
- NATIVE-2: 2-3 weeks (with shared infrastructure)

A year of focused work to "near C99 latency", with intermediate
deliverables every few months.

## Why this is the right shape

Three properties of this design are worth defending against
simpler alternatives:

1. **Gradual.**  No existing Symta code has to type itself to
   keep working.  The type system is a tool you reach for in
   hot paths or where correctness matters.
2. **Layered.**  TYPE-1, TYPE-2, TYPE-3 are pure additions:
   they make types visible but don't change codegen.  TYPE-4
   changes the runtime; everything above is independent.
   Refinements (TYPE-6) and effects (TYPE-9) are opt-in
   research lanes that the optimisation backbone doesn't
   depend on.
3. **Compiler-friendly first, prover-friendly later.**
   Coq-style dependent types are a 10× cost for a 2× benefit
   compared to F\*-style refinements.  Refinements give you
   the bounds-check elision and the off-by-one bug catches;
   dependent types let you prove correctness of red-black tree
   invariants.  Most Symta projects need the first; only a few
   research projects need the second.

This design is what you'd build today if you wanted both the
TypeScript developer experience and the Rust runtime, on a
codebase that wants to stay 22 k LOC of C runtime.

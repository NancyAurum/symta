<img src="logo.webp" alt="Symta logo" width="96" align="right">

# Symta — Roadmap

Symta is a working, self-hosted, AOT-compiled language with a small
generational-GC runtime, a real ECS, an FFI, a stdlib, an examples
tree, and a Windows binary you can run today.

This page is about **what comes next**.  Each milestone below is
scoped, costed, and has a concrete plan in the engine room.  If you
want to help Symta get there, see *How to support* at the bottom.

---

## Where Symta is today

- Self-hosted compiler, ~28 k LOC of Symta + ~10 k LOC of C runtime.
- Bytecode interpreter with a generational GC.
- Static binary: one `symta.exe`, no DLL soup, no Python, no LLVM.
- Pattern matcher, macros, `eval`, quasiquote, gensyms — built in.
- ECS (`cls`, `dsm`, IPS) integrated into the language, not bolted on.
- FFI plugins for SDL2, FreeType, SVG, a small voxel renderer.
- Cold compile of a 21 k-LOC game: **~20 seconds** on a laptop
  (down from 51 s three commits ago).
- Runs Windows x64 today; macOS + Linux build from source.

---

## Milestone 1 — Sub-10-second cold compile *(weeks, not months)*

Symta already shaved cold-compile time from 51 s → 20 s in one sprint
by moving hot dispatch chains from Symta into the C runtime.  The
remaining hot paths follow the same recipe.

**You get:**

- A 21 k-LOC project that compiles in **under 10 s, cold**.
- Edit-compile-run cycles measured in single seconds.
- No change to the source language — your existing code just gets
  faster.

---

## Milestone 2 — Static type system *(one quarter)*

A gradual, inference-driven type system that **augments** Symta
rather than replacing it.  Optional annotations, full backward
compatibility, no Hindley-Milner-flavored ceremony.

**You get:**

- Optional type annotations: `Int X`, `[Int] Xs`, `Text:Int Table`.
- Whole-program type inference where you skip the annotations.
- Compile-time errors for the mistakes that today are runtime
  `bterror`s.
- A type registry available at runtime, so reflection / debug /
  pattern-match introspection still work.
- Foundation for everything below.

Borrows from Common Lisp's `declare`, TypeScript's structural
types, and ML's inference — without their bad parts.  See
[`architecture.md`](architecture.md) for where it slots in.

---

## Milestone 3 — Unboxed numerics & SoA layouts *(one quarter)*

Today every `Int` is a tagged pointer.  Loop-heavy numeric code
pays the boxing tax.  With Milestone 2 in place, the compiler can
prove a `for I in :N` runs over machine ints and emit raw integer
opcodes.

**You get:**

- Tight numeric loops within **2× of C** on the same opcode set.
- Real 64-bit ints, real `f64` doubles — not 60-bit-via-NaN-boxing.
- `[Int]` and `[Flt]` arrays that are actual flat memory, not
  pointer-soups.  ECS components and tensor workloads get
  cache-friendly layouts for free.
- Same source language.  Same `+`, `*`, `^`, `.s`.  The compiler
  picks the right representation.

---

## Milestone 4 — Native AOT compilation *(one half year)*

Once the type schema is frozen, the bytecode→C99→native-binary path
opens up.  Symta currently compiles to bytecode; the runtime
interprets it.  AOT skips the interpreter.

**You get:**

- `symta --native` produces a standalone native executable per
  target architecture.
- x86-64 and ARM64 (Apple Silicon, Raspberry Pi, modern Android).
- Numeric loops within **1.1–1.5× of C** — the boxing layer is
  gone, the interpreter dispatch is gone.
- A redistributable that doesn't need `symta.exe` at runtime.

---

## Milestone 5 — Optional JIT layer *(speculative; ship-if-funded)*

For long-running services and games, a tiered JIT on top of the AOT
core: cold code runs from AOT, hot code recompiles with runtime
profile data.

**You get:**

- Profile-guided specialization on actually-observed types.
- Closes the last gap to hand-written C on irregular workloads.
- Targets the same runtime — opt-in, off by default.

---

## Always-on workstreams

These don't sit on the critical path.  They land in parallel as
funding allows.

| Workstream | What lands |
|---|---|
| **Float-precision round-tripping** | Compiler preserves `1e-12` literals exactly through SIF / SBC. (Today they round to `0.0`.) |
| **Reader / FFI polish** | Better error messages for FFI mismatches; scientific-notation float literals; faster parsing of large source files. |
| **GC tuning** | Larger generations, write-barrier specialization, pacing knobs for game-loop friendly pause budgets. |
| **Stdlib expansion** | More batteries: HTTP client, JSON, `sqlite3`, regex (real engine, not toy). |
| **Editor support** | VS Code grammar, Notepad++ syntax file, indentation rules.  An LSP eventually. |
| **Docs** | A book.  Tutorials by example.  A short paper on the pattern-matcher / `{}` operator design. |

---

## What's already done *(receipts)*

- Self-hosted compiler, with a one-screen bootstrap.
- AOT bytecode + generational GC, statically linked.
- FizzBuzz, quicksort, Prolog-style inference engine, n-body
  simulation, voxel renderer, SDL game runtime — all in the
  `examples/` tree.
- Pattern matcher unified with map / filter / reduce / replace /
  parse / destructure — one syntax for all of them.
- Cold-compile time cut **~2.5× in one week** via runtime
  consolidation.  See [`architecture.md`](architecture.md).

---

## How to support

Symta is dual-licensed MIT OR Apache-2.0.  It is, and will remain,
free to use and embed.  Development is currently funded by one
human writing the code in her spare time.  If you'd like to see the
roadmap above move faster, the following channels exist:

- **GitHub sponsorship** *(coming soon)* — recurring monthly tiers
  that fund weekly hours on the core compiler.
- **One-shot work-for-hire** — if your team needs a Symta feature
  on a schedule (a backend, a stdlib module, an FFI binding), I
  take milestone contracts.  Open an issue tagged `sponsor` to
  start the conversation.
- **Contribute code** — every milestone above breaks down into
  weekend-scoped pull requests.  See [the contributing
  section](README.md#license) and open an issue describing what
  you'd like to take on.
- **Use it in production** and tell me what broke.  Real-world
  pressure shapes the roadmap.

---

*Roadmap last updated: 2026.  Symta is built by Nancy Sadkov.  See
[LICENSE](LICENSE) for copying conditions.*

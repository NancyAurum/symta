# Symta — architecture

A tour of how Symta is put together, current as of May 2026. If
you want the language tour first, read
[`dev/sbe.txt`](dev/sbe.txt) ("Learn Symta by Example") and the
[`examples/`](examples/) folder. If you want to build it, see
[`BUILDING.md`](BUILDING.md). If you want the milestone roadmap,
see [`TODO.md`](TODO.md).


## The high-level picture

Symta is a self-hosted, AOT-compiled Lisp dialect with a small C
runtime. The compile pipeline is:

```
   .s source files
   ┌──────────┐
   │  src/*.s │
   └─────┬────┘
         │  reader        (C, runtime/reader.c — was reader.s pre-2026-05)
         │  macroexpand   (Symta, src/macro.s — runs in the host runtime)
         │  uniquify      (Symta, src/compiler.s — alpha-rename + closure analysis)
         │  SSA + SIF     (Symta, src/compiler.s)
         ▼
   ┌──────────┐  sif_to_sbc  ┌──────────┐
   │ SIF text │  ──────────► │  *.sbc   │ ──► interpreted in runtime/sbc.c
   └──────────┘  (assembler  └──────────┘
                  in C)
```

Three things are unusual relative to other small Lisps:

1. **Self-hosting with C parser front end.** The compiler, macro
   expander, and core library are all written in Symta (`src/*.s`).
   The reader (parser) used to be too, but as of commit
   `dc1e240`/`4124216` (May 2026) the parser lives in
   [`runtime/reader.c`](runtime/reader.c) — ~28× faster than the
   old `src/reader.s` on representative inputs, with a 5-stage
   self-hosting drift test
   ([`tests/bootstrap/drift.sh`](tests/bootstrap/drift.sh))
   proving the codegen reaches a byte-stable fixed point.

2. **Two on-disk forms for bytecode.** SIF (Symta Instruction
   File) is the textual assembly form — one instruction per line,
   readable, diffable. SBC (Symta ByteCode) is the binary the VM
   actually runs. An assembler converts SIF to SBC in C
   (`runtime/sif2sbc.c`); the assembler is small enough to fit on
   one screen.

3. **Generational moving GC.** Up to ~30 generations, bump
   allocation in gen0 (L2-cache-friendly), copy on promotion, with
   a low-bit-tagged 64-bit object representation that keeps ints,
   floats, short text, and the `No` sentinel as GC-free immediates.


## Directory layout

```
symta/
  src/             Compiler + core library + UI (all .s, all Symta)
    reader.s         REMOVED — see runtime/reader.c
    macro.s          Macroexpander; defines core macros
    compiler.s       AST → uniquify → SSA → SIF codegen
    core_.s          Standard library
    cls.s            Component-Oriented (cls / dsm / IPS) ECS
    uim.s            UI manager (widgets, layout, input loop)
    uimgen.s         Auto-stages default widget pictograms
    gfx.s, font.s    FFI wrappers for c/gfx + c/ttf
    slb.s, slb_.s    FFI wrappers for the external vfx voxel lib
    store.s          Pictogram cache + path resolution
    rgb.s            Colour helpers
    cache.s          TTL cache (used by store + font)
    prof.s           Frame-time profiler
    sml.s, sexp.s    Tagged-text + s-expression parsers
    m.s, m3d.s       Math (vec, matrix, 3D)
  sbc/             Bootstrap bytecode (compiler / reader / macro /
                   eval / core_ / go); read at startup by
                   symta.exe so a fresh checkout self-compiles.
  runtime/         C runtime
    reader.c         The Symta parser (ported May 2026)
    tokenize.c       Lexer (always was C)
    bltin.c          Built-in functions (~2 K lines, mostly
                     small wrappers over Symta-callable C)
    gc.c, gc_types.h Generational moving GC
    sbc.c            Bytecode interpreter
    sif.c, sif2sbc.c SIF text → SBC binary assembler
    am.h, nh.h, nb.h, dh.h
                     Adaptive map: the table type that backs
                     every Symta hash. Bitmap → int-hash →
                     generic-hash promotion ladder.
    fs.c             File I/O
    w64/             Windows compat (mmap, dlfcn, ctx, compat)
    osx/, linux/, unix/
                     Per-platform shims
  c/               FFI plugin sources, built into ffi/*.ffi
    gfx/             2D blitter, PNG decode, gamma LUTs
    ui/              SDL2 window + input loop
    ttf/             TrueType rasteriser (stb_truetype)
    svg/             SVG parser (nanosvg)
  ffi/             Pre-built .ffi blobs (per-platform DLLs/dylibs
                   renamed) — gfx.ffi, ui.ffi, ttf.ffi, svg.ffi,
                   zlib.ffi. vfx.ffi gets added by the SoM
                   top-level build; not part of the standalone
                   Symta distribution (see ../vfx).
  sdl/             SDL2 + sidecar DLLs staged into each project
                   by ffi_begin macro ui (auto-stages).
  ttf/             Default font (Inter Regular, OFL); staged by
                   ffi_begin macro ttf into projects that don't
                   already ship their own ttf/.
  pkg/symta/       The "self-rebuild me" project (compiles src/
                   into a refreshed sbc/).
  examples/        Tutorial demos (00-hello through 31-isometric).
                   Doubles as the runtime regression suite via
                   tests/runtime/.
  tests/           Eight regression suites (see "Testing" below)
  dev/             Long-form notes + editor / bench assets — the
                   tutorial draft (sbe.txt), ECS design notes
                   (cls.txt / cls-gc.txt), NCM macro-processor
                   reference (ncm.md), Notepad++ language file,
                   bench-clock setup scripts.  Not required to
                   build or run Symta.
  README.md        Language tour
  BUILDING.md      Per-platform build instructions
  architecture.md  This file
  TODO.md          Milestone roadmap
  blog.md          The C-reader-port story (Jan 2026)
```


## The compilation pipeline

### 1. Reader (`runtime/reader.c`)

Hand-written recursive-descent. Reads the offside-rule indentation
that lets a `:` open a multi-line block without explicit
parentheses, while still letting authors fall back to `(...)`
when they want. The reader emits an AST built from plain Symta
lists — there is no distinct "syntax tree" type, which is what
makes macros so straightforward.

The parser used to live in `src/reader.s` (~520 lines of Symta)
but was ported to C in May 2026 (commits `dc1e240`, `4124216`,
`07ab5c8`). See [`blog.md`](blog.md) for the full story —
including the bisection methodology that surfaced six subtle
bugs in the port by running both implementations side-by-side
and diffing their AST output on every input.

The C port:
- is ~28× faster on representative inputs
  (`benchmark/parser/parser-bench.s`)
- exercises a `tests/bootstrap/drift.sh` 5-stage drift check
  proving stages 2..5 are pairwise byte-identical
- still exposes `text.parse` as a runtime built-in via a thin
  trampoline, so user code can parse Symta source at runtime
  (used by `cfg_file.s`, the inference engine, etc.)

### 2. Macroexpander (`src/macro.s`)

Macros run in the *compiler's* Symta runtime. A macro receives
its arguments unevaluated and returns a new AST. Most of the
language's "keywords" — `when`, `while`, `for`, `times`, `case`,
`dup`, `mtx`, `ffi`, `cls`, `type` — are macros, built on three
primitives:

- `_progn` — sequence of expressions
- `_label` / `_goto` — structured jumps within a function
- `_if` — two-armed conditional

Pattern matching (`case`, the `{}` map operator, `=` inside
`case`, auto-closure variables `~x`/`x~`) is also a macro layer
on top.

Two pieces of macro infrastructure worth knowing:

- **`ffi_begin macro X`** (in macro.s) is the integration point
  for loading an FFI plugin into a project. It looks up
  `Root/ffi/X.ffi`, copies it into the project's `Build/ffi/`,
  and (for the `ui` and `ttf` plugins) also stages the SDL DLLs
  / default font next to `go.exe`. The "macro stages the
  runtime dependencies of a `use`" pattern is what makes
  `use uim` an honest promise instead of a footgun.

- **`use uimgen` is wired automatically** when `uim.s` is
  imported. uimgen.s carries 31 SVG strings in-source for the
  default widget pictograms; its top-level call materialises
  `pic/ui/*.svg` at app startup. Overrides survive: anything
  on disk wins. See `src/uimgen.s` for the design rationale
  (accessibility palette, pic9-friendly geometry).

### 3. Uniquifier (`src/compiler.s` — `uniquify_*`)

Alpha-renames bindings to globally unique labels, computes
captured-variable lists for each closure, builds the closure
environment graph used in code generation.

### 4. SSA + SIF codegen (`src/compiler.s` — `produce_ssa`)

Single-pass SSA with peephole optimisation and tail-call
detection. The SSA stream is serialised to SIF text by
`ssa_to_sif`. Per-function metadata (name, arity, source row/col
for stack traces) is gathered here.

### 5. SIF assembler (`runtime/sif.c`, `runtime/sif2sbc.c`)

Resolves labels, packs immediates, emits SBC. One opcode per
line in SIF; opcode mnemonics listed in `runtime/sif.h`.

### 6. Bytecode interpreter (`runtime/sbc.c`)

`sbc_exec` is the threaded interpreter: dispatches on opcode
bytes via a `switch`, maintains a frame stack, handles closures
and calls, triggers GC when a generation fills. Built-ins
(`runtime/bltin.c`) are registered as ordinary callable closures
backed by C functions.

Slated for upgrade to computed-goto threading (`&&label` GCC
extension) for an estimated 10–30 % throughput win — part of
the runtime-optimisation milestone in [`TODO.md`](TODO.md).


## Object representation

Defined in `runtime/symta.h`. Every Symta value is a 64-bit
`dyn` with a low tag bit indicating "immediate vs heap":

| bits     | meaning                                            |
|----------|----------------------------------------------------|
| 0        | heap flag (1 = pointer into the GC heap)           |
| 1..15    | type tag (`T_INT`, `T_LIST`, `T_TEXT`, ...)        |
| 16..63   | global id / heap index (48 bits)                   |

Heap pointers are stored as offsets into the heap, not raw
addresses — lets the GC relocate a generation without rewriting
older-generation pointers. Each heap object carries a small
header (`gc_head_t`) with size + bytecode-hook info.

Notable types:

- `T_INT`      — 48-bit fixnum
- `T_FLOAT`    — 32-bit float packed in the GID slot
- `T_FIXTEXT`  — short text fitted entirely in the immediate
- `T_TEXT`     — heap-allocated text
- `T_LIST`     — generic list
- `T_HARD_LIST`— compact array-backed list
- `T_VIEW`     — non-owning slice over another list
- `T_BYTES`    — packed byte array
- `T_TBL`      — adaptive map (see "Tables" below)
- `T_CLOSURE`  — code pointer + captured environment
- `T_OBJECT`   — tagged record (the result of `type` / `cls`)
- `T_NO`       — the absence-of-value singleton (additive identity)


## Memory management

A bump-allocating generational copying collector. Most allocations
land in `gen0`, which fits in L2 cache; a young-gen collection
touches only that arena. Immediate objects (ints, floats,
fixtexts, type tags) are not heap allocated at all and so are
GC-free.

Finalisers (`set_finalizer`) and unwind handlers
(`_set_unwind_handler`) hook into the same machinery, giving
Symta precise resource cleanup without manual reference counting.

The `fin` macro (try/finally) is currently exposed in the
language but unimplemented at the VM level.


## Tables: the adaptive map (`runtime/am.h`)

Every Symta `(!)` (hash table) is an "adaptive map" that
promotes itself through a ladder of storage modes as values
diverge:

```
AM_EMPTY ──► AM_BITMAP0 / AM_BITMAP1 ──► AM_INT ──► AM_GENERIC
              all values 0 or all 1     ints       boxed Symta
              (1 bit per key)            (stb_ds)   methods (nh_t)
```

This is what lets ECS columns be small when most entities share
a default value, and fall back to a full hash only when they
actually diverge.

Design notes:
- `nh.h` is the underlying hash table (linear probing,
  xorshift-multiply, 50 % load factor).
- `dh.h` is the GENERIC-mode dispatcher.
- `nb.h` is the bitmap layer.

Several improvements are queued — bitmap+exceptions hybrid,
dense-vector pair for ECS columns, Robin Hood probing,
inlined int/text keys in the GENERIC path. See the runtime
milestone in [`TODO.md`](TODO.md).


## Modules and build artefacts

A Symta "project" lives under `<project>/src/` and must contain
a `go.s` entry module. Running

```sh
symta <project>           # or  symta <project>/src/go.s
```

produces:

- `<project>/sbc/*.sbc`   — compiled bytecode for every dep
- `<project>/go.exe`      — a renamed copy of the runtime that
                            auto-loads `sbc/go.sbc` on launch

Modules import each other via `use foo bar` (first line). Names
resolve against the project's own `src/` first, then the
compiler's own `src/` (so `core_`, `cls`, `uim`, … are always
available). Compilation is incremental: per-module SBCs are
cached; rebuilds only fire when source or a dependency's source
changed.

### SAF archives (`saf/`)

`sbc.saf` is a single-file alternative to a populated `sbc/`
folder — multiple `.sbc` files compressed into one archive. The
runtime auto-mounts a `sbc.saf` if the corresponding `sbc/`
directory is missing, which makes shipping a tool as a single
executable + one data file practical.

### Interpreter mode (`-f`, `-e`)

`symta -f script.s` evaluates one file. `symta -e "say hello"`
evaluates one expression. Both reuse the compiler-pipeline-then-
execute path; the difference is that the SBC is loaded into a
fresh in-memory module rather than written to disk. The
interactive REPL (entered when no source path is given) is the
same code in a loop.


## Foreign function interface

Two strategies, both routed through [`runtime/sffi/`](runtime/sffi/)
(the in-tree FFI dispatcher; one ~200-line `arch_<abi>.c` per ABI,
no JIT, no executable memory — the trampoline is statically
compiled, arg classification runs once per FFI declaration at
`sbc_prepare` time, the hot path is just the per-ABI register-load
+ indirect call):

1. **Low-level**: `_ffi_call (RetType @ArgTypes) Pointer @Args`
   calls any C function given its address. `ffi_load` resolves
   a symbol from a DLL/dylib. Type tags include `ptr`, `int`,
   `s4`, `u4`, `float`, `double`, `text`, `void`.

2. **Macro-driven**: `ffi_begin <lib>` followed by `ffi <name>:
   ...` declarations expands to a set of typed wrapper macros.
   The shared library is staged from `Root/ffi/<lib>.ffi` into
   the project's `ffi/` folder, and (for ui/ttf) sidecar
   dependencies are staged from `Root/sdl/` and `Root/ttf/`.

x86-64 Windows and SysV ABIs are in production; i386 (both),
AArch32, and AArch64 are stubbed in [`runtime/sffi/`](runtime/sffi/)
with calling-convention notes and brought up on demand. The
predecessor (vendored libcinvoke, BSD-3 non-endorsement) was
dropped in May 2026; Symta is now dual MIT / Apache-2 end to end.

Native modules that ship with Symta:

| FFI lib | Source     | Purpose                              |
|---------|------------|--------------------------------------|
| `gfx`   | `c/gfx/`   | 2D blitter, PNG decode, gamma LUTs   |
| `ui`    | `c/ui/`    | SDL2 window + input loop             |
| `ttf`   | `c/ttf/`   | TrueType rasteriser (stb_truetype)   |
| `svg`   | `c/svg/`   | SVG parser (nanosvg)                 |

The voxel-octree `vfx` plugin lives **outside** standalone Symta
(at the SoM project root `../vfx`); it's consumed by the game
and by VoxPie but isn't part of the language distribution. See
`../vfx/README.md`.


## UI Manager (`src/uim.s`)

The widget tree + input + render loop. Widgets are declared
with `cls X.WGT(...)` and composed declaratively with `rowz:`
inside an `Add!:` block. Window opens via
`uim W H Title!"..." Body`.

Key affordances:
- **Headless screenshot mode** — `--screenshot=<path>
  --screenshot-frame=<N>` opens the window, renders N frames,
  saves the framebuffer as PNG, exits. Used by every regression
  test in `tests/uim/`.
- **Modal gate** — `xcl_wgt` flags "a modal dialog is active";
  callers can skip work that the user can't currently see.
  After this landed the game's Management window went from 25
  fps to 60+ fps; the modal-gate idiom is the single biggest
  perf win we've found.
- **Per-frame `tick_fn` hook** — a no-arg-or-one-arg callback
  fired after each frame's render. Used by profilers and by the
  synthetic-event test (`tests/uim/src/tc_synthetic.s`) that
  drives chkbx/slider state programmatically.

The default widget pictograms (button, checkbox, slider chevrons,
window chrome, cursor) are generated by `src/uimgen.s` on first
launch — same "macro stages dependencies" pattern as the SDL
DLLs and default font. Each project's own `pic/ui/*.svg` (or
`.png`) overrides. The game has its own bespoke fantasy set
under `game/pic/ui/`; uimgen's existence-check leaves it alone.


## Component-Oriented programming (`src/cls.s`)

The `type` macro provides classic single-inheritance OOP backed
by vtables. `cls` builds on top to provide an entity-component
system: a `cls foo a b c` declares "anything with parts `a`, `b`,
`c` is a `foo`", and the runtime stores those parts in adaptive
maps keyed by an integer entity id. The SoM game's `unit` type
has ~150 parts — proof the layer scales.

`dsm` (in `src/cls.s`) registers a system that fires on events:
part initialisation, finalisation, external timer ticks. The
standard ECS pattern presented as ordinary Symta function
definitions.

Long-form design notes in [`dev/cls.txt`](dev/cls.txt) and
[`dev/cls-gc.txt`](dev/cls-gc.txt). Closing the dense-iteration
performance gap is part of the runtime milestone in
[`TODO.md`](TODO.md).


## Testing (`tests/`)

Eight regression suites, all driven by shell wrappers around
the `symta.exe` you just built. Each `run.sh` takes an optional
basename prefix for "run one case" and `--update` for
"refresh goldens":

| Suite | What it covers | Cases |
|-------|----------------|-------|
| `tokenizer/` | Lexer: numbers, symbols, strings, comments, positions | 9 |
| `reader/`    | Parser (C reader): indentation, operators, postfix, splice | 12 |
| `macros/`    | Macro expander + DSL behaviour | 13 |
| `runtime/`   | Single-file `.s` examples, golden stdout comparison | 25 + 5 lineno |
| `compiler/`  | Compiler output (`.sbc`) byte-equality goldens | 25 |
| `gfx/`       | gfx-FFI golden image diffs | 29 |
| `uim/`       | UIM widget gallery + synthetic-event coverage | 8 |
| `bootstrap/` | 5-stage drift test: stages 2..5 byte-identical | 1 (≈2 min) |

Total: ~127 cases, ~3–4 minutes for `make test-all` end-to-end.

The drift test is the load-bearing one. It bootstraps the
compiler five times and requires stages 2..5 to be byte-
identical. A failure there means codegen has acquired some
input dependence — hash-iteration order leaking, uninitialised
codegen buffer bytes reaching disk, register-allocator
instability under different inputs. The methodology is the
standard one (`make compare` in GCC, stage3 in Rust); see
`tests/bootstrap/drift.sh` for the full rationale.


## What used to be different

This section exists because someone reading old commits, blog
posts, or `dev/cls.txt` will find references to things that
have since changed:

- **`src/reader.s` no longer exists.** The parser is in
  `runtime/reader.c`. See `blog.md`.
- **`tests-*` at the project root** moved to `symta/tests/*`
  in commit `cca449a`.
- **SDL DLLs / widget pictograms / fonts** used to be committed
  into every consuming project. They're now staged automatically
  by `ffi_begin` from `symta/sdl/`, by `uimgen.s` on first
  launch, and by `ffi_begin` from `symta/ttf/` respectively.
  See `blog.md` "three rounds of one fix".
- **`ncc/`, `ncm/`, `ncu/`, `c/vfx/`** moved out of `symta/`
  to the SoM project root because they're niche tooling not
  appropriate for a "language distribution" download.
- **`saf/` was at symta/saf/**, the Symta Archive File tool —
  still there but no longer load-bearing.


## Glossary

- **SIF**  — Symta Instruction File; textual assembly
- **SBC**  — Symta ByteCode; binary form of SIF
- **SAF**  — Symta Archive File; many SBCs compressed into one blob
- **GID**  — Global Id; the 48-bit upper portion of a `dyn`
- **SES**  — Symta Entity System; the integer-id ECS layer
- **IPS**  — Id-Part-System (the same thing, Symta's preferred name)
- **AM**   — Adaptive Map; the table type that backs every Symta hash
- **No**   — the singleton "no value" / additive identity
- **UIM**  — UI Manager (`src/uim.s`)
- **WGT**  — Widget (the cls type base for everything UIM renders)


## See also

- [`README.md`](README.md) — what Symta is, why use it, code samples
- [`BUILDING.md`](BUILDING.md) — how to build it on each OS
- [`TODO.md`](TODO.md) — milestone roadmap
- [`blog.md`](blog.md) — the C reader port story
- [`dev/sbe.txt`](dev/sbe.txt) — long-form tutorial (~2,700 lines)
- [`dev/cls.txt`](dev/cls.txt) — why the ECS layer looks the way it does
- [`../symta-review.md`](../symta-review.md) — a candid one-week
  review of Symta from an outside perspective, written by Claude
  during the SoM revival

# Symta Architecture

This document is a tour of the Symta implementation -- how source code
becomes a running program and how the pieces fit together.  It is
intended for contributors and for anyone curious about how a small,
self-hosted Lisp dialect is organised.

> If you are new to the language itself, start with [dev/sbe.txt](dev/sbe.txt)
> ("Learn Symta by Example") and the [examples/](examples/) folder.


## High-Level Picture

Symta is a self-hosted, AOT-compiled Lisp dialect with a custom
bytecode runtime written in C.  The pipeline is:

```
   .s source                       runs natively;
   ┌─────────┐                     calls into runtime
   │  src/*.s│                     for built-ins, GC,
   └────┬────┘                     and FFI
        │  parse, macroexpand,
        │  uniquify, SSA
        ▼
   ┌─────────┐    sif_to_sbc   ┌─────────┐    bytecode
   │ SIF text│ ─────────────►  │  *.sbc  │ ───► interpreter
   └─────────┘   (assembler)   └─────────┘     in runtime/
        ▲
        │  produce_ssa
        │
   ┌─────────┐
   │  AST    │ ◄── reader / macroexpander  (also written in Symta)
   └─────────┘
```

Three things are unusual:

1. **Self-hosting.** The compiler, reader, and macroexpander are all
   written in Symta and live in `src/`.  Pre-built bytecode for them
   ships in `sbc/` so a fresh checkout can compile itself.
2. **Two on-disk forms.** SIF (Symta Instruction File) is the human
   readable assembly textual form; SBC (Symta ByteCode) is the binary
   form actually executed.  An assembler converts the former to the
   latter inside the runtime.
3. **Generational moving GC.** A purpose-built GC with up to ~30
   generations and a low-bit-tagged object representation gives Symta
   competitive performance for a dynamic language without requiring
   stop-the-world pauses for short-lived data.


## Directory Layout

```
runtime/      C runtime: GC, bytecode interpreter, built-ins, FFI shim
  w64/        Windows-specific glue (mmap, dlfcn, ctx)
  osx/        macOS-specific glue
  linux/      Linux-specific glue
  unix/       Unix-shared compat
src/          Compiler, core library, and macros (all .s)
  compiler.s  AST -> SSA -> SIF code generator
  reader.s    Source-text tokenizer / parser
  macro.s     Macro expander; defines core macros (when, while, ...)
  core_.s     Standard library (list, text, table, math, ...)
  cls.s       Component-Oriented (cls / dsm / ECS) extensions
  uim.s       UI manager
  gfx.s       Graphics module (FFI to c/gfx)
  ...
sbc/          Bootstrap bytecode for the modules in src/
              (used to build a fresh runtime + compiler from scratch)
saf/          SAF -- Symta Archive File: bundles many .sbc into one blob
ffi/          Pre-built shared libraries (.ffi) for graphics/UI/etc.
c/            Source for the FFI plugins (gfx, ui, vfx, ttf, svg)
cinvoke/      C/Invoke library (third-party): runtime function dispatch
ncc/ ncm/ ncu/ Lower-level tools (call-machine, asm helpers, util libs)
pkg/          Ready-made example projects (hello, symta, tests)
build/        Per-module build directories used by the Makefiles
examples/     Small standalone programs demonstrating language features
dev/          Documentation and design notes (sbe.txt is the tour)
```


## Compilation Pipeline

### 1. Reader (`src/reader.s`)

The reader is an offside-rule parser.  Significant indentation lets a
single `:` open a multi-line block without explicit `(` / `)`, while
still letting authors fall back to parens whenever they prefer.  The
reader emits an AST built from plain Symta lists -- there is no
distinct "syntax tree" type, which is what makes macros so
straightforward.

### 2. Macroexpander (`src/macro.s`)

Macros run in the *compiler's* Symta runtime.  A macro receives its
arguments unevaluated and returns a new AST.  Most of the language's
"keywords" -- `when`, `while`, `for`, `times`, `case`, `dup`, `mtx`,
`ffi`, `cls`, `type`, ... -- are expressed as macros, often built on
top of three lower-level primitives:

- `_progn`   -- sequence of expressions
- `_label` / `_goto` -- structured jumps within a function
- `_if`      -- two-armed conditional

Pattern matching (`case`, the `{}` map operator, `=` inside `case`,
auto-closure variables `~x`/`x~`) is also a macro layer that compiles
down to the primitives above.

### 3. Uniquifier (`src/compiler.s`, `uniquify_*`)

The expander's output may share variable names across nested lambdas.
The uniquifier walks the AST, alpha-renames bindings to globally
unique labels, computes captured-variable lists for each closure, and
emits a closure environment graph used in code generation.

### 4. SSA Code Generator (`src/compiler.s`, `produce_ssa`)

A simple SSA-style intermediate is built where each value gets a
unique register name.  This pass also gathers the per-module tables
that the runtime needs:

- imported library symbols
- text literals
- type tags and method names
- per-function metadata (name, arity, source location)

After peephole optimisation and tail-call detection, the SSA stream
is converted to SIF text by `ssa_to_sif`.

### 5. SIF Assembler (`runtime/sif.c`, `runtime/sif2sbc.c`)

SIF (`.sif`, but usually consumed in-memory as a string) is a
textual assembly format with one instruction per line.  Each opcode
mnemonic is listed in `runtime/sif.h` (see `enum { SBC_NOP, SBC_SUBR,
... }`).  The assembler resolves labels, packs immediates, and
produces an SBC (`.sbc`) binary blob ready for execution.

### 6. Runtime / Bytecode Interpreter (`runtime/sbc.c`)

`sbc_exec` is the threaded interpreter: it dispatches on opcode bytes,
maintains a frame stack, handles closures and calls, and triggers GC
when a generation fills.  Built-ins live in `runtime/bltin.c` and are
registered as ordinary callable closures backed by C functions.


## Object Representation

Defined in `runtime/symta.h`.  Every Symta value is a 64-bit
`dyn` (`void*`), with a low tag bit indicating "immediate vs heap":

| bits        | meaning                                           |
|-------------|---------------------------------------------------|
| 0           | heap flag (1 = pointer into the GC heap)          |
| 1..15       | type tag (`T_INT`, `T_LIST`, `T_TEXT`, ...)       |
| 16..63      | global id / heap index (48 bits)                  |

Heap pointers are stored as offsets into the heap, not raw addresses,
which lets the GC relocate a generation without rewriting the pointers
that live in older generations.  The header (`gc_head_t`, in front of
each heap object) carries size + bytecode-hook info.

Notable types:

- `T_INT`      -- 48-bit fixnum
- `T_FLOAT`    -- 32-bit float packed in the GID slot
- `T_FIXTEXT`  -- short text fitted entirely in the immediate
- `T_TEXT`     -- heap-allocated text
- `T_LIST`     -- generic list
- `T_HARD_LIST`-- compact array-backed list
- `T_VIEW`     -- non-owning slice over another list
- `T_BYTES`    -- packed byte array
- `T_TBL`      -- hash table
- `T_CLOSURE`  -- code pointer + captured environment
- `T_OBJECT`   -- tagged record (the result of `type` / `cls`)
- `T_NO`       -- the absence-of-value singleton (additive identity)


## Memory Management

Symta's GC (`runtime/gc.c`, `runtime/gc_types.h`) is a bump-allocating
generational copying collector with up to `MAX_AGE` generations.  Each
generation roughly doubles in size; an object that survives a minor
collection is promoted into the next-older generation, and the oldest
generations may share the same backing arena.

Two design choices keep pause times short for typical interactive use:

- Most allocations land in `gen0`, which fits in L2 cache; a young-gen
  collection touches only that arena.
- Immediate objects (ints, floats, fixtexts, type tags) are not heap
  allocated at all and so are GC-free.

Finalisers (`set_finalizer`) and unwind handlers
(`_set_unwind_handler`) hook into the same machinery, giving Symta
precise resource cleanup without manual reference counting.


## Modules and Build Artefacts

A Symta "project" lives under `<project>/src/` and must contain a
`go.s` entry module.  Running

```
symta <project>           # or  symta <project>/src/go.s
```

produces:

- `<project>/sbc/*.sbc`   -- compiled bytecode for every dependency
- `<project>/go.exe`      -- a renamed copy of the runtime that
                             auto-loads `sbc/go.sbc` on launch

Modules import each other via `use foo bar` (first line of a file).
Names are resolved against the project's own `src/` first, then the
compiler's own `src/` (so `core_`, `rt_`, `macro` are always
available).  Compilation is incremental: the compiler caches per-module
SBCs and rebuilds only those whose source -- or whose dependencies'
source -- has changed.

### SAF archives (`saf/`)

`sbc.saf` is a single-file alternative to a populated `sbc/` folder:
multiple `.sbc` files compressed into one archive. The runtime
auto-mounts a `sbc.saf` if the corresponding `sbc/` directory is
missing, which makes shipping a tool as a single executable + one
data file practical.

### Interpreter mode (`-f`, `-e`)

In addition to whole-project compilation, `symta` can evaluate a
single file (`symta -f script.s`) or a one-shot expression
(`symta -e "say hello"`).  Interpreter mode reuses the same
compiler-pipeline-then-execute path; the difference is that the SBC
is loaded into a fresh in-memory module rather than written to disk.
The interactive REPL (entered when no source path is given) is the
same code in a loop.


## Foreign Function Interface

Symta provides two FFI strategies:

1. **Low-level**: `_ffi_call (RetType @ArgTypes) Pointer @Args` calls
   any C function given its address.  `ffi_load` resolves a symbol
   from a DLL/dylib.  Type tags include `ptr`, `int`, `s4`, `u4`,
   `float`, `double`, `text`, `void`.
2. **Macro-driven**: `ffi_begin <lib>` followed by `ffi <name>: ...`
   declarations expands to a set of typed wrapper macros.  The shared
   library must live at `ffi/<lib>/lib/main` (the macro copies it
   into the project's `lib/` folder during the build).

Argument marshalling is delegated to **C/Invoke** (`cinvoke/`), a
third-party library that builds platform-specific call trampolines at
runtime.  This avoids per-platform assembly inside Symta itself --
C/Invoke handles AMD64 SysV, AMD64 Windows, and various 32-bit ABIs.

A handful of native modules ship with Symta and are built from
`c/<name>/`:

| FFI lib   | Source        | Purpose                              |
|-----------|---------------|--------------------------------------|
| `gfx`     | `c/gfx/`      | 2D blitter, PNG decode, gamma LUTs   |
| `ui`      | `c/ui/`       | Window + input loop                  |
| `ttf`     | `c/ttf/`      | TrueType rasteriser (stb_truetype)   |
| `svg`     | `c/svg/`      | SVG parser (nanosvg)                 |
| `vfx`     | `c/vfx/`      | Voxel/3D renderer                    |

The pre-built shared libraries are cached in `ffi/<name>.ffi` (a
DLL/dylib renamed to `.ffi`).


## Component-Oriented Programming Layer

The `type` macro provides classic single-inheritance OOP backed by
vtables.  `cls` builds on top of it to provide an entity-component
system: a `cls foo a b c` declares that "anything with an `a`, a `b`,
and a `c` is a `foo`", and the runtime stores those parts in a small
in-memory database keyed by an integer entity id.  This is implemented
in `src/cls.s`, with related helpers in `src/cla.s` and `src/sml.s`.

`dsm` (defined in `src/cls.s`) registers a system that fires on
events such as part initialisation, finalisation, or external timer
ticks -- the standard ECS pattern, but presented as ordinary Symta
function definitions.


## Testing the Compiler

`pkg/tests/src/tests.s` and the `test.s` at the project root are the
quickest way to smoke-test changes after touching the runtime or
compiler.  The `examples/` directory doubles as a regression suite for
the `-f` interpreter path.


## Glossary

- **SIF**  -- Symta Instruction File; textual assembly
- **SBC**  -- Symta ByteCode; binary form of SIF, what the runtime runs
- **SAF**  -- Symta Archive File; many SBCs compressed into one blob
- **GID**  -- Global Id; the 48-bit upper portion of a `dyn`
- **SES**  -- Symta Entity System; the integer-id ECS layer
- **IPS**  -- Id-Part-System, Symta's preferred name for ECS
- **No**   -- the singleton "no value", an identity element for many
              operations (e.g. `No+x == x`, `No.field == No`)

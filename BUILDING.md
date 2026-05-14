# Building Symta

Symta is a self-hosted Lisp dialect with a small C runtime. The
runtime is portable C99 + a thin POSIX shim; the language itself
boots from committed `.sbc` bytecode and recompiles itself on the
first run. There are no third-party build dependencies beyond a
standard POSIX toolchain.

This directory is fully self-contained. You can copy `./symta`
anywhere and build, test, and run without the surrounding repo.

---

## Prerequisites

A POSIX shell, GNU make, GCC, and a few coreutils (`cp`, `rm`,
`mkdir`, `bash`, `tr`, `sed`, `md5sum`). Plugins additionally need:

- **`ui`** — SDL2 + SDL2_mixer headers and import libs.
- **`gfx`** / **`svg`** — libpng and zlib (prebuilt copies live
  under `c/gfx/deps/` and `c/svg/deps/`; the per-plugin Makefiles
  pick them up automatically, so you don't need system installs).

### Windows — `w64devkit`

The shipping Windows binaries (`symta.exe`, `*.ffi`, `*.dll`) were
built with [w64devkit](https://github.com/skeeto/w64devkit), a
portable single-folder MinGW-w64 + busybox bundle. Extract it,
launch `w64devkit.exe`, `cd` into this directory, and `make`.
Everything you need (gcc, make, bash, coreutils, headers, archiver,
linker, ldd) ships inside the zip — no PATH surgery required.

The SDL2 / SDL2_mixer DLLs you need at runtime are checked into
`examples/16-ui/` (and the UIM test harness picks them up the same
way). They're stock upstream binaries, kept in-tree so a freshly-
cloned `symta` runs without a separate SDL download step.

### macOS

```sh
xcode-select --install    # for clang masquerading as gcc + make + ld
brew install sdl2 sdl2_mixer libpng
```

The macOS plugin Makefiles (`Makefile.osx`) link against the
system frameworks; nothing else is required.

### Linux

```sh
sudo apt install build-essential libsdl2-dev libsdl2-mixer-dev \
                 libpng-dev zlib1g-dev
```

Linux support is best-effort: only `gfx` and `ui` have a dedicated
`Makefile.linux`. `ttf`/`svg`/`vfx` fall back to the Windows-flavored
Makefile and may need a `-DWINDOWS` removed if you hit `windows.h`
includes. Patches welcome.

---

## Building everything

```sh
make           # runtime + plugins + examples + test harness
make help      # list of make targets
```

The default target runs in dependency order:

1.  `runtime` — builds `symta` (or `symta.exe` on Windows). Includes
    the in-tree FFI dispatcher [`runtime/sffi/`](runtime/sffi/).
2.  `plugins` — builds `gfx`, `ui`, `ttf`, `svg`, `vfx` and installs
    each `.ffi` into `ffi/` so `symta .` finds them automatically.
3.  `examples` — compiles every subdirectory under `examples/` to
    its own `go.exe`.

You can rebuild a single component:

```sh
make runtime           # just the symta executable
make gfx               # just the gfx plugin
make examples          # recompile every example
```

A clean rebuild from scratch takes ~30s on a modern laptop.

---

## Running

```sh
./symta.exe -f examples/00-hello.s            # single-file mode
./symta.exe examples/16-ui                    # build + link a project
examples/16-ui/go.exe                         # run the linked binary
```

`symta <dir>` looks for `<dir>/src/go.s`, incrementally compiles
every `.s` under `<dir>/src/` to `<dir>/sbc/*.sbc`, then links them
into `<dir>/go.exe`. Subsequent runs only recompile what changed.

The `-f <file>` mode is for single-file scripts: it compiles to an
in-memory image and runs it without writing artifacts to disk.

---

## Testing

```sh
make test-all          # everything, bottom-up
```

The test suites, in the order they should be read when investigating
a regression:

| Target            | What it covers                                          |
|-------------------|---------------------------------------------------------|
| `test-tokenizer`  | Lexer: numbers, symbols, strings, comments, positions   |
| `test-reader`     | Parser: indentation, operators, postfix, splice forms   |
| `test-macros`     | Macro expander + DSL behaviors                          |
| `test-runtime`    | Single-file `.s` examples, golden stdout comparison     |
| `test-compiler`   | Compiler-output (`.sbc`) byte-equality goldens          |
| `test-gfx`        | C blitter (ffi/gfx.ffi) golden-image regression         |
| `test-uim`        | UIM widget gallery, headless screenshot diffs           |
| `test-drift`      | 5-stage self-hosting bootstrap byte-equality            |

Each test harness lives under `tests/<name>/` with a `run.sh` that
takes `--update` to refresh the goldens, or a basename prefix to run
a single case:

```sh
bash tests/tokenizer/run.sh                   # all tokenizer cases
bash tests/tokenizer/run.sh numbers           # just numbers.s
bash tests/tokenizer/run.sh --update          # rewrite goldens
```

`test-drift` is the most demanding suite: it bootstraps the
compiler five times and requires stages 2..5 to be byte-identical.
A failure there means codegen has acquired some kind of input
dependence — usually hash-iteration order or uninitialised buffer
bytes reaching disk. See `tests/bootstrap/drift.sh` for the full
methodology.

---

## Layout cheat-sheet

```
symta/
  src/                  Symta source for the self-hosted compiler
  pkg/symta/src/go.s    Entry point invoked when self-rebuilding
  sbc/                  Committed compiler bytecode (bootstrap input)
  runtime/              C runtime (gc, tokenizer, reader, VM, ffi)
    sffi/               In-tree FFI dispatcher (x86-64 Win64 + SysV)
  c/{gfx,ui,ttf,svg,vfx}/   FFI plugins, one folder each
  ffi/                  Plugin .ffi files installed here on build
  examples/             Tutorial demos (see examples/README if added)
  tests/                Test harnesses (see table above)
  dev/                  Development notes, scratch, internal docs
  blog.md               Story of the C reader port (Jan 2026)
  architecture.md       Overview of how the pieces fit together
  README.md             Tour of the language
  Makefile              This build orchestrator (top-level)
  Makefile.w64/.osx     Per-platform runtime build rules
```

---

## Embedding Symta in another project

There's nothing special about `./symta` as a project root: drop the
folder anywhere, `make`, and you have a working compiler + runtime.
Two integration patterns:

1.  **Out-of-tree project.** Build symta once, then call
    `path/to/symta.exe <your-project>` from your own Makefile. Your
    project gets its own `sbc/`, `lib/`, and `go.exe`; symta only
    needs to be present.
2.  **Vendored.** Copy `./symta` into your repo and add a thin
    top-level Makefile that does `cd symta && make`, then chains
    your project's compile step. This is what the SoM game does;
    look at its root `Makefile` for a worked example.

---

## Known issues

- **`pkg/symta` bootstrap fails on `go.s`** with a parser error
  about `~` — pre-existing and load-bearing. The drift test
  tolerates this and only compares the three sbc files that *do*
  get rewritten (reader, eval, compiler).
- **Linux plugin support is partial** — see Prerequisites above.
- **Windows + busybox bash** lacks process substitution; the test
  runners use repo-local temp files instead. Don't replace them
  with `<(diff ...)` patterns without testing on w64devkit.

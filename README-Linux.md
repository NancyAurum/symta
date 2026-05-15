# Building Symta on Linux

Tested on Ubuntu 24.04 LTS / gcc 13.3. Should work on any glibc-based
distro with a recent gcc. If your distro doesn't have a package named
exactly like the ones below, the libraries are universal — just find
the equivalent.

## Prerequisites

### Always required

```
sudo apt install build-essential libpng-dev zlib1g-dev libfreetype-dev
```

That's enough to build the runtime (`./symta`), the standalone test
suites, and three of the four FFI plugins (`gfx`, `ttf`, `svg`).

### Optional — needed only for the `ui` plugin

```
sudo apt install libsdl2-dev libsdl2-mixer-dev
```

You need this if you want to:

  - run the `uim` widget test suite (`make test-uim`)
  - build any example or project that uses `uim` (the game, voxpie,
    examples/16-ui, examples/31-isometric, etc.)

If you skip SDL, everything else still works: the compiler, the FFI
test suite, the runtime/reader/macro/compiler tests, the `gfx`
golden-image tests, and the drift bootstrap.

## Quick build

From `symta/`:

```
make            # builds runtime + plugins + examples (all that apply)
make runtime    # just ./symta
make plugins    # just gfx + ui + ttf + svg into ffi/
make test-all   # every test suite, bottom-up
```

The top-level orchestrator (`Makefile`) detects Linux automatically
via `uname -s` and picks `Makefile.linux` for runtime / per-plugin
builds.

## What gets built where

| Component         | Source dir          | Build command                                  | Output                  |
|-------------------|---------------------|------------------------------------------------|-------------------------|
| runtime           | `symta/runtime/`    | `make -f Makefile.linux`                       | `symta/symta`           |
| gfx plugin        | `symta/c/gfx/`      | `make -f Makefile.linux` (inside the dir)      | `lib/gfx.ffi`           |
| ui plugin         | `symta/c/ui/`       | `make -f Makefile.linux`                       | `lib/ui.ffi`            |
| ttf plugin        | `symta/c/ttf/`      | `make -f Makefile.linux`                       | `lib/ttf.ffi`           |
| svg plugin        | `symta/c/svg/`      | `make -f Makefile.linux`                       | `out/svg.ffi`           |
| vfx plugin (SoM)  | `vfx/` (repo root)  | `make` (after `make -C ncc`)                   | `lib/vfx.ffi`           |

Plugins are auto-staged into `symta/ffi/<name>.ffi` by the top-level
`make plugins`.

## Layout differences from Windows

The `.ffi` files are real ELF shared objects (`.so` content, renamed).
On Windows they're DLLs. The repo currently tracks the Windows
versions; on Linux you rebuild and overwrite locally — the local
modifications are not meant to be committed.

The compiled `symta` binary is **not** tracked on Linux (only the
Windows `symta.exe` is). Building from source is the only supported
path; the resulting binary is glibc-version-specific and not portable
across distros without `-static-libgcc` and friends.

## Test status

```
make test-tokenizer    # 9/9 (one stdout/stderr interleaving edge case)
make test-reader       # 12/12
make test-macros       # 13/13
make test-runtime      # 25/25 — modulo hash-iteration intermittents
make test-compiler     # 25/25
make test-gfx          # 29/29
make test-ffi          # 17/17 — modulo a 2-line zlib-version golden
make test-uim          # 8/8   — modulo libpng-version golden bytes
make test-drift        # PASS  (3-round bootstrap fixed point)
```

Smoke tests of full applications:

```
( cd ../game        && ../symta/symta . && ./go )
( cd ../voxpie/pkg  && ../../symta/symta . && ./go )
```

Both spawn SDL windows and render frames. Use
`--screenshot=out.png --screenshot-frame=60` to capture a single PNG
headlessly and exit (handy for CI).

## Known platform-specific gotchas

### `-ffast-math` on math-heavy plugins

The vfx renderer used to compile with `-ffast-math`. On Linux + gcc
13, `-ffast-math` implies `-ffinite-math-only`, which lets the
compiler fold `if (t < INFINITY)` to `true` — fatal for code that
uses `INFINITY` as a "ray missed" sentinel. Win64's older gcc didn't
perform that fold. We dropped `-ffast-math` from `vfx/Makefile`.

**If you write math kernels that rely on `Inf` / `NaN` semantics
(termination tests, miss sentinels, "uninitialised" markers), do NOT
build them with `-ffast-math`.** Add explicit max-distance sentinels
instead if you need the speed.

### Glibc PATH_MAX

`realpath()` on Linux glibc wants a buffer ≥ `PATH_MAX` (4096) bytes
or `NULL` (let libc allocate). A short stack buffer triggers a hard
`"*** buffer overflow detected ***"` from FORTIFY. We use the `NULL`
form throughout the runtime now.

### Implicit prototypes truncating pointers

`-std=c99` hides POSIX-only declarations like `strdup` / `strndup`
unless you `#define _GNU_SOURCE`. Without the prototype, the compiler
implicitly types these as `int (*)()`, the returned `char *` is
truncated to its low 32 bits on x86-64, and the next dereference
segfaults inside a hash lookup. If you add a new C source file that
uses POSIX libc, make sure either `_GNU_SOURCE` is set before any
include or the Makefile passes `-D_GNU_SOURCE`.

### Hidden visibility cascading into libc

`runtime/common.h` would historically `#pragma GCC visibility
push(hidden)` to keep internal Symta symbols private. On Linux that
pragma cascades through subsequent system header includes, marking
libc symbols (`exit`, `stderr`, `malloc`, …) as hidden undefined
references. ld then refuses to satisfy them from `libc.so`. The
pragma is now guarded by `!defined(__linux__)`; Win64 + macOS keep
the original behaviour.

## Reporting build issues

If anything breaks, the most useful artefacts are:

  1. The exact `make` command that failed and its full stderr.
  2. Output of `gcc --version`, `uname -a`, `ldd --version`.
  3. `dpkg -l | grep -E 'libsdl2|libpng|zlib|freetype'`.

File an issue at https://github.com/NancySadkov/som with that. The
sffi backend lives at `runtime/sffi/arch_x64_sysv.c` — most "FFI call
crashed" reports trace back there.

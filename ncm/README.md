# ncm — the New-C Macro preprocessor

A drop-in alternative to the C/C++ preprocessor that fixes the
obvious pain points (positional-only args, no recursion, no
arithmetic, `#ifndef X` / `#define X` / `#endif` boilerplate per
header) without giving up `#`-directive compatibility — escaped
`#\`-directives pass through unchanged so the platform's
`gcc -E` can still consume the output.

ncm is the macro layer that [`../../ncc/`](../../ncc/) uses
internally (included as a header-only library), and is what
Symta itself imports via [`runtime/ncm.h`](../runtime/ncm.h)
(a one-line forwarder for `ncm_process_` and friends).  Single
source-of-truth; one test suite to catch bugs before they ship.

## What it improves over cpp

- **Succinct declarations**: `#PI 3.14` instead of
  `#define PI 3.14`; `#if #SYMBOL` instead of `#if defined SYMBOL`.
- **Pragma-once default for `.h` files** — no
  `#ifndef X / #define X / #endif` boilerplate.
- **Keyword + default arguments** on macros:
  `#foo(x,y=123,z=abc) foo(x,y,z)` lets you call
  `foo(456, z=789)`.
- **Arithmetic + recursion** in expansion:
  `#fac(n,x) #if n>0 fac(#[n-1], #[x*n]) #else x #endif` defines
  a proper compile-time factorial.
- **Punctuation rewrites**: `#+ -` rewrites `+` to `-` lexically.
- **Scoped macros**: `#{ … #}` blocks shadow names, just like
  C variables.

The full directive + expression reference lives in
[`../dev/ncm.md`](../dev/ncm.md); architectural notes are at
[`architecture.md`](architecture.md).

## Build

```sh
./build.sh    # POSIX / MinGW / WSL
build.bat     # Windows cmd.exe
```

Both compile `ncm` (`ncm.exe` on Windows) in this directory.
For consumers of the header-only API (ncc, the symta runtime),
`src/ncm.h` is the file that matters; the standalone driver is
mostly there as a smoke target and as host for the regression
suite.

## Tests

```sh
bash tests/run.sh             # all cases
bash tests/run.sh shifts      # one case
bash tests/run.sh --update    # rewrite goldens from current output
```

The runner builds `ncm` if missing, then runs each `tests/cases/*.c`
through it and compares the (normalised) output to
`tests/expected/<case>.out`.  Cases cover:

| Case | What it pins |
|------|--------------|
| `factorial.c`, `genid.c`, `map.c`, `printf.c` | Historical inputs (the original `example/` demos) -- exercise recursion, function macros, variadic spread, ##-escape. |
| `shifts.c`     | NCM-1 regression: `<<` and `>>` in `#[expr]`. |
| `logic.c`      | NCM-2 regression: `&&` and `||` in `#[expr]`; bitwise on same operators. |
| `hex_fmt.c`    | NCM-3 regression: `0x...` numeric inputs to `#("fmt" ...)`.  Also pins `%x` → lowercase / `%X` → uppercase (matches C `printf`).  Pre-May 2026 the case mapping was inverted; fixed once an audit confirmed no caller depended on it. |
| `conditionals.c` | `#if`/`#elif`/`#else`/`#endif` + `#undef`. |
| `fn_macros.c`  | Function-style macros: positional, default args, variadic, recursive expansion. |
| `escapes.c`    | `##` literal, in-string passthrough. |
| `precedence.c` | Operator precedence across all `#[expr]` levels. |

This suite is the layer below `symta/examples/25-lexmacro.s`
+ `tests/runtime/25-lexmacro` — it catches NCM bugs without
needing the rest of Symta to build.

## Layout

```
ncm/
  src/
    ncm.h        Header-only implementation (canonical source-of-truth)
    main.c       Standalone driver (compiles to ncm / ncm.exe)
    ncu_file.h   File-reading helpers (shared shape with ncc/src/)
    ncu_opts.h   `key=value` arg parser
  tests/
    cases/       Input .c files
    expected/    Golden outputs
    run.sh       Test runner
  build.sh       Build standalone ncm (POSIX)
  build.bat      Build standalone ncm.exe (Windows)
```

`stb_ds.h` is shared with the symta runtime at
[`../runtime/stb_ds.h`](../runtime/stb_ds.h); the build scripts
pass `-I../runtime` to find it.

## License

Same MIT / Apache-2 dual licensing as the rest of the Symta tree.
Author: Nancy Sadkov.

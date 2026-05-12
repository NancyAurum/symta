# Symta

> A list-processing Lisp dialect where one terse line replaces fifty
> verbose ones — and stays readable.

Inspired by **REFAL**, **POP-11**, and **MIT PLANNER**, Symta is built
around one idea: the everyday operations that bloat real programs —
filtering, mapping, parsing, replacing, destructuring, classifying —
should each be one short, composable construct. Not a library call.
Not a regex. Not a 30-line for-loop. **One construct.**

```symta
// FizzBuzz, in full:
[:100]{~?%15=\FizzBuzz; ~?%3=\Fizz; ~?%5=\Buzz}

// Quicksort:
qsort@r H,@T = @T{:?<H}^r, H, @T{?<H=}^r

// Substitute %vars% in a template from a table -- in 18 characters:
Form{@"%[V]%"=Data.V}
```

The same `{}` operator does map, filter, group, reduce, replace, parse
and pattern-match — because in real programs those are the same idea
with different bodies.

---

## Why you might care

- **You write Lisp** and miss having concise list-processing on tap.
  Symta has macros, `eval`, quasiquote, gensyms — *and* a pattern
  matcher fused into the syntax.
- **You like APL or J** but wish your read-only colleagues didn't.
  Symta beats J in brevity on most non-linear-algebra tasks while
  staying alphabetic and grep-friendly.
- **You wrangle data, logs, or text.** Replacing "every word matching
  X" or "every digit grouped by line" is a one-liner, not a sprint.
- **You build games or simulations** and have hit the wall where OOP
  starts pretending it scales. Symta ships a real ECS (`cls`, `dsm`,
  IPS) integrated into the language, not bolted on as a library.
- **You implement languages.** Symta is self-hosted, AOT-compiled to
  bytecode, runs on a small generational-GC C runtime (~28k lines of
  Symta + ~10k lines of C), and the compiler fits in your head.
  See [architecture.md](architecture.md).

---

## A 60-second taste

```symta
// Forward and backward through the same shape:
Data name!\Nancy age!37 city!\Amsterdam
Form "Hi! My name is %name%, I'm %age% years old and I live in %city%."
say Form{@"%[V]%"=Data.V}
//   Hi! My name is Nancy, I'm 37 years old and I live in Amsterdam.

say Form("Hi! My name is [Name], I'm [Age] years old and I live in [City]." =: Name Age City)
//   binds Name="Nancy", Age=37, City="Amsterdam"


// Frequency table, sorted by count -- 1 line:
S{~D.?+}.s | ?1 > ??1


// Tree walk: collect ints > 100 anywhere inside a nested structure:
Tree(:_^r@_^r; &~r._<{?>100}<int?)


// 11-line Prolog-style inference engine, two-way:
fact Sbj Verb Obj =
  new Sbj Verb,Obj
  new Obj "[Verb]_by",Sbj

infer Sbj Verb =
  Rs Sbj^each{Verb}.skip^No
  Rs: Rs @Rs{|infer ? Verb}
  Rs.j

who  Verb Sbj = say infer(Sbj "[Verb]_by").text^" " Verb Sbj
what Sbj Verb = say Sbj Verb: infer(Sbj Verb).text^", "
```

Every example above is real, runnable Symta — not pseudocode.

---

## Quickstart

A pre-built `symta.exe` ships in the repo (Windows x64). On macOS or
Linux, build from source:

```sh
make -f Makefile.osx     # or Makefile.w64 on Windows
```

Hello, World:

```sh
echo 'say "Hello, World!"' > hello.s
./symta.exe -f hello.s
```

Or compile a project to a standalone binary:

```sh
mkdir -p myapp/src
echo 'say "Hello, World!"' > myapp/src/go.s
./symta.exe myapp
./myapp/go.exe                # -> Hello, World!
```

Or just open the REPL:

```sh
./symta.exe
```

---

## Learning path

1. **[examples/](examples/)** — 26 progressively richer programs.
   Single-file examples (00–09, 11, 14, 17–20, 22–25) run with
   `symta -f examples/NN-*.s`; project examples (10, 12, 13, 15,
   16, 21) build with `symta examples/NN-name && examples/NN-name/go.exe`.
2. **[dev/sbe.txt](dev/sbe.txt)** — *Learn Symta by Example*, the
   long-form tour of the language: variables, functions, lists,
   pattern matching, OOP, ECS, FFI, macros, the quirky bits, and
   the standard library reference.
3. **[AI.md](AI.md)** — a tight, paste-into-LLM-context cheat sheet
   covering syntax, semantics, the standard library, and common
   gotchas.
4. **[architecture.md](architecture.md)** — for contributors and the
   language-implementation curious: how the compiler, runtime, GC,
   and FFI fit together.

---

## What's in the box

| | |
|--|--|
| `symta.exe`        | The compiler + runtime, statically linked. |
| `src/`             | Compiler, reader, macroexpander, stdlib — all in Symta. |
| `runtime/`         | C runtime: GC, bytecode interpreter, built-ins. |
| `examples/`        | Self-contained example programs. |
| `c/`, `ffi/`       | FFI plugins — graphics, UI, fonts, SVG, voxel renderer. |
| `pkg/`             | Ready-made projects (hello, tests, the symta tool itself). |

---

## License

Symta is dual-licensed under either of:

- [Apache License, Version 2.0](LICENSE-APACHE)
- [MIT License](LICENSE-MIT)

at your option. Contributions are accepted under the same dual
license unless you explicitly say otherwise.

---

*By Nancy Sadkov. With thanks to the /prog/riders of bbs.progrider.org
for early criticism of the language design.*

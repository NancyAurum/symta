# AI.md — Preprompt for working on Symta

> This document is meant to be pasted into an LLM's context as a
> compact reference for writing or modifying Symta code. It is the
> distilled subset of [dev/sbe.md](dev/sbe.md) plus every gotcha
> we've hit in practice. Read [examples/](examples/) for runnable
> programs, [architecture.md](architecture.md) for the
> implementation, and dev/sbe.md for the long form.


## What Symta is

A list-processing Lisp dialect inspired by REFAL, POP-11, and MIT
PLANNER. Self-hosted, AOT-compiled to bytecode, with a small
generational-GC C runtime. Source files have extension `.s`.


## The capitalization rule (foundational)

Every identifier is one of two things, decided by its first letter:

- **Uppercase** (`X`, `Foo`, `Items`) -- a *variable*.
- **Lowercase or non-letter** (`x`, `say`, `+`, `123`) -- a
  *function name* or a *self-quoting symbol*. Bare lowercase words
  evaluate to themselves: `[hello world]` is a 2-element list of
  the symbols `hello` and `world`.

This split is why function calls don't need parens: `say x` is
unambiguously `say(x)` because `x` can't be a variable.


## Declaration vs assignment

```symta
A 5            // declare A with value 5
A = 7          // reassign existing A (must be already declared!)
A *= 3         // C-style compound assignment
&fn = | X => X+1   // assign a verb (function) to a name
```

Common mistake: `Var = value` for a not-yet-declared variable
errors with "undefined variable". Use `Var value` for declaration.


## Functions

```symta
greet Name = say "Hello, [Name]!"      // definition
greet "World"                          // call -- no parens needed
greet("World")                         // parens form also works

factorial N = if N >< 0 then 1 else N * factorial(N-1)

hi Name!"World" = say "Hi, [Name]!"    // keyword arg with default
hi                                     // hi -> "Hi, World!"
hi Name!"there"                        // keyword call

double X = X * 2
say: 3 ^ double                        // ^ is "apply on the left": double(3)

local_helper A B =                     // local fn inside a body
  sq X = X * X
  (sq(A) + sq(B)).float.sqrt
```

Calling forms:
- `f x y z`        -- 3 positional args (most common)
- `f(x y z)`       -- same with parens
- `f: x y z`       -- `f (x y z)`, useful to avoid wrapping
- `x ^ f`          -- `f(x)`
- `x ^ f ^ y`      -- `f(x, y)`


## Control flow

```symta
if C then A else B
if C: A else B                         // sugar for above
when C: A                              // if C then A else No
less C: A                              // if C then No else A
if: C1 = A; C2 = B; 1 = Default        // multi-branch (chain ends with 1)
```

C-style logical:
```symta
A and B          // short-circuit AND
A or B           // short-circuit OR
not C            // 1 if C is 0, else 0
A && B           // logical AND with .bool coercion
A || B           // logical OR with .bool coercion
got X            // 1 if X is not No, else 0
```

Loops:
```symta
times I 5: say I                  // 0..4
for X Xs: say X
Ys map I 5: I*I                   // collect into list
dup I 5: I*I                      // same as map but inline
I 0
while I < 5:
  say I
  I+
pass / done                       // continue / break inside loop body
```

**Multi-statement bodies** under `if`/`while` need `|` separators:

```symta
if X > 0:
  | say "positive"
  | say "and we get here"
  else say "non-positive"
```


## Comparison operators

```symta
A >< B    // equal       (one token, NOT > followed by <)
A <> B    // not equal
A < B     // less        A > B    greater
A << B    // less-eq     A >> B   greater-eq
```

`==`/`!=`/`<=`/`>=` do **not** exist. `=` is binding/match, never
equality.


## Lists

```symta
Xs [hello world 123]              // literal list
Ys: a b c                         // sugar for Ys [a b c]
Ns 1,2,3,4,5                      // comma builds a list

@Xs                               // splice inside another list
[@Xs more @Ys]
Xs.0  Xs.3                        // indexing
Xs.n                              // length
Xs[2:5]                           // slice (zero-based, half-open)
Xs[3:]    Xs[:4]    Xs[0::2]
Xs[N:M:S]                         // start:stop:step
[:5]                              // 1..5  (note: 1-based, inclusive)
[1:5]                             // 1..5
[0:10:2]                          // 0,2,4,6,8,10
\x..N                             // N copies of x
```

Common methods (defined in `core_.s`):
```
.n length      .f flip/reverse    .z sum
.s sort        .j join (flatten)  .i indexed
.l to-list     .x hex             .t to-table
.text join-as-text   .max  .min   .head  .tail  .end
.split^X       .keep F            .skip F
.find F        .locate F          .uniq
.zip           .infix X           .group N
```

Vector arithmetic on equal-length lists is element-wise:
```symta
[1 2 3] + [4 5 6]          // (5 7 9)
[1 2 3] * [4 5 6]          // (4 10 18)
```


## The `{}` map operator -- the heart of Symta

`Xs{Body}` applies `Body` to each element of `Xs` and collects the
results. Inside `Body`:

- `?` is the current element
- `=` introduces a *match clause*: `Pattern = Replacement`
- `:` introduces a *filter*: `:Cond` keeps elements where Cond is true
- non-matching elements pass through unchanged
- `~name` (auto-closure) introduces a hidden var that's *returned*
  by the whole `{}` expression
- `name~` is the same but kept private

Examples:
```symta
10{?*?}                           // squares of 0..9
[1 2 3 4 5]{?%2=}                 // keep odds
[1 2 3 4 5]{:?%2}                 // skip odds (keep evens)
[:15]{~?%15=\FizzBuzz; ~?%3=\Fizz; ~?%5=\Buzz}    // FizzBuzz
"hello"{~D.?+}                    // freq table -> @{h!1 e!1 l!2 o!1}
"1-a, 2-b"{d?=~i+}                // count digits, returns 2
S{@\bad=\good}                    // word replacement in text
[3 1 2].s | ? > ??                // sort descending (?, ?? = pair args)
```


## Pattern matching

Destructuring:
```symta
[X Y Z] Xs                        // bind X,Y,Z to first 3 of Xs
[A @As] Xs                        // head/tail
[@Bs B] Xs                        // lead/last
[@Pre needle @Post] Xs            // split on `needle`
```

`case`:
```symta
classify N = case N:
  0      = "zero"
  1+2+3  = "one to three"         // + is OR in patterns
  X<int? = "int [X]"              // < binds and constrains
  Else   = "other"

palindrome Xs = case Xs.l
  [S @Mid $S] | palindrome Mid    // $X == "equal to value of X"
  []+[X]      | 1
```

Function args can be destructured:
```symta
v_size [X Y] = (X*X+Y*Y).float.sqrt
```

Quicksort, full implementation:
```symta
qsort@r H,@T = @T{:?<H}^r, H, @T{?<H=}^r
```


## Strings

```symta
"Hello, [Name]!"                  // interpolation: [Expr] is evaluated
"verbatim \[1:4\]"                // \[ \] -- literal brackets
\Word                             // single quoted symbol (preserves case)
'multi word string'               // single-quoted text (no interp)
"line\nbreak"                     // \n \t etc work
"contains \"quotes\""             // \" inside "..."
text.parse                        // parse as Symta source -> AST
text.eval                         // parse + evaluate (needs `use eval`)
text.split^' '                    // split on a delimiter
text.utf8                         // -> list of bytes
list.utf8                         // bytes -> text
```


## Hash tables

```symta
T name!\Nancy age!37 city!\Amsterdam
T.name                            // field access
T.iron = 5                        // mutation
T2!                               // empty table
T3 letters!! a b c                // double-! gives a list value
for K,V T: say "[K] -> [V]"
T.has Key   T.got Key   T.del Key
```


## Object orientation

Three flavours:

**`type`** -- classic single-inheritance OOP, vtable-backed:

```symta
type point x y                    // creates `point` ctor, fields x,y
@as_text = "([$x],[$y])"          // method on most-recent type
@distance_from_origin = ($x*$x + $y*$y).float.sqrt

P point 3 4                       // constructor
P.x = 6                           // mutate field

type circle.point X Y R: x!X y!Y r!R    // .point => inherits methods
```

`@name` inside a type body is sugar for `typename.name`. `$field`
inside a method is sugar for `Me.field`.

**`cls`** -- entity-component-system:

```symta
cls person name age               // any entity with name+age IS a person
@as_text = "[$name] ([$age])"
@is_minor = $age < 18

P person \Nancy 37
for X each(person.name): say X    // iterate every entity with that part

P.coords_.xc = 1                  // attach a different cls's parts to P
P.coords_.yc = 2
```

**`cph`** -- "columnar phase", systems that fire on parts:

```symta
cph birthday person.age:          // runs on every entity carrying person_age
  $age = $age+1

cls_run_phase birthday            // explicitly run the phase
```


## Macros

Macros run at compile time. Args arrive unevaluated as AST; return
new AST. Most "keywords" (`when`, `while`, `for`, `case`, ...) are
macros.

Define in a separate `.s` file and `export 'name'`:

```symta
// in mymac.s
export 'pi' 'unless' 'swap' 'repeat'

pi K = K * 3.14159265             // computed at compile time

unless @Cond Body = form: if Cond then No else Body

swap A B = form:                  // need raw AST -- can't be a function
  ~T A                            // ~T = gensym (unique per expansion)
  A = B
  B = ~T

repeat N Body =                   // unroll a loop at compile time
  Copies map I [:N] Body
  form: `|` $@Copies              // `|` wraps multiple stmts;
                                  // $@ splices a list into the AST
```

Use:
```symta
// in go.s
use mymac
say "pi(2.0) = [pi 2.0]"
unless X > 0: say "non-positive"
swap A B
```

Form syntax inside `form:`:
- bare names refer to the macro's own local variables
- `~name` -- gensym (unique symbol for each expansion)
- `$Expr` -- splice value of Expr into the AST
- `$@List` -- splice each element of List into the surrounding form
- ``\`Op` `` -- quote a literal symbol (e.g. backtick-`+`-backtick)


## Foreign Function Interface

Two flavours, both backed by C/Invoke trampolines:

**Declarative** -- `ffi_begin local <lib>` then `ffi <name>: ...`:

```symta
ffi_begin local zlib
ffi zlibVersion.text                              // const char *zlibVersion(void)
ffi crc32.u4 Crc.u4 Buf.ptr Len.u4                // uLong crc32(uLong, ptr, uInt)
ffi compress.int Dst.ptr DstLen.ptr Src.ptr SrcLen.u4

say zlibVersion
```

The first arg of `ffi_begin` is a mode flag (`local`, `export`, or
`macro`). The library is loaded from `ffi/<name>.ffi` (a renamed
DLL/dylib).

Native types: `void`, `int`, `s4`, `u4`, `float`, `double`, `text`,
`ptr`. For raw memory access: `_ffi_set uint8_t Ptr Index Value`,
`_ffi_get uint32_t Ptr Index`, etc. (also `uint8_t/u1`, `int8_t/s1`,
`uint16_t/u2`, `int16_t/s2`, `int32_t/s4`).

**Low-level** -- when you don't want a binding:

```symta
&Crc32Fn ffi_load \zlib \crc32
R _ffi_call \(u4 u4 ptr u4) Crc32Fn 0 P N
```

Note the `\(...)` -- without the leading `\`, parens get parsed as
a function call.

Memory helpers: `ffi_alloc N` (-> raw ptr), `ffi_free P`,
`ffi_memset P V N`.


## Modules

A project lives at `<root>/src/`. The entry point must be `go.s`.
Other modules are sibling `.s` files.

```symta
// in mymod.s
export greet
greet Name = say "Hello, [Name]!"

// in go.s
use mymod
greet "World"
```

`use` must be the first non-comment line of the file. Always-loaded
modules: `rt_`, `core_`, `macro`. Other useful ones: `cls`, `gfx`,
`uim`, `rgb`, `eval`, `reader`, `cache`, `store`, `font`, `svg`.


## Running

```sh
symta -f file.s              # interpret a single file (limited: no `use`)
symta -e "say hello"         # eval one expression
symta path/to/project        # compile project to ./go.exe (entry: src/go.s)
symta                        # REPL
```

`symta -f` works only for code that doesn't need `use` -- the
runtime can only resolve modules already in the compiler's `sbc/`
folder. Examples that need any `use` directive must be project-mode
(have a `src/go.s`).


## Gotchas (saves real debugging time)

1. **`Var = value` requires Var to be already declared.** First use
   is `Var value`. The error is "undefined variable Var".

2. **String interpolation eats `[...]`.** `"Big[2:5]"` is a parse
   error inside a `"..."` string. Compute slices into a temp
   variable, or escape with `\[` / `\]`.

3. **`\#` doesn't lex.** The `#` is not a token starter. Pick a
   different character for placeholders (`\x`, `\*` etc).

4. **`No.field` errors** for most fields. `No` is the additive
   identity (`No + 7 = 7`) but doesn't accept arbitrary methods.
   Test with `got X` instead of poking at `X.field`.

5. **`f x y z` is `f(x, y, z)`**, not `f((x y z))`. To pass one
   compound argument, use `f: x y z` or `f (x y z)`.

6. **Multi-statement bodies need `|` separators** under `if`,
   `while`, `for`. Without them, the parser sees `Var Value` as a
   function call.

7. **`(...)` is parsed as a call**. To write a literal list of types
   for `_ffi_call`, prefix with `\`: `\(u4 ptr u4)`.

8. **`-f` mode can't `use` arbitrary modules.** It only sees
   compiled-in modules under the runtime's `sbc/`. For anything
   beyond `core_` and `rt_`, switch to project mode.

9. **`say a b` prints `(a b)` as a list.** `say` takes one argument;
   passing more makes it print them as a tuple. Use `say "..."` or
   `say: a b` for one-arg.

10. **Methods on integers.** `9.x` calls `int.x` (returns "9", hex
    digit). It does NOT mean "field x of integer 9".

11. **`one`, `two`, `three` are bare lowercase symbols and self-quote**,
    BUT inside a `case` or `{}` body they may collide with a
    function name. Quote them with `\one` if in doubt.

12. **`form:` body has its own gensym rules.** Names without
    `~`-prefix refer to the macro's local vars; `~Name` produces a
    fresh symbol per expansion.

13. **`bad` always prints a stack trace**, even when caught by
    `btrap`. The `bterror` value is correctly returned, but the
    trace fires from inside `bad` unconditionally. Don't use
    "stderr is clean" as a test signal -- check the return value.

14. **Finalizers see partially-reclaimed objects.** By the time a
    `set_finalizer F` callback runs, the object's hashtable fields
    may already be gone. Capture the values you need (file
    descriptor, malloc'd ptr, id) in the finalizer's *closure*,
    not from the object's fields:
    ```
    H handle Id
    set_finalizer H | _ => close_fd Id   // good: Id captured
    set_finalizer H | X => close_fd X.id // bad : X.id may be No
    ```

15. **`@{key!value}` is the literal table syntax** -- nothing inside
    is evaluated. `@{id!Id}` puts the symbol `Id` in the table, not
    the variable's value. For computed values, build the table with
    `T!` then assign fields, or use `@t: map ...`.

16. **Nested `[...]` inside string interpolation breaks the parser.**
    `"[f [1 2 3]]"` does not work. Compute the inner expression
    into a temp variable and interpolate the variable instead.

17. **`fin Body Finalizer` macro is unimplemented** in current
    builds (the `set_unwind_handler` opcode is missing from the VM).
    Use `btrap` + explicit cleanup, or `set_finalizer` for
    GC-driven cleanup.

18. **`have FactSymbols.Name: Default`** is the canonical
    "if-not-set, set-to-Default-and-return" pattern, used everywhere
    in the standard library. Equivalent to:
    ```
    if no FactSymbols.Name: FactSymbols.Name = Default
    FactSymbols.Name
    ```

19. **Method definition syntax `name/default`** uses `/`, not `=`
    or `!`. So `text.trim s/' ' l/1 r/1 = ...` is a method with
    three optional parameters. Inside *call sites*, kwargs use
    `name!value`. Don't write `text.trim r/0` as a call -- the
    `r/0` will be parsed as the file-load path syntax.

20. **`(-5)` parses as a function call**, not as a negative literal,
    in many contexts. Prefer storing in a variable: `Neg -5; f Neg`
    or use a leading binary minus: `f (0-5)`.


## Quick scaffolds

A new project:
```
mkdir -p myapp/src
cat > myapp/src/go.s <<'EOF'
say "Hello from myapp!"
EOF
symta myapp
./myapp/go.exe
```

A new module imported by go.s:
```
cat > myapp/src/util.s <<'EOF'
export greet
greet Name = say "Hello, [Name]!"
EOF
cat > myapp/src/go.s <<'EOF'
use util
greet "World"
EOF
```


## See also

- [examples/](examples/) -- 26 progressively richer programs (00..25)
- [dev/sbe.md](dev/sbe.md) -- the long-form tutorial
- [architecture.md](architecture.md) -- compiler/runtime internals
- [src/core_.s](src/core_.s) -- the standard library, in Symta
- [src/macro.s](src/macro.s) -- the built-in macros
- [src/cls.s](src/cls.s) -- ECS / cls / cph implementation

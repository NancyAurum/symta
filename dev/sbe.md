<img src="../logo.webp" alt="Symta logo" width="96" align="right">

# Learn Symta by Example

A working tour of the language.  You should already know a little
programming — what a variable is, what a function does — but you
don't need to know any specific language to follow along.  Every
example below is real, runnable Symta.

---

## Why Symta exists

Most programs spend most of their lines on three operations:
**transform a list of things, filter it, and put it back together
in a different shape**.  Reading text into structured data and
emitting structured data back as text.  Walking trees.  Matching
patterns.  Replacing the parts that need replacing and leaving
the rest alone.

In C++, Python, or JavaScript, each of those is a different tool:
a for-loop, a regex, a list comprehension, a parser library, a
template engine.  You stitch them together by hand.  By the third
or fourth time on a single project, you're maintaining glue.

Symta says: those are all the same operation with a different
body.  Give them one syntax — `{}` — and let you write the body
in whatever style fits the problem, from a terse one-liner to a
multi-page well-commented function.  The language stays out of
your way.

```symta
// FizzBuzz, in full:
[:100]{~?%15=\FizzBuzz; ~?%3=\Fizz; ~?%5=\Buzz}

// Quicksort:
qsort@r H,@T = @T{:?<H}^r, H, @T{?<H=}^r

// Substitute %vars% in a template from a table -- 18 characters:
Form{@"%[V]%"=Data.V}

// Parse a sentence back into the values that built it:
say Form("Hi! My name is [Name], I'm [Age] years old." =: Name Age)
```

Every snippet above is one expression.  Every snippet is also
something you could spell out across twenty lines if a colleague
preferred — *Symta gives you the dial*, from terse to verbose,
without changing languages.

That's the elevator pitch.  The rest of this document shows you
why, when you've used the language for a week, the terse form
becomes the readable one.

---

## What Symta gives you that other dynamic languages don't

Three concrete promises, in plain English.

### 1.  Lexical scoping — *the variables on screen are the variables you get*

When you write:

```symta
Total 0
till Done:
  Total = Total + 1
```

`Total` is the same name throughout — created once on the first
line, reassigned on the third.  No surprises from a function call
"reaching into" your `Total` from somewhere else.

JavaScript, Python, and Lua were all designed with looser rules
that let nested functions and "global by default" name lookups
collide in subtle ways.  The classic trap is a `for i in ...:`
loop where `i` outlives the loop body and silently shows up in
the next function you wrote because nothing said it shouldn't.

Symta does what most modern languages now copy from Scheme and
ML: every name is visible exactly where the source code says it
is.  When you read a Symta function, the variables you see are
*all of them*; nothing leaks in from elsewhere and nothing leaks
out.

### 2.  A type system that isn't broken

Symta is dynamically typed — values carry their type, variables
don't.  That part is shared with Python, JS, and Lua.  What
Symta does differently is the *arithmetic of types*:

```symta
1 + 1            //  2     int + int   = int
1.0 + 1.0        //  2.0   flt + flt   = flt
1 + "1"          //  Err   int + text  = error, not "11" or 2
"abc" + "def"    //  abcdef text concatenation, explicit
```

JavaScript famously evaluates `1 + "1"` to `"11"` (string), then
`1 + 1 + "1"` to `"21"`, then `"1" + 1 + 1` to `"111"`.  Python
3 raises `TypeError`.  Symta raises a typed `bterror` you can
catch — and which carries the source location of the offending
expression.

There is no implicit `null`-to-anything conversion.  There is no
"truthy if you squint" rule; Symta's `if` takes a boolean, and
the empty list is *not* false the way it is in Lua, because
"empty" and "false" mean different things.  None of this requires
ceremony — you write the same arithmetic you always would, but
the runtime doesn't lie to you about types you didn't ask for.

### 3.  One operator does map, filter, reduce, replace, parse, and destructure

This is the headline feature.  The same `{}` syntax does what
five different libraries do in most languages:

```symta
List{X = X+1}         // map:    add 1 to every element
List{X = X if X>0}    // filter: keep positives (drop on `=` to nothing)
List{X => X+R}        // reduce: accumulate via R
List{X = Replacement} // replace: depends on the pattern X
"hello"{"l"=\L}       // replace: text in / text out  ("heLLo")
Form{"[Var]"=T.Var}   // template substitution
```

The body inside `{}` is a tiny pattern-action language.  You
learn it once; you use it everywhere from sorting numbers to
parsing a JSON document.  And — this is the secret of why Symta
stays concise without becoming line noise — the patterns
**compose**.  You can put a `{}` inside a `{}` inside a `{}`.
Each layer reads like a normal sentence.

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


// Tree walk: collect every int > 100 anywhere inside a nested structure:
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

If any of that looks like noise: don't worry.  Below is the same
tour at human speed.

---

# Part I — The Basics

## Installation and your first program

A pre-built `symta.exe` ships in the repository (Windows x64).
On macOS / Linux, build from source:

```sh
make -f Makefile.osx       # or Makefile.w64 on Windows, Makefile.linux
```

The compiler is self-hosted, so it's a small make.  When it's
done, you have one binary that is the compiler, the runtime, and
the REPL.

Open a REPL:

```sh
./symta.exe
```

Run a one-file program:

```sh
echo 'say "Hello, World!"' > hello.s
./symta.exe -f hello.s
```

Compile a project directory to a redistributable binary:

```sh
mkdir -p myapp/src
echo 'say "Hello, World!"' > myapp/src/go.s
./symta.exe myapp           # produces myapp/go.exe
./myapp/go.exe              # -> Hello, World!
```

A "project" is any folder with a `src/go.s`.  Symta walks the
imports, compiles everything, and links a single executable that
needs nothing else at runtime.

## Saying things

```symta
say "Hello, World!"
say 42
say 3.14
say [1 2 3]
say name!\Nancy age!37
```

`say` prints anything, with a newline.  `say_` (with an
underscore) prints without one.  Symta's print is *introspective*:
lists print as lists, tables print with their keys, errors print
with their source location.  No more `print(json.dumps(obj))`.

Inside a double-quoted string, `[...]` interpolates:

```symta
Name \World
say "Hello, [Name]!"             // -> Hello, World!
say "[Name].length = [Name.n]"   // -> World.length = 5
```

Anything inside the brackets is an expression; the whole string
is rebuilt at runtime.

## Numbers and arithmetic

Two numeric types you'll meet day one: `int` (62-bit signed
integer — yes, sixty-two; the runtime uses two bits for type
tagging) and `float` (boxed IEEE-754 double).

```symta
2 + 3              // 5
10 / 3             // 3        integer division
10 / 3.0           // 3.33...  float division (mixed promotes)
10 % 3             // 1        modulo
2 ^ 10             // 1024     power
abs(-5)            // 5
min(3 7 2 8 1)     // 1
max(3 7 2 8 1)     // 8
```

Arithmetic is normal infix.  Function calls don't need parens when
the meaning is unambiguous: `min 3 7 2` is the same as
`min(3 7 2)`, the same as `min(3, 7, 2)`.  Use whichever reads
best.

Comparisons return either `1` or `No`:

```symta
3 < 5              // 1
3 < 1              // No
3 >< 3             // 1     -- structural equality, on anything
"abc" >< "abc"     // 1
[1 2 3] >< [1 2 3] // 1
```

`No` is the universal "not present / not truthy" value — see the
sidebar on `No` in the chapter on Conditionals.  It's *not* the
same as the empty list or the integer zero, but it's truthy under
the same rules.

## Variables — names that bind

```symta
X 42                // X is bound to 42
Y "hello"           // Y is "hello"
X = X + 1           // reassign X to 43
```

Two forms:

- `X EXPR` — binds `X` to the value of `EXPR`.  First time only.
- `X = EXPR` — reassigns `X`.  `X` must already exist in scope.

The distinction matters because Symta will tell you, at compile
time, if you tried to assign to a name that didn't exist.  This
single rule kills the most common typo bug in dynamic languages:

```symta
Conut 0                       // -- typo: should be `Count`
till Done:
  Count = Count + 1           // compile error: Count not bound
```

In Python that would silently create a global `Count` and never
touch the typo'd `Conut`.  In Symta it fails before the program
runs.

Variables start with an uppercase letter or end with `_`.
Functions and methods start with lowercase.  Atom literals
(symbols) start with `\`: `\red`, `\monday`, `\AI`.  The
distinction is syntactic, not enforced semantics — but every
Symta programmer reads `Count` and immediately thinks "that's a
variable, it can be reassigned".

---

# Part II — Functions

## Definition and call

A function is a name `=` followed by a body.  Arguments come
between the name and the `=`:

```symta
double X = X * 2
say double 21                   // 42

add X Y = X + Y
say add 2 3                     // 5
say add(2, 3)                   // also 5
say 2.add(3)                    // also 5 -- method-call syntax
```

The last expression in the body is the return value.  No
`return` keyword needed for the normal case.

Bodies can be many lines.  Use indentation, or use `|` at the
front of a continuation:

```symta
hypot A B =
  Asq A * A
  Bsq B * B
  (Asq + Bsq) ^ 0.5

// Or:
hypot A B = (A*A + B*B) ^ 0.5
```

The compiler doesn't care which one you write.  Pick whichever
the next person to read this file will thank you for.

## Closures and higher-order functions

Functions are first-class values.  They capture the names they
see when they were defined — that's the *lexical scoping*
promise from up top:

```symta
make_counter Start =
  N Start
  // return an anonymous function that closes over N
  =N | N = N+1 | N

c1 make_counter 100
say c1.                         // 100
say c1.                         // 101
say c1.                         // 102
```

`make_counter 100` returns a closure.  Each call advances its
private `N`.  Two separate counters don't share state because
each got its own `N` at creation time.

The `=N | ...` syntax is a lambda — an anonymous function that
takes `N` (or no args if you omit the first segment).

## Currying and partial application

Underscores stand in for arguments not yet supplied:

```symta
add X Y = X + Y
inc add 1 _                     // a function: \X -> 1 + X
say inc 5                       // 6

[1 2 3]{add 10 ?}               // (11 12 13)  -- ? is the lambda arg
```

The bare `?` inside a `{}` body is shorthand for "the current
element".  Inside an explicit lambda you name your own
parameter; inside a `{}` you can use `?` and skip the naming
ceremony.

## Multiple return values

A function can return a list and the caller can destructure it
on the spot:

```symta
divmod A B = [A/B, A%B]

[Q R] divmod 17 5               // Q=3, R=2
say "Quotient [Q], remainder [R]"
```

Or, more idiomatically:

```symta
Q R divmod 17 5                 // same thing, no brackets needed
```

The compiler sees two names on the left and expects a 2-element
list on the right; it pulls them apart automatically.

---

# Part III — Lists

The single most important data type in Symta.

## Construction

```symta
[]                              // empty list
[1 2 3]                         // explicit literal
1,2,3                           // comma-separated, no brackets
Xs 5,6,7                        // bound to a variable
:5                              // (1 2 3 4 5) -- range from 1 to N
:0,5                            // (0 1 2 3 4 5) -- inclusive range from-to
:100                            // (1 2 3 ... 100)
```

Symta uses *parenthesised* notation when *printing* lists,
because that's what fits naturally with how the language reads
them.  But you don't write `(1 2 3)` to build one — that's a
function call.  Use `[1 2 3]` or `1,2,3`.

## Indexing

```symta
Xs 10,20,30,40,50

Xs.0                             // 10  -- first
Xs.1                             // 20
Xs.~                             // 50  -- last
Xs.~~                            // 40  -- second to last
Xs.n                             // 5   -- length
```

`~` ("twiddle") means "from the end".  This is one of the few
syntactic conventions you have to learn — Symta uses it everywhere
positions are involved.  Once you've seen `Xs.~`, `Xs.~~`,
`Xs.~~~` is obvious.

## Slicing

```symta
Xs 10,20,30,40,50

Xs.(:3)                          // (10 20 30)  -- first 3
Xs.(2:4)                         // (30 40)     -- positions 2..4
Xs.(~~:~)                        // (40 50)     -- last 2
```

## Iteration: the `{}` operator

```symta
// Map: apply a body to every element.
[1 2 3]{? + 1}                  // (2 3 4)

// Filter: drop elements where the body returns nothing.
[1 -2 3 -4 5]{? if ?>0}         // (1 3 5)

// Map + filter, together:
[1 2 3 4 5]{? * 2 if ?%2><0}    // (4 8)  -- doubles of evens

// Both element and index:
[10 20 30]{[I X] = "[I]:[X]"}   // ("0:10" "1:20" "2:30")
```

The pattern on the left of the body's `=` matches the input; the
expression on the right is what comes out.  Drop the `= EXPR` to
get plain filtering; drop the pattern to use `?`.

## Reduction

```symta
// Sum, written as a fold:
[1 2 3 4 5]{? => ? + R}         // 15

// Same thing, idiomatically:
[1 2 3 4 5].sum                 // 15

// Maximum element by some key:
People{? => if ?.age > R.age then ? else R}
```

`=>` is "fold step": `?` is the current element, `R` is the
running accumulator.  The initial `R` defaults to the first
element; you can give it explicitly with `R!START`.

## Replacement and parsing — the magic trick

```symta
// Replace every "a" with "A" in a string:
"banana"{"a"=\A}                 // "bAnAnA"

// Replace every name in a list:
[\alice \bob \carol]{\bob=\BOB}  // (alice BOB carol)

// And in reverse -- pull the values OUT of a string:
"name=Nancy, age=37"("name=[N], age=[A]" =: N A)
//  -> binds N="Nancy", A="37" (or 37 if you typed `[A<int]`)
```

The `=` inside `{}` is *bidirectional*.  Forwards it substitutes;
backwards it extracts.  This is what makes the FizzBuzz
one-liner readable: the body of `{}` is a tiny pattern language
that you happen to be using to define a numeric rule.

---

# Part IV — Control Flow

## Conditionals

```symta
if X > 0: say "positive"
if X > 0 then say "positive" else say "not positive"

// Multi-branch:
case X
  0     | say "zero"
  N<1.is_int | say "got int [N]"
  Else  | say "other: [X]"

// Just want to run a block when something is true:
when Has_files:
  say "[Files.n] files found"
  process Files

// Inverse:
less X.is_int: bad "Expected an integer, got [X]"
```

`case` matches by pattern, top to bottom.  `Else` catches
anything left.  `N<int?` reads "name this match `N`, and only
match if it's an integer."

### What is `No`?

`No` is the universal "no value" — what you'd call `null`,
`nil`, `None`, or `nullptr` in another language.  It compares
unequal to everything except itself, it's falsy under `if`, and
it's the value functions return when there's nothing to say.
But unlike Java's `null`:

- Dereferencing `No.foo` returns `No`, not a crash.  Optional
  navigation is the default.
- Comparing `No` to numbers behaves consistently
  (`No < 5` is `1` because "no value" is less than any value).
- The compiler tracks `No` through type inference.

You'll write `if got X: ...` more than `if X <> No: ...`,
because `got` is the idiomatic "does this value exist".

## Loops

```symta
// Range loop:
for I in :10:
  say "i = [I]"

// Range with explicit limits:
for I in 1:100:
  say I

// Loop over a list:
for Name in [\alice \bob \carol]:
  say "Hello, [Name]"

// Loop forever until a condition is set:
Done 0
till Done:
  // ... body that eventually sets Done = 1 ...

// While-style:
while X.has_more:
  X = X.advance

// Just iterate, no index:
xs^each{ ... }
```

Indentation is significant; the body of the loop is everything
indented further than the `for`.  Tabs and spaces both work, as
long as you're consistent within a function.

---

# Part V — Text

Strings are sequences of characters, immutable, UTF-8 internally.

```symta
S "hello world"

S.n                              // 11    length
S.0                              // h     first char
S.~                              // d     last char
S.upper                          // HELLO WORLD
S.title                          // Hello World

S.split(" ")                     // ("hello" "world")
S{"l"=\L}                        // "heLLo worLd"

S("[A] [B]" =: A B)              // A="hello", B="world"
```

Interpolation works inside double quotes:

```symta
N 5
say "I have [N] apples"          // "I have 5 apples"
say "next: [N+1]"                // "next: 6"
say "list: [1,2,3]"              // "list: (1 2 3)"
```

Use single quotes for the rare case you want a literal string
with no interpolation: `'no $1 here'` — though you'll also see
backslash-escapes in double quotes: `"no \[bracket\] here"`.

---

# Part VI — Tables (Hash Maps)

Tables map keys to values.  Keys can be any value.

```symta
// Literal:
T name!\Nancy age!37 city!\Amsterdam

// Access:
T.name                           // \Nancy
T.age                            // 37
T.\city                          // \Amsterdam  -- atom-keyed access

// Update (returns a new table; tables are persistent):
T.age = 38

// Bulk-build from rows:
People @t: rowz:
  name age city
  \alice  30  \utrecht
  \bob    25  \rotterdam

People.alice.age                 // 30
```

Tables print readably: `say T` gives you back the literal you
typed.

---

# Part VII — Pattern Matching

The single most expressive form in Symta.  Pattern matching is
how you destructure data, how `case` decides which branch to
take, and how `{}` knows what to map over.

```symta
// Bind first and rest:
H,@T 1,2,3,4,5
// H=1, T=(2 3 4 5)

// Bind first two and rest:
[A B @Rest] 1,2,3,4,5
// A=1, B=2, Rest=(3 4 5)

// First two and last:
[A B @_ Z] 1,2,3,4,5
// A=1, B=2, Z=5  (underscore = "I don't care")

// Match a specific value in position:
[\start @Args] L                 // succeeds only if L starts with \start

// Constraint:
[N<1.is_int @_] L                // first element must be int; bind it as N
```

Patterns chain.  Inside a `case` or a function head:

```symta
length L = case L
  []     | 0
  H,@T   | 1 + length T

// Or, more compactly:
length: []=0; H,@T = 1 + length T
```

That last line is one expression containing two pattern
clauses.  Symta picks the first that matches the input.

### The guard syntax

`<` after a name in a pattern adds a constraint:

```symta
classify N = case N
  X<0      | "negative"
  0        | "zero"
  X<int?   | "positive int"
  X<float? | "positive float"
  _        | "other"
```

`<` reads as "such that".  `X<int?` is "bind X, such that X is
an integer."

---

# Part VIII — Objects and ECS

Symta's object system is *not* class-hierarchy OOP.  It's an
Entity-Component-System, which is a fancy name for "everything
is a row in a small relational database".  The headline syntax,
though, looks like classes:

```symta
cls point x y

P new x,3 y,4
say P.x                          // 3
say P.y                          // 4
P.x = 10                         // P is now {x=10, y=4}
```

Behind the scenes, "make a point" attaches an `x` component and
a `y` component to a fresh entity id.  Querying `P.x` is a hash
lookup keyed on that id.

The benefit: an entity can pick up extra components at runtime
without being declared as a subclass of anything.

```symta
P.color = \red                   // P now also has a color
P.color                          // \red
P.weight                         // No -- entity doesn't have a weight
```

And you can iterate by component:

```symta
// Every entity that has a `color`:
color^each{?, ?.color}
```

This is the design Symta is built around.  The full essay on
why is in [`cls.md`](cls.md) — it's worth reading once you have
something with more than a hundred entities of more than three
types.

---

# Part IX — Macros and Quasiquotation

Symta is a Lisp.  Every program is data, and every data shape
that names something can be transformed by a macro.

```symta
// A macro that swaps two variables:
swap! A B =
  let T A in A = B; B = T

// Use it:
X 1; Y 2
swap! X Y
say [X Y]                        // (2 1)
```

The `!` suffix marks `swap!` as a macro at the call site.  Inside
the macro body, ordinary Symta runs at compile time; the macro
returns code which is then compiled in place.

For non-trivial macros, you assemble code with the quasiquote
syntax — backtick to suppress evaluation, `$` to splice values
back in:

```symta
// A macro that times the execution of an expression:
time_it Expr =
  `let Start = clock() in
     R = $Expr
     say "took [clock()-Start] ms"
     R`
```

The killer use of macros in practice is *embedded DSLs* — a few
lines of macro define a syntax for SQL queries, graphics
primitives, parser combinators, or state machines, that looks
like part of the language.

If you have ever bounced off Lisp because "code is data" felt
abstract: Symta's macros are deliberately friendlier than
Common Lisp's.  Indentation is preserved; error messages are
specific; the macro and the expanded code both have source
positions, so a stack trace lands on a real line in your
program.

---

# Part X — Modules and Building

Every `.s` file is a module.  Imports look like:

```symta
use math                         // imports `math` from runtime
use my_helpers                   // imports a sibling .s file

say sin(1.0)
say my_helpers_function(42)
```

The `use` is resolved against, in order:

1.  Sibling files in the same `src/` directory
2.  Files under `pkg/<name>/src/`
3.  Built-in modules shipping with the runtime

Names you intend to expose from your module are declared in the
top-line `export`:

```symta
export hello_world greet
hello_world = "Hello, World!"
greet Name = "Hi, [Name]"
private_helper = 42              // visible only inside this module
```

Build a project:

```sh
symta myproj                     # produces myproj/go.exe
```

A project is a directory with a `src/go.s`.  Symta walks the
imports and emits one statically-linked executable.

---

# Part XI — Error Handling

`bad` raises a runtime error with a message:

```symta
divide A B =
  less B: bad "division by zero"
  A / B
```

Catch with `btrap`:

```symta
Result btrap: divide(10, 0)
if Result.is_bterror:
  say "got error: [Result.text]"
else
  say Result
```

A `bterror` is just a value — you don't unwind the stack
mid-expression unless you ask to with `btjump`.  That makes
error-aware code something you can think about locally:

```symta
[10 5 0 2]{divide 10 ?}.btrap
```

Each `divide` either returns a value or a `bterror`.  The list
that comes out is `(1 2 BTERR 5)` with the error sitting in its
own slot; no exception breaks the iteration.

---

# Part XII — Foreign Function Interface

Symta calls into native code without an external build step.
Declare the C signature in the source:

```symta
use ffi

ffi_begin libc
  ffi int   abs    int
  ffi int   atoi   text
  ffi text  getenv text
ffi_end

say abs(-5)                      // 5
say atoi("42")                   // 42
say getenv("HOME")               // /home/...
```

The runtime resolves the symbol at module load, generates a
trampoline, and you call it like an ordinary Symta function.

For richer cases — SDL2 windows, audio, FreeType text rendering
— Symta ships pre-built `.ffi` plugin blobs.  `use uim` brings
in the UI toolkit; `use gfx` brings in the graphics layer.

---

# Part XIII — Getting help

Symta has a self-documenting REPL.  At the prompt:

```
> help
help                — print this message
help <symbol>       — show documentation for <symbol>
exit / quit         — leave the REPL

> help sum
sum
  Sum a list of numbers.

  [1 2 3 4 5].sum  // -> 15

  Empty list returns 0.  Mixed int/float promotes to float.

> help case
case
  Multi-branch pattern matching.

  case Expr
    PatA | branch_a
    PatB | branch_b
    Else | default

  ...
```

Documentation is attached to source declarations with
`help_set`:

```symta
help_set \sum 'Sum a list of numbers.
Empty list returns 0; mixed int/float promotes to float.
Example:  [1 2 3].sum  // -> 6'

sum L = L{? => ?+R}
```

At REPL time, `help \sum` reads it back.  The documentation is
*in* the source code where it belongs, so it never goes stale,
and the REPL is the way to read it.

```symta
help                            // print general usage
help \sum                       // show docs for `sum`
help_names()                    // list every documented symbol
```

Two refinements are on the milestone roadmap:

1.  **A `doc` macro** so that the same docstring can be written
    immediately *before* the definition without an explicit
    `\sym` repetition:
    ```symta
    doc 'Sum a list of numbers.'
    sum L = L{? => ?+R}
    ```
    The macro picks up the next top-level definition's name and
    routes the string to `help_set` for you.

2.  **A dedicated SBC section** for docstrings, sitting beside
    the existing line-number and (future) type-introspection
    side-tables.  Once that lands, docs are *not* loaded into
    memory until they're asked for — only the offset table is.
    External tooling can read docs without instantiating the
    runtime.

Both are non-breaking — the `help` / `help_set` / `help_names`
API stays the same.

The stdlib reference that previous versions of this document
tried to inline is gone deliberately.  Use `help`.

---

# Where to go next

You now have enough Symta to be dangerous.  The most useful
follow-ups, in order:

1.  **`examples/`** — Twenty-six progressively richer programs.
    Run a single-file example with `symta -f examples/NN-*.s`;
    build a project example with `symta examples/NN-name &&
    examples/NN-name/go.exe`.

2.  **`AI.md`** — A tight one-page cheat sheet meant to be
    pasted into an LLM's context window so it can help you write
    Symta.  Doubles as a quick-reference for humans.

3.  **`dev/cls.md`** — The design essay for the ECS, the `cls` /
    `dsm` / IPS macros, and the GC integration.  Worth reading
    when your program grows past a few hundred entities.

4.  **`architecture.md`** — How the compiler, runtime, and FFI
    actually work.  Read this when you want to contribute or
    when you want to embed Symta in something larger.

5.  **The `examples/27-relational.s` Prolog-style inference
    engine** — about 100 lines of Symta that does what a small
    chunk of a logic programming language does.  A good test
    of whether you've internalised the patterns from this
    document.

If any chapter felt unclear, file an issue at
`https://github.com/NancySadkov/som` — concrete suggestions
("Chapter VII would be clearer if it covered X first") land
faster than abstract ones.

Happy hacking.

---

*Symta is dual-licensed under MIT OR Apache-2.0.*

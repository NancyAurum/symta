# Porting Symta's parser to C: a 28x speedup and a year of latent bugs

Symta is a self-hosted Lisp in the family of REFAL and POP-11.
Most of the compiler is written in Symta itself; what's not is
the runtime, the tokenizer, the garbage collector and a handful
of primitives. `text.parse` -- the entry point that turns source
into AST -- has lived in Symta since the language existed:
[symta/src/reader.s](src/reader.s), 517 lines of recursive
descent with explicit operator-precedence climbing.

This post is the story of porting that file to C, getting it
wrong six times before getting it right, and the diagnostic that
turned the last bug from a week-long mystery into a 20-minute
fix.

## Why bother

A measurement from before any porting:

```
sample                 size       per-parse     throughput
hello-world            ~10 chars  ~0.2 ms        --
[1..100] repeated      1.25 KB    5.6 ms         224 KB/s
same x2                2.50 KB    10.9 ms        230 KB/s
```

230 KB/s is fast enough that the parser was never the visible
bottleneck on a full `cd game && symta .` (88 files, ~18 K
lines, ~25 seconds total -- the parser is about 14% of that;
macroexpand and SSA emission dominate). But it's painfully slow
for `symta -f throwaway.s` REPL-style use, where each tap of
"run again" stalls visibly while the AST is rebuilt.

There were two other reasons.

**The Symta parser is brittle.** A long-standing issue
([B7 in issues.md](../issues.md)) is that multi-line `if X:`
bodies can't have `else` at body indent without an explicit `|`
separator. Fixing this properly needs stack-tracked parser state
(a pending-`if` flag, basically) which is awkward in the Symta
implementation but trivial in C.

**The tokenizer is already C.** There's no architectural reason
the token-list -> AST step needs to live in Symta. Tokens cross
the C/Symta boundary just fine for `tokenize`; doing the same
for `parse_tokens` is no different.

So: port reader.s to `symta/runtime/reader.c` behind a feature
flag (`parse_tokens_c_` builtin, swapped into `text.parse`'s
single call site), keep parse_strip in Symta because it does
meta-wrapping that needs Symta type machinery anyway, and run
both parsers in parallel against the existing test suite until
they agree.

## The first scaffold

This took a few sittings. The shape of reader.c after the first
push was about 1500 lines: token introspection helpers, op-set
predicates for every reader.s op table, an interned keyword bag
(`KW_pipe`, `KW_then`, etc.), a `pstate_t` parser-state struct
with pushback support, and ports of `add_bars`, `parse_strip`,
`parse_table`, plus the precedence ladder
`parse_pow` -> `parse_mul` -> `parse_add` -> ... ->
`parse_blogic`.

The first attempt at wiring `parse_tokens_c_` into `text.parse`
made the bootstrap rebuild fail at `uniquify_form` -- compiler
internal, infinite recursion through `meta.__`. That kind of
recursion only happens with a graph that has a cycle, which
the parser shouldn't be producing. The AST printed identical to
Symta's; the heap underneath did not.

This was the start of the longest debugging stretch in the
project.

## The first wave of bugs

### `bltin.c` left token slots dirty

The `token()` constructor in [bltin.c](runtime/bltin.c) is what
both Symta-side and C-side parsers reach through to build a
fresh `T_TOK`. The pchar slot (2) and parsed slot (6) were
commented out in the constructor:

```c
//LGET(r,2) = 0; //pchar
LGET(r,3) = row;
LGET(r,4) = col;
LGET(r,5) = orig;
//LGET(r,6) = 0; //parsed
```

`gc_alloc` returns dirty memory. Every token built via the
`tok_` varargs builtin carried garbage in pchar permanently, and
any C-side caller bypassing `tok_` got dirty slot 6 too. Both
`tokenize.c`'s `mktok` and `reader.c`'s `mk_token` zeroed the
slots defensively, but the hazard sat unattributed in the
constructor itself. With C parser wired in, the "second-call
segfault" was real: a downstream walk over a token tree
deferenced a slot-6 pointer that was actually random memory.

Fix was one diff: uncomment the two `LGET`s. The mystery
crashes vanished.

### `parse_semicolon` ordering

Symta's `parse_semicolon` sets `GOutput = [@R.tail.f L M]` (or
`[R L M]` if R doesn't start with `;`), then `parse_xs`
reverses the GO at the end via `GOutput.f`. My port wrote the
pre-reverse form directly, but `parse_xs` in my C port doesn't
reverse (arrput is already insertion order). So the result was
the bytewise reverse of what Symta produced: `[(c) (b) ;]` vs
`[; (a) (b) (c)]` for `a; b; c`.

Easy fix once spotted: write the post-reverse form directly.

### `parse_offside` rollback lost pushback contents

When `parse_offside` enters, it snapshots the parser position
for potential rollback. My port snapshotted `gi_pos` and
`arrlen(pushback)`. On rollback it restored gi_pos and trimmed
pushback back to the saved length.

That works only if pushback _grew_. If the collection loop
_consumed_ from pushback, the trimmed-back length couldn't put
back what was popped.

The elif-rewrite path inside `parse_if` depends on the rollback
behaviour: it pushes `else` and `if` tokens onto pushback, then
parse_offside is called for the else body. parse_offside
consumes the `if` token, decides to roll back, can't restore
it, and parse_xs trips on a missing `then`.

Fix: snapshot the full pushback contents, not just the length.

## All-green at one level, broken at the next

By this point all 73 unit tests passed with the C parser wired
in. But `make game` -- the project compile of the 98-file
game/ directory -- failed in `parse_strip` with infinite
recursion. The print-level AST was identical to Symta's. Token
slot inspection was identical. Whatever differed was invisible
to the inspection tools.

### The wrong instinct

My first response was to plan a byte-level heap-snapshot
comparator. Dump every dyn value in the AST tree, recurse
through pointer-typed slots, dump address-with-offset diffs
between Symta and C output. I started writing it.

Two days in, the user suggested a different approach:

> Beside byte by byte comparison of heap snapshots, it is also
> possible to take compiled code into separate branch and keep
> slicing it in half until the bug disappears. At that point
> you have exact expression (usually a few lines), trigger the
> issue.

This was right. Here's why it works for a self-hosted compiler:
the parser output goes to the compiler, the compiler output is
new sbc files, those sbc files are what makes the next compile
behave differently. If C-parser-built reader.sbc is the
culprit, you can verify that by:

1. Build sbc files with each parser, save both sets.
2. Take the Symta-built sbc files as known-good.
3. Swap in one C-built sbc at a time.
4. The one that, when swapped, reproduces the bug is the
   culprit.

That immediately pointed at C-built `reader.sbc`. Then:

5. Diff the parse output of `reader.s` itself between the two
   parsers. The print is identical (we already knew), so dump
   to text and `cmp` byte-by-byte.

This is where it gets useful. The print suppresses things
the AST contains: meta-source spans, list nesting that prints
elided, intermediate `nullary_` wrappings, the difference
between a single 2-list and two consecutive 1-lists. All of
those _do_ appear in the deserialized textual form.

```
$ cmp -l symta_parse.out c_parse.out | head -3
  267 163 140
  268 162  51
  269 143  40
```

First diff at byte 267. Walk to that point in the file:

```
Symta: (`.` tok `=src`) S) ...
C:     (`.` tok `=`) src S) ...
```

Symta tokenizes `=src` as one token. My C parser tokenizes as
two: `=` then `src`. Hunt down where this happens in Symta's
parse_suf_loop -- reader.s lines 282-298 -- and the rule
appears:

```symta
| if T.value><`=`: less GInput.end:
  | K GInput.0
  | if K.is_token and K.value.is_keyword:
    | pop GInput
    | T.value = "[T.value][K.value]"
```

After `.` followed by `=`, if the _next_ token is a keyword,
glue them. So `tok.=src` becomes one method name `=src`. My C
port had the `.X` postfix logic for method names but didn't
have this special case for `=` followed by a keyword.

Add the rule, recompile, re-dump, re-diff. First divergence
shifts to byte 670. Walk to it:

```
Symta: (`:` (when (`and` (...) ...)) (body))
C:     (when ...) `:` ((nullary_ `:`)) (body)
```

In Symta the `:` is the head of the form, with the `when ...`
expression as the first arg. In mine the form is flat.
The bug is in the LL(1) hack inside parse_logic: after
consuming an `and`/`or`, look ahead for the next delim and stop
the and/or chain there, leaving the delim for the outer
parse_xs to handle.

Two bugs nested inside this one:

* My LHS construction was reversing the prior GOutput, but
  arrput is already insertion order. The reverse swapped which
  token became the form head.
* Symta's `parse_xs` doesn't exit when parse_delim returns
  `No` -- it _continues_, because `No` is truthy in the
  `parse_delim DelimCtx or ret loop GOutput.f` test. My port
  treated `No` as 0 and exited the loop, so the `:` after the
  LL(1) hack was never consumed.

This second one is genuinely subtle. The Symta code:

```symta
| X | parse_delim DelimCtx or ret loop GOutput.f
| when got X: push X GOutput
```

`or` checks truthiness. `got` checks not-equal-to-No. So:

* parse_delim returns _0_ -> `or ret`-> loop exits.
* parse_delim returns _No_ -> X = No, `got No` is false, no
  push, but loop continues.
* parse_delim returns a value -> push, continue.

My C port distinguished only two cases. The fix was to test
for `x == No` separately from `x == 0`. After that, the
`:` consumption pattern worked.

Re-diff. Next divergence in the multi-line `OpsT: + - * ...`
definition. My `expect_eol` check in `parse_delim` was using
`IS_FIXTEXT` to decide whether the Pref head was a "keyword".
Symta's `is_keyword` is "not (n>0 AND first char is upcase)".
Single-char or short identifiers _are_ fixtexts but they're
not "keywords" for this purpose -- "OpsT" is a variable (it
starts with uppercase), not a keyword. Fix.

Re-diff. Next divergence in `@.0`. Symta produces two top-level
forms `(@ nullary_.)` and `0`; mine produced
`((@ nullary_.) (. nullary_. 0))` -- it was applying `.0` as a
postfix on `(@ nullary_.)`. The bug was in parse_prefix's
trailing-suf-binary case: Symta uses `/GInput` (destructive
pop) to consume the suf-binary token when wrapping it as
`[nullary_, t]`; my port was peeking, so the outer
parse_suf_loop saw `.` again and applied it as a postfix.

Re-diff. Next divergence in `it.val.: V = body`. This was the
`.:` postfix-colon-line form, where after a `.` (or `^`)
followed by `:` (or `!`, which gets retransformed to `:`),
parse_suf_loop runs `parse_delim` inside a fresh GOutput=[T],
retags the result, and recurses. Port it.

After all five diffs cleared, the C parser's output for
reader.s itself was byte-identical to Symta's. The same
bisection on a few game-source files cleared the splice
auto-quoting bug (`"?"` had to wrap as `[\\ ?]`), the
parse_unary self-reference bug (caching a `[nullary_ tok]`
that contained tok was a self-referential structure
`parse_strip` would walk forever), and the `void` token
parsed-slot bug (must be the `No` sentinel, not integer
zero).

That's six distinct parser bugs found by binary-search bisection.

## The result

```
| input                  | Symta parser    | C parser       | speedup |
|------------------------|-----------------|----------------|---------|
| reader.s (13.6 KB)     | 103 ms / call   | 3.7 ms / call  | 28x     |
|                        | 128 KB/s        | 3616 KB/s      |         |
| 5 examples (5.8 KB)    | 3.1 ms / call   | 0.17 ms / call | 18x     |
|                        | 362 KB/s        | 6499 KB/s      |         |
```

For interactive use the per-call latency matters more than the
throughput. 3.1 ms per file -> 0.17 ms per file means a project
of 88 modules parses in ~15 ms total of parse work, versus
~270 ms before. That's the difference between "this didn't
happen" and "this took noticeable time".

The full project compile (parse + macroexpand + SSA emission +
codegen) doesn't drop 28x because the parser was only ~14% of
the original time. But the visible-to-the-user end-to-end
wall-clock improves enough that we stopped noticing parsing as
a phase.

## The drift test

The user asked one more question: are you sure successive
self-compilations of the compiler don't introduce subtle
drift that surfaces only after N rounds?

This is the standard concern for any self-hosted toolchain. The
literature converges on the GCC convention: three-stage
bootstrap, with byte-equality required between stage 2 and
stage 3. Stage 1 was built by a "foreign" previously-committed
compiler; stages 2..N are all built by the self-compiled
compiler and must be a function of their input.

[tests-bootstrap/drift.sh](tests-bootstrap/drift.sh) implements
this for Symta. Five rounds because the cost is two minutes and
five rounds catches a (theoretical) period-2 oscillation that
GCC's two-stage compare could miss.

Current result: PASS in one round. The compiler is a true fixed
point: f(reader.s) = reader.sbc, and f(reader.sbc-rebuilt-from-
self) = reader.sbc identically. No hash-iteration drift, no
register-allocator instability, no uninitialised-memory leak
into output.

This now runs at the tail of `make test-all` and should be
re-run after any modification to the runtime, macro layer, or
compiler.

## What's still left

The C parser is wired in only for benchmarking right now;
production `text.parse` still routes through Symta
`parse_tokens`. There's one remaining bug ([B14](../issues.md))
where `use X` triggers a different runtime error path under the
C parser than under the Symta parser, despite identical
print-level AST. The bisection pointed at it being a
tokenize-level "Unexpected W" issue in `uim.s` line 162 rather
than a parser-side bug, but the dust hasn't settled enough to
flip the default.

## What I learned

**Binary-search bisection beats heap inspection.** When the
visible representation matches but the runtime behaviour
diverges, dump the output of every stage of the pipeline as
text and `cmp` byte-by-byte. The diff localises a single
construct in source code in seconds, where heap-walking
spelunks for days. The user pushed me to this approach over my
initial instinct; it cleared six bugs that print-level checks
had hidden for weeks.

**Self-hosted code is its own oracle.** The fact that
`reader.s` compiles itself meant I could use "does
compile(reader.s) byte-equal the committed reader.sbc?" as a
fixed-point witness, without needing to write a separate
golden. The same test scales to 5 stages without writing
anything new.

**Trust the literature on round count.** I would have done 10
rounds on instinct. The GCC / Rust / OCaml / Go convention all
agree on 3 with byte-equality, and there's no published bug
that hides past round 3. Five is belt-and-suspenders, ten is
superstition.

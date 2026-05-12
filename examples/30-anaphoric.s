// 30-anaphoric.s -- anaphoric macros and Symta's `/L` pop family
//
// Symta's `if`, `while`, `till` accept an explicit variable
// (introduced via `Var <- pattern` or just `Var /Source`) and
// behave anaphorically -- the body sees the bound variable. The
// `On Lisp` aif/awhile become plain `if X ...:` / `while X ...:`
// with an explicit name, which the docs argue is friendlier than
// implicit `it` because multiple binds and pattern filters are
// possible without inventing more anaphors.
//
// See sbe.txt's "Pattern matching inside of `if` and `while` with
// the use of /L" chapter.
//
// Skipped (broken upstream):
//   `if X<-(?>0) /L`  bad match case on `>` pattern with `<-`
//   `'...'{d?=/l~^[a b c]}`  undefined I__N at compile
//   `while X *l~^[...]+`     same
//
// Run:  symta -f examples/30-anaphoric.s


// ----------------------------------------------------------------
// 1. `/L` is `pop L`, returns the head, mutates L to the tail.
// ----------------------------------------------------------------
L: a b c d
say "pop /L  -> [/L]"               // a
say "pop /L  -> [/L]"               // b
say "L now   -> [L]"                // (c d)


// ----------------------------------------------------------------
// 2. Anaphoric `while X /L`:  pop a value, bind it to X, run body,
//    until L is empty.
// ----------------------------------------------------------------
say "anaphoric while on a literal list:"
while X /[one two three four]: say "  [X]"


// ----------------------------------------------------------------
// 3. `while X<-pat /L`: break when the popped value matches `pat`.
//    Here `<-three` means "stop if X equals the keyword three".
// ----------------------------------------------------------------
say "anaphoric while with break-on-match:"
while X<-three /[one two three four]: say "  [X]"
//   prints one, two -- breaks on three because <-three matched.


// ----------------------------------------------------------------
// 4. `/[a b c]` inside a quick-lambda `{}` is the auto-closure pop.
//    For every `d?` (digit) we replace it with the next value off
//    the list.
// ----------------------------------------------------------------
say "auto-closure pop inside {}:"
Replaced '1-a, 2-b and 3-c'{d?=/[one two three]}
say "  digits-to-words: [Replaced]"


// ----------------------------------------------------------------
// 5. Anaphoric `if`: peel one value, run body if list was non-empty.
// ----------------------------------------------------------------
say "anaphoric if:"
LL: hello world
if X /LL: say "  X=[X]  LL after pop=[LL]"


// ----------------------------------------------------------------
// 6. `while X *L+`: C-style increment iteration. The `*L+` post-
//    increments L the way Common Lisp `pop` would, in a context
//    where the test value is L's head.
// ----------------------------------------------------------------
say "C-style while with post-increment:"
Xs: alpha beta gamma delta
while X *Xs+: say "  [X]"


// ----------------------------------------------------------------
// 7. Pre/post increment on a NON-lvalue: introduces an anonymous
//    auto-closure counter. `0+` starts at 0 then bumps; `5+`
//    likewise. Together they walk 5..14.
// ----------------------------------------------------------------
say "non-lvalue ++ counters:"
while 0+ < 10: say "  [5+]"


// ----------------------------------------------------------------
// 8. `@@it` anaphoric binding -- the closest Symta gets to On Lisp's
//    implicit `it`. The macro `got @@it expr` evaluates `expr`,
//    binds the result to `it`, returns 1 unless the result is No.
// ----------------------------------------------------------------
say "@@it explicit anaphor:"
get_words = [one two three]
when got @@it get_words: say "  got: [it]"

// In a while: peel from Xs until No appears.
say "@@it in a while:"
Xs2: 0 1 2 3 No 4 5 6
while not Xs2.end and got @@it pop Xs2: say "  [it]"

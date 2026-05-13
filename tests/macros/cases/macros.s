// macros.s -- exercises the macro-authoring primitives.
//
// `mac:` declares a local macro inline.
// `prx`  unrolls a body N times at compile time.
// `form` is the quasi-quote helper for AST construction.
// `mexlet` rebinds a name inside the macro body during expansion.
//
// Run with:  symta -f tests-macros/cases/macros.s


// ----------------------------------------------------------------
// 1. `mac:` -- declare a macro inline, no separate module.
//
// The macro expander runs at COMPILE time. `incr` is replaced by
// `Y + 1` everywhere it's invoked below.
// ----------------------------------------------------------------
mac: incr Y = form Y + 1

say "mac: incr:"
say: incr 41
say: incr 99


// ----------------------------------------------------------------
// 2. `prx` -- compile-time loop unrolling.
//    `prx EXPR | A; B; C` produces `EXPR_with_A; EXPR_with_B; ...`
//    `_0` in EXPR is replaced with the current branch's value.
//    `__` is a counter, incremented per branch.
// ----------------------------------------------------------------
say "prx loop unroll:"
prx say hello _0 | \World; \Symta; \Reader


// ----------------------------------------------------------------
// 3. `prx` with explicit counter and a numeric expansion.
// ----------------------------------------------------------------
say "prx with __:"
prx say "iter [__]:" loop_ _0 | a; b; c


// ----------------------------------------------------------------
// 4. `form` -- quasi-quote. Returns an AST that the surrounding
//    macro can splice in.
// ----------------------------------------------------------------
mac: ratio A B = form (A * 1.0) / B

say "form / division macro:"
say: ratio 7 2


// ----------------------------------------------------------------
// 5. `~Var` inside `form` -- auto-gensym, unique per expansion so
//    the macro doesn't accidentally shadow a caller's variable.
// ----------------------------------------------------------------
mac: square_ax X = form:
  ~T X
  ~T * ~T

T 7  // outer T -- must NOT collide with the macro's ~T
say "auto-gensym ~T inside form (outer T=[T]):"
say: square_ax 5
say "T after macro call: [T]"


// ----------------------------------------------------------------
// 6. Multi-statement macro body via `form` block.
// ----------------------------------------------------------------
mac: with_log Body = form:
  say "entering"
  ~R Body
  say "leaving"
  ~R

say "multi-stmt form body:"
say: with_log 41 + 1


// ----------------------------------------------------------------
// 7. Macro that builds its body programmatically with `form` +
//    list-comprehension. Compile-time loop unroll.
//
// We need an Args list at macro-time and `$@Args` splices it into
// the form. `form: \`|\` $@Bodies` wraps the bodies in a `|` block.
// ----------------------------------------------------------------
mac: repeat N Body =
  Copies map I [:N] Body
  form: `|` $@Copies

say "compile-time-unrolled body via splice:"
repeat 3:
  say "hi"

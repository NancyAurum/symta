// mymac.s -- a small library of macros consumed by go.s

export 'pi' 'unless' 'swap' 'repeat'

// A trivial computed-at-compile-time helper. `pi K` becomes a
// numeric constant the moment `mymac` is compiled.
pi K = K * 3.14159265

// `unless C: Body`  ==  `if C then No else Body`
unless @Cond Body = form: if Cond then No else Body

// `swap A B` -- exchange two l-values. We can't do this as a
// function because we need the *names*, not the values.
// `~T` declares a fresh symbol unique to each expansion (gensym).
swap A B = form:
  ~T A
  A = B
  B = ~T

// `repeat N Body` -- compile-time-unrolled N copies of Body.
// At macro time we hold N as an integer and Body as raw AST;
// build the list of N copies and wrap them in a `|`-block so the
// macro returns a single expression node.
repeat N Body =
  Copies map I [:N] Body
  form: `|` $@Copies

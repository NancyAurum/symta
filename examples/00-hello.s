// 00-hello.s -- printing text
//
// `say` is the basic output function. It prints its argument
// followed by a newline. Comments start with `//`.
//
// Run:  symta -f examples/00-hello.s

say "Hello, World!"

// Lowercase bare words don't need quotes; they are symbols.
say hello

// String interpolation: `[Expr]` inside a "..." string evaluates
// the expression and inserts its textual representation.
Name "Symta"
say "Hello from [Name]!"

// `\Word` quotes a single word starting with uppercase.
say \World

// `: ` (colon-space) is sugar for `(...)` enclosing the rest of the line.
//   say: "result is " "ok"   ==   say ("result is " "ok")
say: "Goodbye, World!"

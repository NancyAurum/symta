// 29-subscript.s -- Xs[...] subscript operations
//
// The `[...]` postfix is heavily overloaded. sbe.txt's "Advanced
// List Operations" chapter enumerates every shape. This file
// exercises each at RUNTIME (printing the actual result) so the
// regression net catches semantics changes, not just parse-shape
// changes. The parser side is covered by tests-reader/11-subscript.s.
//
// Each label is its own `say` line; the result is bound to a
// temp first to avoid double-quoted-string-splice eating the
// `~`/`!`/`[` tokens that appear inside many of these forms.
//
// Skipped (broken upstream, filed in issues.md):
//   "(...)"[0!'{' ~!'}']    fixtext/bigtext is not `=`-mutable
//   Xs[2!@Ys]               B10 -- `@list` inside `[...]` hits
//                                   `unknown symbol _insert`
//   Xs[-I]                  documented but not implemented
//   Xs[!N:M]                `!` at start with a slice triggers
//                           "undefined variable `|`"
//   Xs[N!]                  locate-by-value (Xs[3!]) crashes
//
// Run:  symta -f examples/29-subscript.s


// ----------------------------------------------------------------
// 1. Range constructions inside literal brackets.
// ----------------------------------------------------------------
R_neg [20:-10:2]
R_alpha [\a:\e]
R_up [:9]
say "ranges:"
say "  20 down -10 step 2 = [R_neg]"
say "  alpha range a..e   = [R_alpha]"
say "  1..9               = [R_up]"


// ----------------------------------------------------------------
// 2. Basic index forms.
// ----------------------------------------------------------------
Xs [10 20 30 40 50 60 70 80]
A Xs[3]
B Xs[1+2]
C Xs[]
say "Xs               = [Xs]"
say "Xs index 3       = [A]"
say "Xs index 1+2     = [B]"
say "Xs empty (.end)  = [C]"


// ----------------------------------------------------------------
// 3. Multi-index forms.
// ----------------------------------------------------------------
M3 Xs[0 2 5]
Ys [1 3 5]
M4 Xs[@Ys]
say "Xs three indices    = [M3]"
say "Xs spread index list = [M4]"


// ----------------------------------------------------------------
// 4. Slice / take / drop forms.
// ----------------------------------------------------------------
Sl1 Xs[2:5]
Sl2 Xs[3:]
Sl3 Xs[:3]
Sl4 Xs[:]
Lst Xs[~]
Lead Xs[:~]
Rev Xs[~:0]
Tail Xs[~:]
say "Xs slice 2..5    = [Sl1]"
say "Xs drop first 3  = [Sl2]"
say "Xs take first 3  = [Sl3]"
say "Xs full copy     = [Sl4]"
say "Xs last          = [Lst]"
say "Xs lead          = [Lead]"
say "Xs reversed lead = [Rev]"
say "Xs last as list  = [Tail]"


// ----------------------------------------------------------------
// 5. Slice-with-step.
// ----------------------------------------------------------------
S1 Xs[0::2]
S2 Xs[~+1:0:2]
say "Xs step 2          = [S1]"
say "Xs reverse step 2  = [S2]"


// ----------------------------------------------------------------
// 6. Replacement at index.
// ----------------------------------------------------------------
R1 Xs[2!99]
R2 Xs[1!99 3!88]
R3 Xs[0!99 ~!11]
say "Xs replace at 2     = [R1]"
say "Xs replace two pos  = [R2]"
say "Xs replace ends     = [R3]"


// ----------------------------------------------------------------
// 7. Additive insert (@N!V) -- doesn't remove the original.
// ----------------------------------------------------------------
I1 Xs[@2!99]
say "Xs insert at 2: [I1]"


// ----------------------------------------------------------------
// 8. Error-suppressing `[!N]` -- returns No on out-of-bounds.
// ----------------------------------------------------------------
E1 Xs[!9]
E2 Xs[!100]
E3 Xs[!2]
say "Xs error-suppress 9   = [E1]"
say "Xs error-suppress 100 = [E2]"
say "Xs error-suppress 2   = [E3]"


// ----------------------------------------------------------------
// 9. Hash-table multi-key subscript.
// ----------------------------------------------------------------
T name!\Nancy age!37 city!\Amsterdam
T1 T[name age]
T2 T[name age city]
say "T two keys       = [T1]"
say "T all three keys = [T2]"


// ----------------------------------------------------------------
// 10. Path-component replacement (the practical sbe.txt example).
// ----------------------------------------------------------------
P "/usr/local/bin"
Pr P.split^'/'[~!\hello].text^'/'
say "path rewrite: [Pr]"


// ----------------------------------------------------------------
// 11. Python-comparison slice cases. In Symta `Xs[N:M:S]` is
// `Xs[M:N:S].f` -- so [0:6:1] gives 6 elements, [0:7:1] gives 7
// (out-of-bounds permitted), and step-2 forms give the expected
// every-other-element sublist.
// ----------------------------------------------------------------
ABC [1 2 3 4 5 6 7]
P1 ABC[0:6:1]
P2 ABC[0:7:1]
P3 ABC[0:6:2]
P4 ABC[1:6:2]
P5 ABC[1:7:2]
say "python-comparison slices:"
say "  start 0 end 6 step 1 -> [P1]"
say "  start 0 end 7 step 1 -> [P2]"
say "  start 0 end 6 step 2 -> [P3]"
say "  start 1 end 6 step 2 -> [P4]"
say "  start 1 end 7 step 2 -> [P5]"

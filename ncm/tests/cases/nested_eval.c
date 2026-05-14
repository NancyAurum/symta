// NCM-7 regression: `eval_term` used to `strtol()` macro body / arg
// values literally, so any expression hidden behind a name silently
// returned the leading-integer prefix (zero for letter-led names).
// The fix runs `eval_expr_standalone` instead, which means macros
// chain through `#[expr]` like users expect.
#A 5
#B A
#C 2*A+1
#D B+C
//
// Bare-name expansion in #[expr]:
B_is_5  = #[B]              // was 0, now 5
C_is_11 = #[C]              // was 2 (strtol stopped at '*'), now 11
D_is_16 = #[D]              // was 0 (strtol stopped at 'B'), now 16
//
// Arithmetic combining macros:
sum     = #[A + B + C]      // 5 + 5 + 11 = 21
mix     = #[A * B + C]      // 5*5 + 11 = 36
//
// Nested-call evaluation in #[expr]:
#double(X) #[2*X]
dbl5    = #[double(5)]      // 10
dbl_dbl = #[double(double(5))]  // was 0 (inner expanded to text
                                 // "10" but the outer #[2 * 10]
                                 // never re-evaluated), now 20
//
// Macro-to-macro chains:
#quadruple(X) double(double(X))
q5      = quadruple(5)      // 20

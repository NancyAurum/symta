// lambda.s -- exercises lambda forms.
// Run with:  symta -f tests-macros/cases/lambda.s

// --- 1. Bare `=>` arrow lambda ---
F (X => X * 2)
say: F 21                   // 42

// --- 2. Multi-arg lambda ---
Add (X Y => X + Y)
say: Add 3 4                // 7

// --- 3. Lambda as argument to map / keep ---
say [1 2 3].map(X => X*X)       // (1 4 9)
say [1 2 3 4 5].keep(X => X%2)  // odds

// --- 4. Lambda with multi-statement body via `|` ---
Compute (X =>
  | Y X * 2
  | Z Y + 1
  | Z)
say: Compute 10            // 21

// --- 6. .keep / .skip with lambdas ---
say [1 2 3 4 5 6 7 8 9 10].keep(X => X > 5)
say [1 2 3 4 5 6 7 8 9 10].skip(X => X > 5)

// --- 7. Named function (top-level def) ---
square X = X * X
say: square 9               // 81

// --- 8. Recursive function ---
fact N = if N << 1 then 1 else N * fact(N-1)
say: fact 5                 // 120
say: fact 10                // 3628800

// --- 9. Closure over outer variable ---
mk_counter Init =
  N Init
  Step => | N = N + Step; N

C1 mk_counter 0
C2 mk_counter 100
say: C1 1                   // 1
say: C1 1                   // 2
say: C2 10                  // 110 (separate closure)
say: C1 1                   // 3

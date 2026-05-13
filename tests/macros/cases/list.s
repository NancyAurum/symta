// list.s -- exercises list literals, destructuring, splicing.
// Run with:  symta -f tests-macros/cases/list.s

// --- 1. Literal list with [] ---
say [1 2 3]
say []
say [\a \b \c]

// --- 2. Colon-line builds list at end of line ---
Xs: 10 20 30
say Xs                      // (10 20 30)

// --- 3. Destructuring assignment ---
[A B C] Xs
say "A=[A] B=[B] C=[C]"

[H @T] Xs
say "head=[H] tail=[T]"

[@L Last] Xs
say "lead=[L] last=[Last]"

// --- 4. Nested destructuring ---
Pairs: [1 2] [3 4] [5 6]
[[A B] @_] Pairs
say "first pair: A=[A] B=[B]"

// --- 5. List splicing into another list ---
Mid: 2 3 4
say [1 @Mid 5]              // (1 2 3 4 5)

// --- 6. Indexing ---
say Xs.0                    // 10
say Xs.1                    // 20
say Xs.~                    // 30 (last)
say Xs.n                    // 3

// --- 7. Tail / lead / head ---
say Xs.tail                 // (20 30)
say Xs.lead                 // (10 20)
say Xs.head                 // 10 (first)
say Xs.f                    // reversed: (30 20 10)
say [[1 2] [3 4] [5 6]].j   // flatten one level: (1 2 3 4 5 6)

// --- 8. List from range ---
say [:5]                    // (1 2 3 4 5)

// --- 9. .map / .keep / .skip ---
say [1 2 3 4 5].map(X => X*10)
say [1 2 3 4 5].keep(X => X > 2)
say [1 2 3 4 5].skip(X => X > 2)

// --- 10. .all / .any (predicates) ---
say [1 2 3 4 5].all(X => X > 0)        // 1
say [1 2 3 4 5].all(X => X > 2)        // 0
say [1 2 3 4 5].any(X => X >< 3)       // 1 (well, truthy)
say [1 2 3 4 5].any(X => X >< 99)      // 0

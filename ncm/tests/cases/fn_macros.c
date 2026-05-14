// Function-style macros: positional, default-valued, variadic.
#sq(X) ((X)*(X))
#area(W,H) sq(W) + sq(H)

a = sq(7)
b = area(3,4)
c = area(2+1, 4)

// Defaults
#greet(Name=World) hello, Name!
g0 = greet()
g1 = greet(NCM)

// Variadic
#all([Xs]) [Xs]
v0 = all(1, 2, 3, 4)
// FILE-AS-TODO NCM-5: `all()` (variadic with zero args) segfaults
//                    NCM.  Uncovered by this test suite; tracked in
//                    TODO.md.  Commented out until NCM-5 is fixed.
// v1 = all()

// Recursive expansion: macro body referencing another macro
#double(X) #[2*X]
#quadruple(X) double(double(X))
q  = quadruple(5)

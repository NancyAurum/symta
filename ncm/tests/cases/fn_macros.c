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
v1 = all()             // NCM-5 regression: zero-arg variadic
v2 = all(only)

// Variadic with a leading positional arg, varying variadic counts.
#cmd(Name, [Args]) Name(Args)
c0 = cmd(foo)          // empty variadic again
c1 = cmd(foo, 1, 2, 3)

// The implicit Xs_n binding -- length of the variadic capture.
#count([Xs]) Xs_n=<Xs_n>
n0 = count()
n3 = count(a, b, c)

// Recursive expansion: macro body referencing another macro
#double(X) #[2*X]
#quadruple(X) double(double(X))
q  = quadruple(5)

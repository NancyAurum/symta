// 04-lists.s -- lists, the bread and butter of Symta
//
// Run:  symta -f examples/04-lists.s

// Square brackets build a literal list. Bare lowercase words
// don't need quotes.
Xs [hello world 123]
say "Xs = [Xs]"

// `Ys: a b c` is sugar for `Ys [a b c]`.
Ys: an other list
say "Ys = [Ys]"

// `@Xs` splices a list inside another list.
Zs: @Xs @Ys
say "Zs = [Zs]"

// Comma-separated literals: `1,2,3` is `[1 2 3]`.
Ns 1,2,3,4,5
say "Ns = [Ns]"

// Indexing with `.` (zero-based).
Letters: a b c d e f
say "Letters.0 = [Letters.0]"
say "Letters.3 = [Letters.3]"

// Mutation with `=` writes a new value at the index.
Letters.3 = \X
say "after edit: [Letters]"

// Useful built-in methods (defined in core_.s):
L: 5 2 8 1 9 3
say "n (length):   [L.n]"
say "z (sum):      [L.z]"
say "max / min:    [L.max] / [L.min]"
say "f (flip/rev): [L.f]"
say "s (sorted):   [L.s]"

// Vector arithmetic: element-wise on equal-length lists.
A [1 2 3]
B [4 5 6]
say "A+B = [A+B]"
say "A*B = [A*B]"

// Slicing with `Xs[Start:Stop:Step]`. Inside a "..." string the
// `[` `]` are reserved for interpolation, so to print a literal
// pair, escape with `\[` `\]` -- or print the result alone.
Big [10 20 30 40 50 60 70 80 90 100]
S1 Big[2:5]
say "Big\[2:5\]  = [S1]"            // elements 2..4
S2 Big[3:]
say "Big\[3:\]   = [S2]"            // from 3 to end
S3 Big[:4]
say "Big\[:4\]   = [S3]"            // first 4
S4 Big[0::2]
say "Big\[0::2\] = [S4]"            // every second element
say "first: [Big.0]   last: [Big.(Big.n-1)]"

// Integer ranges.
R1 [:5]
R2 [1:5]
R3 [0:10:2]
say "\[:5\]    = [R1]"
say "\[1:5\]   = [R2]"
say "\[0:10:2\] = [R3]"

// `..` repeats a value N times.
R4 \x..5
say "x..5 = [R4]"

// 28-tight.s -- compact list-processing idioms
//
// Symta has special syntax for the operations that show up over
// and over in list processing. Many things that take a page in
// other languages fit on one line here. The point of this file
// is to give those idioms names, show what they expand to, and
// what each piece is doing.
//
// Run:  symta -f examples/28-tight.s


// ----------------------------------------------------------------
// 1. Sum / product / fold via `{}` and the `&~name` auto-closure.
//
//    `&~s` introduces a hidden accumulator named `~s`, lazily
//    initialised on first use. `~s+?` updates it with the
//    current element `?`. The auto-closure variable is what
//    falls out of the `{}` map, replacing the usual list-of-
//    results.
// ----------------------------------------------------------------
say "sum 1..9: [[:9]{&~s+?}]"
say "product 1..9: [[:9]{&~s^1*?}]"


// ----------------------------------------------------------------
// 2. `{?*?}` -- shorthand for `X => X*X`. `?` is the current
//    element.
// ----------------------------------------------------------------
say "squares 1..9: [[:9]{?*?}]"


// ----------------------------------------------------------------
// 3. Replace digits in a text by their occurrence position.
//
//    `T{d? = ~~}` -- on chars matching the `d?` predicate (is_d),
//    map to `~~`, which is the count of how many lambda calls
//    have fired so far. Non-matching chars pass through unchanged.
// ----------------------------------------------------------------
T "1-a, 2-b and 3-c"
say "digits to positions: [T{d? = ~~}]"


// ----------------------------------------------------------------
// 4. Character frequency counter -- the one-liner.
//
//    `~D.?+` -- in implicitly declared table `D`, increment slot
//    `?` and return D. Equivalent to a 6-line loop in most
//    languages.
// ----------------------------------------------------------------
Freq "the quick brown fox jumps over the lazy dog"{~D.?+}
say "char freq size: [Freq.n]"
say "freq of 'o':  [Freq.o]"
say "freq of ' ':  [Freq.(' ')]"


// ----------------------------------------------------------------
// 5. Parenthesis nesting levels per character. `n~` is a hidden
//    counter, post-increment `n~+` returns it then bumps,
//    `--n~` is pre-decrement.
// ----------------------------------------------------------------
E '((b*2)-4*a*c)*0.5'
say "paren levels: [E{n~ : '(' = n~+; ')' = --n~}]"


// ----------------------------------------------------------------
// 6. Quick-sort -- pattern matching + recursion.
//
//    `qsort@r H,@T = ...`: `@r` declares an alias `r` for `qsort`
//    inside the body, so the recursive call can be written
//    plainly. The pattern `H,@T` peels off head and tail.
// ----------------------------------------------------------------
qsort@r H,@T = @T{:?<H}^r, H, @T{?<H=}^r
say "qsort:  [qsort [3 1 4 1 5 9 2 6 5 3 5]]"


// ----------------------------------------------------------------
// 7. FizzBuzz -- the multi-case `{}` map.
//
//    `~?` is the current element wrapped as a copy that survives
//    a non-match. `=\Fizz` returns the symbol `Fizz` when the
//    case matches. Order matters: 15 before 3 before 5, so
//    FizzBuzz wins over Fizz on multiples of both.
// ----------------------------------------------------------------
FB [1:15]{~?%15 = \FizzBuzz; ~?%3 = \Fizz; ~?%5 = \Buzz}
say "fizzbuzz 1..15: [FB]"


// ----------------------------------------------------------------
// 8. Flatten a nested list (only one level deep at a time).
//    `.j` joins a list-of-lists into a single list.
// ----------------------------------------------------------------
say "join: [[[1 2] [3 4 5] [6]].j]"


// ----------------------------------------------------------------
// 9. Transpose / zip: `Xs{A,B = ~i.A ~j.B}` would split a list of
//    pairs into two parallel lists. The same logic via `.zip`:
// ----------------------------------------------------------------
say "zip:  [[[1 a] [2 b] [3 c]].zip]"


// ----------------------------------------------------------------
// 10. Group, sum, count -- aggregate operations in one pass.
// ----------------------------------------------------------------
Words "alpha bravo charlie delta echo foxtrot".split^' '
say "words: [Words]"
say "group 2: [Words.group^2]"
say "char count: [Words{&~s+?.n}]"

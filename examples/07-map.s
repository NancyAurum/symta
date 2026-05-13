// 07-map.s -- the `{}` map operator
//
// `Xs{Body}` applies Body to each element of Xs and collects the
// results. Inside Body:
//   - `?` is the current element
//   - `=` introduces a pattern->replacement (non-matching items
//     pass through unchanged)
//   - `:` filters; `:Cond` keeps elements where Cond is true
//   - `~x` are auto-closure variables that survive the loop
//
// Run:  symta -f examples/07-map.s

// Square every integer 0..9.
say 10{?*?}

// Keep only odd numbers.
say [1 2 3 4 5 6 7 8 9 10]{?%2=}

// Skip odd numbers (keep evens).
say [1 2 3 4 5 6 7 8 9 10]{:?%2}

// FizzBuzz the Symta way: fall-through cases inside `{}`.
say [:15]{~?%15=\FizzBuzz; ~?%3=\Fizz; ~?%5=\Buzz}

// String -> string: replace a word with another.
S "Now is a bad time to live, really bad!"
say S{@\bad=\good}

// Frequency table from a string. `~D.?+` says: take the
// auto-closure table `~D`, look up the current char, post-increment.
F "hello world"{~D.?+}
// Sort the [key value] pairs by key so the printed order is
// deterministic across hash-seed randomisation. Without this,
// `say F` prints the table in whatever order hash iteration
// happens to walk -- fine in practice, awful for golden-file
// regression tests.
say F.l.s(| ?.0 < ??.0)

// Reverse a list.
say [a b c d e].f

// Sum a list (uses `.z`).
say [1 2 3 4 5].z

// Count occurrences of a digit via auto-closure integer `~i`.
// `~i+` post-increments; `~i` is implicitly returned by the map.
N "1-a, 2-b and 3-c"{d?=~i+}
say "digits seen: [N]"

// `.s` sorts; with a comparator lambda we can sort by anything.
// Here, sort words by descending length, alphabetical on ties.
// Without the lexical tie-break the order of equal-length words
// depends on Symta's internal sort stability, which isn't a
// contract the test should pin.
Words: alpha beta gamma delta epsilon zeta
Sorted Words.s | ?.n > ??.n or (?.n >< ??.n and ? < ??)
say "longest first: [Sorted]"

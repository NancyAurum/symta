// 36-paren-patterns.s -- X(...) parenthesised pattern idioms
//
// Symta's `X(...)` postfix is a pattern-shape lambda: think of
// `X(pat1 pat2 ...)` as "match X against this shape, return what
// the pattern says".  Distinct from `X[...]` (subscript -- see
// 29-subscript.s) and `X{...}` (map/filter -- see 28-tight.s).
//
// Conventions used below:
//   _   anonymous "anything"
//   ~   the head of the current list (in the pattern body) or
//       the index counter in `/~`
//   @_  "rest of the list, don't bind"
//   @~  "rest as a list" (binds when on LHS of `=`)
//   :   leading `:` means "boolean only" -- return 1/0 instead
//       of the matched value or the original X
//   ;   separator between successive alternative patterns
//   ^r  recursive call to the enclosing pattern lambda
//   =:  the do-nothing pattern that returns its input unchanged
//
// Skipped (broken upstream, mirrored from tests/macros/cases/idioms.s):
//   L(:@_ -P&=0)   -- "bad match case `&`"
//   L(/~ P @_)     -- "invalid `~` expr"
//   L(@_ ~<P @_)   -- "invalid `~` expr"
//
// Run:  symta -f examples/36-paren-patterns.s


// ----------------------------------------------------------------
// 1. Shape predicates: ":pat" returns 1/0.
// ----------------------------------------------------------------
say "shape predicates:"
A [1 2 3](:_ _ _)
B [1 2](:_ _ _)
say "  three-elem matched by (:_ _ _): [A]"        // 1
say "  two-elem matched by (:_ _ _):   [B]"        // 0


// ----------------------------------------------------------------
// 2. Match-with-default: "pat = result" returns result on match,
//    otherwise the original X.
// ----------------------------------------------------------------
C [1 2 3](_ _ _=1)
D [1 2](_ _ _=1)
say "match-with-default:"
say "  three-elem (_ _ _=1): [C]"                  // 1
say "  two-elem   (_ _ _=1): [D]"                  // (1 2)


// ----------------------------------------------------------------
// 3. Atom-level replacement.
// ----------------------------------------------------------------
say "atom replacement:"
X1 \bad
X2 \xyz
R1 X1(bad=\good)
R2 X2(bad=\good)
say "  bad (bad=\\good): [R1]"                      // good
say "  xyz (bad=\\good): [R2]"                      // xyz

// Trailing arg before `:` provides the default for the no-match
// case (replaces "return X unchanged" with "return that value").
R3 X1(123:bad=\good)
R4 X2(123:bad=\good)
say "atom replacement with default:"
say "  bad (123:bad=\\good): [R3]"                  // good
say "  xyz (123:bad=\\good): [R4]"                  // 123


// ----------------------------------------------------------------
// 4. List destructuring shapes.
// ----------------------------------------------------------------
Xs2 [10 20]
Xs3 [10 20 30 40]
T1 Xs3(_@~)             // tail
T2 Xs2(~ ~)             // 2-elem keep
T3 Xs3(~ ~)             // 2-elem check, fails on 4-elem
T4 Xs3(_ ~ ~@)          // pick 2nd and 3rd
say "list destructure:"
say "  (10 20 30 40)(_@~)    = [T1]"               // (20 30 40)
say "  (10 20)(~ ~)          = [T2]"               // (10 20)
say "  (10 20 30 40)(~ ~)    = [T3]"               // No
say "  (10 20 30 40)(_ ~ ~@) = [T4]"               // (20 30)


// ----------------------------------------------------------------
// 5. Recursive pattern lambdas via `^r`.
// ----------------------------------------------------------------
F [:9](1:N@R = N*R^r)
say "factorial 1..9 (via recursive pattern): [F]"  // 362880

Path "/usr/local/bin"
Components Path(_:"[A]/[B]"=A@r^B)
say "path split on '/': [Components]"               // ("" usr local bin)


// ----------------------------------------------------------------
// 6. Tree flattening.  `=:` is the identity pattern (no rewrite),
//    used as a catch-all for non-list leaves.
// ----------------------------------------------------------------
Tree [1 [2 [3 4]] 5]
Flat Tree(_:H@T = @H^r@T^r;=:)
say "flatten nested tree: [Flat]"                  // (1 2 3 4 5)


// ----------------------------------------------------------------
// 7. Whole-tree atom replacement.  Same recursive shape as flatten
//    plus a final `hate=\love` rewrite that fires on every leaf.
// ----------------------------------------------------------------
HateTree [hate [a hate] hate b]
Loved HateTree(:H@T = H^r@T^r;=:;hate=\love)
say "replace hate with love: [Loved]"               // (love (a love) love b)


// ----------------------------------------------------------------
// 8. Tree walk that collects values matching a predicate.
//    `~r._<{?>100}<int?` collects (anon recursive accumulator `~r`)
//    every int > 100 it sees.
// ----------------------------------------------------------------
NumTree [10 [200 [50 300]] 500]
Big NumTree(:_^r@_^r;&~r._<{?>100}<int?)
say "ints > 100 in tree: [Big]"                     // (200 300 500)


// ----------------------------------------------------------------
// 9. List-shape `L(...)` idioms.
//
// `:@_ pred@` -- "skip prefix, then pred matches, then anything"
// = "is there at least one elem matching pred?"
// ----------------------------------------------------------------
L5 [1 2 3 4 5]
H1 L5(:@_ ?>3@)
H2 L5(:@_ ?>9@)
say "any > 3?  [H1]"                                // 1
say "any > 9?  [H2]"                                // 0

D1 L5(/3 @~)
D2 L5(@~ /2)
D3 L5(/1 @~ /1)
say "drop first 3:        [D1]"                     // (4 5)
say "drop last 2:         [D2]"                     // (1 2 3)
say "drop ends (1 and 1): [D3]"                     // (2 3 4)

L6 [10 20 30]
F1 L6(~@)
R5 L6(_@~)
say "first elem: [F1]"                              // 10
say "rest:       [R5]"                              // (20 30)

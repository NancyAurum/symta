// 14-quirks.s -- the surprising parts
//
// Symta has a handful of features that catch even Lispers off
// guard. Knowing them up front saves debugging time later.
//
// Run:  symta -f examples/14-quirks.s


// 1. No is the additive identity for everything. Useful for
//    increment-if-missing logic without a guard.
say "No + 7      = [No + 7]"
say "0   + No   = [0 + No]"
say "got No     -> [got No]"
say "got 0      -> [got 0]"            // got = "value is not No"
say ""


// 2. Auto-closure variables. A `~name` inside `{}` introduces a
//    hidden variable and *returns* it from the call.
//    `~D.?+` -- "in hidden table D, increment slot ?".
W "the quick brown fox the the quick"
Counts W.split^' '{~D.?+}
say "word counts: [Counts]"

// `~i+` -- a hidden counter that survives the loop.
N "1-a, 2-b and 3-c"{d?=~i+}
say "digits seen: [N]"
say ""


// 3. `?` and `??` are the first and second positional args inside
//    a quick-lambda or comparator.
DescSorted [5 1 4 2 3].s | ? > ??
say "sorted desc: [DescSorted]"
say ""


// 4. Inside string interpolation, `[...]` IS the interpolation
//    syntax -- so a literal `[` or `]` inside a "..." string must
//    be escaped, and slice syntax must go through a temp var.
Sub [10 20 30 40 50][1:4]
say "sub          = [Sub]"
say "the slice spelled out: \[1:4\]"
say ""


// 5. `\` quotes the very next token to a literal symbol. So
//    `\Hello` is the symbol Hello (preserving the H). `\#`
//    doesn't lex (the `#` isn't a token-starter); use a
//    different glyph for placeholders.
Quoted \Hello
say "quoted: [Quoted]"
Stars \x..5
say "stars:  [Stars]"
say ""


// 6. List comma syntax. `1,2,3` is `[1 2 3]`. Comma is a list
//    builder, not a statement separator.
say "1,2,3 -> [1,2,3]"
say ""


// 7. Method call on a literal. Methods dispatch on the type tag,
//    so `9.x` calls `int.x` (hex). It does NOT mean "field x of 9".
say "9.x   = [9.x]"
say "255.x = [255.x]"
say ""


// 8. The standard library uses one-letter shortcut names. Don't
//    use these as your own variables in core code:
//      .s   sort       .f   flip/reverse     .z   sum
//      .j   join       .l   to-list          .n   length
//      .i   indexed    .x   hex              .t   to-table
L1 [3 1 2].s
L2 [1 2 3].f
L3 [1 2 3 4].z
say "\[3 1 2\].s   = [L1]"
say "\[1 2 3\].f   = [L2]"
say "\[1 2 3 4\].z = [L3]"
say ""


// 9. `=` inside `case` and `{}` is a *match clause*, not an
//    assignment. Non-matching elements pass through unchanged.
Mapped [1 2 3 4 5]{1=\one; 3=\three}
say "match-and-replace: [Mapped]"
say ""


// 10. Multi-statement bodies under `if`/`while` etc. should be
//     separated with `|` (or `;` to share an auto-closure scope).
//     Bare newlines inside an inline `if X: A` body don't always
//     do what you'd expect.
do_things X = if X > 0:
    | say "positive"
    | say "and we get here"
  else say "non-positive"

do_things 5
do_things 0

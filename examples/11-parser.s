// 11-parser.s -- parsing and serialization
//
// Symta source IS lists of lists. Parsing produces an AST you can
// walk with the same tools you use for any other list. The reverse
// direction is `O.as_text`, which serializes a value to a textual,
// reader-friendly form.
//
// Run:  symta -f examples/11-parser.s


// -----------------------------------------------------------------
// 1. Parse a small program. `text.parse` returns a list of forms,
//    one per top-level expression in the source.
// -----------------------------------------------------------------
Src "
fac N = if N >< 0: 1 else N*fac(N-1)
say: fac 5
"

Forms Src.parse
say "parsed [Forms.n] form(s):"
for F Forms: say "  [F]"
say ""


// -----------------------------------------------------------------
// 2. Walk the AST recursively. Each node is either an atom (number,
//    text, symbol) or a list. Here we count the operator at the
//    head of each compound form.
// -----------------------------------------------------------------
count_heads Tree =
  Counts!
  visit X = if X.is_list and X.n >> 1:
    | Head X.0
    | if Head.is_text: Counts.Head = Counts.Head + 1
    | for C X: visit C
  visit Tree
  Counts

Heads count_heads Forms
say "compound-expression heads:"
for K,V Heads: say "  [K] -> [V]"
say ""


// -----------------------------------------------------------------
// 3. as_text -- serialize any value to a printable, parseable form.
// -----------------------------------------------------------------
Data: 1 [2 3 [4 5]] foo "with spaces"
T Data.as_text
say "serialized:  [T]"
say ""


// -----------------------------------------------------------------
// 4. Hash tables print as `@{key!val ...}`. The same form parses
//    back as an AST; to actually rebuild the table at runtime, use
//    `eval` (which needs the eval module -- see the symta tool).
// -----------------------------------------------------------------
H name!\Nancy age!37 city!\Amsterdam
say "table:       [H.as_text]"
say "key access:  [H.name] / [H.age] / [H.city]"
say ""


// -----------------------------------------------------------------
// 5. Programs ARE lists. Build an AST programmatically.
//    The result is itself valid Symta source.
// -----------------------------------------------------------------
Body [`+` \X 1]
Lambda [`=` [\inc \X] Body]
say "synthesized: [Lambda.as_text]"

// 33-csv.s -- CSV parsing, transformation, and emission, no library
//
// In Python this is `import csv, io` + a few lines of plumbing.
// In Symta the entire parser is forty characters of REFAL term
// rewriting on the character stream.  Transformations afterwards
// are ordinary `{}` operations on the parsed table.
//
// Run:  symta -f examples/33-csv.s


CSV "name,age,city,salary
alice,30,Utrecht,50000
bob,25,Berlin,45000
carol,28,Amsterdam,60000
dave,40,Rotterdam,55000"


// ----------------------------------------------------------------
// 1. Parse, the REFAL way.
//
//   CSV.l                              char list
//   {@A "\n" = A; @A=A}                group consecutive chars
//                                      separated by `\n` -- the
//                                      classic REFAL "split on
//                                      delimiter" pattern: match a
//                                      run followed by a separator,
//                                      keep the run, drop the sep.
//                                      The second clause catches
//                                      the trailing piece that has
//                                      no `\n` after it.
//   {text}                             join each char-run back to
//                                      a text value (one per line)
//   {"[~],[~],[~],[~]"}                for each line, match the
//                                      string-pattern with four
//                                      anonymous `~` captures.  The
//                                      result of the body is the
//                                      list of captures, so each
//                                      line becomes a 4-element
//                                      list.
//
// Pipeline reads left-to-right; no intermediate variables, no
// loops, no library.
// ----------------------------------------------------------------
Rows CSV.l{@A "\n" = A; @A=A}{text}{"[~],[~],[~],[~]"}
Hdr  Rows.head
Body Rows.tail
say "header:    [Hdr]"
say "row count: [Body.n]"
say ""


// ----------------------------------------------------------------
// 2. Aggregate: sum the `age` column.
// ----------------------------------------------------------------
Ages Body{[N A C S] = A.int}
say "ages:      [Ages]"
say "total age: [Ages.z]"
say "mean age:  [Ages.z / Ages.n]"
say ""


// ----------------------------------------------------------------
// 3. Filter: keep rows where age >= 28.
// ----------------------------------------------------------------
Older Body.keep(| [N A C S] => A.int >> 28)
say "rows with age >= 28:"
for Row Older: say "  [Row]"
say ""


// ----------------------------------------------------------------
// 4. Project: just (name, monthly_salary) pairs.
// ----------------------------------------------------------------
Monthly Body{[N A C S] = [N (S.int / 12)]}
say "name, monthly salary:"
for [N M] Monthly: say "  [N]: [M]"
say ""


// ----------------------------------------------------------------
// 5. Augment: add a computed `tax` column (30 % of salary).
// ----------------------------------------------------------------
Taxed Body{[N A C S] = [N A C S (S.int * 30 / 100)]}
say "rows with tax column appended:"
for Row Taxed: say "  [Row]"
say ""


// ----------------------------------------------------------------
// 6. Emit back as CSV.  Mirror of the parse:
//     - interpolate every cell into text
//     - join cells by ','
//     - join lines by '\n'
// ----------------------------------------------------------------
Hdr2 [@Hdr \tax]
Lines [Hdr2.text(',') @Taxed{Row = Row{"[?]"}.text(',')}]
say "--- re-emitted CSV ---"
say Lines.text('\n')

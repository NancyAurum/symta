// 33-csv.s -- CSV parsing, transformation, and emission, no library
//
// A common everyday task in Python looks like:
//
//   import csv, io
//   reader = csv.reader(io.StringIO(text))
//   rows = [r for r in reader]
//   header, body = rows[0], rows[1:]
//   ...
//
// In Symta the entire pipeline is the standard `{}` operator
// applied a few times.  No library imports.
//
// Run:  symta -f examples/33-csv.s

CSV "name,age,city,salary
alice,30,Utrecht,50000
bob,25,Berlin,45000
carol,28,Amsterdam,60000
dave,40,Rotterdam,55000"


// ----------------------------------------------------------------
// 1. Parse: split on newlines, then split each line on commas.
//    Two `.split` calls and a `{}` map -- end of parser.
// ----------------------------------------------------------------
Rows CSV.split^'\n'{?.split^','}
Hdr  Rows.head
Body Rows.tail
say "header:   [Hdr]"
say "row count: [Body.n]"
say ""


// ----------------------------------------------------------------
// 2. Aggregate: sum the `age` column.
//    `{[N A C S] = A.int}` is a REFAL pattern that destructures
//    every 4-element row and emits just the age (as int).
// ----------------------------------------------------------------
Ages Body{[N A C S] = A.int}
say "ages:      [Ages]"
say "total age: [Ages.z]"
say "mean age:  [Ages.z / Ages.n]"
say ""


// ----------------------------------------------------------------
// 3. Filter: keep rows where age >= 28.
//    `.keep` takes a predicate; we destructure the row inside.
// ----------------------------------------------------------------
Older Body.keep(| [N A C S] => A.int >> 28)
say "rows with age >= 28:"
for Row Older: say "  [Row]"
say ""


// ----------------------------------------------------------------
// 4. Project + rename: just (name, monthly_salary) pairs.
// ----------------------------------------------------------------
Monthly Body{[N A C S] = [N (S.int / 12)]}
say "name, monthly salary:"
for [N M] Monthly: say "  [N]: [M]"
say ""


// ----------------------------------------------------------------
// 5. Add a computed column: a 30%-tax estimate per row.
// ----------------------------------------------------------------
Taxed Body{[N A C S] = [N A C S (S.int * 30 / 100)]}
say "rows with `tax` column appended:"
for Row Taxed: say "  [Row]"
say ""


// ----------------------------------------------------------------
// 6. Emit back as CSV.  Every cell -> text via interpolation,
//    then join with commas; every row -> line, join with newlines.
// ----------------------------------------------------------------
Hdr2 [@Hdr \tax]
Lines [Hdr2.text(',') @Taxed{Row = Row{"[?]"}.text(',')}]
say "--- re-emitted CSV (with tax column) ---"
say Lines.text('\n')

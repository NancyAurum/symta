// 35-json.s -- JSON parsing, querying, transformation, no library
//
// In Python, the equivalent pipeline pulls in `import json`, plus
// usually `from collections import OrderedDict` if order matters,
// plus a few helper utilities for the transformations below.
//
// In Symta, JSON syntax is one term-rewriting pass away from
// Symta's own table-and-list syntax.  Once rewritten, Symta's
// built-in reader parses it.  No external library, no JSON
// parser to maintain.
//
// (This handles the common case -- unquoted-able identifiers,
//  integers, nested objects and arrays.  Real-world JSON with
//  embedded commas, quoted strings containing colons, escape
//  sequences, etc. needs a few more substitutions; the framework
//  is the same.)
//
// Run:  symta -f examples/35-json.s


// ----------------------------------------------------------------
// 1. Input: a JSON document as plain text.
// ----------------------------------------------------------------
J '{"users":[{"name":"alice","age":30,"role":"admin"},{"name":"bob","age":25,"role":"user"},{"name":"carol","age":28,"role":"user"}],"total":3}'

say "raw JSON ([J.n] chars):"
say J
say ""


// ----------------------------------------------------------------
// 2. Parse: rewrite JSON tokens into Symta tokens.
//
//   "..."  -> ...      (drop string quotes -- Symta atoms self-quote)
//   ,      -> SPACE    (whitespace separates Symta items)
//   :      -> !        (Symta's key marker)
//   {      -> @{       (Symta's table literal)
//
// Each substitution is a one-liner.  Chain them and feed the
// result to `text.parse`.
// ----------------------------------------------------------------
// `[a b c]` parses in Symta as ``(`[]` a b c)`` -- the `[]` symbol
// is the call-target.  We strip that marker recursively after
// parsing so JSON arrays become plain Symta lists.
clean V =
  if V.is_list
    then if V.0 >< `[]`
           then V.tail{? ^ clean}
           else V{? ^ clean}
    else if V.is_table
      then V.l{[K Val] = [K (Val ^ clean)]}.t
      else V

parse_json Str =
  S Str
  S = S.split('"').text('')
  S = S.split(',').text(' ')
  S = S.split(':').text('!')
  S = S.split('{').text('@{')
  S.parse.0.0 ^ clean


// ----------------------------------------------------------------
// 3. Use it.
// ----------------------------------------------------------------
T parse_json J
say "parsed total: [T.total]"
say "users count:  [T.users.n]"
say ""


// ----------------------------------------------------------------
// 4. Query: pick the names of every admin user.
//    `.keep` + `{}` map, both pattern-destructuring on the row.
// ----------------------------------------------------------------
Admins T.users.keep(| U => U.role >< \admin)
say "admins:"
for U Admins: say "  [U.name] (age [U.age])"
say ""


// ----------------------------------------------------------------
// 5. Transform: bump every user's age by one (a birthday pass).
//    `@{...}` is a *literal* table -- contents aren't evaluated.
//    For computed-value tables we build them empty then assign.
// ----------------------------------------------------------------
bump_age U =
  R!
  R.name = U.name
  R.age  = U.age + 1
  R.role = U.role
  R

Updated T.users{? ^ bump_age}
say "after a year:"
for U Updated: say "  [U.name]: age [U.age]"
say ""


// ----------------------------------------------------------------
// 6. Re-emit as JSON.  Mirror of the parse: every Symta token gets
//    rewritten back to its JSON spelling, then concatenated.
// ----------------------------------------------------------------
quote X = "\"[X]\""

json_of V =
  if V.is_int then "[V]"
  else if V.is_text then quote V
  else if V.is_list
    then "\[" + V{? ^ json_of}.text(',') + "\]"
    else if V.is_table
      then "{" + V.l{[K Val] = (quote K) + ":" + (json_of Val)}.text(',') + "}"
      else "null"

NewT!
NewT.users = Updated
NewT.total = T.total
say "--- re-emitted JSON ---"
say: json_of NewT

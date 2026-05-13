// string.s -- exercises string literals and interpolation.
// Run with:  symta -f tests-macros/cases/string.s

// --- 1. Plain string literal ---
say "hello"
say "with spaces"
say ""                      // empty

// --- 2. Interpolation with [Expr] ---
N 42
say "answer is [N]"

X 3
Y 4
say "[X]*[Y] = [X*Y]"

// --- 3. Escape sequences ---
say "tab:\there"
say "newline:\nNext"
say "quote: \"q\""

// --- 4. Concatenation via interpolation ---
A "foo"
B "bar"
say "[A][B]"                // foobar

// --- 5. .text on list joins values into a string ---
say [\a \b \c].text                // abc (no separator)
say ["1" "2" "3"].text(',')        // 1,2,3

// --- 7. .n returns length ---
say "abcde".n               // 5
say [].n                    // 0

// --- 8. Interpolation of expressions ---
Items: 1 2 3
say "first=[Items.0] last=[Items.~]"     // first=1 last=3

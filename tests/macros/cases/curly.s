// curly.s -- exercises the `{}` map/filter macro and its
// surrounding family (?, ??, ~v auto-closures, :filter, =replace).
//
// Run with:  symta -f tests-macros/cases/curly.s

// --- 1. Plain map: ? is the current element ---
say [1 2 3 4 5]{?*?}        // squares
say 5{?+1}                  // 5 elements 0..4 mapped to +1

// --- 2. Filter: :Cond keeps elements where Cond is truthy ---
say [1 2 3 4 5]{:?%2}       // keep odds (where ?%2 != 0)
say [1 2 3 4 5]{:?>2}       // keep > 2

// --- 3. Replace: Pat=Replacement leaves non-matches alone ---
say [1 2 3 4 5]{2=\two}     // replace 2 with \two
say [1 2 3 4 5]{?<3=\small} // replace items <3

// --- 4. Auto-closure ~v (state that survives the loop) ---
N "abc123"{d?=~i+}                  // count digits (~i++)
say "digit count: [N]"
F "abracadabra"{~D.?+}              // freq table via auto-closure dict
say "freq a: [F.\a]  freq b: [F.\b]"

// --- 5. Multi-arg (??): pair-wise comparator ---
Words: alpha beta gamma delta epsilon
Sorted Words.s(?.n > ??.n)          // sort by descending length
say Sorted

// --- 6. Range form: N{body} maps over 0..N-1 ---
say 4{?}                            // [0 1 2 3]
say 4{?*10}                         // [0 10 20 30]

// --- 7. Empty input ---
say []{?+1}                         // []
say 0{?}                            // []

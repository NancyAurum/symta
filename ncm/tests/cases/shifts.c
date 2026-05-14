// NCM-1 regression: << and >> in #[expr] used to drop the second
// '<' / '>' before parsing the operand, so the shift produced a
// silently-wrong value (typically 0 or l unchanged).
#A 5
#B 3

A<<2  = #[A<<2]
A<<B  = #[A<<B]
100>>2 = #[100>>2]
100>>B = #[100>>B]

// Nested in a larger expression:
mix = #[(A<<2) + (B<<1)]

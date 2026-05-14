// NCM-2 regression: && and || used to be eaten by eval_bitwise as
// the lone '&' / '|', so #[1 && 1] silently returned 0.  Bitwise
// is preserved in its own row to confirm we didn't break it.
1_AND_1  = #[1 && 1]
1_AND_0  = #[1 && 0]
0_OR_1   = #[0 || 1]
0_OR_0   = #[0 || 0]
chain    = #[1 && 1 && 1]
short_c  = #[0 && 1]

// bitwise still works on the same operators-without-doubling
bit_and  = #[3 & 5]
bit_or   = #[3 | 5]
bit_xor  = #[3 ^ 5]

// precedence: && binds looser than &, so a & b && c == (a & b) && c
prec     = #[3 & 4 && 1]

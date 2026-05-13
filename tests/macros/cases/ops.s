// ops.s -- exercises arithmetic, comparison, logical, bit-ops.
// Run with:  symta -f tests-macros/cases/ops.s

// --- 1. Basic arithmetic ---
say: 2 + 3                  // 5
say: 10 - 4                 // 6
say: 6 * 7                  // 42
say: 22 / 5                 // 4 (integer division)
say: 22 % 5                 // 2

// --- 2. Negative literal ---
N -5
say N                       // -5
say: 0 - 5                  // -5

// --- 3. Chained arithmetic with parens for grouping ---
say: 1 + 2 + 3              // 6
say: (10 - 3) - 2           // 5  (- is binary; chain via parens)
say: 2 * 3 + 4              // 10 (Symta: left-to-right, no precedence)

// --- 4. Comparison ---
say: 3 < 5                  // 1
say: 5 < 3                  // 0
say: 3 << 3                 // 1 (<=)
say: 3 >> 3                 // 1 (>=)
say: 5 > 3                  // 1
say: 3 >< 3                 // 1 (==)
say: 3 >< 4                 // 0
say: 3 <> 4                 // 1 (!=)

// --- 5. Equality on values vs symbols ---
say: \red >< \red           // 1
say: \red >< \blue          // 0
say: "abc" >< "abc"         // 1
say: "abc" <> "xyz"         // 1

// --- 6. min / max ---
say: min 3 7 1 9            // 1
say: max 3 7 1 9            // 9

// --- 7. Bitwise (-+- = ior, -*- = and, -^- = xor) ---
say: 0xF0 -+- 0x0F          // 0xFF = 255
say: 0xF0 -*- 0x33          // 0x30 = 48
say: 0xF0 -^- 0xFF          // 0x0F = 15
say: 1 -<- 4                // 16  (shl)
say: 32 ->- 2               // 8   (shr)

// --- 8. Float arithmetic ---
say: 1.5 + 2.5              // 4.0
say: 3.0 * 4.0              // 12.0
say: 1.0 / 2.0              // 0.5

// --- 9. Mixed int/float ---
say: 1 + 1.5                // 2.5
say: 3 * 0.5                // 1.5

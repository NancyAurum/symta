// block.s -- exercises block separator macros (`|` and `;`).
// Run with:  symta -f tests-macros/cases/block.s

// --- 1. `|` joins multi-line statements within a function ---
go1 =
  | A 10
  | B 20
  | say "A+B = [A+B]"
go1                         // 30

// --- 2. `|` indented body ---
go2 X =
  Y X + 1
  Z Y + 2
  say "X=[X] Y=[Y] Z=[Z]"
go2 5

// --- 3. `;` is an alternate block separator (semicolon-style) ---
go3 = ; A 1 ; B 2 ; say "[A] [B]"
go3

// --- 4. Multi-line lambda body via `|` ---
F (X =>
  | Y X * 2
  | Z Y + 5
  | Z)
say: F 10                  // 25

// --- 5. Statement vs expression: `|` is required consistently ---
go5 =
  | T 0
  | T = T + 100
  | T = T + 23
  | say T
go5                         // 123

// --- 6. Block returns last expression (when wrapped in function call) ---
sum_block =
  | A 5
  | B 7
  | A + B
say "block result: [sum_block]"     // 12

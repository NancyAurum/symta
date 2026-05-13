// assign.s -- exercises assignment forms.
// Run with:  symta -f tests-macros/cases/assign.s

// --- 1. Simple assignment ---
X 10
say X                       // 10
X = X + 1
say X                       // 11

// --- 2. Multiple introduction in a block ---
go1 =
  A 1
  B 2
  C 3
  say "A=[A] B=[B] C=[C]"

go1

// --- 3. Compound assignment via expansion macros ---
go2 =
  N 0
  N = N + 5
  say "+= equiv: [N]"
  N = N - 2
  say "-= equiv: [N]"
  N = N * 3
  say "*= equiv: [N]"

go2

// --- 4. Increment / decrement ---
go3 =
  K 0
  K+
  K+
  K+
  say "after 3 K+: [K]"     // 3
  K-
  say "after K-: [K]"       // 2

go3

// --- 5. Method-call assignment: T.field = ... ---
T (!)
T.name = "alice"
T.age = 30
say "[T.name] [T.age]"      // alice 30

// --- 6. Element assignment: A.i = v ---
A: 10 20 30 40
A.1 = 999
say A                       // (10 999 30 40)

// --- 7. Field-shorthand via $ (only valid inside methods/types) ---
//    $field expands to Me.field; tested in cls.s and game code,
//    not at toplevel.

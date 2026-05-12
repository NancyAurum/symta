// 23-control.s -- non-local return, errors, named blocks
//
// Three layers, smallest to lowest level:
//
//   1. `pass` / `done` -- continue / break inside `for`, `while`,
//      `till`. Always available, no setup.
//
//   2. `named L: ... ret L Value` -- structured early return from
//      a labelled block. Cleanest way to bail out of a deep loop
//      with a value.
//
//   3. `btrap` / `btland` / `btjump` -- the actual primitive. A
//      caller wraps a body in `btrap`, the body throws with `bad
//      "..."`, and `btrap` returns a `bterror` instead of letting
//      the program crash. Used to validate user input or handle
//      errors from FFI calls.
//
// Run:  symta -f examples/23-control.s


// ----------------------------------------------------------------
// 1. `pass` and `done` -- the loop versions of `continue` / `break`.
// ----------------------------------------------------------------
say "first odd >= 4 in 1..9:"
for I [1:9]:
  when I < 4: pass                       // skip
  when I%2 >< 0: pass                    // skip evens
  say "  found: [I]"
  done                                   // break out
say ""


// ----------------------------------------------------------------
// 2. `named L: ... ret L Value` -- structured early return.
//    The block's value is whatever the inner `ret L X` supplied,
//    or whatever falls out of the bottom otherwise.
// ----------------------------------------------------------------
find_first Xs Pred =
  named Out:
    for X Xs:
      when Pred X: ret Out X
    No                                   // value if `for` falls through

is_prime N =
  if N < 2: ret 0
  for I [2:N-1]:
    when N % I >< 0: ret 0
  1

say: find_first [4 6 8 9 10 11 14] &is_prime   // -> 11
say: find_first [4 6 8 10] &is_prime           // -> No
say ""


// ----------------------------------------------------------------
// 3. `bad Msg` raises a runtime error.  Wrap the call in `btrap`
//    to catch it as a `bterror` value instead of crashing.
// ----------------------------------------------------------------
withdraw Account Amount =
  if Amount < 0: bad "amount must be non-negative"
  if Amount > Account.balance:
    bad "overdraft: tried [Amount], have [Account.balance]"
  Account.balance = Account.balance - Amount
  Account.balance


Acct balance!100

// happy path -- no btrap needed if we know the args are good
say "ok deduction: [withdraw Acct 30]"

// hostile input -- catch the error
R btrap: => withdraw Acct 9999
if R.is_bterror:
  say "blocked: [R.text]"
else say "remaining: [R]"

R2 btrap: => withdraw Acct (-5)
if R2.is_bterror: say "blocked: [R2.text]"
else say "remaining: [R2]"

say "balance after attempts: [Acct.balance]"
say ""


// ----------------------------------------------------------------
// 4. The actual primitive: `btland: K => ...` produces a "landing
//    site" that `btjump K Value` warps back to. `btrap` is built
//    on top of this. You rarely need to use it directly, but here
//    it is for the curious.
// ----------------------------------------------------------------
// We can sum a list, bailing the moment we see a negative. The
// `btland: K => ...` lambda gets a continuation `K`; `btjump K V`
// makes the whole `btland` form return `V`.
sum_until_neg Xs =
  btland: K =>
    Acc 0
    for X Xs:
      when X < 0: btjump K Acc
      Acc = Acc + X
    Acc

L1 [1 2 3 4 5]
L2 [1 2 -7 4 5]
say "sum_until_neg [L1] = [sum_until_neg L1]"
say "sum_until_neg [L2] = [sum_until_neg L2]"
say ""


// ----------------------------------------------------------------
// 5. Recursive search with btland: bail out of a deep walk the
//    moment we find a hit, no need to thread "did we find it?"
//    flags through the recursion.
// ----------------------------------------------------------------
first_int Tree =
  btland: K =>
    visit X =
      if X.is_int: btjump K X
      when X.is_list: for C X: visit C
    visit Tree
    No

say "first int in nested tree: [first_int [a [b [c d]] [42 [99]]]]"
say "first int in flat list:   [first_int [a b c]]"

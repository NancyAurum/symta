// 20-data-structures.s -- writing your own generic types
//
// Two small generic types with private state behind methods, the
// canonical pattern for adding new building blocks to Symta.
//
//   * `prng` -- a reproducible random number generator (LCG)
//   * `queue` -- a fixed-size circular FIFO buffer
//
// Run:  symta -f examples/20-data-structures.s


// ----------------------------------------------------------------
// 1. PRNG -- linear-congruential generator with explicit seed.
//    Useful for reproducible "random" sequences in games, fuzzers,
//    and tests; the system `rand` is not seedable.
// ----------------------------------------------------------------
type prng: seed = $clear

prng.clear = $seed = 2147483647.rand           // pick a starting seed

// MINSTD: x_{n+1} = 16807 * x_n  (mod 2^31 - 1)
prng.n N =
  $seed = ($seed*16807) % 2147483647
  $seed % N

prng.f Range =                                  // float in 0..Range
  $seed = ($seed*16807) % 2147483647
  $seed.float / 2147483647.0 * Range


R prng
R.seed = 42                                     // reproducibility

say "four rolls (1..100):"
say "  [R.n 100], [R.n 100], [R.n 100], [R.n 100]"

R.seed = 42                                     // reset
say "same seed reproduces them:"
say "  [R.n 100], [R.n 100], [R.n 100], [R.n 100]"
say ""

R.seed = 1
Floats dup I 5: R.f 1.0
say "five floats in 0..1: [Floats]"
say ""


// ----------------------------------------------------------------
// 2. Queue -- circular buffer with head/tail indices `a` and `b`.
//    `a == b` means empty.  Constant-time push/pop.
// ----------------------------------------------------------------
type queue Size: xs a b
  = $xs = dup I Size 0                          // pre-allocate slots

queue.end = $a >< $b

queue.push Item =
  $xs.($b+) = Item                              // write, then post-inc
  when $b >< $xs.n: $b = 0                      // wrap

queue.pop =
  as $xs.($a+):                                 // read, then post-inc
    when $a >< $xs.n: $a = 0                    // wrap


Q queue 8
for X [10 20 30 40 50]: Q.push X

say "pop in order: [Q.pop] [Q.pop] [Q.pop]"
Q.push 60
Q.push 70
say "after more pushes, drain:"
till Q.end: say "  [Q.pop]"


// ----------------------------------------------------------------
// 3. Both types nested in one expression. PRNG drives a queue
//    whose contents we drain in arrival order.
// ----------------------------------------------------------------
say ""
say "10 random rolls flowing through a queue:"
G prng
G.seed = 99
QQ queue 16
times I 10: QQ.push (G.n 100)

till QQ.end: say_ " [QQ.pop]"
say ""

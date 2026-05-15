// 26-huffman.s -- Huffman codes for a string
//
// Two implementations of the same algorithm, side by side.
// Doubles as a stress test for the macro/compiler -- any botched
// modification tends to break this file first.
//
//   1. Readable walk-through (~80 lines): a sorted-list priority
//      queue, named helpers, named tree-walk function.
//   2. Dense one-liner-per-step variant (5 working lines): uses
//      the heap module (`use heap //heap.s`) and the `X(...)`
//      paren-pattern lambda for the tree walk.
//
// Demonstrates:
//
//   * Auto-closure `~D.?+` -- the famous one-liner frequency
//     counter: implicitly declared table D, increment entry for
//     the current char `?`, return D.
//   * Repeated extraction of the two smallest items to build the
//     tree bottom-up. First version uses a sorted list as the
//     priority queue; second uses heap.s for an O(log n) push/pop.
//   * Recursive tree walk with an accumulator of bits.
//   * Decorate-sort-undecorate for deterministic ordering when
//     keys would otherwise collide.
//
// Run:  symta -f examples/26-huffman.s

use heap //heap.s


S "osteoclasts undergo apoptosis at the end of the bone resorption phase"


// ----------------------------------------------------------------
// 1. Count character frequencies.
// ----------------------------------------------------------------
Freqs S{~D.?+}
say "freq table size: [Freqs.n]"


// ----------------------------------------------------------------
// 2. Build the Huffman tree.
//
// Start with one [freq char] leaf per character. Repeatedly pop
// the two smallest-freq nodes and replace them with a single
// internal node whose freq is the sum and whose value is the
// 2-list [left right]. The final remaining node is the root.
//
// Decorate with a 2-key sort tuple so equal-frequency leaves are
// ordered predictably (lex-compare on [freq, textual-rendering]).
// ----------------------------------------------------------------
key X = [X.0 "[X.1]"]
sort_pool Xs = Xs{X => [key X X]}.s(?.0 < ??.0){X => X.1}

Pool map K,V Freqs: [V K]
Pool = sort_pool Pool

till Pool.n < 2:
  [Fa A] Pool.0
  [Fb B] Pool.1
  Pool =: [Fa+Fb [A B]] @Pool.drop(2)
  Pool = sort_pool Pool

Tree Pool.0.1


// ----------------------------------------------------------------
// 3. Walk the tree to derive a bit string for each leaf.
//    Leaves are single characters; internal nodes are 2-lists.
// ----------------------------------------------------------------
walk Node Prefix Acc =
  if Node.is_list
  then | Acc = walk Node.0 [@Prefix 0] Acc
       | walk Node.1 [@Prefix 1] Acc
  else [@Acc [Node Prefix]]

Codes walk Tree [] []
say "total codes: [Codes.n]"


// ----------------------------------------------------------------
// 4. Print, shortest codes first (ties broken by character).
//    Decorate-sort-undecorate: pair each entry with a sort key,
//    sort by that key, then strip it back off.
// ----------------------------------------------------------------
SortedCodes Codes{X => [[X.1.n X.0] X]}.s(?.0 < ??.0){X => X.1}
say "first 8 codes (shortest):"
for [Ch Bits] SortedCodes.take(8):
  Joined Bits{X => "[X]"}.text
  say "  '[Ch]' -> [Joined]"


// ================================================================
// Compact variant -- same algorithm in 5 lines of body.
//
//   H heap; S{~D.?+}{|H.push@?f}     count + heap-push in one pass
//   (H.n-1){2{H.pop:}(A,B C,D=...)}   merge two smallest, repeat
//   C [[]H.pop.1](:D,[L R] = ...)     unroll tree to (char, bits)
//
// Each `{}` and `(...)` macro stays heavy in the macroexpander
// and compiler, so this file failing usually means a regression
// in one of those.
// ================================================================
S2 "osteoclasts undergo apoptosis at the end of the bone resorption phase"
H heap; S2{~D.?+}{|H.push@?f}
(H.n-1){2{H.pop:}(A,B C,D=H.push A+C B,D):}
C2 [[]H.pop.1](:D,[L R] = @[0,@D L]^r @[1,@D R]^r; D,C=:C,D)

// Heap rotations are randomised (heap.s: `1.rand`-driven meld),
// so equal-priority merges happen in arbitrary order.  For
// characters with the same frequency the codelen can come back
// as either 3 or 4 depending on which side of the tree they
// landed on (the algorithm guarantees the *multiset* of lengths
// is optimal, not the per-character assignment).  Print the
// sorted multiset of codelens so the regression diff stays
// reproducible while still exercising the whole pipeline.
say "compact variant -- codelens sorted ascending:"
say C2.map([_ Bits] => Bits.n).s
say "total compressed bits: [C2{[Ch Bits] => Bits.n*S2.cnt^Ch}{&~s+?}]"

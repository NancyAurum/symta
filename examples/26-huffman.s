// 26-huffman.s -- Huffman codes for a string
//
// Builds an optimal prefix-code tree for the characters of a
// fixed string, then walks the tree to derive each character's
// bit code. Demonstrates:
//
//   * Auto-closure `~D.?+` -- the famous one-liner frequency
//     counter: implicitly declared table D, increment entry for
//     the current char `?`, return D.
//   * Repeated extraction of the two smallest items to build the
//     tree bottom-up. We use a sorted list as the priority queue
//     so the example stays a single file; for production code,
//     symta/src/heap.s gives an O(log n) version.
//   * Recursive tree walk with an accumulator of bits.
//   * Decorate-sort-undecorate for deterministic ordering when
//     keys would otherwise collide.
//
// Run:  symta -f examples/26-huffman.s

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

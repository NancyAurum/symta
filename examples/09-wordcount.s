// 09-wordcount.s -- a small program tying it all together.
//
// Tokenises a paragraph, builds a frequency table, and prints the
// words sorted by count (descending), then alphabetically on ties.
//
// Demonstrates: hash tables, the {} map operator, sorting with a
// comparator lambda, pattern destructuring in `for`.
//
// Run:  symta -f examples/09-wordcount.s

Text "
the quick brown fox jumps over the lazy dog
the dog barks at the fox the fox runs away
the quick brown fox jumps again the lazy dog yawns
"

// Replace newlines and punctuation with spaces, then split on
// spaces, then drop empty tokens.
Cleaned Text.l{\`\n`+\`,`+\`.`+\`!`+\`?`=\` `}
Tokens Cleaned.text.split^' '
Words Tokens.keep | T => T.n > 0
say "found [Words.n] words"

// Frequency table. `~D` is an auto-closure hashtable implicitly
// returned by the {} map; `~D.?+` looks up the word and
// post-increments its count.
Freq Words{~D.?+}

// Convert the table to a list of (word count) pairs.
Pairs Freq.l

// Sort: by count desc, then word asc as tiebreaker. The `??` is
// the second arg passed to the comparator.
//   ?  -- left  pair  [W1 N1]
//   ?? -- right pair  [W2 N2]
cmp A B =
  [W1 N1] A
  [W2 N2] B
  if N1 >< N2: W1 < W2 else N1 > N2
Sorted Pairs.s | cmp ? ??

// Print the top 7 as a tiny histogram. `\x..Cnt` builds a list
// of `Cnt` copies of `x`; `.text` joins single-char items.
say "top words:"
Top Sorted[:7]
for [Word Cnt] Top:                  // destructure the pair
  Bar \x..Cnt
  say "  [Word]   [Cnt]  [Bar.text]"

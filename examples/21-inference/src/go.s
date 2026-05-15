// 21-inference -- Prolog-style two-way inference
//
// Symta makes a forward + backward inference engine fit in roughly
// 15 lines. Each `fact A V B` records both directions so a single
// table-of-tables can answer either "what does A V?" or "who V's B?".
// A small recursive `chain` walks transitive `is`-edges so type
// taxonomies fall out for free.
//
// This is the canonical "why Symta exists" demo, adapted from the
// Relation Oriented Programming chapter of dev/sbe.md.
//
// Run:  symta examples/21-inference && examples/21-inference/go.exe

use cls


// ----------------------------------------------------------------
// 1. Knowledge base. `KB.<verb>` is a table {Sbj -> [Obj ...]}.
//    `KB."<verb>_by"` is its mirror image used for backward queries.
// ----------------------------------------------------------------
KB!

push_edge Tbl K V =
  if no Tbl.K: Tbl.K = []
  Tbl.K = [@Tbl.K V]

ensure Verb =
  if no KB.Verb: KB.Verb = (!)
  KB.Verb

fact Sbj Verb Obj =
  push_edge (ensure Verb) Sbj Obj
  push_edge (ensure "[Verb]_by") Obj Sbj


// ----------------------------------------------------------------
// 2. Querying.
// ----------------------------------------------------------------

// direct facts only.
direct Sbj Verb =
  if no KB.Verb: ret []
  Hits KB.Verb.Sbj
  if no Hits: ret []
  Hits

// `is` and `is_by` are the two transitive verbs (taxonomic chains).
is_transitive Verb = Verb >< 'is' or Verb >< 'is_by'

// transitive closure: collect direct hits AND every node reachable
// through an `is`-chain from each of them.
chain Sbj Verb =
  Direct direct Sbj Verb
  if not is_transitive(Verb): ret Direct
  Inner map N Direct: chain N Verb
  [@Direct @Inner.j].uniq


// ----------------------------------------------------------------
// 3. Pretty-printing wrappers. The bare-symbol arg style means
//    callers write `what nancy is` -- no quotes, no parens.
// ----------------------------------------------------------------
what Sbj Verb =
  Ans (chain Sbj Verb).text(', ')
  if Ans >< '': Ans = '(nothing known)'
  say "  [Sbj] [Verb]: [Ans]"

who Verb Obj =
  Ans (chain Obj "[Verb]_by").text(', ')
  if Ans >< '': Ans = '(nobody)'
  say "  [Verb] [Obj]: [Ans]"


// ----------------------------------------------------------------
// 4. The world. Three taxonomic chains and a couple of plain verbs.
// ----------------------------------------------------------------
fact socrates is philosopher
fact philosopher is human
fact human is mortal
fact mortal is finite

fact bernd is hater
fact hater is stupid

fact nancy is lisper
fact lisper is smart
fact nancy is 'sex_worker'

fact nancy owns guitar
fact nancy owns cat
fact bernd owns laptop


// ----------------------------------------------------------------
// 5. Forward queries: walk the chain.
// ----------------------------------------------------------------
say "--- forward: what is X? ---"
what socrates is        // -> philosopher, human, mortal, finite
what nancy is           // -> lisper, smart, sex_worker
what bernd is           // -> hater, stupid
say ""

say "--- forward: ordinary verbs ---"
what nancy owns         // -> guitar, cat
what bernd owns         // -> laptop
what nancy hugs         // -> (nothing known)
say ""


// ----------------------------------------------------------------
// 6. Backward queries: same machinery, mirror table.
// ----------------------------------------------------------------
say "--- backward: who is X? ---"
who is mortal           // -> socrates, philosopher, human (transitive)
who is smart            // -> nancy, lisper
who is hater            // -> bernd
say ""

say "--- backward: ordinary verbs ---"
who owns guitar         // -> nancy
who owns laptop         // -> bernd
who owns yacht          // -> (nobody)
say ""


// ----------------------------------------------------------------
// 7. Adding a fact mid-program is just another `fact` call;
//    subsequent queries see it instantly.
// ----------------------------------------------------------------
say "--- mid-program update ---"
fact nancy owns yacht
who owns yacht          // -> nancy
fact yacht is luxury
fact luxury is expensive
what nancy owns         // -> guitar, cat, yacht
//    ...and yacht's transitive `is` chain is independent of `owns`.
what yacht is           // -> luxury, expensive

// 27-relational.s -- relational programming with simple primitives
//
// Symta's full `cls`/`rcls` macros (in symta/src/cls.s) build a
// proper entity-component-system on top of the runtime. For a
// self-contained example we sketch the same idea with just two
// hash tables: one for forward relations (subject -> [object...])
// and one for backward (object -> [subject...]). The query
// surface is tiny -- `fact`, `who`, `what` -- yet expressive
// enough for the chaining and inversion at the heart of the
// "relation-oriented programming" chapter of sbe.txt.
//
// Run:  symta -f examples/27-relational.s


// ----------------------------------------------------------------
// 1. Storage and the three operators.
// ----------------------------------------------------------------
Fwd !            // (subject, verb) -> [object1 object2 ...]
Bwd !            // (object,  verb) -> [subject1 subject2 ...]

key S V = "[S]/[V]"

// `tbl.X or default` returns `No` (the missing-key marker) AS-IS
// because `No` is truthy in Symta. We need `got` to distinguish
// "I have a real list" from "this key was never set".
fact S V O =
  K_fwd key S V
  K_bwd key O V
  Cur_fwd Fwd.K_fwd
  Cur_bwd Bwd.K_bwd
  when no Cur_fwd: Cur_fwd = []
  when no Cur_bwd: Cur_bwd = []
  Fwd.K_fwd = [@Cur_fwd O]
  Bwd.K_bwd = [@Cur_bwd S]
  No

what S V =
  R Fwd.(key S V)
  if got R then R else []
who V O =
  R Bwd.(key O V)
  if got R then R else []


// ----------------------------------------------------------------
// 2. Populate. Mirrors the example from sbe.txt's
//    "Relation Oriented Programming" section.
// ----------------------------------------------------------------
fact \alice      is \programmer
fact \bob        is \musician
fact \programmer is \human
fact \musician   is \human
fact \human      is \mortal
fact \alice      wrote \quicksort
fact \alice      wrote \essay
fact \bob        wrote \sonata


// ----------------------------------------------------------------
// 3. Direct queries (no inference yet).
// ----------------------------------------------------------------
say "what does alice do?     [what \alice is]"
say "what did alice write?   [what \alice wrote]"
say "who wrote sonata?       [who \wrote \sonata]"
say "who is directly human?  [who \is \human]"
say ""


// ----------------------------------------------------------------
// 4. Inference: chase `is` chains transitively.
//    alice is programmer, programmer is human, human is mortal
//    =>  alice is mortal
// ----------------------------------------------------------------
infer Sbj Verb =
  Direct what Sbj Verb
  Transitive []
  for O Direct:
    Transitive =: @Transitive @(infer O Verb)
  // De-duplicate while preserving first-seen order.
  Seen !
  Out []
  for X [@Direct @Transitive]:
    when no Seen.X:
      Seen.X = 1
      Out =: @Out X
  Out

say "alice is (closure): [infer \alice \is]"
say "bob   is (closure): [infer \bob   \is]"
say ""


// ----------------------------------------------------------------
// 5. The inverse query, `who is X`, transitively.
//    who is mortal => human, programmer, musician, alice, bob
// ----------------------------------------------------------------
infer_back Verb Obj =
  Direct who Verb Obj
  Transitive []
  for S Direct:
    Transitive =: @Transitive @(infer_back Verb S)
  Seen !
  Out []
  for X [@Direct @Transitive]:
    when no Seen.X:
      Seen.X = 1
      Out =: @Out X
  Out

say "who is mortal (closure): [infer_back \is \mortal]"
say "who is human  (closure): [infer_back \is \human]"

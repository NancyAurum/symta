// 18-tables.s -- hash tables in depth
//
// Symta tables look like Lua/JS objects but are first-class values
// with their own literal syntax. The `@t: ...` form turns any list
// of `[key value]` pairs (or `key,value` pairs) into a table.
//
// Run:  symta -f examples/18-tables.s


// ----------------------------------------------------------------
// 1. Three ways to build a table.
// ----------------------------------------------------------------
T1 name!\Nancy age!37 city!\Amsterdam              // literal form
say "literal:    [T1]"

T2!                                                 // empty
T2.lat = 52.37
T2.lon = 4.89
say "by .field:  [T2]"

// Comprehension: turn a list of pairs into a table.
Codes [\fr 33  \de 49  \nl 31  \de 49]              // duplicates allowed
T3 @t: map [Cc Dial] Codes.group^2: Cc,Dial         // last wins on dup
say "from list:  [T3]"
say ""


// ----------------------------------------------------------------
// 2. Querying. `.has` checks the key, `.got` checks the value
//    (true unless No), `.l` returns a list of [key value] pairs.
// ----------------------------------------------------------------
say "has age?    [T1.has age]"
say "has email?  [T1.has email]"
say "got age?    [T1.got age]"
say "T1.l =      [T1.l]"
say ""


// ----------------------------------------------------------------
// 3. Iteration. `for K,V T:` walks the table. Order is *not*
//    specified -- Symta tables are unordered hash maps. If you
//    need deterministic output, sort the pairs first (see
//    example 6 below for the top-N pattern).
// ----------------------------------------------------------------
say "fields:"
for [K V] T1.l.s: say "  [K] = [V]"   // sort for stable output
say ""


// ----------------------------------------------------------------
// 4. Mutate / delete. Assigning to a missing key adds it; assigning
//    No removes nothing -- use `.del` for that.
// ----------------------------------------------------------------
T4 a!1 b!2 c!3
T4.d = 4                                            // add
T4.b = 20                                           // update
T4.del a                                            // remove
say "after edits: [T4]"
say ""


// ----------------------------------------------------------------
// 5. Sets via `.bag`. Turn a list into a membership table.
// ----------------------------------------------------------------
Stops "is the of and a in to".split^' '
StopBag Stops.bag                                    // -> @{is!1 the!1 ...}
say "stop?(of)  = [StopBag.has of]"
say "stop?(cat) = [StopBag.has cat]"
say ""


// ----------------------------------------------------------------
// 6. Top-N pattern: count, list-ify, sort, slice.
// ----------------------------------------------------------------
Words "the quick brown fox the lazy dog the quick fox".split^' '
Freq Words{~D.?+}                                   // {} returns ~D
Pairs Freq.l
// Sort by count descending. We add an alphabetical tie-breaker so
// words with equal counts ("quick" and "fox" both at 2) come out
// in a fixed order rather than whatever hash-iteration produces --
// otherwise the top-N print is non-deterministic across runs.
Sorted Pairs.s | ?.1 > ??.1 or (?.1 >< ??.1 and ?.0 < ??.0)
say "top 3 words:"
for [W N] Sorted[:3]: say "  [W]: [N]"
say ""


// ----------------------------------------------------------------
// 7. Tables of tables.
// ----------------------------------------------------------------
People @t: map [N Age City] [[\Nancy 37 \Amsterdam] [\Bernd 26 \Berlin]]:
  N,(age!Age city!City)

say "Nancy's age:  [People.\Nancy.age]"
say "Bernd's city: [People.\Bernd.city]"

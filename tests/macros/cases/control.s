// control.s -- exercises control-flow macros.
// Run with:  symta -f tests-macros/cases/control.s

// --- 1. if/then/else ---
say: if 1 > 0 then "yes" else "no"
say: if 0 > 1 then "yes" else "no"
say: if 5 then "truthy" else "falsy"
say: if No then "truthy" else "falsy"

// --- 2. if with `:` body separator (block style) ---
go1 X =
  if X > 0:
    say "pos [X]"

go1 5

// --- 3. when (= if-without-else, falls through) ---
go2 X =
  when X > 0: say "saw positive [X]"
  when X < 0: say "saw negative [X]"
  when X >< 0: say "saw zero"

go2 5
go2 -3
go2 0

// --- 4. less (= unless / when-not) ---
go3 X =
  less X > 10: say "not big: [X]"

go3 5
go3 99

// --- 5. and / or short-circuit (binary infix) ---
say: 1 and 2                // both truthy -> 2
say: 1 and 0                // falsy stops it -> 0
say: 0 or  7                // first truthy -> 7
say: 0 or  0                // both falsy   -> 0

// --- 6. while loop ---
N 3
While_log:
while N > 0:
  push N While_log
  N = N - 1
say "while gave: [While_log]"

// --- 7. till loop (loop until cond) ---
M 0
Till_log:
till M >> 3:
  push M Till_log
  M = M + 1
say "till gave: [Till_log]"

// --- 8. for over a list ---
For_log:
for X [10 20 30]:
  push X+1 For_log
say "for gave: [For_log.f]"

// --- 9. for with index unpacking K,V ---
KV_log:
for K,V [\a \b \c].i:
  push "[K]:[V]" KV_log
say "for kv: [KV_log.f]"

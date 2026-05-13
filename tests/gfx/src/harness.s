// Golden-image test harness for the gfx C library.
//
// Each test is a function `Name body` that produces a `gfx` and returns it.
// `register Name BuilderFn` queues it. `run_all_cases` runs every registered
// test, writes the produced PNG to tests/out/<name>.png, and compares against
// tests/golden/<name>.png if it exists. On first run there is no golden, so
// every test reports "NEW" and the user is expected to inspect the .png files
// in tests/out/ and copy the good ones into tests/golden/ to promote them.

use gfx
export register run_all_cases gold_dir out_dir

Tests []

register Name Fn = Tests =: @Tests [Name Fn]

gold_dir = "golden/"
out_dir  = "out/"

// pixel-diff two same-size gfxes; returns [N_pixels_differing, max_channel_delta].
// allows a per-channel tolerance to soak rounding noise from non-deterministic
// FP paths (we use 0 for exact-match tests).
diff_gfxes A B Tolerance =
  W A.w
  H A.h
  less W >< B.w and H >< B.h: ret [-1 -1] //size mismatch sentinel
  Ndiff 0
  MaxD 0
  times Y H: times X W:
    AP A.get X Y
    BP B.get X Y
    when AP <> BP:
      DR (AP ->- 16) -*- 0xFF
      DR -= (BP ->- 16) -*- 0xFF
      DR = DR.abs
      DG (AP ->- 8) -*- 0xFF
      DG -= (BP ->- 8) -*- 0xFF
      DG = DG.abs
      DB AP -*- 0xFF
      DB -= BP -*- 0xFF
      DB = DB.abs
      DA (AP ->- 24) -*- 0xFF
      DA -= (BP ->- 24) -*- 0xFF
      DA = DA.abs
      M max (max DR DG) (max DB DA)
      when M > Tolerance:
        Ndiff+
        if M > MaxD: MaxD = M
  [Ndiff MaxD]


run_one Name Fn Tolerance =
  say "  - [Name]"
  Out "[out_dir][Name].png"
  Gold "[gold_dir][Name].png"
  Produced Fn()
  less Produced.is_gfx:
    say "    FAIL: test did not return a gfx (got [Produced])"
    ret \fail
  Produced.save Out
  less Gold.exists:
    say "    NEW: [Out]; copy to [Gold] to promote"
    ret \new
  Golden gfx Gold
  Diffs diff_gfxes Produced Golden Tolerance
  Ndiff Diffs.0
  MaxD Diffs.1
  Produced.free
  Golden.free
  if Ndiff < 0:
    say "    FAIL: size mismatch (produced vs golden)"
    ret \fail
  if Ndiff > 0:
    say "    FAIL: [Ndiff] pixels differ, max channel delta=[MaxD]"
    ret \fail
  \pass


run_all_cases =
  // Ensure out/ exists -- `make clean` nukes it. out_dir is a 0-arg
  // function (lowercase name); call it first to get the string.
  OD out_dir()
  less OD.exists: OD.mkpath
  Pass 0
  Fail 0
  New 0
  for [Name Fn] Tests:
    R run_one Name Fn 0
    case R:
      pass = Pass+
      new = New+
      fail = Fail+
  say ""
  say "Summary: [Pass] passed, [Fail] failed, [New] new (review and promote)"
  if Fail > 0: bad "tests failed"
  0

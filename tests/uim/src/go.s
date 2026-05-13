// UIM widget regression harness.
//
// One Symta project that hosts every UIM test case. Each case is a
// `cases` entry that builds a widget tree and renders into a 60-frame
// window in headless screenshot mode. The shell driver in
// ../run.sh selects a case via `--case=NAME` and compares the PNG it
// produces with a golden in ../baselines/.
//
// Adding a new case: write a function `tc_<name> W H` that calls
// `uim W H Title!".." TestUI`, then append it to the dispatch table
// below. The shell driver picks the case name up via the same lookup.
//
// CLI flags consumed in this file (in addition to UIM's own
// --screenshot* args):
//   --case=NAME    which test to run; default: gallery640
//   --w=N --h=N    override window WxH (otherwise per-case default)

use cls uim gfx rgb store
    tc_buttons tc_inputs tc_lists tc_gallery tc_isometric tc_synthetic
    tc_windows

// Defaults — overridable on the command line.
RunCase \gallery640
ArgW No
ArgH No
ArgPngcmpA No
ArgPngcmpB No

for A main_args():
  when A.is_text:
    if: A.begin '--case='     = RunCase    = "[A.drop 7]"
        A.begin '--w='        = ArgW       = (A.drop 4).int
        A.begin '--h='        = ArgH       = (A.drop 4).int
        A.begin '--pngcmp-a=' = ArgPngcmpA = "[A.drop 11]"
        A.begin '--pngcmp-b=' = ArgPngcmpB = "[A.drop 11]"
        1 = 0

// --pngcmp=A,B short-circuits the test harness: load both PNGs via
// the gfx plugin and compare pixel-for-pixel. Prints `MATCH` to
// stdout when the decoded pixels are identical, `DIFF ...` otherwise.
// This lets `tests/uim/run.sh` compare screenshots by CONTENT instead
// of by the raw PNG bytestream, which varies harmlessly with the
// libpng version (zlib chunk choice, filter selection, ancillary
// chunk ordering, etc.) -- the pixels are what we're actually testing.
when got ArgPngcmpA:
  // Per-channel tolerance. Cross-platform we've seen up to ~20 per
  // channel on text AA fringes (FreeType version differences) and on
  // title-bar / window-chrome backgrounds (SDL2 default theme
  // varies). 32 (~12.5% of full range) absorbs every benign
  // platform drift we've measured so far while still catching real
  // regressions -- a wrong-colour widget would show ±100+. Past
  // tolerance the test reports the worst offending pixel so the
  // first deltas of a real change are still easy to investigate.
  Tolerance 32
  GA gfx ArgPngcmpA
  GB gfx ArgPngcmpB
  if GA.w <> GB.w or GA.h <> GB.h:
    say "DIFF size [GA.w]x[GA.h] vs [GB.w]x[GB.h]"
  else
    Same 1
    MaxDelta 0
    DiffPixel No
    for Y GA.h: for X GA.w:
      when Same:
        PA GA.get(X Y)
        PB GB.get(X Y)
        // Compare channels independently; max(|chan_a - chan_b|).
        // gfx packs RGBA as 0xAABBGGRR (Symta low byte = R).
        // `-*-` is bitwise AND, `->-` is shift-right in Symta.
        DR (PA       -*- 0xFF) - (PB       -*- 0xFF)
        DG (PA ->-  8 -*- 0xFF) - (PB ->-  8 -*- 0xFF)
        DB (PA ->- 16 -*- 0xFF) - (PB ->- 16 -*- 0xFF)
        DA (PA ->- 24 -*- 0xFF) - (PB ->- 24 -*- 0xFF)
        D max DR.abs DG.abs DB.abs DA.abs
        when D > MaxDelta: MaxDelta = D
        when D > Tolerance:
          Same = 0
          DiffPixel =: X Y
    if Same
      then say "MATCH (max channel delta=[MaxDelta])"
      else say "DIFF pixel [DiffPixel.0],[DiffPixel.1]: [GA.get(DiffPixel.0 DiffPixel.1).x] vs [GB.get(DiffPixel.0 DiffPixel.1).x] (tolerance=[Tolerance])"
  halt

W ArgW(No=640)
H ArgH(No=480)

case RunCase:
  'buttons'    = tc_buttons    W H
  'inputs'     = tc_inputs     W H
  'lists'      = tc_lists      W H
  'gallery'    = tc_gallery    W H
  'gallery640' = tc_gallery    640 480
  'gallery800' = tc_gallery    800 600
  'isometric'  = tc_isometric  W H
  'synthetic'  = tc_synthetic  W H
  'windows'    = tc_windows    W H
  Else         = say "no such case: [RunCase]"

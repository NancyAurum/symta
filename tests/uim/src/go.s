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

use cls uim rgb store
    tc_buttons tc_inputs tc_lists tc_gallery tc_isometric tc_synthetic
    tc_windows

// Defaults — overridable on the command line.
RunCase \gallery640
ArgW No
ArgH No

for A main_args():
  when A.is_text:
    if: A.begin '--case=' = RunCase = "[A.drop 7]"
        A.begin '--w='    = ArgW    = (A.drop 4).int
        A.begin '--h='    = ArgH    = (A.drop 4).int
        1 = 0

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

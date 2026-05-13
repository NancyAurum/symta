// Synthetic-event test case.
//
// Drives widget state programmatically via uim.tick_fn instead of
// going through SDL's input queue: a per-frame hook pokes the
// widgets directly at frame 30, then the rendered scene at the
// screenshot frame reflects the "after interaction" state. The
// byte-equality check pins both the drawing pipeline and the
// activation handlers in one shot.

use cls uim rgb prof

export tc_synthetic

// Frame at which we poke widget state. Anything < screenshot-frame.
PokeFrame 30

// Mutable handles populated by tc_synthetic; harness_tick reads
// them at PokeFrame.
HCb        [0]
HSld       [0]
HClicks    [0]

harness_tick UIM =
  when UIM.frame >< PokeFrame:
    Cb HCb.0
    Sld HSld.0
    Cb.press                  // toggle: 0 -> 1
    Sld.val_ = 0.7            // direct field write — the field is mutable
    HClicks.0 = HClicks.0 + 1
    'out_lbl'.upd
      "after:  cb=[Cb.val] slider=[Sld.val_] clicks=[HClicks.0]"

tc_synthetic W H =
  Cb chkbx 0 (_ =>)
  Sld slider W!200 H!18 val!0.0
  Btn btn "click me"
  Out ftx Id!out_lbl "before: cb=0  slider=0.0  clicks=0" color!RGB_WHITE
  HCb.0 = Cb
  HSld.0 = Sld
  HClicks.0 = 0

  TestUI bg 0x101820 Add!:rowz:
    ftx "synthetic-event drive: state at frame [PokeFrame]" color!RGB_YELLOW
    Cb
    Sld
    Btn
    Out

  prof_register_tick_fn &harness_tick
  uim W H Title!"UIM test: synthetic events" TestUI

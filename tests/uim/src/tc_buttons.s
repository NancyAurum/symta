// Button-family widgets: btn, chkbx, knob.
//
// Each widget appears on its own row in the rowz: block; the row
// macro stacks them vertically with default padding. This is enough
// to exercise the button/checkbox/knob draw paths and to catch any
// regression in the glyph pipeline that feeds them.

use cls uim rgb

export tc_buttons

tc_buttons W H =
  TestUI bg 0x10101a Add!:rowz:
    ftx "btn — three different widths" color!RGB_WHITE
    btn "Short"            W!60  H!28
    btn "Medium"           W!100 H!28
    btn "Long caption"     W!200 H!28

    ftx "btn — colored captions via inline tags" color!RGB_WHITE
    btn "<%yellow:Yel>" W!80 H!40
    btn "<%pink:Pink>"  W!80 H!40
    btn "<%cyan:Cyan>"  W!80 H!40

    ftx "chkbx + knob" color!RGB_WHITE
    chkbx 1 (_ =>)
    chkbx 0 (_ =>)
    chkbx 1 (_ =>)
    knob

  uim W H Title!"UIM test: buttons" TestUI

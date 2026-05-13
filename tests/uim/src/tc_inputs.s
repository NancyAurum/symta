// Slider + text-input widgets.

use cls uim rgb

export tc_inputs

tc_inputs W H =
  TestUI bg 0x101820 Add!:rowz:
    ftx "horizontal sliders" color!RGB_WHITE
    slider W!200 H!18 val!0.0
    slider W!200 H!18 val!0.5
    slider W!200 H!18 val!1.0

    ftx "knob" color!RGB_WHITE
    knob

    ftx "edln text input" color!RGB_WHITE
    edln W!280

    ftx "edbx multiline editor" color!RGB_WHITE
    edbx W!280 H!80 "first line\nsecond line\nthird line"

  uim W H Title!"UIM test: inputs" TestUI

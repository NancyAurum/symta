// Mixed widget gallery — a denser screen exercising more code paths
// per frame. Used for the 640x480 and 800x600 resolution variants.

use cls uim rgb

export tc_gallery

tc_gallery W H =
  TestUI bg 0x101020 Add!:rowz:
    ftx O!abs X!10 Y!10 color!RGB_YELLOW
        "<b:Symta UIM widget gallery — [W]x[H]>"

    // Left column: buttons stacked.
    [10 40 100 24],"Action"
    [10 70 100 24],"Cancel"
    [10 100 100 24],"Help"

    // Middle column: sliders.
    slider X!130 Y!40  W!120 H!18 val!0.25
    slider X!130 Y!70  W!120 H!18 val!0.50
    slider X!130 Y!100 W!120 H!18 val!0.75

    // Right column: checkboxes + a knob.
    chkbx 1 X!270 Y!42
    chkbx 0 X!270 Y!72
    chkbx 1 X!270 Y!102
    knob    X!310 Y!42

    // Bottom strip: tabs with an lbox / wrapped text per tab.
    htabs l0:
      'Items',l0  ! lbox 6 ['Sword' 'Shield' 'Potion' 'Bow' 'Ring' 'Staff' 'Tome']
      'Info',l1   ! ftx "<%white:Symta UIM at [W]x[H]. Resize-friendly.>"
      'About',l2  ! ftx "<%cyan:Built from cls uim. ~30 widget classes; same gfx blit pipeline.>"

  uim W H Title!"UIM gallery [W]x[H]" TestUI

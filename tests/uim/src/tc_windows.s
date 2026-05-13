// Windowing-family widgets: lay grid, drpl (droplist), wnd
// (draggable window), fsview (filesystem browser).
//
// Adapted from the legacy game/src/ui_test2.s / ui_test3.s pair,
// which themselves diverged from this code only in their export
// names — the rendered widget tree is identical. Folding both into
// a single test case here removes the duplication and pulls the
// fsview / drpl / wnd surface area into the regression net.

use cls uim rgb store

export tc_windows

tc_windows W H =
  TestUI bg RGB_BLACK Add!:rowz:
    // 5x2 grid of hourglass pictograms via `lay` (positional layout).
    lay 5 o!nl N!5,10 X!10 Y!20: dup 10: pic 'test/hourglass'

    // Droplist with three labelled options; default-picked is item 1.
    drpl X!50 Y!50 o!base ['Item 0',123 'Item 1',456 'Item 2',789]
         Act!|X => say picked X

    // File-load dialog around a filesystem view. The Path field
    // anchors fsview at ../tokenizer/cases/ — checked-in source
    // files that never mutate. That keeps the rendered listing
    // identical across runs so the byte-equality diff stays
    // meaningful (vs the default `main_path` which is the build
    // dir and varies with every clean).
    wnd id!fileDlg "File Dialog":rowz:
      fsview s Path!"../tokenizer/cases/" o!nl lock!1 Act!: X =>
        say file is X

    // Draggable window with a body label and a Close button. The
    // `@hide` / `@wset` setup mirrors the original — it's how the
    // outer window registers its own id (`wnd1`) so the Close button
    // can address it by name.
    @hide: @wset abc 123: wnd X!10 Y!10 id!wnd1 "DRAG ME!!!":rowz:
      ftx align!c | => "['wnd1'.q(No=0,0;W=W.xy)]"
      btn o!nl align!c "Close"| => 'wnd1'.xhide

  uim W H Title!"UIM test: windows" TestUI

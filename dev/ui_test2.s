use cls uim rgb store
export run_ui_test2


TestUI bg RGB_BLACK Add!:rowz:
  lay 5 o!nl N!5,10 X!10 Y!20: dup 10: pic 'test/hourglass'
  drpl X!50 Y!50 o!base ['Item 0',123 'Item 1',456 'Item 2',789]
       Act!|X => say picked X
  wnd id!fileDlg "File Dialog":rowz:
    fsview s root!"C:/" o!nl  lock!1 Act!: X =>
      say file is X
      if no X: 'wnd1'.xshow
  @hide: @wset abc 123: wnd X!10 Y!10 id!wnd1 "DRAG ME!!!":rowz:
    ftx align!c | => "['wnd1'.q(No=0,0;W=W.xy)]"
    btn o!nl align!c "Close"| => 'wnd1'.xhide

run_ui_test2 = uim 640 480 Title!"Symta UI Example 2" TestUI

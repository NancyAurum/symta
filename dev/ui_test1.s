use cls uim rgb
export run_ui_test1

Text "</flow y!5 x!5 w!5>Hey, <u:you<s><s><s> <%pink:sissy>>! Welcome to the <%blue:component oriented UI>. <pic `test/tiger` y!-5 h!50>
      This is a <b:multiline formatted> text <i:by the way!>
      Lorem ipsum dolor sit amet, <pic `test/hourglass` h!24> consectetur adipiscing elit, <%gray:sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud </hg> exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. <n> Duis <%white|b:aute <%yellow|i:irure> dolor> in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id   est laborum.> even more stuff goes here.. pad pad pad pad pad pad pad pad pad pad pad pad pad pad pad pad!! HIIIIII!!!!!!! Helllo!"


TestUI bg RGB_BLACK Add!:rowz:
  t,c,[0 5],'<i:This is <u:Our Title> Text>'//title centered, and moved 5px down
  bg RGB_NIGHT O!nl Id!bg X!20 Y!20 W![s 20] H![s 20] Add!:rowz:
    ftx Text Id!out X!5 W!-5 H!1.0 Cursor!0 Wrap!base Add!:rowz:
      //bx 3 X!40 Y!20: [flow],"FLOW!",|=> flow.upd 'Text flows around me!!!'
      btn id!flow wrap!20 "FLOW!"| => | flow.upd 'Text flows around me!!!'
      //[flow],"FLOW!",|=> | flow.upd 'Text flows around me!!!'
      //r,[flow2 -50 100],"Another",|=> flow2.upd 'Yaay!'
      pic 'test/hourglass' Id!hg O!ph
  ph,[r1 5],"Right",|=> | out.upd 'Clicky-clicky!'; r1.hide
  pvh,[r2 0 5],"Right2",|=> | out.upd 'Clicked like a pro!'; r2.dim
  slider Id!sld X!4 Y!5 W!16 H!(=>max 40 bg.q(Q.h:No=0)-70)
         O!pvh act!: V => 'out'.pos = V
  [18 5],"Ok",|=> out.upd 'Good girl!'
  ph,[agree 5],"I disagree!",| => | out.upd 'We dont care!'; agree.upd 'Agree'
  lbox O!ph X!0.2 W!128 5 [item0 'abc' '<pic `test/tiger` y!-5 h!50><s>item2']
       Act!| Index => say picked Index
  htabs l0 id!tabs:
    'Tab0',l0 ! ftx "Tab 0 Content"
    'Tab1',l1 ! ftx "Tab 1 Content"
  [-18 10],'Another slider'
  slider O!ph val!0.7 act!: V => out.upd "Slider is now [V]"
  [0 10],'BIG SLIDER'
  slider val!0.3 X!5 W!0.4 H!50 act!: => out.upd "Big slider moved!!!"
  ph,[20 0 130 20],| => "Count: <%green:[uim.frame]>"
  pic 'test/hourglass' O!ph X!: => (uim.frame%100)(:?>50=100-Q)
  r,_,[-8 -20 0.25 30],"<0xFF5050:Exit> <pic `test/robot`>",| => uim.exit
  wnd id!fileDlg "File Dialog":rowz:
    fsview s root!"C:/" o!nl lock!1 Act!: X =>
      say file is X

run_ui_test1 = uim 640 480 Title!"Symta UI Example 1" TestUI

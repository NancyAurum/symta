use m edico
export menubar

type menubar_blocker.widget w h menubar

menubar_blocker.render = Me

menubar_blocker.input In =
| case In
  [mice left+right 1 P] | $menubar.close_menu

type menubar.$base Menus:
   base button!0 lay!0 blocker
   cur_lay!0 bar_w!0 bar_h!0 enabled_check!(Menu=>1)
   cur_menu closed_menu
| $blocker = hidden: menubar_blocker 800 600 Me
| MenuBs @join: map [Title @Items] Menus: $add_menu(Title Items)
| Dlg dlg w!$bar_w h!$bar_h: [[0 0 $blocker] @MenuBs]
| Dlg   input_split Dlg: Base In => $process_input(Base In)
| $base =  Dlg

menubar.add_menu Title Items =
| Lay   hidden: layV Items
| Button   edico button text!Title: Icon =>
  | when $closed_menu><Title:
    | $closed_menu =  0
    | ret  
  | less $enabled_check()(Me Title): ret  
  | $cur_menu =  Title
  | $cur_lay =  Lay
  | Lay.show =  1
  | $blocker.show =  1
| R   Button.render
| X   $bar_w
| $bar_w += R.w
| $bar_h =  R.h
| [X 0 Button],[X $bar_h Lay]

menubar.close_menu =
| $closed_menu =  $cur_menu
| $cur_menu =  0
| less $cur_lay: ret  
| $cur_lay.show =  0
| $blocker.show =  0
| $cur_lay =  0


menubar.render =
| if $cur_lay then
  | $base.w =  800
  | $base.h =  600
  else
  | $base.w =  $bar_w
  | $base.h =  $bar_h
| $base.render


menubar.process_input Base In =
| case In
  [mice left+right 0 P] | $close_menu
| Base.input(In)


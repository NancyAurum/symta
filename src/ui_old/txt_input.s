use m ui/data
export txt_input

type txt_input.widget Text w!140 nc!No state!normal:
  w!W
  h
  cycle
  text_
  state!State
  init
  on_edit
  info_fn
  nc
  edit_mode!normal
  editing
  prev_focus_widget!No
  cursor
  infoline!""
| $value =  Text
| $nc =  Nc
| when no $nc: $nc =  $w/9
txt_input.render =
| when $init <> $state
  | $h = @h: get_img "ui_litem_[$state]"
  | $init =  $state
| Me

txt_input.value = $text_

txt_input.=value Text =
| less Text.is_text: Text =  "[Text]"
| $init =  0
| $text_ =  Text
| $cursor =  Text.n

parse_int Default Text =
| when Text.end: ret   Default
| Sign   1
| when Text.0><'-':
  | Sign =  -1
  | Text =  Text.tail
  | when Text.end: ret   Default
| if Text.all(?is_digit) then Text.int*Sign else Default

txt_input.int Default = parse_int Default $value

txt_input.float Default = parse_int Default $value

txt_input.draw G PX PY =
| BG   get_img "ui_litem_[$state]"
| G.blit(PX PY BG.rect(0 0 $w-10 BG.h))
| G.blit(PX+$w-10 PY BG.rect(BG.w-10 0 10 BG.h))
| Color   if $state >< normal then 0xffffff
          else if $state >< picked then 0xf4e020
          else if $state >< disabled then 0xd5d5d5
          else 0xffffff
| Text   $value
| when $state >< picked and (clock)*60.0%60.0<30.0:
  | Text =  [Text.upto($cursor) "<cur>" Text.wout($cursor)].text
| G.text(PX+2 PY Text font!small center![No BG.h] color!Color)
| $cycle++

txt_input.done_editing =
| less $editing: ret  
| $editing =  0
| when $prev_focus_widget:
  | gui.focus_pop
  | $prev_focus_widget =  0
| $state =  \normal

txt_input.input In = case In
  [focus State P]
   | when $edit_mode><label: ret  
   | $state =  if State then \picked else \normal
   | when $editing and $state><\normal: $done_editing
  [mice left 0 P]
   | when $edit_mode><normal and not $editing:
     | $prev_focus_widget =  1
     | gui.focus_push Me
     | $editing =  1
     | $state =  \picked
  [mice right 0 P] 
   | if $edit_mode><label and not $editing then
     | $prev_focus_widget =  1
     | gui.focus_push Me
     | $editing =  1
     | $state =  \picked
     else if $editing then $done_editing
     else
  [key return 1]
   | when $editing: $done_editing
  [key left 1] | when $cursor>0: $cursor--
  [key right 1] | when $cursor<$value.n: $cursor++
  [key backspace 1]
   | when $edit_mode><label and not $editing: ret  
   | when $cursor>0:
     | T   $value
     | C   $cursor
     | $value =  [T.upto($cursor-1) T.wout($cursor)].text
     | $cursor =  C-1
     | when $on_edit(): $on_edit()($value)
  [key K<1.n 1]
   | when $edit_mode><label and not $editing: ret  
   | when $value.n < $nc:
     | C   $cursor
     | T   $value
     | $value =  [T.upto($cursor) K T.wout($cursor)].text
     | $cursor =  C+1
     | when $on_edit(): $on_edit()($value)


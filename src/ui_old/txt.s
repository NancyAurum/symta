use ui/data
export txt

type txt.widget font_name value_ bold!0 alpha!0 center!0 wrap!No
                color!No shadow!No
                :
   w!1 h!1 v font bold!Bold alpha!Alpha center!Center wrap!Wrap
   color!Color shadow!Shadow
   over on_click!0 data
| $font = get_font $font_name

txt.value =
| Text   "[if $value_.is_fn then $value_()() else $value_]"
| when got $wrap: Text =  $font.format $wrap Text
| Text

txt.update_wh Text =
| F   $font
| $w =  Text.lines()(L => F.width(L)).max
| $h =  F.height

txt.=value V =
| $value_ =  V
| $update_wh($value)

txt.render = Me

txt.draw G X Y =
| Text   $value
| $update_wh(Text)
| when $center: X += $wrap/2-$w/2
| $font.draw(G X Y Text bold!$bold alpha!$alpha color!$color shadow!$shadow)

txt.as_text = "#(txt [$value])"

txt.input In =
| case In
  [mice over S P] | $over =  S
  //[mice left 1 P] | less $disabled: $pressed <= 1
  [mice left 0 P]
  | when $over and $on_click:
    | when $on_click: $on_click()(Me)

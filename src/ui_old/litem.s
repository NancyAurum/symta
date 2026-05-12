use ui/data
export litem

type litem.widget Text w!140 state!normal:
  text_!Text w!W h state!State init

litem.render =
| when $init <> $state
  | $h =  @h: get_img "ui_litem_[$state]"
  | $init =  $state
| Me

litem.text = $text_

litem.=text Text =
| $init =  0
| $text_ =  Text

litem.draw G PX PY =
| BG   get_img "ui_litem_[$state]"
| G.blit(PX PY BG.rect(0 0 10 BG.h))
| G.blit(PX+$w-10 PY BG.rect(BG.w-10 0 10 BG.h))
| G.quad([10 0] [$w-10 0] [10 $h] [$w-10 $h])
| G.blit(PX PY BG.rect(10 0 BG.w-20 BG.h))
| Color   if $state >< normal then 0xffffff
          else if $state >< picked then 0xf4e020
          else if $state >< disabled then 0xd5d5d5
          else 0xffffff
| G.text(PX+2 PY $text_ font!small center![No BG.h] color!Color)

litem.input In = case In
  [mice left 1 P] | $state =  case $state normal\picked picked\normal X X

use ui/data
export pbar

//progress bar
type pbar.widget V: value_!V.clip(0 100) bg!No
pbar.render =
| have $bg: get_img "ui_bar_bg"
| Me
pbar.value = $value_
pbar.set_value New = $value_ = New.clip 0 100
pbar.draw G X Y =
| G.blit X Y $bg
| G.rectangle 0x347004 1 X+3 Y+3 152*$value_/100 14

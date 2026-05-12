use ui/data
export button

type button.widget Text Fn state!normal skin!medium_large:
  text!Text value on_click!Fn state_!State sprite over w h
  infoline!""
  =
| $sprite =  get_spr "ui_button_[Skin]"
| $w =  $sprite.'normal'.w
| $h =  $sprite.'normal'.h

button.state = $state_

button.=state V =
| when V><normal and $state><pressed: ret  
| $state_ =  V

button.render = Me

button.draw G PX PY =
| State   $state_
| when State >< normal and $over: State =  \over
| Sprite   $sprite
| BG   Sprite.|case State over normal Else State
| G.blit(PX PY BG)
| SF   Sprite.font
| less got SF: ret  
| ShiftX   0
| ShiftY   0
| case State pressed
  | ShiftX =  SF.5
  | ShiftY =  SF.6
| FontName   SF.| case State pressed+over 3 disabled 4 _ 2
| FontArgs   []
| less FontName.is_text: case FontName
  [FN @FAs]
    | FontArgs =  FAs
    | FontName =  FN
| Bold   FontArgs.any(bold)
| Text   $text
| when Text.is_fn: Text =  Text()
| G.text(PX+ShiftX+SF.0 PY+ShiftY+SF.1 Text
         bold!Bold font!FontName center![BG.w BG.h])

button.input In = case In
  [mice over S P] | $over =  S
  [mice left 1 P] | case $state normal: $state_ =  \pressed
  [mice left 0 P] | case $state pressed
                    | when $over: $on_click()()
                    | $state_ =  \normal
button.as_text = "#(button [$text])"

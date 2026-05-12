use ui/data
export pic

type pic.widget Src: src!Src value on_click!0 state_!normal over w!1 h!1
pic.state = $state_
pic.=state V =
| when V><normal and $state><pressed: ret  
| $state_ =  V
pic.render = Me
pic.draw G PX PY =
| Img   if $src.is_text then get_img("[$src]")
        else if $src.is_fn then ($src)(Me)
        else ret  
| $w =  Img.w
| $h =  Img.h
| G.blit(PX PY Img)
pic.input In = case In
  [mice over S P] | $over =  S
  [mice left 1 P] | case $state normal: $state_ =  \pressed
  [mice left 0 P] | case $state pressed
                    | when $over and $on_click: $on_click()()
                    | $state_ =  \normal

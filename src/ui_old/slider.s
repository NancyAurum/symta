use m ui/data
export slider

type slider_.widget Dir Size F:
     dir!Dir
     size!Size
     f!F
     pos!0.0
     state!normal
     skin!No
     w!1 h!1
     delta!0.01
| $value =  0.0
slider_.value = $pos
slider_.=value V =
| NP   V.clip(0.0 1.0)
| when $pos <> NP:
  | $pos =  NP
  | ($f)($pos)
slider_.render =
| S   get_img "ui_slider_[$dir]_normal"
| $w =  S.w
| $h =  S.h
| if $dir >< v then $h =  $size else $w =  $size
| Me
slider_.inc = $value += $delta
slider_.dec = $value -= $delta
slider_.draw G PX PY =
| BG   get_img "ui_slider_[$dir]_normal"
| K   get_img "ui_slider_knob"
| I   0
| when $dir >< v
  | while I < $size
    | G.blit(PX PY+I BG.rect(0 0 BG.w (min BG.h $size-I)))
    | I += BG.h
  | G.blit(PX+1 PY+($pos*($size-K.h).float).int K)
| when $dir >< h
  | while I < $size
    | G.blit(PX+I PY BG.rect(0 0 (min BG.w $size-I) BG.h))
    | I += BG.w
  | G.blit(PX+($pos*($size-K.w).float).int PY+1 K)
slider_.input In = case In
  [mice_move _ P] | when $state >< pressed:
                    | NP   @clip 0 $size: if $dir >< v then P.1 else P.0
                    | $value =  NP.float/$size.float
  [mice left 1 P] | when $state >< normal
                    | $state =  \pressed
                    | $input(mice_move,P,P)
  [mice left 0 P] | when $state >< pressed: $state =  \normal

type arrow.widget D Fn state!normal skin!arrow:
  direction!D on_click!Fn state!State sprite
| $sprite = get_spr "ui_[Skin]"
arrow.render = $sprite."[$direction]_[$state]"
arrow.input In = case In
  [mice left 1 P] | when $state >< normal
                    | $state =  \pressed
                    | Repeat | => when $state >< pressed
                                  | $on_click()()
                                  | 1
                    | gui.add_timer 0.25 Repeat
  [mice left 0 P] | when $state >< pressed
                    | $on_click()()
                    | $state =  \normal
arrow.as_text = "#(arrow [$direction] state![$state])"

/*slider D Sz F =
| S = No
| Dec = => S.dec
| Inc = => S.inc
| if D >< v
  then | A = arrow up Dec
       | B = arrow down Inc
       | S <= slider_ D max{0 Sz-A.render.h*2} F
       | layV A,S,B
  else | A = arrow left Dec
       | B = arrow right Inc
       | S <= slider_ D max{Sz-A.render.w*2} F
       | layH A,S,B*/

type slider.$base D Sz F: base bar
| Bar   No
| Dec | => Bar.dec
| Inc | => Bar.inc
| Lay   if D >< v
  then | A   arrow up Dec
       | B   arrow down Inc
       | Bar =  slider_ D max(0 Sz-A.render.h*2) F
       | layV A,Bar,B
  else | A   arrow left Dec
       | B   arrow right Inc
       | Bar =  slider_ D max(Sz-A.render.w*2) F
       | layH A,Bar,B
| $bar =  Bar
| $base =  Lay

slider.value = $bar.value
slider.=value V = $bar.value =  V


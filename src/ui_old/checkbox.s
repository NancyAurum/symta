use m ui/data txt
export checkbox

type checkbox_.widget State Fn skin!"checkbox":
  skin!Skin value_!State on_switch!Fn w h over pressed disabled
  infoline!""
  =
| G  get_img "ui_[$skin][$value]"
| $w =  G.w
| $h =  G.h

checkbox_.value = $value_
checkbox_.=value V =
| when V >< $value_: ret  
| V =  if V then 1 else 0
| $value_ =  V
| when $on_switch: $on_switch()(V)

checkbox_.=qvalue V =
| when V >< $value_: ret  
| V =  if V then 1 else 0
| $value_ =  V

checkbox_.render = Me

checkbox_.draw G PX PY =
| CB  get_img "ui_[$skin][$value]"
| G.blit(PX PY CB)

checkbox_.input In =
| when $disabled: ret  
| case In
  [mice over S P] | $over =  S
  [mice left 1 P] | less $disabled: $pressed =  1
  [mice left 0 P]
  | $pressed =  0
  | when $over: $value =  not $value_


cb_label_on_click Label = Label.data.switch

type checkbox.$base State Fn text!No skin!"checkbox": base cb label =
| $cb =  checkbox_ State Fn skin!Skin
| when no Text:
  | $base =  $cb
  | ret   Me
| $label =  txt medium Text
| $label.data =  Me
| $label.on_click =  &cb_label_on_click
| LabelDlg   dlg: mtx |   0   4 | $label
| $base =  layH s!4 [$cb LabelDlg]

checkbox.as_text = "#(checkbox [$value])"

checkbox.switch = $cb.value =  not $cb.value

checkbox.value = $cb.value
checkbox.=value V = $cb.value =  V

//quiet value doesnt trigger on_click
checkbox.=qvalue V = $cb.qvalue =  V

checkbox.render = $base.render

use ui/data litem
export droplist

type droplist.widget Xs F w!160 arrowSprite!ui_arrow_dl:
     f!F w!W h!1 ih xs![] drop rs picked over above_all p
     arrowSprite!ArrowSprite
     =
| less Xs.n: Xs =  [' ']
| $xs =  Xs((litem ? w!$w))
| ($f)($text)
droplist.text = $xs.$picked.text
droplist.=text T =
| $xs.$picked.text =  T
| ($f)($text)
droplist.render =
| $rs =  map X $xs X.render
| case $rs [R@_]: $ih =  R.h
| when $drop: $h =  $ih*$rs.n
| less $drop: $h =  $ih
| Me
droplist.draw G PX PY =
| when $drop
  | Y   0
  | for R $rs
    | G.blit(PX PY+Y R)
    | Y += R.h
| less $drop
  | G.blit(PX PY $rs.$picked)
  | A   get_spr($arrowSprite).'down_normal'
  | G.blit(PX+$w-A.w PY A)
| $rs =  0
| No
droplist.input In = case In
  [mice over S P] | $over =  S
                  | $xs.$p.state =  case S 1\picked 0\normal
  [mice_move _ P] | when $drop
                    | $xs.$p.state =  \normal
                    | $p =  (P.1/$ih).clip(0 $xs.n-1)
                    | $xs.$p.state =  \picked
  [mice left 1 P] | $over =  1
                  | $drop =  1
                  | $p =  $picked
                  | $above_all =  1
                  | $xs.$p.state =  \picked
  [mice left 0 P] | $drop =  0
                  | $xs.$p.state =  \normal
                  | $picked =  $p
                  | ($f)($text)
droplist.pick Name =
| for I,X $xs.i: when X.text >< Name
  | $picked =  I
  | ret  


use m ui/data litem
export lbox

type lbox.$box W H Xs F:
    f!F ih!No lines box picked o!No ls xs_!0
    =
| $xs =  Xs
| Pad   get_img "ui_litem_disabled"
| LH   Pad.h
| $lines =  max 1 H/LH
| $ls =  dup $lines: litem '' w!W
| LHH   $lines*LH
| $box =  layV: if LHH><H then $ls else [@$ls Pad.cut(0 0 W LH)]
| $offset =  0

lbox.=xs Xs = $xs_ =  Xs

lbox.xs = $xs_

lbox.offset = $o

lbox.=offset NO =
| when NO >< $o: ret  
| $o =  max 0: @clip 0 $xs.n-1 NO
| times K $lines
  | I   $o + K
  | Item   $ls.K
  | if I < $xs.n
    then | T   $xs.I
         | when T.is_list: T =  T.1
         | Item.text =  T
         | Item.state =  if I >< $picked then \picked else \normal
    else | Item.text =  ''
         | Item.state =  \disabled

lbox.slide P = $offset =  @int $data.n.float*P

lbox.value = $xs.$picked

lbox.data = $xs

lbox.=data Ys =
| $xs =  Ys
| $picked =  0
| $o =  No
| $offset =  0

lbox.pick NP =
| less $xs.n: ret  
| NP =  @clip 0 $xs.n-1 NP
| when NP <> $picked:
  | K   $picked - $o
  | when K >> 0 and K < $lines: $box.items.K.state =  \normal
  | $picked =  NP
  | K   NP-$o
  | when K >> 0 and K < $lines: $box.items.K.state =  \picked
| ($f)($xs.NP)

lbox.input In = case In
  [mice left 1 P] | have $ih: $box.items.0.render.h
                  | $pick(P.1/$ih+$o)

lbox.itemAt Point XY WH = [Me XY WH] //override lay`s method

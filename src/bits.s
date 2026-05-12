export bits

type bits n: xs =
| $xs = (($n+7)/8).bytes
| $clear(0)

bits.clear Bit =
| Byte   if Bit then 0xFF else 0
| for I $xs.n: $xs.I =  Byte

bits.`.`  N = $xs.(N ->- 3).get(N-*-7)

bits.`=`  N V =
| I   N ->- 3
| $xs.I =  $xs.I.set(N-*-7 V)

bits.l = map I $n $I

bits.active = $l.i.keep(?1)(?0)

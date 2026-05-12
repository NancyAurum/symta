export stack

type stack Init: xs used =
  if Init.is_int: $xs = dup Init
  else
    $xs = dup Init.n
    $init Init

stack.init Xs =
  $clear
  for X Xs: $push X

stack.push X = $xs.($used+) = X

stack.pop =
  $used-
  $xs.$used

stack.alloc @Xs =
  Item $pop
  Item.init @Xs
  Item

stack.n = $xs^_size

stack.clear = $used = 0

stack.remove Item =
  Xs []
  while $used:
    X $pop
    less X >< Item: push X Xs
  till Xs.end: $push Xs^pop

stack.`.` Index =
  less _tag Index: bad "stack.`.`: Index isnt integer"
  $xs.($used - Index)

stack.l = dup I $used: $xs.I

stack.map F = $l.map(F)


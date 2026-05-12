// Randomized Meldable Heap
export heap

type rmh_node.no_copy key value parent left right
rmh_node.as_text = "#(rmh_node [$key] [$value])"

rmh_node.copy = rmh_node $key $value $parent $left.copy $right.copy

rmh_merge H1 H2 =
  less H1: ret H2
  less H2: ret H1
  when H2.key < H1.key: ret: rmh_merge H2 H1
  if 1.rand
  then | H1.left =  rmh_merge H1.left H2
       | when H1.left: H1.left.parent =  H1
  else | H1.right =  rmh_merge H1.right H2
       | when H1.right: H1.right.parent =  H1
  H1

type heap.no_copy: root n

heap.copy =
  H heap
  H.root = $root.copy
  H.n = $n
  H

heap.push Key Value =
  U rmh_node Key Value 0 0 0
  $root = rmh_merge U $root
  $root.parent = 0
  ++$n
  Me

heap.pop =
  less $root: ret 0
  R $root
  $root = rmh_merge R.left R.right
  --$n
  R.key,R.value

heap.end = if $root then 0 else 1

heap.l =
  Xs $copy
  Ys []
  while@@it Xs.pop: push it Ys
  Ys.f

heap.map F = $l{F}

use cls
export cla

//array of columnar objects
cls cla n: = newIds_ $n
@as_text = "$<cla:[$id]:[$n]>"
@'.' K = ($id + 1 + K).as_id.trycls


#if 0

Names: \Alice \Anna \Nancy \Michelle \Nataly \Nika

cls person name
@as_text = "$<person:[$id]:[$name]>"

Arr cla 10 //columnar array of 10 elements

pushId_ Arr.2.id //start adding persons at index 2
for N Names: person N
popId_

times I Arr.n: say I Arr.I


halt

#endif
use m txt button slider
export sldbox

//slider box
type sldbox.$base bg pause!0:
  base title buttons width click![0 No] pause!Pause result
  slider
  countCur countMin countMax
| BG   $bg
| $width =  BG.w
| HeaderWidth   147
| BoxWidth   260
| $title =  txt titleb '' bold!1 alpha!20 center!1 wrap!HeaderWidth
| $slider =  slider h 240: X =>
  | $countCur =  @int: @round ($countMax-$countMin).float*X
  | $countCur += $countMin
| button_click Result = 
  | $result =  Result
  | when $pause: $pause()(0)
  | $show =  0
  | if no $click.1 then ($click.0)($result $countCur)
    else ($click.0)($click.1 $result $countCur)
| Buttons No
| Buttons =  3(I => button 'Text' skin!medium_small: =>
                    | button_click Buttons.I.value)
| $buttons =  map B Buttons: hidden B
| $base =  hidden: dlg: mtx
  | 0      0 | BG
  | 74     6 | $title
  | 16    44 | txt titleb bold!1 center!1 wrap!BoxWidth: => "[$countCur]"
  | 28   76  | $slider
  | 28   128 | layH s!5 $buttons

sldbox.run_ Range Buttons Title Click =
| less Range.is_list: Range =  0,0,Range
| $countCur =  Range.0
| $countMin =  Range.1
| $countMax =  Range.2
| $slider.value =  max 0.0: min 1.0
        ($countCur-$countMin).float/($countMax-$countMin).float
| $result =  0
| $title.value =  Title
| $click <= if Click.is_list then Click else [Click No]
| for B $buttons: B.show =  0
| for [I [Value Text]] Buttons.i
  | B   $buttons.I
  | B.show =  1
  | B.text =  Text
  | B.value =  Value
| $show =  1
| when $pause: $pause()(1)

sldbox.run Range title/'How Many?' buttons/[ok,'Ok'] click/0 =
| $run_(Range Buttons Title Click)


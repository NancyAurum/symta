use m txt button
export msgbox

MsgBoxActive   0

type msgbox.$base bg margin![0 0] pause!0:
  base title body buttons width margin!Margin click![0 No] pause!Pause result
  object!0
  buttons_next!0
  text_next!0
  click_next!0
  queue![]
| BG   $bg
| MX   $margin.0
| MY   $margin.1
| $width =  BG.w
| $title =  txt titleb '' bold!1 alpha!20 center!1 wrap!$width-MX*2
| $body =  txt mediumb '' bold!1 alpha!20          wrap!$width-MX*2
| button_click Result =
  | $result =  Result
  | MsgBoxActive =  0
  | when $pause: $pause()(0)
  | $show =  0
  | if no $click.1 then ($click.0)($result)
    else ($click.0)($click.1 $result)
  | less $queue.end:
    | Args pop $queue
    | $run_ @Args
| Buttons No
| Buttons =  3(I => button 'Text' skin!medium_small: =>
                    | button_click Buttons.I.value)
| $buttons =  map B Buttons: hidden B
| $base =  hidden: dlg: mtx
  | 0      0     | BG
  | MX+6  MY+10  | $title
  | MX+6  MY+40  | $body
  | MX+20 MY+295 | layH s!5 $buttons



msgbox.run_ Buttons Title Text Object Click =
| when MsgBoxActive:
  | $queue =: @$queue [Buttons Title Text Object Click] 
  | ret
| $text_next = []
| $buttons_next = 0
| $click_next = 0
| if Text.is_list and Text.n><0: Text = ''
| if Text.is_list and Text.n><1: Text = Text.0
| if Text.is_list:
    $text_next = Text.tail
    $buttons_next = Buttons
    $click_next = Click
    Text = Text.head
    Buttons = [ok,'Ok']
    Click = | V => $run_ $buttons_next Title $text_next $object $click_next
| MsgBoxActive =  1
| $object =  Object
| $result =  0
| $title.value = Title
| $body.value = Text
| $click <= if Click.is_list then Click else [Click No]
| for B $buttons: B.show =  0
| for [I [Value Text]] Buttons.i
  | B   $buttons.I
  | B.show =  1
  | B.text =  Text
  | B.value =  Value
| $show =  1
| when $pause: $pause()(1)

msgbox.run Title Text buttons/[ok,'Ok'] object/0 click/0 =
| $run_ Buttons Title Text Object Click

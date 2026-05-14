use gfx
export uim gui tabs hidden layV layH dlg spacer input_split
       sound_load sound_free sound_play sound_playing

//ffi_begin local ui
ffi_begin macro ui
ffi show_gfx.text Gfx.ptr
ffi show_close.void
ffi show_get_events.text
ffi show_cursor.void State.int
ffi show_sleep.void Milliseconds.u4
ffi show_get_ticks.u4

ffi show_sound_load.int Filename.text Music.int
ffi show_sound_free.void Id.int
ffi show_sound_play.int Id.int Channel.int Volume.int Loop.int
ffi show_sound_playing.int Channel.int

ffi swSetTitle.int Title.text

event_loop F =
  Events show_get_events.parse.0
  when got Events.locate(? >< quit): ret [No]
  G F(Events)
  less G.is_gfx: ret [G]
  Result show_gfx G.handle
  when Result <> '': bad "show: [Result]"
  No

show F = @.0|while no (~r = event_loop F):; show_close



//FIXME: create a default widgets

UIM   No

gui = UIM

widget.input In =
widget.items = No
widget.render = Me
widget.draw G PX PY =
widget.popup = 0
widget.infoline = '' //text to draw at info line
widget.pointer = 0
widget.parent = 
widget.=parent P = 
widget.itemAt Point XY WH =
| Items   $items
| when no Items: ret [Me XY WH]
| Item   Items.find(I => Point.in(I.meta_))
| when no Item: ret [Me XY WH]
| [IX IY W H]   Item.meta_
| Item.itemAt(Point-[IX IY] XY+[IX IY] [W H])
widget.x = 0
widget.y = 0
widget.w = 0
widget.h = 0
widget.above_all = 0 //draw above all other widgets
widget.wants_focus = 0
widget.wants_focus_rect = 0

type spacer.widget W H: w!W h!H
spacer.as_text = "#(spacer [$w] [$h])"

type tabs.~ Init Tabs: tab tabName table!Tabs | $pick(Init)
tabs.pick TabName ResetFocus!1 =
| when ResetFocus: when $tab: when got@@it gui:
  | while got it.focus_widget: it.focus_pop
| $tabName =  TabName
| $tab =  $table.TabName
| when no $tab: bad "tabs.pick: no [TabName]"
tabs.as_text = "#(tabs [$tab])"
tabs._ Method Args =
| Args.0 =  Args.0.tab
| Args.apply_method(Method)

type hidden.~ widget show!0: show!Show spacer!spacer(0 0)
hidden.as_text = "#(hidden show![$show] [$widget])"
hidden._ Method Args =
| Args.0 =  if $show then Args.0.widget else Args.0.spacer
| Args.apply_method(Method)

type canvas.widget W H P: w!W h!H paint!P
canvas.draw G PX PY = case Me (F<No).paint: F G PX PY $w $h 

type layV.widget Xs s!S: w!1 h!1 spacing!S items!Xs((meta ? [0 0 1 1]))
layV.draw G X Y =
| S   $spacing
| Is   $items
| Rs   Is(?render)
| $w =  Rs(?w).max
| $h =  Rs(?h).infix(S).sum
| N   0
| for R Rs
  | W R.w
  | H R.h
  | Rect Is^pop.meta_
  | RX 0
  | RY N
  | G.blit(X+RX Y+RY R)
  | Rect <=: RX RY W H
  | N = N+H+S

type layH.widget Xs s!S: w!1 h!1 spacing!S items!Xs((meta ? [0 0 1 1]))
layH.draw G X Y =
| S   $spacing
| Is   $items
| Rs   Is(?render)
| $h =  Rs(?h).max
| $w =  Rs(?w).infix(S).sum
| N   0
| for R Rs
  | W   R.w
  | H   R.h
  | Rect   Is^pop.meta_
  | RX   N
  | RY   0
  | G.blit(X+RX Y+RY R)
  | Rect <=: RX RY W H
  | N =  N+W+S

type dlg.widget Xs w!No h!No: w!W h!H ws items rs dynamic_wh
| $dynamic_wh =  no $w or no $h
| $ws =  Xs([X Y W]=>[X Y (meta W [0 0 1 1])])
| $items =  $ws()(?2).flip
| NonWidget   $items.find(?is_widget^not)
| when got NonWidget: bad "[NonWidget] is not a widget"
dlg.render =
| when got@@it $items.locate(?above_all):
  | swap $items.0 $items.it
  | swap $ws.($ws.n-1) $ws.($ws.n-it-1)
  | $items.0.above_all =  0
| have $w: $ws()(?0 + ?2.render.w).max
| have $h: $ws()(?1 + ?2.render.h).max
| Me
dlg.draw G PX PY =
| MaxW   0
| MaxH   0
| DynamicWH   $dynamic_wh
| for [X Y Widget] $ws
  | R   Widget.render
  | Rect   Widget.meta_
  | X += R.x
  | Y += R.y
  | when DynamicWH:
    | MaxW =  max MaxW X+R.w
    | MaxH =  max MaxH Y+R.h
  | Rect <=: X Y R.w R.h
  | G.blit(PX+X PY+Y R)
| when DynamicWH:
  | $w =  MaxW
  | $h =  MaxH

type input_split_item.$base_ parent base_
input_split_item.input In = $parent.handler_()($base_ In)

type input_split.$base base handler_: kids![]
input_split.itemAt Point XY WH =
| [Item WXY WWH]   $base.itemAt(Point XY WH)
| for Wrap $kids: when same Wrap.base_ Item: ret [Wrap WXY WWH]
| Wrap   input_split_item Me Item
| $kids =  [Wrap @$kids]
| [Wrap WXY WWH]
input_split.wrap_item Item = //hack to allow wrapping by external code
| for Wrap $kids: when same Wrap.base_ Item: ret Wrap
| Wrap   input_split_item Me Item
| $kids =  [Wrap @$kids]
| Wrap

//ui manager
type uim Root Cursor!host Title!"Symta":
  root!Root
  timers![]
  mice_xy![0 0]
  widget_cursor //cursor current widgets provides
  result!No
  fb!No
  keys!(!)
  popup!0
  mice_widget!(widget) //widget under cursor
  focus_stack!(dup 100 No)
  focus_sp!0
  focus_widget!No //widget currently receiving keyboard input
  focus_xy![0 0]
  focus_wh![0 0]
  mice_focus
  mice_focus_xy![0 0]
  click_time!(!)
  cursor!Cursor //defaut cursor
  host_cursor!0
  fps!1
  fpsT!0.0
  frame!0
  fpsGoal!1 // goal frames per second
  fpsD!30.0 //FPS divisor
| UIM = Me
| $limit_fps(60)
| $fb =  gfx 1 1
| $set_title Title
| show: Es => | UIM.input(Es)
              | UIM.render
| when got $fb
  | $fb.free
  | $fb =  No
| R   $result
| $result =  No
| UIM =  No
| ret R
uim.set_title Title = swSetTitle Title
uim.set_cursor C = $cursor = C
uim.limit_fps GoalFPS =
| when $fpsGoal >< GoalFPS: ret
| $fpsGoal =  GoalFPS
| $fpsD =  $fpsGoal.float+8.0
| $fpsT = 0.0
// calculates current framerate and adjusts sleeping accordingly
uim.update_frame_timing StartTime FinishTime =
| DT   FinishTime-StartTime
| $fpsT += DT
| when $frame%24 >< 0:
  | $fps =  @int 24.0/$fpsT
  | $fpsT =  0.0
  | when $fps < $fpsGoal and $fpsD < $fpsGoal.float*2.0: $fpsD += 1.0
  | when $fps > $fpsGoal and $fpsD > $fpsGoal.float/2.0: $fpsD -= 1.0
| $frame+
| SleepTime   1.0/$fpsD - DT
| SleepTime
uim.render =
| FB   $fb
| when no FB: ret No
| StartTime   $ticks
| R   $root.render
| W   R.w
| H   R.h
| when W <> FB.w or H <> FB.h:
  | FB.free
  | FB =  gfx W H
  | $fb =  FB
| FB.blit(0 0 R)
| when got@@fw $focus_widget:
  | when fw.wants_focus_rect:
    | P $focus_xy+[fw.x fw.y]
    | WH if fw.w and fw.h then [fw.w fw.h] else $focus_wh
    | FB.rectangle(0xFFFF00 0 P.0-1 P.1-1 WH.0+2 WH.1+2)
| C   $widget_cursor
| when got C:
  | XY   UIM.mice_xy
  | CG   if C then C else $cursor
  | when got CG and host <> CG:
    | when $host_cursor: show_cursor 0
    | $host_cursor =  0
    | FB.blit(XY.0 XY.1 CG)
  | when host >< CG and not $host_cursor:
    | show_cursor 1
    | $host_cursor =  1
    | Pop No
  | when $popup
    | R   $popup.render
    | FB.blit(XY.0 XY.1-R.h R)
| FinishTime   $ticks
| SleepSeconds   $update_frame_timing(StartTime FinishTime)
| when SleepSeconds > 0.0: $sleep(SleepSeconds)
| FB.pixels //sync hack
| FB
uim.add_timer Interval Handler =
| Me.timers =  [@Me.timers [Interval (clock)+Interval Handler]]
uim.update_timers Time =
| Ts   $timers
| less Ts.n: ret 0
| $timers =  [] // user code can insert additional timers
| Remove   []
| for [N T] Ts.i: case T [Interval Expiration Fn]:
  | when Time >> Expiration
    | if got Fn() then Ts.N.1 =  (Time)+Interval
      else push N Remove
| when Remove.n
  | N   0
  | Ts =  Ts.skip(X=>Remove.locate(N+)^got)
| when $timers.n: Ts =  [@Ts @$timers]
| $timers =  Ts
| 0
uim.widget_under_cursor = $root.itemAt($mice_xy [0 0] [0 0])
uim.input Es =
| T   clock
| $update_timers(T)
| [NW NW_XY NW_WH]   $widget_under_cursor //new widget
| $popup =  NW.popup
| $widget_cursor =  NW.pointer
| for E Es: case E
  [mice_move XY]
    | $mice_xy <= XY
    | if $mice_focus
      then $mice_focus.input([mice_move XY XY-$mice_focus_xy])
      else NW.input([mice_move XY XY-NW_XY])
    | MW   $mice_widget
    | less same MW NW:
      | when got MW: MW.input([mice over 0 XY])
      | $mice_widget =  NW
      | NW.input([mice over 1 XY])
  [mice Button State]
    | MP   $mice_xy
    | MW   $mice_widget
    | less same MW NW:
      | when got MW: MW.input([mice over 0 MP])
      | $mice_widget =  NW
      | NW.input([mice over 1 MP])
    | if $mice_focus
      then | LastClickTime   $click_time.Button
           | when got LastClickTime and T-LastClickTime < 0.25:
             | NW.input([mice "double_[Button]" 1 MP-NW_XY])
           | $click_time.Button =  T
           | $mice_focus.input([mice Button State MP-$mice_focus_xy])
           | less State: $mice_focus =  0
      else | $mice_focus = NW
           | $mice_focus_xy <= NW_XY
           | NW.input([mice Button State MP-NW_XY])
    | when State and NW.wants_focus:
      | $focus_xy =  NW_XY
      | $focus_wh =  NW_WH
      | FW   $focus_widget
      | less same FW NW:
        | when got FW: FW.input([focus 0 MP-$focus_xy])
        | $focus_widget =  NW
        | NW.input([focus 1 MP-NW_XY])
  [key Key State] | $keys.Key =  State
                  | D   if got $focus_widget then $focus_widget
                        else if not NW.wants_focus then NW //explicit focus?
                        else 0
                  | when D: D.input([key Key State])
  Else |
| No
uim.exit @Result =
| $result =  case Result [R] R Else No
| $fb =  No

// sleep for a number of seconds
uim.sleep Seconds = show_sleep: @int Seconds*1000.0

// get the number of seconds since the ui initialization.
uim.ticks = show_get_ticks().float/1000.0

uim.focus_push NewFocusWidget =
| FW   $focus_widget
| when got FW: FW.input([focus 0 $focus_xy])
| when $focus_sp><$focus_stack.n: $focus_sp =  0
| $focus_stack.($focus_sp+) =  $focus_widget
| $focus_widget =  NewFocusWidget
| when got NewFocusWidget: NewFocusWidget.input([focus 1 $focus_xy])

uim.focus_pop =
| $focus_sp-
| when $focus_sp><-1: $focus_sp = $focus_stack.n-1
| PrevFocusWidget   $focus_stack.$focus_sp
| $focus_stack.$focus_sp =  No
| when got $focus_widget: $focus_widget.input([focus 0 $focus_xy])
| $focus_widget =  PrevFocusWidget
| when got PrevFocusWidget: PrevFocusWidget.input([focus 1 $focus_xy])



sound_load Filename music/0 = show_sound_load Filename Music
sound_free Id = show_sound_free Id
sound_play Id channel/-1 volume/0.5 loop/0 =
| show_sound_play Id Channel (Volume*1000.0).int Loop
sound_playing Channel = show_sound_playing Channel

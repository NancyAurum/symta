use cls heap gfx font svg rgb cache store prof uimgen
export uim gui uim_stl bg spc frm ftx knob btn pic slider htabs tabs lay
       chkbx lbox edln edbx drpl msgbx
       pic9 wnd fsview menubar tip
       sound_load sound_free sound_play sound_playing

/*

Symta UI.

Each widget is an cet entity, 

- Several basic widgets are provided:
  - Formatted text label and text edit (ftx).
  - Listbox (lbox)
  - Tabs stack with optional header (htabs)
  - Movable window with a title bar (wnd)

TODO:

* wgt.add should perform unkid
  otherwise it is too error-prone
  we already had a bug item_slot.s, which exposed itself only after some time,
  and took enough time to debug.

- Check box
- Radio buttons
- Context menu
- Tooltip

- A way to save and reset positions of all windows in a unform manner.
  I.e. uim.s should provide a support for desktop.

- Whese several hierarchical dimmers are active, only the topmost should be drawn.
  dimmer's draw function should check for any other active dimmers and
  establish Z-order

- Windows should have the following properties:
  - a '-' icon to allow minimizing/collapsing-to-header
  - allowed or disallowed to be moved out of the view
  - snap to edges
  - if minimized snap header to the bottom.


- Window title bar which appears only when cursor is over the window. Otherwise it is ghosted and passes input through. It should be an icon, near x and -

- Ctrl-draggin a window should move it even if not clicked on a header

- no.uim_clear_rect_cache_ should be called either once in second
  at random places inside the second.
  Or when one of the widget'same XY or WH pairs get updated.
  In fact we should keep XY, WH, Pad, MinWH and MaxWH inside a single list
  And save a copy of it after each rect gets recalced.
  Just watch for fn's which must handled separately

- lbox needs proper border margin
  long time ago it used a hack with LBOX_BORDER_SZ


- `draw_focus_rect` should be delayed till all widgets are drawn
  and then drawn by the uim itself as an overlay

- Fix the render threading

- A way to match widget width to the widget above it.

- Fast mode, where after initial layout, widgets get updated only when they
  either have active flag, under mouse cursor, have focus or were explicitly
  marked for update.

- A way to free wgts not present inside of the tree

- Clicking anywhere isnide window should bring it on top.

- Picking arbitrary widget shapes


- Default file picker window,
  which should also employ user specified style

- Default progress bar window

- Default slider window.

- Text scroll with a slider
- 
- To make UI mouse faster, we can use double buffering,
  and return the finished buffer and mouse on top.


- Create several example apps
  - Symta REPL or even BASIC IDE with file list on the left
  - A calculator app, which should a few lines of code,
    without any external methods.

- Create several example games solely with Symta GUI widgets
  - Tetris
  - Dogfighter https://www.youtube.com/watch?v=4fWUbmm45Os
  - stagecast

- onUpd and onClick prevent serializing the widgets
  That can be solved with symbolic ids for handles, which then get
  registered with UIM. Or these can immediately connect to some widget's
  upd.
- The main problems with serializing the widgets graph to disk are
  - Repeated references to other $id objects should be replaced with
    a temporary id - special ${id_!N} type
  - The function callbacks should be referenced through a callback manager
    any attempt to serialize raw callbacks should produce an error.
  - Gfxes should be referenced through resource manager.
    any attempt to serialize raw callbacks should produce an error.

- Act! should take raw db calls like \uim.exit

*/


//ffi_begin local ui
ffi_begin macro ui
ffi show_gfx.text Gfx.ptr
ffi show_close.void
ffi show_get_events.text
ffi show_cursor.void State.int
ffi show_sleep.void Milliseconds.u4
ffi show_wh.text

ffi show_sound_load.int Filename.text Music.int
ffi show_sound_free.void Id.int
ffi show_sound_play.int Id.int Channel.int Volume.int Loop.int
ffi show_sound_playing.int Channel.int

ffi show_set_title.int Title.text

ffi show_read_clipboard.text
ffi show_write_clipboard.void Text.text

ffi show_size.void Type.int

sound_load Filename Music!0 = show_sound_load Filename Music
sound_free Id = show_sound_free Id
sound_play Id Channel!-1 Volume!0.5 Loop!0 =
  show_sound_play Id Channel (Volume*1000.0).int Loop
sound_playing Channel = show_sound_playing Channel


event_loop F =
  EventsText show_get_events()
  if EventsText><error: bad 'UI initialization failed'
  Events if EventsText.n then EventsText.sexp(list!1) else []
  when got Events.locate(? >< quit): ret [No]
  G F(Events)
  less G.is_gfx: ret [G]
  Result show_gfx G.handle
  when Result <> '': bad "show: [Result]"
  No

//eval function `F` with current input while it returns gfx
//otherwise close window
show F = @.0|while no (~r = event_loop F):; show_close


#WGT_FONT No


UIM No
GStl!  // style registry
GIds!

gui = UIM
RectCache!

// Module-level "geometry dirty" flag. Any widget setter that affects
// layout flips this on. The render loop reads it to decide whether the
// per-frame rect-cache flush is needed. See ui_speedup.md (W1).
GeomDirty 1

no.uim_clear_rect_cache_ =
  RectCache = !
  GeomDirty = 0

no.uim_dirty_geom_ = GeomDirty = 1
no.uim_geom_dirty_ = GeomDirty

// Per-phase timing overlay toggle. Flip via `uim_show_timings 1` or
// from cfg with `uim_show_timings: cfg.ui.show_timings.@0`. When set,
// uim.render draws a small FPS + per-phase breakdown in the top-left
// corner. See ui_speedup.md Part 9 (measurement protocol).
ShowTimings 0
no.uim_show_timings V = ShowTimings = V
no.uim_show_timings_ = ShowTimings


#LSZ_NORMAL 20
#LSZ_TITLE  35



//converts a list form of expressions to widgets
list_to_widget L =
  Sz \n
  Align 0
  P 0  //position
  T No //text
  A No //action
  O \pv //origin
  Id 0
  case L:
    S<1.is_int+t+n @LL = 
      Sz = S
      L = LL
  case L:
    AA<l+r+c @LL = 
      Align = AA
      L = LL
  case L:
    OO<1.is_text @LL = 
      O = OO
      L = LL
  L._(:[&T]; [&P @[&T @[&A]]])
  case P:
    I<1.is_text @PP = 
      Id = I
      P = PP
  X,Y,W,H case P:
            [X] = X,0,auto,auto
            X Y = X,Y,auto,auto
            X Y W = X,Y,W,auto
            X Y W H = X,Y,W,H
            Else = 0,0,auto,auto
  R if got A: btn T A O!O X!X Y!Y W!W H!H       Align!Align Id!Id
    else      ftx T   O!O X!X Y!Y W!W H!H sz!Sz Align!Align Id!Id
  R

#:uim

cls wgt! //! = provide a type predicate, `if Object.wgt: ...`
    XY!0,0
    o!pv //pv - previus widget, vertically
         //ph - previous widget horizontally
         //pvh - previous o!ph widget vertically
         //base - x,y of the base widget
         //abs - absolute
         //nl - new line
         //_ - bottom
    align!0 //l - left
            //r - right
            //c - center on Y axis
            //cc - center on X and Y axes
            //br - bottom right
            //fix_cc - align only once an then reuse the previous XY
    W!auto  //integer value - exact size in pixels
            //float value - proportion of the base size
            //rim - rims around content
            //negative value - expand to entire base rect minus the value
            //[s M] - unoccupied base minus margin
            //auto - widget itself decides
            //next/prev - use next widget's width/height
    H!auto //same args aas H
    Act!0
    GID!0 //GUI Identifier
    Add!No
    Drg!No //used to implement windows/movable/draggable items
            //No - non-movable
            //0 - stays on the same level when moved
            //1 - brought on top when moved
            //GID,MovableType
    PW!0 //pad width
    PH!0 //pad height
    Tip!No
    :
        //O (origin) values:
        //  abs - XY is absolute from the top left corner
        //  pv - XY is relative to the prev widget's left bottom corner
        //       NOTE: ph and pvh widgets are skipped, while look for
        //             the previous widget. Use pvh if you want to link with
        //             pv/pvh widgets
        //  nl - Y is relative to the prev widget's bottom corner
        //       X is relative to the super widget's XY
        //  ph - XY is relative to the prev widgets top right corner
        //  pvh - XY is relative to the prev widget's left bottom corner
        //  over - overlays the previous item, using its XY
        //  base - XY is relative to the super widget XY
    base!0 //superior widget
    kid0!0 //subordinate widgets
    prev!0 next!0 //for linking into the kids list
    hidden!0 //when 1, isn't drawn and gets no input
    dimmed!0 //when 1, input to it gets blocked
             //and its color is dimmed
    rectr_!0 //guard agaisnt recursive @rect calls
    clients![]
    over!0
    pressed!0
    wh_!W,H
    xy_!XY
    tagfrm!0
    pad!PW(A,B=A),PH(A,B=A) //pad width and height with this these values
    min!PW(:A,B=B),PH(:A,B=B) //minimum width and height
    max!No,No
    mice!0,0 //last mice position inside the widget
    fix_align_xy!!0
    fix_rim!!0 //used to prevent jerking dynamically updated button labels
               //changing the button size, leading to jerkiness
    prev_rim_w!!No
    =
    if got Tip: $ub.wgtTip = Tip
    if got Drg: $ub.wgtDrag = Drg
    if GID: Me.gid = GID
    if Act: $set_act Act
    if got Add:
      if Add.is_wgt: Add = [Add]
      U $cls
      for W Add: U.add W

@'=gid' NewGID = 
  $ub.gid = NewGID
  GIds.NewGID = Me 

@x = $rect.0
@y = $rect.1
@w = $rect.2
@h = $rect.3

@ex = $rect(X,_,W,_ = X+W)
@ey = $rect(_,Y,_,H = Y+H)

@key = $ub.wgt_key //user provided key value

@'=key' Key = $ub.wgt_key = Key

@wset Name Value =
  $ub."wgt_[Name]" = Value
  Me
@wget Name = $ub."wgt_[Name]"

@wants_focus = 0

@solid = 1 //if 0, widget doesn't grab input from the widgets it cover

@has_focus = gui().focus.id >< $id

@draw_focus_rect FB Color!gui().stl_fcs Sz!1 =
  X,Y,W,H $rect+[-1 -1 2 2]
  FB.fat_rect Sz Color X Y W H

@contains Wgt = //check if Wgt is one of our kids or we ourselves
  Id $id
  if Id><Wgt.id: ret 1
  for PW Wgt.wpath:
    if PW.id><Id: ret 1
  0

@mice_cursor = 0 //No = hide cursor
                 //0 = use root widget's cursor
                 //gfx = draw a gfx
                 //host = use host OS cursor

@end_draw_kids FB =

@restricts_input W XY = 0

@val = No

@kids =
  Ks:
  K $kid0
  while K:
    push K Ks
    K = K.next
  Ks.f

@shown_kids =
  Ks:
  K $kid0
  while K:
    less K.hidden-*-1: push K Ks
    K = K.next
  Ks.f

@xy =
  X,Y $xy_
  if X.is_fn: X = X()
  if Y.is_fn: Y = Y()
  when X.is_int and Y.is_int: ret X,Y
  Base $base
  less Base: ret 0,0
  less X.is_int:
    BR Base.rect
    M,N if X.is_list: BR.3,X.0 else BR.2,X
    X = @int: @round M*N
  less Y.is_int:
    BR Base.rect
    M,N if Y.is_list: BR.2,Y.0 else BR.3,Y
    Y = @int: @round M*N
  X,Y


// Geometry-affecting setters invalidate the cached layout. Without this
// the per-frame rect-cache flush is the only thing keeping things sane.
// See ui_speedup.md (W1).
@'=xy' XY =
  $xy_ = XY
  $clear_rect_cache_r
  No.uim_dirty_geom_

@tip = $ub.wgtTip(No="")
@'=tip' V = $ub.wgtTip = V


WHRec 0

@xspace Margin = //calculate space availalbe for growing vertically
  B $base
  BR B.rect
  Prev $prev
  Next $next
  WW BR.2 - $xy.0 - Margin
  if $o(0:pv+nl): Prev = 0
  if Prev:
    WW -= Prev.rect.2
  if Next and not Next.o(0:pv+nl+abs+base+'_'):
    WW -= Next.rect.2
    if Next.o(0:ph): WW -= Next.xy.0 //FIXME: walk entire ph section
  if WW < 0: WW = 0
  WW

@yspace Margin = //calculate space availalbe for growing horizonatally
  B $base
  BR B.rect
  Prev $prev
  Next $next
  XY $xy + $oxy_(0 0)
  HH BR.3 - XY.1 - Margin //FIXME: allow function as margin
  if Prev: less $o(0:pv+nl+abs):
    HH -= Prev.oxy_(@Prev.wh).1 - BR.1
  //if Next and Next.o(0:pv+nl+base): Next = 0
  if WHRec:
    HH = 0
    Next = 0
  if Next:
    Last 0
    WHRec = 1
    N Next
    while N.next:
      if N.o(1:base+abs+'_'+ph+pv=0): Last = N
      N = N.next
    if Last:
      LR Last.rect
      HH -= LR.1 + LR.3
    WHRec = 0
    N Next
    while N.next:
      N.clear_rect_cache_r
      N = N.next
  if HH < 0: HH = 0
  HH

RimRec 0
RimStack:

@rimW =
  if $fix_rim:
    if $fix_rim > 1: --$fix_rim
    else
      if got $prev_rim_w: ret $prev_rim_w
  Ks $shown_kids
  if Ks.end: ret 0
  RimRec = 1
  K0 Ks.0
  X,Y,_,_ K0.rect
  X -= K0.xy_.0(0:1.is_int=Q)
  Rim 0
  for K Ks:
    if K.o >< abs: pass
    KX,KY,KW,KH K.rect
    Rim = max Rim KX+KW-X
  RimRec = 0
  $prev_rim_w = Rim
  Rim

@rimH =
  Ks $shown_kids
  if Ks.end: ret 0
  K0 Ks.0
  RimRec = 1
  X,Y,_,_ K0.rect
  Y -= K0.xy_.1(0:1.is_int=Q)
  Rim 0
  for K Ks:
    if K.o >< abs: pass
    KX,KY,KW,KH K.rect
    //FIXME: handle K.o(0:pv+nl)
    if K.o >< _: KY = Y+KH
    Rim = max Rim KY+KH-Y
  if Ks.n>1: Ks{clear_rect_cache_r} //kid rects are interdependent
  RimRec = 0
  Rim


@wh =
  B $base
  W,H $wh_
  if W><rim: W = $rimW
  if H><rim: H = $rimH
  less W.is_int:
    if: W.is_text = W = W(auto=$auto_w;prev=$prev.rect.2;next=$next.rect.2)
        W.is_fn = W = W()
        H(0:s+[s _]) = W = $xspace W(0:[s M]=M) 
    less W.is_int:
      BR if B: B.rect else 0,0,UIM.w,UIM.h
      M,N if W.is_list: BR.3,W.0 else BR.2,W
      W = @int M*N
  less H.is_int:
    if: H.is_text = H = H(auto=$auto_h;prev=$prev.rect.3;next=$next.rect.3)
        H.is_fn = H = H()
        H(0:s+[s _]) = H = $yspace H(0:[s M]=M) 
            /*when H(0:s+[s _]):
              Margin H(0:[s M]=M) 
              H = 0
              less RimRec:
                //$rect = 0,0,0,0
                H = max 0 $base.rect.3 - Margin - B.rimH*/
    less H.is_int:
      BR if B: B.rect else 0,0,UIM.w,UIM.h
      M,N if H.is_list: BR.2,H.0 else BR.3,H
      H = @int M*N
  if W < 0 or H < 0:
    BR if B: B.rect else 0,0,UIM.w,UIM.h
    if W < 0: W = BR.2 - $xy.0 + W
    if H < 0: H = BR.3 - $xy.1 + H
  PW,PH [W H] + $pad //padded width and height
  MinW,MinH $min //FIXME: implement list.min which mins two lists
  if MinW.is_float: MinW = @int B.w * MinW
  if MinH.is_float: MinH = @int B.h * MinH
  if PW < MinW: PW = MinW
  if PH < MinH: PH = MinH
  MaxW,MaxH $max
  if got MaxW and PW > MaxW: PW = MaxW
  if got MaxH and PH > MaxH: PH = MaxH
  ret PW,PH

@'=wh' WH = $wh_ = WH


@auto_w = $base($rimW:0=UIM.w)
@auto_h = $base($rimH:0=UIM.h)

//does the below conflict with auto_w and auto_h?
@auto_wh = $xywh[2:4]

@act @As =
  Act $get_act
  if no Act: ret
  if Act.is_list:
    As =: Act.1 @As
    Act = Act.0
  Act(@As.take(Act.nargs(-1=As.n)))
@get_act = $ub.wgt_act  //action this widget performs, when interacted with
@set_act Act =
  $ub.wgt_act = Act
  Me


@update =

@postupdate = //called after the children get updated

//Watch for circular depenencies!
//For example, button text's xy uses align!cc,
//which depends on button's WH, yet button's WH depend on text's wh
@align_xy =
  Align $align
  less Align: ret 0,0
  RW,RH $rect[2:4]
  BW,BH $base.rectk_(RW RH)[2:4]
  case Align:
    c = [(BW-RW)/2 0]
    r = [BW-RW 0]
    cc = [(BW-RW)/2 (BH-RH)/2]
    fix_cc =
      if $fix_align_xy: $fix_align_xy
      else
        XY: (BW-RW)/2 (BH-RH)/2
        $fix_align_xy = XY
        XY
    br = [BW-RW BH-RH]
    bl = [0 BH-RH]
    Else = bad "wgt.align_xy: invalid align=[Align]"

@oxy_ W H = //calc xy with base origin
  Base $base
  less Base: ret 0,0
  BR Base.rect
  X,Y $xy
  less X.is_int:
    M,N if X.is_list: BR.3,X.0 else BR.2,X
    X = @int M*N
  less Y.is_int:
    M,N if Y.is_list: BR.2,Y.0 else BR.3,Y
    Y = @int M*N
  OXY 0
  O $o
  Prev $prev
  _label again
  if: O >< pv =
        less Prev: | O = \base; _goto again
        while Prev.o(pv+nl=0) and Prev.prev: Prev = Prev.prev
        PR Prev.rect
        OXY = PR.0, PR.1+PR.3
      O >< nl = //new line
        less Prev: | O = \base; _goto again
        while Prev.o(pv+nl=0) and Prev.prev: Prev = Prev.prev
        PR Prev.rect
        OXY = BR.0, PR.1+PR.3
      O >< pvh =
        less Prev: | O = \base; _goto again
        PR Prev.rect
        OXY = PR.0, PR.1+PR.3
        Prev.clear_rect_cache_r //HACK: scalable wgt could fetch our rect
      O >< ph =
        less Prev: | O = \base; _goto again
        PR Prev.rect
        less Prev.wh_.0(0:s+[s _]):
          Prev.clear_rect_cache_r ///HACK:scalable wgt could fetch our rect
        OXY = PR.0+PR.2, PR.1
      O >< '_' =
        OXY = BR.0, BR.1+BR.3 - H.@0
      O >< abs = OXY = 0,0
      \base = OXY = BR.0, BR.1
      1 = bad '[Me].oxy_: invalid O![O]'
  OXY + [X Y]

@xywh =
  W,H $wh
  R $rect
  //update it, since $oxy_ and align_xy are circulary dependent
  $rect =: R.0 R.1 W H
  :@$oxy_(W H) W H


@clear_rect_cache_r =
  for K $kids: K.clear_rect_cache_r
  RectCache.del $id

@clear_rect_cache =
  RectCache.del $id

@rect_ = RectCache.$id

//WARNING: @rect is includes a ton of circular dependencies.
//         proceed with great caution.
@rectk_ W H = //kids rect calculation routines should call this one
  less $rectr_: ret $rect
  Id $id
  Rect RectCache.Id
  if $rectr_ and $wh_.any(rim):
    Pad $pad
    X,Y Rect[0:2]
    W,H [W H]+Pad
    Rect = X,Y,W,H
  Rect    

@rect =
  Id $id
  Rect RectCache.Id
  if no Rect:
    $rectr_+
    RectCache.Id =: @$oxy_(No No) @$min //temporary
    Rect = $xywh
    Rect += [@$align_xy 0 0]
    RectCache.Id = Rect
    $rectr_-
  Rect

@'=rect' Rect = RectCache.$id = Rect

@'=xywh' X,Y,W,H =
  XY X,Y
  O $o
  if: O >< pv and $prev =
        PR $prev.rect
        XY -= [PR.0 PR.1] + [0 PR.3]
      O >< ph and $prev =
        PR $prev.rect
        XY -= [PR.0 PR.1] + [PR.2 0]
      $base = XY -= $base.xy
  $xy_ = XY
  $wh_ = W,H
  $clear_rect_cache_r
  No.uim_dirty_geom_

@input E = //case E [mice Button State]: say $cls

//redefine catches input of its child widgets, returns non-0
//no children will never get input
@catch E = 0

@add W =
  less W.is_wgt: W = list_to_widget W
  Ks $kid0
  if Ks:
    while Ks.next: Ks = Ks.next
    Ks.next = W
  else $kid0 = W
  W.next = 0
  W.prev = Ks
  W.base = Me
  No.uim_dirty_geom_

@addAt X Y W =
  W.o = \base
  W.xy_ = X,Y
  $add W

//cls wxy.WGT(o=base,x=X,y=Y) Wgt: = $add Wgt


@unkid =
  Prev $prev
  Next $next
  Base $base
  $base = 0
  $prev = 0
  $next = 0
  if Next: Next.prev = Prev
  if Prev: Prev.next = Next
  else Base.kid0 = Next
  No.uim_dirty_geom_

@wpath_r =: @$base([]:-B=B.wpath_r) Me

@wpath = $wpath_r[:~]

@press =
@unpress =

@pressR =
@unpressR =

@make_top =
  Base $base
  $unkid
  Base.add Me
  Me

#DIMRGB 0x3F000000

@dimmer =
  Base $base
  Dimmer Base.kids.find(?gid><dimmer_)
  if no Dimmer:
    Dimmer = @hide: bg DIMRGB id!dimmer_ O!abs
                       W!(=>UIM.root.w) H!(=>UIM.root.h)
    Base.add Dimmer
  Dimmer

@is_xcl = UIM.xcl_wgt(Q.id >< $id: No=0)

@xcl_cancel = $ub.'xcl_cancel'

//xclusively show above all other widgets, optionally dimming them
@xcl Dim!DIMRGB =
  UI gui
  MiceWGT UI.mice_widget
  if got MiceWGT: //ensure prev mice wgt gets mice left event
    MiceWGT.bcast [mice over 0]
  D $dimmer
  D.color = Dim
  D.make_top.show.set_act $xcl_cancel
  UI.xcl_push $make_top
  Me

@unxcl =
  UI gui
  XW UI.xcl_wgt
  less got XW and XW.id >< $id: ret
  XW UI.xcl_pop
  if no XW or XW.base.id <> $base.id: $dimmer.hide
  if got XW: XW.make_top
  Me

@xshow @As = $xcl.show(@As)

@xhide @As = $hide(@As).unxcl

@drag_begin CurXY =

@drag_move PressXY CurXY = $xy_ = CurXY - PressXY

@drag_end PressXY CurXY =

#UI_DRG_SAME  0
#UI_DRG_TOP   1

#PRESSED_LMB 1
#PRESSED_RMB 2

#MVBL_AS_VARS DrgType,Wgt Drg(Q,Me:Type,Id = Type,Id.q)
//if $over: $act
@full_input E =
  Drg $ub.wgtDrag
  case E:
    mice over State = $over = State
    mice_move _ =
      XY UIM.mice_xy
      $mice = XY - $rect[:2]
      if got Drg:
        if $pressed><PRESSED_LMB:
          MVBL_AS_VARS
          if DrgType><UI_DRG_TOP: Wgt.make_top
          Wgt.drag_move UIM.keys.pressXY XY
    mice left 1 =
      WgtXY $rect[:2]
      XY UIM.mice_xy
      $mice = XY - WgtXY
      if got Drg:
        MVBL_AS_VARS
        if DrgType><UI_DRG_TOP: Wgt.make_top
        Wgt.drag_begin XY
        WgtXY = Wgt.xy
      UIM.keys.pressXY = XY-WgtXY
      $pressed = PRESSED_LMB
      $over = 1 //HACK: menubar's @xcl resets $over state; also tablets
      $cls.press
    mice left 0 =
      XY UIM.mice_xy
      $pressed = 0
      $cls.unpress
      PressXY UIM.keys.pressXY
      UIM.keys.pressXY = No
      if got Drg:
        MVBL_AS_VARS
        Wgt.drag_end PressXY XY
    mice right 1 =
      $mice = UIM.mice_xy - $rect[:2]
      $pressed = PRESSED_RMB
      $over = 1
      $cls.pressR
    mice right 0 =
      $pressed = 0
      $cls.unpressR
  $input E

@miceAnchor = UIM.keys.pressXY

@bcast E = //broadcast
  TopWgt gui().xcl_wgt
  if got TopWgt and not TopWgt.contains Me:
    less $gid><dimmer_: ret //modal widget asked us to block input to others
  for PW $wpath:
    if PW.dimmed: ret
    if PW.catch E: ret
  if $dimmed: ret
  $full_input E

@shown = not $hidden

@on_show =

@on_hide =

@hide @As =
  Me.hidden = Me.hidden.bitSet 0 1
  $on_hide @As
  No.uim_dirty_geom_
  Me
@show @As =
  Me.hidden = Me.hidden.bitSet 0 0
  $on_show @As
  No.uim_dirty_geom_
  Me
@'=show' X = if X: $show else $hide
@toggle @As = if $hidden: $show @As else $hide
@xtoggle @As = if $hidden: $xshow @As else $xhide
@ghost =
  Me.hidden = Me.hidden.bitSet 1 1
  Me
@unghost =
  Me.hidden = Me.hidden.bitSet 1 0
  Me
@dim =
  Me.dimmed = 1
  $dim_r 1
  Me
@undim =
  Me.dimmed = 0
  $dim_r 0
  Me

@truly_hidden = $hidden-*-1

@wnd_base =
  W Me
  while W and W.ub.'cls' <> 'wnd': W = W.base
  W

@close =
  W $wnd_base
  less W: bad 'wgt.close got called on a widget without a window'
  W.close

//FIXME: guard against recursion. use the `kid0` field for that.
@dim_r Dim =
  Cls $cls
  if Dim:
    Cls.dimmed = Cls.dimmed -+- 2
  else
    Dim = Cls.dimmed-*-1
    Cls.dimmed = Dim
  $kid0(:0;_.dim_r(Dim))

UIMFrame 0

GWgtDL No //Draw list

@renew_tagfrm = $tagfrm = UIMFrame+2

@placed =
  TF $tagfrm
  not TF or UIMFrame<TF

//FIXME: guard against recursion. use the `kid0` field for that.
@draw_list_r =
  W $cls
  W.next(:0;_.draw_list_r)
  less W.hidden: when W.placed:
    push [W] GWgtDL
    W.kid0(:0;_.draw_list_r)
    push W GWgtDL


@draw_list =
  GWgtDL =:
  $draw_list_r
  R GWgtDL
  GWgtDL = No
  R

@update_r =
  W $cls
  N W.next //fetch it now in case wkdget removes itself
  less W.hidden-*-1:
    W.update //update before kids, since it can affect the kids
    W.kid0(:0;_.update_r)
    W.postupdate
  N(:0;_.update_r)

@gid = $ub.gid

@draw FB =
  //X,Y,W,H $xywh
  //FB.rectangle $color 1 X Y W H

@echo Msg =
  Msg: $ub.gid Me @Msg
  for C $clients: $client.post Msg

@post Msg =

#if 0
//The bellow fails in case when widget is outside of parent]
//So we use reverse draw list a look globally.
@at XY =
  if XY.in $rect: ret $kid0(Me: -W = W.at XY or Me)
  $next(-W = W.at XY)

#endif

@at XY =
  GWgtDL =:
  $draw_list_r
  Ws GWgtDL.skip(?is_list).f
  GWgtDL = No
  for W Ws:
    if XY.in W.rect and W.solid and W.includes XY:
      less W.wpath.any(?restricts_input(W XY)):
        ret W        
  0

@includes XY = 1 //returns 1 if widget processes input at specific point

@notifier_xy =: 0 $rect.3-70 reverse

NotifyMsgs:

@notify Msg =
  if got UIM: UIM.notify Msg
  push Msg NotifyMsgs

cls notifier.WGT(w=0,h=0) color!RGB_WHITE := 
@draw FB =
  X,Y,@As UIM.root.notifier_xy
  Reverse As(:[reverse@_])
  C 24
  Clock clock
  FadeT 5.0
  FadeA 0xff.float
  Lines:
  Color $color
  for [Expires Chars] UIM.notes.f: when Clock < Expires:
    Ls Chars.text.split('\n')
    A 0
    TTL Expires Chars Expires-Clock
    when TTL < FadeT: A = max 0: @int FadeA-FadeA*(TTL/FadeT)
    AC | Color -+- (A -<- 24)
    if Reverse: Ls = Ls.f
    for L Ls: push [L AC] Lines
    push [No AC] Lines
  StartDX if Reverse: 18 else 2
  less Reverse:
    Y += Lines.n*8
    Lines = Lines.f
  for Text,AC Lines.f:
    less got Text:
      "*".draw FB 0 Y-C+StartDX 20 color!AC shadow!0x000000
      pass
    Text.draw FB 16 Y-C 20 color!AC shadow!0x000000
    C += 16


for D u,d,l,r:
  GStl."barw[D]" = base!"ui/barw[D]" act!"ui/barw[D]a" dim!"ui/barw[D]"
//Stl.slider = 0.00,@[up down left right]{D="<pic arrow_[D] y!-1>"}
//font!FontName,Color,Bold
GStl.btn = wh!60,20 pic!'ui/btn' csz!40 sz!20 font!No,RGB_BLACK,1
GStl.tbtn = wh!60,20 pic!'ui/btn' csz!40 sz!25 font!No,RGB_WHITE,1
GStl.tab = wh!60,20 pic!'ui/tab' csz!210,160 sz!17 font!No,RGB_YELLOW,0
           pick!\tabd
GStl.tabd = wh!60,20 pic!'ui/tabd' csz!210,160 sz!17 font!No,RGB_BLACK,0
GStl.drpl = wh!60,20 pic!'ui/drpl' csz!80,80 sz!20,10 font!No,RGB_WHITE,1
GStl.knob = base!'ui/knob' //act!'ui/knob' dim!'ui/knob'



//UI Manager
type uim W H Root Cursor!host Title!No Size!any NoteLife!10.0 MaxNotes!8:
  w!W
  h!H
  root
  size!Size
  timers![]
  maxNotes!MaxNotes
  noteLife!NoteLife
  mice_xy!0,0
  mice_lmb!0
  mice_rmb!0
  result!No
  fb!No
  keys!(!)
  popup!0
  focus_stack!No..100
  focus_sp!0
  focus_wgt!No //widget currently receiving keyboard input
  xcl_stack!No..100
  xcl_sp!0
  xcl_wgt!No //widget to which input is currently limited
  mice_focus
  click_time!(!)
  cursor!Cursor //default cursor
  host_cursor!0
  fps!1
  fpsT!0.0
  frame!1
  turbo!0
  fpsGoal!1 // goal frames per second
  fpsD!30.0 //FPS divisor
  notes!No
  dummy!(wgt)
  mice_widget!(wgt)
  file_save_dlg
  file_load_dlg
  file_handler_fn
  stl_fg  ! RGB_WHITE   //font and pictogram color
  stl_fcs ! RGB_YELLOW  //selected font color (focus rect color)
  stl_wrk ! RGB_NIGHT   //working area color
  stl_alt ! RGB_DARK    //working area 2nd color - for lists
  stl_ctl ! RGB_GRAY    //control widget color
  stl_sel ! RGB_GRAY    //selected control widget color (picked tab, radio...)
  stl_edg ! RGB_DIM     //selection rectangle color
  screenshot_path!No   //if set, save the FB to this path and exit
  screenshot_frame!60  //frame on which to take the screenshot
  hit_test_list!No     //widget list used by `at XY`, populated per frame
  tick_fn!0            //per-frame hook for profilers / test harnesses
  // Per-phase timing — read via uim.t_update_ms / uim.t_draw_ms etc.
  // Useful for ui_speedup work (see Part 9 of ui_speedup.md). Updated
  // every frame; cheap to read.
  t_update_ms!0.0      //time spent in update_widgets last frame, ms
  t_draw_ms!0.0        //time spent in draw_widgets last frame, ms
  t_present_ms!0.0     //time spent in FB.pixels sync hack, ms
  t_total_ms!0.0       //sum of the above
  =
  Root.ub.'cls'(:ftx+lay+wnd = Root = bg RGB_BLACK Add!Root) //HACK
  $root = Root
  UIM = Me
  GIds.uim = Me
  $limit_fps 60
  $fb = gfx $w $h
  $set_title Title(No=(main_path).split^'/'(main_path:@_ ~ _))
  show_size: case $size:
    fixed = 1
    full = 2
    Else = 0
  for Msg NotifyMsgs.f: UIM.notify Msg
  NotifyMsgs = []
  $parse_screenshot_args
  when prof_pending_tick_fn: $tick_fn = prof_pending_tick_fn
  show: Es => UIM.adapt_wh
              //if $frame%14 >< 0: No.uim_clear_rect_cache_
              UIM.input Es
              FB UIM.render
              UIM.maybe_screenshot
              UIM.tick  //per-frame hook for profilers / harnesses
              FB //the show loop wants the gfx, not the screenshot side-effect
  when got $fb:
    $fb.free
    $fb = No
  R $result
  $result = No
  UIM = No
  ret R

@set_gid N V = GIds.N = V

uim_stl = GStl

@stl = GStl

ShownNotes []

@updated_notes =
  if no $notes:
    $notes = $maxNotes{[0.0 ""]:}
    $root.add: notifier id!notifier O!abs
  Clock clock
  Used $notes.keep(?0 > Clock)
  Free $notes.skip(?0 > Clock)
  Used,Free,Clock

@notify Text =
  Used,Free,Clock $updated_notes
  less Free.n: push Used^pop Free
  N Free.0
  N.0 = Clock + $noteLife
  N.1 = Text
  $notes =: @Used @Free

@clear_notes = for N $notes: N.0 = 0.0

@aspect = $w.float/$h

//ResizeCnt 24

@resize W H =
  if W><$w and $h><H: ret
  //if ResizeCnt+ > 24: halt //avoid endless resize loops
  $w = W
  $h = H
  //FIXME: pick random frame, so stuttering wont be sound pronounced
  No.uim_clear_rect_cache_

@adapt_wh =
  case show_wh().sexp: W,H =
    $resize W H

@key Key = $keys.Key >< 1

@set_title Title = show_set_title Title

// Default per-frame tick — does nothing. Profilers / harnesses set
// uim.tick_fn to a lambda that gets called each frame after render,
// useful for state machines that drive the game from outside.
@tick =
  Fn $tick_fn
  when Fn: Fn(Me)

// Headless screenshot mode for golden-image regression tests.
// Triggered by command-line args: `./go --screenshot=<path> [--screenshot-frame=<N>]`
// The window still opens (SDL needs a surface) but it self-exits after one frame.
@parse_screenshot_args =
  for A main_args():
    less A.is_text: pass
    if: A.begin '--screenshot=' =
          $screenshot_path = A.drop 13
        A.begin '--screenshot-frame=' =
          $screenshot_frame = (A.drop 19).int

@maybe_screenshot =
  less got $screenshot_path: ret
  when $frame << $screenshot_frame: ret
  less got $fb: ret  //already exited by a previous screenshot tick
  Path $screenshot_path
  $screenshot_path = No //one-shot; don't re-enter on next loop iteration
  Dir Path.url.0
  if Dir and not Dir.exists: Dir.mkpath
  $fb.save Path
  say "uim: wrote screenshot [Path] at frame [$frame]"
  $exit screenshot_done

@limit_fps GoalFPS =
  when $fpsGoal >< GoalFPS: ret
  $fpsGoal = GoalFPS
  $fpsD = $fpsGoal.float+8.0
  $fpsT = 0.0

// calculates current framerate and adjusts sleeping accordingly
@update_frame_timing StartTime FinishTime =
  DT FinishTime-StartTime
  $fpsT += DT
  when $frame%24 >< 0:
    $fps = @int 24.0/$fpsT
    $fpsT = 0.0
    when $fps < $fpsGoal and $fpsD < $fpsGoal.float*2.0: $fpsD += 1.0
    when $fps > $fpsGoal and $fpsD > $fpsGoal.float/2.0: $fpsD -= 1.0
  $frame+
  UIMFrame = $frame
  SleepTime 1.0/$fpsD - DT
  SleepTime

@update_widgets =
  $root.update_r
  // PHASE 3 (W1) was: clear the rect cache only when a widget setter
  // flipped GeomDirty. The trouble: many widgets mutate $xy_/$wh_
  // directly during their .update() (bypassing the =xy/=xywh setters
  // that flip the flag), so the cache went stale and layout broke.
  // For now: clear every frame as the original code did. The other
  // perf wins (pic9 compose cache, font cache, hit-test list cache,
  // disk-cache removal in store.s) still apply. A proper fix is to
  // either route every direct field write through the setter, or to
  // hash xy_/wh_ across the tree after update_r and detect changes.
  No.uim_clear_rect_cache_
  DL $root.draw_list.f
  // Cache the widget-only subset (filter out the [W] begin markers)
  // so `at XY` can hit-test without rebuilding the tree on every
  // mouse-move event. See ui_speedup.md (W6). DL is already in
  // reverse-draw-order (.f above), i.e. top-most-widget first --
  // which is what we want for hit-testing.
  $hit_test_list = DL.skip(?is_list)
  DL{Q.rect:[@_]}

@cursor_gfx =
  Cursor $mice_widget.mice_cursor
  less Cursor:
    Cursor = $cursor
    less Cursor:
      Cursor = $root.mice_cursor
      less Cursor: Cursor = \host //root defaults ot OS cursor
  less Cursor><host: less Cursor.is_gfx:
    if: Cursor.is_text = Cursor = get_pic Cursor
        Cursor.is_fn = Cursor = Cursor()
  Cursor

@set_cursor Cursor = $cursor = Cursor

@draw_cursor FB =
  if $frame<<1: ret //ensure SDL is inited
  CG $cursor_gfx
  if CG >< host:
    less $host_cursor:
      show_cursor 1
      $host_cursor = 1
    ret
  if $host_cursor:
    show_cursor 0
    $host_cursor = 0
  if no CG: ret
  FB.blit @$mice_xy CG

//each(wgt){cls}{[?order @?xy],?}.s(?0 < ??0){1}
@draw_widgets FB =
  when $w <> FB.w or $h <> FB.h:
    FB.free
    FB = gfx $w $h
    $fb = FB
  Profiling prof_enabled
  for W $root.draw_list:
    if W.is_list: W.0.end_draw_kids FB
    else
      if Profiling:
        T0 clock
        W.draw FB
        T1 clock
        prof_record_widget_draw W T1-T0
        prof_incr \widget_drawn
      else
        W.draw FB
  $draw_cursor FB
  when No.uim_show_timings_: $draw_timings_overlay FB
  FB

@draw_timings_overlay FB =
  // Black-shadowed bright-white text in the top-left so it doesn't
  // depend on a tab being open. Updated each frame; cheap enough that
  // its own draw doesn't skew the numbers materially.
  Line1 "FPS [$fps] total [$t_total_ms.@0.round]ms"
  Line2 "upd [$t_update_ms.@0.round] draw [$t_draw_ms.@0.round] present [$t_present_ms.@0.round]"
  Line1.draw FB 4 4 14 color!RGB_WHITE shadow!RGB_BLACK
  Line2.draw FB 4 20 14 color!RGB_WHITE shadow!RGB_BLACK

@render =
  //say: show_wh
  FB $fb
  when no FB: ret No
  StartTime clock
  $update_widgets
  T1 clock
  FB = $draw_widgets FB
  T2 clock
  $t_update_ms = (T1-StartTime)*1000.0
  $t_draw_ms = (T2-T1)*1000.0
  SleepSeconds $update_frame_timing StartTime T2
  less $turbo: when SleepSeconds > 0.0: sleep SleepSeconds
  FB.pixels //sync hack
  T3 clock
  $t_present_ms = (T3-T2)*1000.0
  $t_total_ms = $t_update_ms + $t_draw_ms + $t_present_ms
  cache_clean
  FB

@add_timer Interval Handler =
  $timers =: @$timers [Interval (clock)+Interval Handler]

@update_timers Time =
  Ts $timers
  less Ts.n: ret 0
  $timers =: // user code can insert additional timers
  Remove:
  for [N T] Ts.i: case T: Interval Expiration Fn =
    when Time >> Expiration:
      if got Fn() then Ts.N.1 = Time+Interval else push N Remove
  when Remove.n: | N 0; Ts = Ts.skip(X=>Remove.locate(N+)^got)
  $timers = @Ts @$timers
  0

@at XY =
  // Walks the cached hit-test list (populated by update_widgets in
  // reverse-draw-order, i.e. top-most-widget first). Falls back to
  // $root.at if the cache is cold (`uim.input` runs once before the
  // first `uim.render` populates the cache). See ui_speedup.md (W6).
  Ws $hit_test_list
  less got Ws:
    ret $root.at^XY(0+1.dimmed=$dummy)
  Hit 0
  for W Ws:
    if XY.in W.rect and W.solid and W.includes XY:
      less W.wpath.any(?restricts_input(W XY)):
        Hit = W
        done
  Hit(0+1.dimmed=$dummy)

@update_mice XY =
  PrevXY $mice_xy
  PrevWGT $mice_widget
  if XY<>PrevXY:
    $mice_xy <= XY
    if $mice_focus: $mice_focus.bcast [mice_move XY-PrevXY]
    else if got PrevWGT: PrevWGT.bcast [mice_move XY-PrevXY]
  NewWidget $at $mice_xy
  if NewWidget.id <> PrevWGT.id:
    when got PrevWGT: PrevWGT.bcast [mice over 0]
    $mice_widget = NewWidget
    NewWidget.bcast [mice over 1]
  NewWidget

@input Es =
  T clock
  $update_timers T
  MiceWgt $update_mice $mice_xy //get new widget under mice cursor
  for E Es: case E:
    mice_move XY = $update_mice XY
    mice Button State =
      if Button><left: $mice_lmb = State
      else if Button><right: $mice_rmb = State
      MP $mice_xy
      if $mice_focus
      then | LastClickTime $click_time.Button
           | when got LastClickTime and T-LastClickTime < 0.25:
             | MiceWgt.bcast [mice "double_[Button]"]
           | $click_time.Button =  T
           | $mice_focus.bcast [mice Button State]
           | less State: $mice_focus =  0
      else | $mice_focus = MiceWgt
           | MiceWgt.bcast [mice Button State]
      when State and MiceWgt.wants_focus:
      | FW $focus_wgt
      | less same FW MiceWgt:
        | when got FW: FW.bcast [focus 0 MiceWgt]
        | $focus_wgt = MiceWgt
        | MiceWgt.bcast [focus 1 FW]
    key Key State =
      $keys.Key = State
      D if got $focus_wgt then $focus_wgt
        else if not MiceWgt.wants_focus then MiceWgt //explicit focus?
        else 0
      when D: D.bcast [key Key State]
    resize NewW NewH = $resize NewW NewH
  No

@clipboard = show_read_clipboard

uim.'=clipboard' Text = show_write_clipboard Text

@focus_push NewFocusWidget =
  FW $focus_wgt
  when got FW: FW.bcast [focus 0 NewFocusWidget]
  when $focus_sp><$focus_stack.n: $focus_sp = 0
  $focus_stack.($focus_sp+) = $focus_wgt
  $focus_wgt = NewFocusWidget
  when got NewFocusWidget: NewFocusWidget.bcast [focus 1 FW]

@focus_pop =
  $focus_sp-
  when $focus_sp><-1: $focus_sp = $focus_stack.n-1
  PrevFocusWidget $focus_stack.$focus_sp
  $focus_stack.$focus_sp = No
  FW $focus_wgt
  when got FW: FW.bcast [focus 0 PrevFocusWidget]
  $focus_wgt = PrevFocusWidget
  when got PrevFocusWidget: PrevFocusWidget.bcast [focus 1 FW]

@focus = $focus_wgt(No=$dummy)

@refocus Widget =
  $focus_pop
  $focus_push Widget

@xcl_push NewXclWidget =
  when $xcl_sp><$xcl_stack.n: $xcl_sp = 0
  $xcl_stack.($xcl_sp+) = $xcl_wgt
  $xcl_wgt = NewXclWidget

@xcl_pop =
  $xcl_sp-
  when $xcl_sp><-1: $xcl_sp = $xcl_stack.n-1
  PrevXclWidget $xcl_stack.$xcl_sp
  $xcl_stack.$xcl_sp = No
  $xcl_wgt = PrevXclWidget
  PrevXclWidget
  
@exit @Result =
  $result = Result(No:[R]=R)
  $fb = No


text.q = GIds.Me
text.val = GIds.Me.val
text.upd V = GIds.Me.val = V
text.__ Method Args =
  MN Method^_method_name
  O GIds.Me
  if no O: bad "method `text.[MN]` is undefined"
  Args.0 = O
  Args.apply_method(Method)


/* WIDGETS

TODO:
- A ftx connected with a slider

*/


//FIXME:
//- cursor doesn't move onto empty lines
//- add indendation
cls ftx.WGT //represents one paragraph of text
  Val
  sz!n //font height
  auto_dim!4,4
  text_h!0
  font!WGT_FONT
  color!No
  shadow!No
  bold!0
  btn!0 //if 1 then act as a button
  cursor!No
  anchor!No
  wrap!No //No - no wrap
          //base - wrap to fit base width
          //auto - wrap text to fit W
          //integer values - wrap to pixel border
  :
  val_!0
  parsed!0
  adaptInfo!0
  pos_!0.0
  kid_rects![]
  // Per-widget rendered-text cache. For static (no-cursor, no inline
  // kid widget) ftx, glyphs are blitted once into an offscreen and the
  // per-frame cost collapses to one FB.blit. Dynamic-text ftx (fn-
  // valued val_) still benefits when the lambda returns a stable
  // result: the smlval parse runs but the glyph loop is skipped.
  // See ui_speedup.md G7 / ui_profile.log.
  text_cache!No
  text_cache_key!No
  =
  case Sz:
    t = $sz = LSZ_TITLE
    n = $sz = LSZ_NORMAL
  $val = Val
  if no Anchor: $anchor = Cursor
  for K $kids: K.renew_tagfrm //prevent from drawing, unless refed

@tip = $base.tip

@unpress = if $over: $act Me

@wants_focus = got $cursor

@restricts_input W XY = not (XY.in $rect) and $solid and $includes XY

@auto_w = $auto_dim.0
@auto_h = $auto_dim.1

@update_auto_wh Text X Y W H =
  $auto_dim = $draw2 0 0 X Y W H No No
  $adaptInfo =: $font $sz Text W H $kid_rects

@update_kid_rects =
  PlacedKids $kids.keep(?placed)
  //PlacedKids{K => say K K.rect}
  //$kid_rects = PlacedKids{rect}{X,Y,W,H = X,Y,X+W,Y+H}
  $kid_rects = PlacedKids{xy_ wh}{[X,Y W,H] = X,Y,X+W,Y+H}

@update =
  X,Y,W,H $rect
  WH $wh_
  Text $val_
  if Text.is_fn: Text = Text()
  if WH.any auto and $adaptInfo <> [$font $sz Text W H $kid_rects]:
    $update_auto_wh Text X Y W H
  K $kid0
  less K:
    $kid_rects = []
    ret
  $update_kid_rects

@draw2 FB Color X Y W H Pick Range =
  RangeX No
  Cursor $cursor
  if got Range:
    AI $adaptInfo
    AH if AI: AI.3 else H
    P $pos_
    RS $calc_shift P H
    Range = RS,AH
  if got Cursor and (FB or Pick.is_list): //shift view to see cursor
    CXY $ub.lblcur
    if got CXY:
      CX,CY CXY
      ShiftX W - (CX-X) - 2 //FIXME: RangeX should be recalced in @update
                            //       and only chage if cursor is out of range
      if ShiftX < 0 and: RangeX = -ShiftX,W
  if got Cursor and not $has_focus: Cursor = No
  R $smlval.draw FB X Y $sz font!$font
           color!Color bold!$bold shadow!$shadow
           wrap!$wrap(auto=W;base=$base.rect.2)
           range!Range
           rangeX!RangeX
           pick!Pick
           cursor!Cursor
           base!Me
  //$update_kid_rects
  R


@draw FB =
  X,Y,W,H $rect
  if $btn: Y += $pressed
  FB.scissor X Y W H
  Color $color
  less Color.is_int:
    if no Color: Color = gui().stl_fg
    else Color = Color.as_rgb
  if $dimmed: Color = Color.rgb_mul^|0.50
  // Pure-text fast path: if there are no inline kid widgets and no
  // editing cursor, the rendered glyph layout depends only on the
  // SML + style fields. Cache it as an offscreen and replay as a
  // single blit. Comparing smlval (deep list eq) handles both
  // static and lambda-valued text -- a lambda that returns stable
  // text gets caching for free.
  if not got $cursor and not $kid0 and W > 0 and H > 0:
    SVT $smlval
    Key W,H,Color,$sz,$font,$bold,$shadow,$wrap,SVT
    if got $text_cache and $text_cache_key >< Key:
      FB.blit X Y $text_cache
      ret
    when got $text_cache: $text_cache.free
    Cache gfx W H
    Cache.clear 0xFF000000 //transparent (inverted alpha)
    $draw2 Cache Color 0 0 W H No 1
    FB.blit X Y Cache
    $text_cache = Cache
    $text_cache_key = Key
    ret
  $draw2 FB Color X Y W H No 1

@end_draw_kids FB =
  Cursor $cursor
  if got Cursor:
    $ub.lblcur = $cursor_xy Cursor
    Anchor $anchor
    if Cursor <> Anchor:
      if Anchor > Cursor: swap Anchor Cursor
      FH $sz
      CX,CY $cursor_xy Anchor
      AX,AY $cursor_xy Cursor
      SX CX
      SY CY
      EX AX
      EY AY
      if SX > EX: swap SX EX
      EY += FH
      //FIXME: handle x-shifted case when 
      X,Y,W,H $rect
      if AX > CX:
        FB.rectangle RGB_WHITE 2 SX SY+FH/4 EX-SX EY-SY //center
      else
        FB.rectangle RGB_WHITE 2 SX SY+FH/4+FH EX-SX EY-SY-FH*2 //center
      FB.rectangle RGB_WHITE 2 X SY+FH/4+FH SX-X EY-SY-FH //left wing
      FB.rectangle RGB_WHITE 2 EX SY+FH/4 X+W EY-SY-FH //right wing
  FB.pop_scissor

@cursor_at ScreenXY =
  Rect $wgt_.rect
  X,Y,W,H Rect
  $draw2 0 0 X Y W H ScreenXY 1

@cursor_xy Cursor =
  Rect $wgt_.rect
  X,Y,W,H Rect
  AI $adaptInfo
  AH if AI: AI.3 else $sz
  P $pos_
  RS $calc_shift P AH
  $draw2 0 0 X Y W H Cursor 1


@set_cursor Cursor =
  $cursor = Cursor
  $anchor = Cursor

@mv_left =
  Cursor $cursor
  if no Cursor: ret
  if Cursor << 0: ret
  P $smlval
  Cursor-
  if Cursor < 0: ret
  Item P.Cursor
  while 1:
    case Item: 1.is_text+[s+n]+[pic+['/'@]@] =
      $set_cursor Cursor
      ret
    if --Cursor < 0: done
    Item = P.Cursor

@n = $smlval.n

@mv_end =
  Cursor $cursor
  if no Cursor: ret
  $set_cursor $n

@mv_right =
  Cursor $cursor
  if no Cursor: ret
  P $smlval
  N P.n
  Cursor+
  if Cursor >> N:
    if Cursor >< N: $set_cursor Cursor
    ret
  Item P.Cursor
  while 1:
    case Item: 1.is_text+[s+n]+[pic+['/'@]@] =
      $set_cursor Cursor
      ret
    if ++Cursor >> N: done
    Item = P.Cursor

@mv_down =
  Cursor $cursor
  if no Cursor: ret
  X,Y $cursor_xy Cursor
  $set_cursor: $cursor_at X,Y+$sz+$sz/2

@mv_up =
  Cursor $cursor
  if no Cursor: ret
  X,Y $cursor_xy Cursor
  $set_cursor: $cursor_at X,Y-$sz

@input E =
  $base.full_input E
  case E:
    mice_move _ = if got $cursor and $pressed:
                     $cursor = $cursor_at UIM.mice_xy
    mice left 1 = if got $cursor: $set_cursor: $cursor_at UIM.mice_xy
    key left 1 = $mv_left
    key right 1 = $mv_right
    key up 1 = $mv_up
    key down 1 = $mv_down
    key Key<backspace+delete 1 =
      if got $cursor:
        if $cursor >< $anchor:
          if $cursor:
            $mv_left
            if Key<>backspace: $mv_right
            $del_at $cursor
          else
            if Key><delete:
              $mv_right
              Cur $cursor
              if Cur:
                $mv_left
                if Cur <> $cursor: $del_at $cursor
        else
          $del_region $anchor $cursor
          $cursor = min $cursor $anchor
          $anchor = $cursor
    key Key 1 =
      if got $cursor and (Key.n><1 or Key >< enter):
        if:
          Key >< 'v' and uim.key^\lctrl =
            $cursor = $insert_at $cursor uim.clipboard
            $anchor = $cursor
          Key >< 'c' and uim.key^\lctrl =
            'uim'.clipboard = $copy_region $anchor $cursor
          Key >< 'x' and uim.key^\lctrl =
            'uim'.clipboard = $copy_region $anchor $cursor
            $del_region $anchor $cursor
            $cursor = min $cursor $anchor
            $anchor = $cursor
          1 = 
            if Key >< enter:
              if got $ub.noNL:
                $ub.onEnter(Q(Me):-No)
                ret
              Key = '\n' 
            $insert_at $cursor Key
            $set_cursor: $cursor+1

@copy_region A B =
  if A > B: swap A B
  P $smlval
  R P.drop(A).upto(B-A+1)
  R R.cunsml
  R

@del_region A B =
  if A > B: swap A B
  P $smlval{X=X}
  I A
  R:
  while I < B:
    Item P.I
    case Item: 1.is_text+[s+n]+[pic+['/'@]@] = P.I = []
    I+
  $val = P.skip^[].unsml

@insert_at Cursor Text =
  P $smlval
  LT Text.l(' '=[s]; '\n'=[n])
  P =: @P.take(Cursor) @LT @P.drop(Cursor)
  $val = P.unsml
  Cursor + LT.n

@del_at Cursor =
  P $smlval
  if P.end: ret
  P =: @P.take(Cursor) @P.drop(Cursor+1)
  $val = P.unsml

@pos = $pos_

@smlval =
  Parsed $parsed
  if Parsed: ret Parsed
  V $val_
  if V.is_fn:
    V = V()
    less V.is_text: V = "[V]"
    ret V.sml
  Parsed = V.sml
  $parsed = Parsed
  Cursor $cursor
  if got Cursor and Parsed.n << Cursor: $set_cursor: Parsed.n
  Parsed

@val = $smlval.cunsml //use $val_ to access the raw SML val

@'=val' V =
  less V.is_text or V.is_fn: V = "[V]"
  OldV $val_
  if V <> OldV:
    $val_ = V
    $parsed = 0
    if OldV: $ub.onEdit(Q(V):No)

@calc_shift P H = //allows shifting its kids
  if P.is_fn: P = P()
  TH $text_h-H
  if TH < H: TH = H
  RS (TH*P).int
  RS

@'=pos' P =
  $pos_ = P
  $calc_shift P $rect.3


#UI_TAB_BG 1
#UI_TAB_FG 2

cls btn.WGT(act=,w=rim,h=rim)
  Val
  Act
  Font!WGT_FONT
  sz!No
  Wrap!No //integer values produce multiline button labels
  tab!0
  style!btn
  FixLabel!0
  :
  style_
  bg
  label
  =
  //Wrap Sz*8
  Stl $resolve_style
  StlW,StlH Stl.wh
  LW,LH W,H
  if LW><auto: W = \rim
  if LH><auto: H = \rim
  NonRim | W(0:1.is_float) or H(0:1.is_float)
  if LW<>rim or LH<>rim:
    $wh_ = if NonRim: [W H] else [\rim \rim]
    if LW(:auto+rim): LW = StlW
    if LH(:auto+rim): LH = StlH
  else
    LW = StlW
    LH = StlH
  if no Sz: $sz = $calc_font_size LH(:1.is_float=StlH)
  if FixLabel: $fix_rim = 24
  LabelALign if FixLabel: \fix_cc else \cc
  Label ftx Val sz!$sz align!LabelALign btn!1 wrap!Wrap
  $label = Label
  if NonRim:
    W = 1.0
    H = 1.0
    LW = StlW
    LH = StlH
  MW,MH $min
  LW += MW - MW/3 //HACK!!!
  LH += MH - MH/3
  BG pic9 W!W H!H 0 0 btn!1 PW!0,LW PH!0,LH
  $bg = BG
  $resolve_style
  BG.add Label
  $add BG
  //Label.min = $min
  //Label.xy_ = $min/2
  //$add Label

@resolve_style =
  Stl $style
  if Stl.is_fn: Stl = Stl()
  if Stl.is_text: Stl = GStl.Stl
  Tab $tab
  if Tab:
    if Tab <> UI_TAB_FG:
      PickStyle Stl.pick
      if got PickStyle:
        Stl = GStl.PickStyle
  Label $label
  if no Label: ret Stl
  if same $style_ Stl: ret Stl
  $style_ = Stl
  Font,Color,Bold Stl.font
  Label.color = Color
  Label.bold = Bold
  Label.font = Font
  BG $bg
  Sz Stl.sz
  BG.picname = Stl.pic
  BG.srcSz = Stl.csz
  BG.sz = Sz
  W,H $wh_
  BG.pad = W(Sz(A,B=A)*2:1.is_float=0),H(Sz(A,B=B)/2:1.is_float=0)
  Stl


@calc_font_size H = $sz(No=H)

@val = $label.val
@'=val' V = $label.val = V

@font = $label.font
@'=font' Font = $label.font = Font

@update =
  X,Y,W,H $rect
  Label $label
  Label.sz = $calc_font_size H
  Stl $resolve_style
  Label $label
  BG   $bg
  Over $over
  Dim  $dimmed
  Tab  $tab
  Pressed $pressed
  Label.pressed = Pressed
  BG.pressed = Pressed
  BG.over = Over

@unpress = if $over: $act Me

//$wgt_.input E //call parent's input
@catch E =
  $full_input E
  $input E
  1


cls bg.WGT Color: color!Color(1.is_text=Color.as_rgb) =
@unpress = if $over: $act Me
@draw FB =
  Color $color
  if Color <> RGB_NONE: FB.rectangle Color 1 @$rect

cls spc.WGT
@unpress = if $over: $act Me


cls bx.WGT M Item fg!No bg!No
  :
  m!M
  =
  if M.is_int: $m = M..4
  S spc
  S.add Item
  $add S
  
@update =
  T,R,B,L $m
  S $kid0
  Item S.kid0
  X,Y,W,H Item.rect
  S.xy_ =: T L
  S.wh_ =: W H
  $wh_ =: W+L+R H+T+B


@draw FB =
  X,Y,W,H $rect
  prx (G _0; if got G: G.rectangle G _1 X Y W H) | $bg 1; $fg 0





//used for stuff like slider bar knobs and arrows icons
//basically a button without a label
cls knob.WGT(o=base,w=25,h=25) style!knob

@input E = $base.full_input E

@unpress = if $over: $act Me

@draw FB =
  X,Y,W,H $rect
  DoHL | $over or $pressed
  GUI gui
  Stl $style(S<1.is_text = GUI.stl.S)
  Gfx if: $dimmed = Stl.dim(No=Stl.base)
          $pressed = Stl.act(No=Stl.base)
          1 = Stl.base
  if got Gfx:
    if Gfx.is_text: Gfx = get_pic Gfx w!W h!H
    if DoHL: Gfx.brighten 30
    FB.blit X Y Gfx
    ret
  BC GUI.stl_ctl
  OC BC
  if DoHL: OC = GUI.stl_edg.rgb_mul^|1.50
  if $dimmed:
    BC = GUI.stl_dim
    OC = BC
  //draw bar
  BM 2 //bar marging
  BX,BY,BW,BH [X+BM Y+BM H-BM*2 H-BM*2]  //bar X,Y,W,H
  prx FB.rectangle @0:
    BC 1 BX BY BW BH
    OC 0 BX BY BW BH
    OC 0 BX+1 BY+1 BW-2 BH-2 

//slider tray
cls stray.WGT(w=120,h=20) pos!0.0 delta!0.1 full!No
  :=
  $add: knob

@inc = $val += $delta
@dec = $val -= $delta

@val = $pos

@'=val' V =
  V = V.clip(0.0 1.0)
  PrevPos $pos
  if PrevPos <> V:
    $pos = V
    $act V

@dir =
  W,H $wh
  if H>W: \v else \h

@update =
  X,Y,W,H $rect
  Bar $kid0
  if H>W:
    BW,BH W,W
    Bar.wh = BW,BH
    P ($pos*(H-BH)).int.clip(0 H-BH-1)
    Bar.xy = 0,P
  else
    BW,BH H,H
    Bar.wh = BW,BH
    P ($pos*(W-BW)).int.clip(0 W-BW-1)
    Bar.xy = P,0

@draw FB =
  FB.rectangle gui().stl_wrk 1 @$rect

@set_pos_xy XY =
  MX,MY XY
  X,Y,W,H $rect
  Bar $kid0
  BW,BH Bar.wh
  if $dir <> h:
    swap MX MY
    swap X Y
    swap W H
    swap BW BH
  P MX
  BW2 BW/2
  P = P.clip BW2 W-BW2
  P = (P - BW/2).float/(W-BW)
  $val = P

@input E =
  case E:
    mice_move _ = when $pressed: $set_pos_xy $mice
    mice left 1 = $set_pos_xy $mice

cls slider.WGT(w=120,h=20)
  Val!0.0
  full!No //in percent
  v!-1 //vertical slider
  tray!id*
  :
  val_!Val
  =


slider.update =
  X,Y,W,H $rect
  NV H > W
  if $v <> NV:
    $v = NV
    Val if $kid0: $kid0.next.val else $val_
    Act $get_act //FIXME: user can update act, during runtime
    $kid0 = 0
    if NV:
      TH | => | X,Y,W,H $rect; H - 2*W
      prx $add (@0):
        knob W!W H!W style!barwu Act!| => $tray.q.dec
        stray o!nl id!$tray W!W H!TH pos!Val full!No act!Act
        knob o!nl W!W H!W style!barwd Act!| => $tray.q.inc
    else
      TW | => | X,Y,W,H $rect; W - 2*H
      prx $add (@0):
        knob W!H H!H style!barwl Act!| => $tray.q.dec
        stray id!$tray W!TW H!H o!ph pos!Val full!No act!Act
        knob W!H H!H o!ph style!barwr Act!| => $tray.q.inc

cls pic.WGT name adapt!No solid!1: auto!(W><auto and H><auto) pic =
  if $name.is_gfx:
    $pic = $name
    $name = No
    $wh = $pic.w,$pic.h


OPAQUE_TSHLD 225

@includes PX,PY =
  if $solid<>alpha: ret 1
  $pic(No=1; G = | X,Y,@_ $rect; G.get(PX-X PY-Y).unrgba.3<OPAQUE_TSHLD)

@update =
  G No
  Name $name
  if no Name: ret
  if Name.is_fn:
    Name = Name()
    if Name.is_gfx:
      $pic = Name
      $wh_ = Name.w,Name.h
      ret
  if $auto:
    Adapt $adapt
    if got Adapt:
      Adapt if: Adapt.is_fn = Adapt() 
                Adapt.is_wgt = Adapt.rect.3
                1 = Adapt
      AWidth,AHeight if Adapt.is_list: Adapt else No,Adapt
      if AWidth.is_wgt: AWidth = AWidth.rect.2
      if AHeight.is_wgt: AHeight = AHeight.rect.3
      G = get_pic Name W!AWidth H!-AHeight
    else
      G = get_pic Name
    $wh_ = G.w,G.h
  else
    X,Y,W,H $rect
    G = get_pic Name W!W H!H
  $pic = G //FIXME: should we allow saving locally objects from the cache
           //       wiothut locking them?

@draw FB =
  X,Y,W,H $rect
  FB.blit X Y $pic


//all its ids will be places over a bg
cls tabs.WGT(w=rim,h=rim) Init Tabs: tabName table!Tabs = $pick Init

@tab = $kid0

@pick TabName ResetFocus!1 =
  PrevTab $tab
  when ResetFocus: when PrevTab: when got@@it gui:
    while got it.focus_wgt: it.focus_pop
  $tabName = TabName
  Tab $table.TabName
  when no Tab: bad "tabs.pick: no [TabName]"
  $kid0(:-Kid = Kid.unkid)
  $add Tab
  $xy_ = Tab.xy_
  $wh_ = Tab.wh_{auto=\rim}

@update =



cls htabs.WGT(w=rim,h=rim) Init Tabs onPick!0: tabs header =
  Header,Widgets Tabs.l.zip
  Titles,Ids Header.zip
  Tabs = [Ids Widgets].zip.t
  L spc W!rim H!rim
  Hdr pic9 Y!5 21 sz!7 'ui/thdr' W!rim H!rim PW!5 PH!5
  CX 0
  $header = map Title,Id Header:
    TabType Id(UI_TAB_BG:$Init=UI_TAB_FG)
    B btn Id!"_hd:[Id]" X!CX Y!0 o!ph tab!TabType style!tab Title: Btn =>
       $pick Btn.gid.drop^4
    CX = 0
    Hdr.add B
    B
  M WND_BORDER/2+2 //margin
  TabsL spc X!M Y!M W!rim H!rim
  TabsWidget tabs Init Tabs
  $tabs = TabsWidget
  TabsL.add TabsWidget
  Frame pic9 Y!-4 48 sz!24 'ui/wfrm' W!rim H!rim PW!M PH!M
  Frame.add TabsL
  L.add Hdr
  L.add Frame
  $add L

@pick Tab =
  for B $header: B.tab = UI_TAB_BG
  "_hd:[Tab]".q.tab = UI_TAB_FG
  $onPick(-F = F(Tab))
  $tabs.pick Tab

@current = $tabs.tabName

//lays its kids in a rect
//FIXME: centering items
//FIXME: implement scrolling
//FIXME: lay widget should auto add horizontal and vertical if XS is No
//       spacing. I.e. it should be able to arrange
//       items into all kinds of rects or tables

cls lay.WGT XS Kids N!No limitW!No
  :
  xys //X,Y spacing
  n //number of elements per line
  =
  YS 0
  if N.is_list:
    YS = N.1 //it also specifies vertical spacing
    N = N.0
  $xys = XS,YS
  $n = N
  for Kid Kids: $add Kid

lay.add Kid =
  Kid.ub.layOXY = Kid.xy_
  Kid.o = \base
  $wgt_.add Kid

lay.update =
  XS,YS $xys
  N $n(No=999999)
  LimitW $limitW(No=9999999)
  CX CY I MaxW MaxH 0
  Ks $kids.skip(?truly_hidden)
  for K Ks:
    OX,OY K.ub.layOXY
    KW,KH K.wh
    MaxH = max MaxH KH+OY
    K.xy_ = OX+CX,OY+CY
    CX += KW+OX+XS
    I+
    if CX >> LimitW or I >> N: //next row?
      MaxW = max MaxW CX-XS
      CY += MaxH + YS
      CX = 0
      MaxH = 0
      I = 0
  if CX: CY += MaxH
  MaxW = max MaxW CX //adjust to max row width
  MaxW -= XS
  if Ks.n < N: MaxW -= XS
  if MaxW < 0: MaxW = 0
  NewWH MaxW,CY
  if NewWH <> $wh_:
    $wh_ = NewWH













cls edln.WGT(w=256,h=rim)
  Text Cursor!0 Enter!No Edit!No Max!No Mode!normal Sz!n
  :
  picked!0
  =
  if got Enter: Cursor = No
  L ftx cursor!Cursor X!2 Sz!Sz Text
  L.max = W-2,No
  if got Cursor: L.ub.noNL = 1
  if got Edit: L.ub.onEdit = Edit
  if got Enter:
    $ub.onEnter = Enter
    L.ub.onEnter = | W => $confirm_entry
  $add: L

@wants_focus = got $kid0.cursor

@press = $act Me 0

@val = $kid0.val
@'=val' V = $kid0.val = V

@int @Default = $val.try_int Default(0:[X]=X)

@mv_end = $kid0.mv_end

@ftx = $kid0

EdlnRec 0 //FIXME: hack to prevent input being called multiple times
@input E =
  if EdlnRec: ret
  EdlnRec+
  Ftx $ftx
  case E:
    mice left 1 =
      if got Ftx.cursor:
        R Ftx.rect
        if $mice.0 > R.2: Ftx.mv_end
    mice right 1 =
      if got $ub.onEnter:
        if got Ftx.cursor: $confirm_entry
        else $begin_entry $mice
    mice double_left = $act Me 1
    focus 1 _ = gui().refocus Ftx
    focus 0 W =
      if got $ub.onEnter:
        if not same W.id $id and not same W.id Ftx.id:
          $confirm_entry
  EdlnRec-

@begin_entry XY =
  T $ftx
  $ub.litem_oldval = $val
  T.cursor = T.cursor_at XY+$rect[:2]
  T.anchor = T.cursor
  T.ub.noNL = 1

@index = $ub.litem_index

@'=index' Val = $ub.litem_index = Val

@oldval = $ub.litem_oldval

@entry_mode = got $ftx.cursor

@confirm_entry =
  less got $ub.onEnter: ret
  T $ftx
  if got T.cursor:
    T.cursor = No
    $ub.onEnter(:-No=Q($index $oldval $val))

@catch E =
  if EdlnRec: ret
  EdlnRec+
  $full_input E
  $input E
  EdlnRec-
  0

@update =
  $kid0.color = if $picked: gui().stl_fcs else gui().stl_fg

@draw FB =
  Idx $ub.litem_index(No=0)
  Color if $picked: gui().stl_sel
        else if Idx%2: gui().stl_alt else gui().stl_wrk
  X,Y,W,H $rect
  FB.rectangle Color 1 X Y W H
  Ftx $kid0
  if got Ftx.cursor and Ftx.has_focus: $draw_focus_rect FB



cls frm.WGT(w=auto,h=auto) Items sz!2 color!RGB_GRAY: =
  $pad = Sz,Sz
  Spc spc X!Sz Y!Sz W!rim H!rim
  for Item Items: Spc.add Item
  $wgt_.add Spc
 
@draw FB =
  FB.fat_rect $sz $color @$rect

@add W =
  $kid0.add W


//edit box
EDBX_MARGIN 3
cls edbx.WGT(w=256,h=128) Text: ftx =
  $ftx = ftx W!$w-EDBX_MARGIN H!$h-EDBX_MARGIN Text Cursor!0 Wrap!base
  $add: frm sz!EDBX_MARGIN [|bg RGB_NIGHT Add![$ftx]]

@val = $ftx.val
@'=val' V = $ftx.val = V
@int = $ftx.int



#LBOX_BORDER_SZ 2

lbox_item_click Item Done =
  Me Item.base
  while $ub.'cls' <> lbox: Me = $base
  Index Item.index
  $pick Index
  if Index < $xs.n and Done: $act $val 1
  
cls lbox.WGT(w=256,h=rim) nlines Xs Picked!No Enter!No
  :
  xs_!Xs offset_ picked_!Picked items slider
  =
  $wh_ = rim,rim
  $items = map I $nlines:
    Item edln o!nl cursor!No W!W Enter!Enter Xs[!I](No="")
              Act!&lbox_item_click
    Item.index = I
    Item
  BG frm sz!LBOX_BORDER_SZ $items
  $add BG
  $slider = slider O!ph W!20 H!(=>BG.h) act!: |V => $offset = @int V*$xs_.n
  $add $slider
  $offset = 0

@pick Index =
  if Index >> $xs.n or $picked_ >< Index: ret
  $picked_ = Index
  Items $items
  for Item Items: Item.picked = Item.index >< Index
  $act $val 0


@'=xs' Xs = $xs_ = Xs

@xs = $xs_

@offset = $offset_

lbox.=offset NO =
  if NO >< $offset_: ret
  Xs $xs
  N Xs.n
  O max 0: NO.clip 0 N-1 
  $offset_ = O
  for Item $items:
    Item.confirm_entry
    I O+
    Item.picked = I >< $picked_
    Item.index = I
    if I < N:
      T Xs.I
      when T.is_list: T = T.1
      Item.val = T
    else
      Item.val = ''

@slide P = $offset = @int $data.n.float*P

@picked =
  P $picked_
  if no P: ret
  Xs $xs
  if P >> Xs.n: ret No
  $picked_

@litem = $items.($picked - $offset_)

@val =
  P $picked
  if no P: ret No
  $xs.P{Val,Title=Val}

@title =
  P $picked
  if no P: ret No
  $xs.P{Val,Title=Title}

@data = $xs

@'=data' Ys =
  $xs = Ys
  $picked_ = No
  $offset_ = No
  $offset = 0


//FIXME: allow tiling for borders, and insides
//       otherwise things like flowers and patterns could be messed up
//FIXME2: allow variation for tiles
cls pic9.WGT srcSz picname sz!SrcSz btn!0 /*if 1 act as button*/ cut!0: pics
    composed_!0      // cached composed pic9 surface
    composed_key_!No // key invalidating composed_
  =
@tip = $base(B<-No = B.tip)

// Composes the 9 (or 4) source slices into a single offscreen gfx, sized
// W×H, so the per-frame cost collapses from 9 cache lookups + 9 blits
// into a single blit. The composition is cached on the widget itself,
// keyed by (picname, W, H, dimmed, cut). Hover-highlight (`HL`) is
// applied to the cached gfx as a one-shot blit-state on each draw, just
// like the original code does to the individual slices.
@compose_pic9 W H Cut PicName =
  SSz $srcSz
  SCDX,SCDY if SSz.is_list: SSz else SSz,SSz
  Src get_pic PicName
  SMW Src.w-2*SCDX
  SMH Src.h-2*SCDY
  T gfx W H
  T.clear RGB_NONE
  less SMW and SMH: //2x2 tiling fallback
    CX 0
    CY 0
    DX 0
    DY 0
    times I 4:
      GW,GH W/2,H/2
      SW,SH SCDX,SCDY
      G get_pic_sub PicName W!GW H!GH CX CY SW SH Index!I
      T.blit DX DY G
      if (I+1)%2:
        CX += SW
        DX += GW
      else
        CX = 0
        DX = 0
        CY += SH
        DY += GH
    ret T
  Sz $sz
  CDX,CDY if Sz.is_list: Sz else Sz,Sz
  CDX min CDX W/2
  CDY min CDY H/2
  MW W-2*CDX
  MH H-2*CDY
  CX 0
  CY 0
  DX 0
  DY 0
  times I 9:
    GW,GH I(:0+2+6+8=CDX,CDY;1+7=MW,CDY;3+5=CDX,MH;4=MW,MH)
    SW,SH I(:0+2+6+8=SCDX,SCDY;1+7=SMW,SCDY;3+5=SCDX,SMH;4=SMW,SMH)
    if Cut:
      if Cut -*- 2x0001:
        if:
          I < 3 = GH = 0
          I < 6 = GH += CDY
      if Cut -*- 2x0010:
        if:
          I(:2+5+8) = GW = 0
          I(:1+4+7) = GW += CDX
      if Cut -*- 2x0100:
        if:
          I > 5 = GH = 0
          I > 2 = GH += CDY
      if Cut -*- 2x1000:
        if:
          I(:0+3+6) = GW = 0
          I(:1+4+7) = GW += CDX
    // Source and destination cursors must be tracked separately.
    // CX,CY is the SOURCE sub-rect origin in the master PIC, advanced
    // by source widths SW/SH. DX,DY is the DESTINATION position in
    // the composed T, advanced by destination widths GW/GH (which
    // differ from SW/SH when the widget is wider/taller than the
    // PIC). Earlier versions used CX,CY for both, leaving gaps.
    G get_pic_sub PicName W!GW H!GH CX CY SW SH Index!I
    T.blit DX DY G
    if (I+1)%3:
      CX += SW
      DX += GW
    else
      CX = 0
      DX = 0
      CY += SH
      DY += GH
  T

@draw FB =
  X,Y,W,H $rect
  HL 0 //highlight
  PicName $picname
  if $btn:
    if $dimmed: PicName = "clrz!7F7F7F:[PicName]"
    else
      Y += $pressed
      HL = if $over: 30 else 0
  Cut $cut
  Key W,H,Cut,PicName
  less $composed_key_ >< Key:
    when $composed_: $composed_.free
    $composed_ = $compose_pic9 W H Cut PicName
    $composed_key_ = Key
  if HL: $composed_.brighten HL
  FB.blit X Y $composed_


cls wndicn.WGT(w=20,h=20) picname: =

@draw FB =
  X,Y,W,H $rect
  G get_pic $picname W!W H!H
  if $over: G.brighten if $pressed: 60 else 30
  FB.blit X Y+$pressed G


@unpress = if $over: $act Me


#WND_HDRSZ 24
#WND_BORDER 10
cls wnd.WGT(o=base,add=No,xy=X(No=0)\,Y(No=0),x=X!No,y=Y!No,w=rim,h=rim)
  Title
  Items
  onShow!No
  onHide!No
  canHide!1
  xcl_cancel!No
  : hdr frm center!No title
  =
  if no X or no Y: $center = 1
  GID $gid
  if no GID:
    GID = _wnd*
    $gid = GID
  $title = ftx o!base X!12 Y!4 Title
  Hdr
    pic9 O!base W!(=>max 100 $frm.wh.0) H!WND_HDRSZ drg!UI_DRG_TOP,GID
         20 'ui/whdr'
         sz!8 cut!2x0100 Add![$title]
  $hdr = Hdr
  //FIXME: add min and max icons
  if $canHide:
    Icn wndicn o!base X!-2 Y!2 align!r 'ui/wcls' act!| => $xhide
    Hdr.add Icn
  $add Hdr
  Frame pic9 48 sz!24 'ui/wfrm' W!rim H!rim PW!WND_BORDER PH!WND_BORDER
  Spc spc X!WND_BORDER Y!WND_BORDER W!rim H!rim
  $frm = Frame
  if got Items:
    for Item Items:
      Spc.add Item
  Frame.add Spc
  $add Frame

@set_title Title = $title.val = Title

@on_show @As =
  $onShow(Q():No)
  $frm.kid0.kid0.on_show @As

@on_hide @As =
  $onHide(Q():No)
  $frm.kid0.kid0.on_hide @As

@update =
  if got $center:
    $center = No
    //FIXME: hack to ensure frame and title get rim calculated
    $update_r
    $clear_rect_cache_r
    BX,BY,BW,BH $base.rect
    X,Y,W,H $rect
    $xy_ =: BW/2 - W/2
            BH/2 - H/2
    $clear_rect_cache_r

@close = if $is_xcl: $xhide else  $hide

//FIXME: new folder button (use existing edln)
//FIXME: pick folders as files, for folders with stuff like saves
cls fsview.WGT(w=256,h=rim)
  type //l=load file, s=save file, f=pick folder
  Path!(main_path) //where to start browsing fs
  lock!0 //lock exploration to $root
  filter!0 //filter files before displaying them
  onCancel!0
  foldersAsFiles!0
  : root lbox edln okBtn updList updVal =
  $wh_ = rim,rim
  $lbox = lbox 10 W!W [item0 item1 item2]
            //Enter!| Idx OldVal NewVal => say rename OldVal NewVal
            Act!| Val Done =>
                    if Val(:"[_]/") and not $foldersAsFiles:
                      less Done: $cd1 Val
                    else
                      E $edln
                      if got E:
                        E.val = Val
                        E.mv_end
                      $okBtn.undim
                      if Done: $do_pick
  $add: $lbox
  if $is_save:
    $edln = edln O!nl X!5 Y!10 W!W '' Act!: V => $updVal=1
    $add: $edln
  $set_path Path $lock 
  OkTitle $type(f=\Here;s=\Save;l=\Load)
  $okBtn = @dim: btn o!nl W!86 X!5 Y!5 OkTitle: => $do_pick
  $type(f = $okBtn.undim)
  $add: $okBtn
  $add: btn o!ph W!86 X!5 "Cancel" | => | $close; $onCancel(Q():0)

@on_show Filter!0 FoldersAsFiles!0 =
  $filter = Filter
  $foldersAsFiles = FoldersAsFiles

@set_path Path Lock!0 =
  Root,Name,Ext Path.url
  $root = Root
  $lock = if Lock: $root else 0
  $updList = 1 
  $val = ['' Name Ext].unurl

@is_save = $type >< s

@path = if $type >< f: $root else"[$root][$val]"

@val = $edln(Q.val:No=$kid0.val)

@'=val' V =
  if got $edln:
    $edln.val = V
    $updVal = 1

@do_pick = | $close; $act $path

@cd1 Sub = 
  if Sub <> '../': $cd "[$root][Sub]"
  else $cd "[$root.split^'/'[:-2].text^'/']/"

@cd NewRoot =
  $root = NewRoot
  $updList = 1

@update =
  if $updVal: when got@@it $edln: it.val.: V =
     if V.n and $filter(Q("[$root]/[V]"):0=1): $okBtn.undim
     else $okBtn.dim
  if $updList:
    $kid0.data = $list_folder $root
    $updList = 0

@list_folder Path =
  Items Path.items
  Files,Folders Items{~y.Q:"[_]/"=~x.Q}
  Filter $filter
  if Filter: Files = Files.keep(F => Filter("[Path]/[F]"))
  Up if $lock and Path.n << $lock.n: []
     else Path(['../']:'/'+"[_]:/"=[])
  : @Up @Folders.s @Files.s


uim.init_file_save_dlg =
  $file_save_dlg = @hide: wnd "Save":rowz:
    fsview id!fsSaveView s root!(main_path) o!nl Act!: File =>
      if ($file_handler_fn)(File): $file_save_dlg.xhide
  $root.add $file_save_dlg

uim.init_file_load_dlg =
  $file_load_dlg = @hide: wnd "Load":rowz:
    fsview id!fsLoadView l root!(main_path) o!nl Act!: File =>
      if ($file_handler_fn)(File): $file_load_dlg.xhide
  $root.add $file_load_dlg

uim.save Path Fn Lock!0 Filter!0 FoldersAsFiles!0 =
  less $file_save_dlg: $init_file_save_dlg
  $file_handler_fn = Fn
  fsSaveView.set_path Path lock!Lock
  $file_save_dlg.xshow Filter!Filter FoldersAsFiles!FoldersAsFiles

uim.load Path Fn Lock!0 Filter!0 FoldersAsFiles!0 =
  less $file_load_dlg: $init_file_load_dlg
  $file_handler_fn = Fn
  fsLoadView.set_path Path lock!Lock
  $file_load_dlg.xshow Filter!Filter FoldersAsFiles!FoldersAsFiles


#MBR_BORDERX 4
#MBR_BORDERY 4
#MBR_BORDER 6

cls menubar.WGT Menus:
  lay!0 sublays![]
  xcl_cancel!|=>$close_menu
  =
  Frame pic9 48 X!-2 Y!-12 sz!16,16 'ui/wfrm' W!rim H!rim
        PW!MBR_BORDERX PH!MBR_BORDERY
  $lay = lay 0 []
  Frame.add $lay
  $add Frame
  map [Title @Items] Menus: $add_menu Title Items

@solid = 0

@close_menu = | $unxcl; $sublays{hide}

@add_menu Title Items =
  Id 0
  Title(:[&Title &Id])
  less Id: Id = "menubar_[Title]_"
  //HACK: since normal bindins don't create closure
  bind_hide_fn Fn = B => | $close_menu; Fn(@[B].take(Fn.nargs))
  WItems map Item Items:
    Fn No
    Key No
    Name No
    case Item:
      No =
      N,V =
        Name = N
        if V.is_fn: Fn = V
        else
          Key = V
          Fn = | B => $act B.key Title B
      N =
        Name = N
        Fn = | B => $act B.key(No=B.val) Title B
    if no Name: spc W!32 H!16
    else
      B btn W!128 Name: bind_hide_fn Fn
      B.key = Key
      B

  WTitle btn Title Y!8 Id!Id: B =>
    LId "lay[B.gid]"
    if $is_xcl and $sublays.any(L => L.gid><LId and L.shown):
      $close_menu
    else
      less $is_xcl: $xcl
      $sublays{L = if L.gid><LId and L.hidden: L.show else L.hide}
  SubLay lay X!MBR_BORDER Y!MBR_BORDER 0 N!1 WItems
  Frame @hide: pic9 48 'ui/wfrm' O!abs Id!"lay[Id]"
                 X!(=>WTitle.x) Y!(=>WTitle.y+WTitle.h)
                 sz!24 W!rim H!rim PW!MBR_BORDER PH!MBR_BORDER
  Frame.add SubLay
  push Frame $sublays
  $lay.add WTitle
  $add Frame

//FIXME: nicer widget graphics
#DBX_BORDERX 4
#DBX_BORDERY 4
#DBX_BORDER 6
cls drpl.WGT items Init!0: //droplist
  hdr drplst litems
  =
  bind_hide_fn Fn = B =>
    $hdr.val = B.val
    $drplst.xhide
    Fn(@[B].take(Fn.nargs))
  InitKey No
  InitVal No
  $litems = map I,Item Items.i:
    Fn No
    Key No
    Name No
    case Item:
      No =
      N,V =
        Name = N
        if V.is_fn: Fn = V
        else
          Key = V
          Fn = | B => $act B.key B
      N =
        Name = N
        Fn = | B => $act B.key(No=B.val) B
    if no Name: spc W!32 H!16
    else
      B btn Name style!drpl W!128: bind_hide_fn Fn
      B.key = Key
      if Key >< Init: InitKey = I
      if Name >< Init: InitVal = I
      B
  if:
    got InitKey = Init = InitKey
    got InitVal=  Init = InitVal
  Title if Init < $litems.n: $litems.Init.val else ''
  //HACK: since normal bindins don't create closure
  $hdr = btn Title style!drpl: B =>
    L $drplst(No = $init_drop_list)
    if L.hidden: L.xshow else L.xhide
  $add $hdr
@init_drop_list =
  DrpLst lay X!DBX_BORDER Y!DBX_BORDER 0 N!1 $litems
  Frame @hide: pic9 48 'ui/wfrm' O!abs
                 X!(=>$hdr.x)
                 //HACK: bellow assumes item's H is proportional to hdr's H
                 Y!(=>|H $hdr.h; min $hdr.y+H uim.root.h-H*$litems.n)
                 sz!24 W!rim H!rim PW!DBX_BORDER PH!DBX_BORDER
  Frame.add DrpLst
  Frame.ub.'xcl_cancel' = X => $drplst.xhide
  $drplst = Frame
  uim.root.add Frame
  Frame
@auto_w = $hdr.w
@auto_h = $hdr.h
@val = $hdr(H=H.key(No=H.val))
@'=val' V = $hdr.val = V
@solid = 0

cls tip.WGT(o=_,w=rim,h=auto) Sz!16:
  ftx auto_h!Sz
  =
  $ftx = ftx color!white X!5 sz!Sz H!1.0 "" PW!5
  $add $ftx

@solid = 0
@auto_w = $base.w
@val = $ftx.val
@'=val' V = $ftx.val = V

@draw FB =
  X,Y,W,H $rect
  FB.rectangle 0xF020202F 1 @$rect
  
@update =
  Txt uim.mice_widget.tip
  if Txt.is_fn: Txt = Txt()
  $ftx.val = Txt
  if Txt >< "": $ghost else $unghost

MBMGN 40
cls msgbx.WGT(w=0.6) align!cc begin!0 end!0:
  title body btns
  result object!0
  btns_next!0
  text_next!0
  act_next!0
  queue![] // to allow several message boxes in a row
  active!0
  =
  //$max.0 = 600
  mk_btn I = btn PW!0,64 'Text': => $btn_click $btns.I.ub.opaque
  $title = ftx "" X!-MBMGN*3/2  sz!24 bold!1 align!c
             wrap!(=>$w - MBMGN*2) color!black
  $body = ftx "" O!nl Y!5 bold!1 wrap!(=>$w - MBMGN*2) color!black
  $btns = 3{I=mk_btn I}
  S spc W!1.0 Y!5 X!MBMGN PW!MBMGN PH!10
  BG pic9 W!1.0 40 'ui/btn' sz!MBMGN
  BG.add S
  S.add: $title
  S.add: $body
  S.add: lay O!nl X!-MBMGN*3/2 Y!10 align!c 5 $btns
  $add BG


@btn_click Result =
  $result = Result
  $active = 0
  when $end: $end()()
  $xhide
  $act $result
  less $queue.end:
    Args pop $queue
    $run_ @Args

@update =
  if $active or $queue.end: ret
  Args pop $queue
  $run_ @Args

@run_ Btns Title Text Object Act =
  $text_next = []
  $btns_next = 0
  $act_next = 0
  if Text.is_list and Text.n><0: Text = ''
  if Text.is_list and Text.n><1: Text = Text.0
  if Text.is_list:
    $text_next = Text.tail
    $btns_next = Btns
    $act_next = Act
    Text = Text.head
    Btns = [ok,'Ok']
    Act = | V => $run_ $btns_next Title $text_next $object $act_next
  $active = 1
  $object = Object
  $result = 0
  $title.val = Title
  $body.val = Text
  $set_act Act
  for B $btns: B.hide
  for [I [Value Text]] Btns.i:
    B $btns.I
    B.show
    B.val = Text
    B.ub.opaque = Value
  $xshow
  when $begin: $begin()()

@run Title Text Btns![ok,'Ok'] Object!0 Act!No =
  $show
  $queue =: @$queue [Btns Title Text Object Act]
  //$run_ Btns Title Text Object Act
  Me

cls chkbx.WGT(w=16,h=16,act=) val Act pic!'ui/cb':  =
chkbx.draw FB =
  X,Y,W,H $rect
  G get_pic W!W H!H "[$pic][$val]"
  FB.blit X Y+3 G
  //FB.rectangle 0xFFFFFF $val X Y W H
chkbx.press =
  $val = not $val
  $act $val

use prof
export gfx

ffi_begin macro gfx
ffi new_gfx_: new_gfx.ptr W.u4 H.u4
ffi free_gfx.void Gfx.ptr
ffi gfx_scaled.void Gfx.ptr Src.ptr
ffi gfx_scaled_smooth.void Gfx.ptr Src.ptr
//ffi gfx_load_png.ptr Filename.text
ffi gfx_from_png.ptr Data.ptr Size.int
ffi gfx_save_png.void Filename.text Gfx.ptr
ffi gfx_w.u4 Gfx.ptr
ffi gfx_h.u4 Gfx.ptr
ffi gfx_x.int Gfx.ptr
ffi gfx_y.int Gfx.ptr
ffi gfx_set_xy.void Gfx.ptr X.int Y.int
ffi gfx_get.u4 Gfx.ptr X.int Y.int
ffi gfx_rget.u4 Gfx.ptr X.int Y.int
ffi gfx_set.void Gfx.ptr X.int Y.int Color.u4
ffi gfx_dbg.int Gfx.ptr NewDbgState.int
ffi gfx_clear.void Gfx.ptr Color.u4
ffi gfx_line.void Gfx.ptr Color.u4 SX.int SY.int DX.int DY.int
ffi gfx_rect.void Gfx.ptr Color.u4 Fill.int X.int Y.int W.int H.int
ffi gfx_set_blit_quad.void Gfx.ptr
          X0.int Y0.int X1.int Y1.int X2.int Y2.int X3.int Y3.int
ffi gfx_circle.void Gfx.ptr Color.u4 Fill.int X.int Y.int R.int
ffi gfx_triangle.void Gfx.ptr Color.u4 AX.int AY.int BX.int BY.int CX.int CY.int
ffi gfx_pixels.ptr Gfx.ptr
ffi gfx_mask.ptr Gfx.ptr
ffi gfx_filter.void Gfx.ptr Dst.ptr Kernel.ptr Size.int
ffi gfx_set_bflags_flip_x.void Gfx.ptr
ffi gfx_set_bflags_flip_y.void Gfx.ptr
ffi gfx_set_blit_saturation.void Gfx.ptr Factor.float
ffi gfx_set_blit_bright.void Gfx.ptr Amount.int
ffi gfx_set_blit_fade.void Gfx.ptr Amount.int
ffi gfx_set_gamma.void Gfx.ptr Gamma.float
ffi gfx_set_blit_light.void Gfx.ptr Energy.float Color.u4
ffi gfx_set_blit_additive.void Gfx.ptr Energy.float Color.u4
ffi gfx_set_blit_rect.void Gfx.ptr BX.int BY.int BW.int BH.int
ffi gfx_scissor.void Gfx.ptr X.int Y.int W.int H.int
ffi gfx_pop_scissor.void Gfx.ptr
ffi gfx_set_blit_eraser.void Gfx.ptr
ffi gfx_recolorable.void Gfx.ptr Hue.u4 Range.u4
ffi gfx_recolorable_xs_begin.ptr Gfx.ptr NRecolors.int
ffi gfx_recolorable_xs_end.void Gfx.ptr
ffi gfx_recolor_xs.ptr Gfx.ptr NRecolors.int
ffi gfx_recolor.ptr Gfx.ptr Color.u4 Brightness.float
ffi gfx_get_recolored.ptr Gfx.ptr
ffi gfx_set_recolored.void Gfx.ptr Recolored.ptr
ffi gfx_is_recolorable.int Gfx.ptr
ffi gfx_blit.void Gfx.ptr X.int Y.int Src.ptr
ffi gfx_rle.ptr Gfx.ptr
ffi gfx_margins.ptr Gfx.ptr
ffi gfx_outline_cut.void Gfx.ptr Off.u4 On.u4
ffi gfx_make_heightmap.void Gfx.ptr Stencil.ptr Expt.int Smooth.float Seed.u4
ffi gfx_count.u4 Gfx.ptr Color.u4
ffi gfx_colorize.void Gfx.ptr Color.u4
ffi gfx_hue.void Gfx.ptr Hue.int
ffi gfx_skewed.void Gfx.ptr Src.ptr Amount.int
ffi gfx_median_blur.void Gfx.ptr Src.ptr KernelSize.int


gfx_load_png Filename =
  [Data Size] raw_file_ Filename
  less Data: bad "couldnt read [Filename]"
  R gfx_from_png Data Size
  ffi_free Data
  R

type gfx.no_copy @As: handle id!-1 w h = case As:
  [W<1.is_int H<1.is_int] = $resize(W H)
  [Filename<1.is_text] = $load(Filename)
gfx.load Filename =
| when $handle: free_gfx $handle
| $handle =  gfx_load_png Filename
| $w =  gfx_w $handle
| $h =  gfx_h $handle
gfx.free =
| $id =  -1
| when $handle:
  | free_gfx $handle
  | $handle =  0
gfx.resize W H = //resizes the canvas, discarding the image
  when $handle: free_gfx $handle
  GH new_gfx_ W H //FIXME: GH is a hack, because new_gfx_ FFI call is bugged
  $handle = GH
  $w = W
  $h = H
  Me
gfx.scaled Src = gfx_scaled Me.handle Src.handle
gfx.scaled_smooth Src = gfx_scaled_smooth Me.handle Src.handle
gfx.median_blur Amount =
  W $w
  H $h
  T gfx W H
  gfx_median_blur T.handle $handle Amount
  T
gfx.scaled_median Src =
  DW $w
  DH $h
  SW Src.w
  SH Src.h
  if DW < SW or DH < SH:
    $scaled_smooth Src
    ret
  TW 1
  TH 1
  while TW < DW: TW *= 2
  while TH < DH: TH *= 2
  BlurAmount if TW > TH: TW/SW else TH/SH
  BlurAmount += 1
  less BlurAmount%2: BlurAmount+
  T gfx TW TH
  T.scaled Src
  TT T.median_blur BlurAmount
  TTT gfx DW DH
  TTT.scaled_smooth TT
  TTT.filter unsharp Dst!Me
  T.free
  TT.free
  TTT.free
gfx.rescale W H Type!0 =
  MW $w
  MH $h
  when MW><W and MH><H: ret
  T gfx W H
  if Type:
    if Type><2: T.scaled_median Me
    else
      if W and H and MW and MH:
        T.scaled_smooth Me
        /*TT gfx T.w T.h
        TT.scaled_smooth Me
        TT.filter unsharp Dst!T times!1
        TT.free*/
  else T.scaled Me
  when $handle: free_gfx $handle
  $handle =  T.handle
  T.handle =  0
  $w =  W
  $h =  H
  Me
//downsample using box blue and decimation
gfx.downbox Q =
  N 0
  P 1
  while P < Q:
    P *= 2
    N+
  L gfx $w+2 $h+2
  L.clear 0xFF000000
  L.blit 1 1 Me
  T L.filter boxblur times!N //box blur + NN gives better result than lerp
  L.free
  R gfx T.w/Q T.h/Q
  R.clear 0xFF000000
  R.scaled T
  T.free
  R
gfx.skewed Amount =
| W $w + Amount
| H $h
| T gfx W H
| gfx_skewed T.handle $handle Amount
| T
gfx.set_handle NewHandle =
  $free
  $handle = NewHandle
  $w = gfx_w $handle
  $h = gfx_h $handle
gfx.colorize Color = gfx_colorize $handle Color
gfx.hue Hue = gfx_hue $handle Hue
gfx.xy = [(gfx_x $handle) (gfx_y $handle)]
gfx.=xy [X Y] = gfx_set_xy $handle X Y
gfx.get X Y = gfx_get $handle X Y
gfx.rget X Y = gfx_rget $handle X Y //RLE safe get
gfx.set X Y Color = gfx_set $handle X Y Color
gfx.dbg NewDebugState = gfx_dbg $handle NewDebugState
gfx.clear Color = gfx_clear $handle Color
gfx.line Color A B = gfx_line $handle Color A.0 A.1 B.0 B.1
gfx.lines Color Ss =
  if Ss.end: ret
  Prev pop Ss
  till Ss.end:
    S pop Ss
    $line Color Prev S
    Prev = S
gfx.rectangle Color Fill X Y W H = gfx_rect $handle Color Fill X Y W H
gfx.circle Color Fill C R = gfx_circle $handle Color Fill C.0 C.1 R
gfx.triangle Color A B C = gfx_triangle $handle Color A.0 A.1 B.0 B.1 C.0 C.1
gfx.quad A B C D = gfx_set_blit_quad $handle A.0 A.1 B.0 B.1 C.0 C.1 D.0 D.1
gfx.save Filename = gfx_save_png Filename $handle
gfx.pixels = gfx_pixels $handle
gfx.mask = gfx_mask $handle
gfx.rect X Y W H =
| gfx_set_blit_rect $handle X Y W H
| Me
gfx.scissor X Y W H =
  gfx_scissor $handle X Y W H
  Me
gfx.pop_scissor =
  gfx_pop_scissor $handle
  Me
gfx.flop = //flip by x during blit (mirror)
| gfx_set_bflags_flip_x $handle
| Me
gfx.flip = //flip by y during blit
| gfx_set_bflags_flip_y $handle
| Me
gfx.recolorable Hue Range = //prepare specific hue for recoloring (matte)
  gfx_recolorable $handle Hue Range
  Me
gfx.recolor Color Brightness = //recolors the previously specified hue
  gfx_recolor $handle Color Brightness
  Me
gfx.recolorable_xs Colors = //assings a set of colors inside gfx for recoloring
  NRecolors   Colors.n
  P gfx_recolorable_xs_begin $handle NRecolors
  times I NRecolors: _ffi_set uint32_t P I Colors.I
  gfx_recolorable_xs_end $handle
  Me
gfx.recolor_xs Colors = //recolors the set of colors specified by recolorable_xs
  NRecolors   Colors.n
  P   gfx_recolor_xs $handle NRecolors
  times I NRecolors: _ffi_set uint32_t P I Colors.I
  Me

//save recolor for future use with gfx.as_recolored
gfx.recolored = gfx_get_recolored $handle

//use saved recolor
gfx.as_recolored Recolored =
| gfx_set_recolored $handle Recolored
| Me

//returns true if gfx already got recolor structure ready
gfx.is_recolorable = gfx_is_recolorable $handle

gfx.saturate Factor =
| gfx_set_blit_saturation $handle Factor
| Me
gfx.brighten Amount = //brighten or darken
| gfx_set_blit_bright $handle Amount
| Me
gfx.gamma Gamma =
| gfx_set_gamma $handle Gamma
| Me
gfx.light Energy Color = //blit as light, coloring texture underneath
| gfx_set_blit_light $handle Energy Color
| Me
gfx.additive Energy Color = //additive blending
| gfx_set_blit_additive $handle Energy Color
| Me
gfx.fade Amount =
| gfx_set_blit_fade $handle Amount
| Me
gfx.eraser = gfx_set_blit_eraser $handle
gfx.blit X Y Src =
| prof_incr \blit
| if Src.is_gfx
  then _type gfx Src:
    //_ffi_call \(void ptr int int ptr) FFI_gfx_gfx_blit_ $handle X Y Src.handle
    gfx_blit $handle X Y Src.handle
  else Src.draw(Me X Y)
| Me
gfx.rle =
| RG   gfx_rle $handle
| less RG: ret 0
| free_gfx $handle
| $handle =  RG
| 1
gfx.margins =
| P   gfx_margins $handle
| [(_ffi_get uint32_t P 0) (_ffi_get uint32_t P 1)
   (_ffi_get uint32_t P 2) (_ffi_get uint32_t P 3)]
gfx.cut X Y W H =
| G   gfx W H
| G.clear(0xFF000000) // transparent
| SX,SY   $xy
| G.blit(-SX -SY Me.rect(X Y W H))
| G
gfx.copy = $cut(0 0 $w $h)
gfx.deep_copy = $cut(0 0 $w $h)
gfx.non_rle =
| R   gfx $w $h
| R.clear(0xFF000000)
| XY   $xy
| R.blit(-XY.0 -XY.1 Me)
| R.xy =  $xy
| R
gfx.frames W H =
| GW   $w
| dup I GW*$h/(W*H): $cut(I*W%GW I*W/GW*H W H)
gfx.render = Me
gfx.as_text = "#(gfx [$w] [$h])"
gfx.tile G = //tile gfx G to cover whole $w * $h space
| W   $w
| H   $h
| TW   G.w
| TH   G.h
| times Y (H+TH-1)/TH:
  | times X (W+TW-1)/TW:
    | $blit(X*TW Y*TH G)

gfx._filter Kernel Dst =
| Sz   Kernel.n
| less Dst: Dst =  gfx $w $h
| K   ffi_alloc Sz*4
| times I Sz: _ffi_set uint32_t K I Kernel.I.int32
| gfx_filter $handle Dst.handle K Sz
| ffi_free K
| Dst

BoxBlurKernel   1/9.0..9 
GaussKernel  
  [1  4  6  4 1
   4 16 24 16 4
   6 24 36 24 6
   4 16 24 16 4
   1  4  6  4 1]/256.0
IdentityKernel   [0.0 0.0 0.0
                  0.0 1.0 0.0
                  0.0 0.0 0.0]
SharpKernel   [ 0.0 -1.0  0.0
               -1.0  5.0 -1.0
                0.0 -1.0  0.0]
UnsharpMask
  [1  4    6  4 1
   4 16   24 16 4
   6 24 -476 24 6
   4 16   24 16 4
   1  4    6  4 1] * -1.0 / 256
DiskBlur //bokeh blur
  [0 0 1 1 1 0 0
   0 1 1 1 1 1 0
   1 1 1 1 1 1 1
   1 1 1 1 1 1 1
   1 1 1 1 1 1 1
   0 1 1 1 1 1 0
   0 0 1 1 1 0 0] / 37.0
get_kernel_by_name Name =
| if Name><gauss then GaussKernel
  else if Name><boxblur then BoxBlurKernel
  else if Name><identity then IdentityKernel
  else if Name><sharpen then SharpKernel
  else if Name><unsharp then UnsharpMask
  else if Name><disk then DiskBlur
  else bad "gfx.filter: unknown kernel [Name]"

gfx.filter Kernel dst!0 times!1 =
| when Kernel.is_text: Kernel =  get_kernel_by_name Kernel
| when Times><1: ret $_filter(Kernel Dst)
| when Times><0: ret $_filter(IdentityKernel Dst)
| R   $_filter(Kernel 0)
| times I Times-2
  | NG   R._filter(Kernel 0)
  | R.free
  | R =  NG
| R._filter(Kernel Dst)


gfx.matte = //separate alpha channel as grayscale
| W   $w
| H   $h
| O   gfx W H
| times Y H: times X W:
  | A   $get(X Y) ->- 24
  | C   (A -<- 16) + (A -<- 8) + A
  | O.set(X Y C)
| O

gfx.extremum =
| Min   0
| Max   0
| MinXY   [0 0]
| MaxXY   [0 0]
| times Y $h: times X $w:
  | V   $get(X Y)
  | when Min > V:
    | Min =  V
    | MinXY =  X,Y
  | when Max < V:
    | Max =  V
    | MaxXY =  X,Y
| Min,MinXY,Max,MaxXY

//count the number of some pixel occurence
gfx.count Color = gfx_count $handle Color

/*
gfx.count Value =
| Count 0
| NPixels $w*$h
| Pixels $pixels
| Ptr 0
| while Ptr < NPixels:
  | Pixel _ffi_get uint32_t Pixels Ptr
  | when Pixel >< Value: Count+
  | Ptr+
| Count
*/

//FIXME: this needs proper implementation
gfx.invert = for Y $h: for X $w: $set X Y 255-$get(X Y)


gfx.outline_cut Off On =
  gfx_outline_cut $handle Off On
  0

gfx.make_heightmap Stencil Expt Smooth Seed =
  gfx_make_heightmap $handle Stencil.handle Expt Smooth Seed
  0

gfx.fat_rect Sz Color X Y W H =
  times I Sz: $rectangle Color 0 X+I Y+I W-I*2 H-I*2

//FB.text PX+DX-4 PY+$h-12 16 CT Color!white
gfx.text X Y Size Text @As = Text.sml.draw Me X Y Size @As


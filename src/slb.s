//Voxel Processing Library
use gfx m m3d slb_
export slb camera

#:slb

Bug 0


MinDim 2
MaxDim 1024

type camera:
   origin![0.0 0.0 0.0] //camera x,y,z
   angle![0.0 0.0 0.0]  //camera angle
   focal_length!100.0
   perspective!1
   shading!default      //what shading method to use for render
   scale!0.5            //camera lens scale
   screen_w!256
   screen_h!256

camera.canvas_scale = 128.0

camera.init C =
  $origin   = C.origin
  $angle    = C.angle
  $scale    = C.scale
  $screen_w = C.screen_w
  $screen_h = C.screen_h

camera.view_matrix = IdMtx.rot $angle*DEG
camera.direction = [0.0 0.0 1.0].mm $view_matrix.zip
camera.xdir = [1.0 0.0 0.0].mm $view_matrix.zip
camera.ydir = [0.0 1.0 0.0].mm $view_matrix.zip

camera.prepare =
  Flags 0
  Flags = Flags.bitSetM SLB_CAM_PERSPECTIVE $perspective
  Scale $scale/$canvas_scale
  O $origin
  M $view_matrix.ll
  vxCamera O.0 O.1 O.2
           M.0.0 M.0.1 M.0.2
           M.1.0 M.1.1 M.1.2
           M.2.0 M.2.1 M.2.2
           Scale $focal_length $screen_w $screen_h
           Flags



camera.normalize_angle =
  RX,RY,RZ $angle%360.0
  when RX < 0.0: RX = 360.0 + RX
  when RY < 0.0: RY = 360.0 + RY
  when RZ < 0.0: RZ = 360.0 + RZ
  $angle =: RX RY RZ


SlbId 0

type slb W H D App!0 Value!0 Nil!0 Abyss!0: //Slab - voxel graphics item
  id
  handle
  w!D h!H d!D
  app!App
  xyz_![0.0 0.0 0.0]         //items location inside canvas
  center_![0.0 0.0 0.0]      //center of this item
  angle_![0.0 0.0 0.0]       //rotation around center
  color![1.0 1.0 1.0]        //intensity for lights
  scale_!1.0
  scale_box![1.0 1.0 1.0]
  label_!""
  visible!1  //show it in viewport
  ui_layer //UI layer item inside the layer list
  cfg!(!)
  flags
  revision  //revision serial counter, so views can sync with us.
  index!-1  //index inside of a saving list
  group     //group this layer belongs to
  items![]  //for groups, lists of items inside of it
  =
  $id = SlbId+
  $d = 1
  $resize W H D
  $vxSetNilAbyss Nil Abyss
  $clear Value

slb.add_item Item =
  if Item.group.is_slb: Item.group.remove_item Item
  if Item.id >< $id:
    Item.group = 0
    ret
  Item.group = Me
  $items =: @$items Item

slb.remove_item Item =
  $items = $items.skip(?id >< Item.id)

slb.xyz = $xyz_
slb.=xyz V =
  if $is_group:
    D V-$xyz_
    for S $items: S.xyz += D
  $xyz_ = V

slb.center = $center_
slb.=center V = $center_ = V

slb.angle = $angle_
slb.=angle V = $angle_ = V

slb.scale = $scale_
slb.=scale V = $scale_ = V

RevisionCount 0

slb.revise = $revision = RevisionCount+

slb.label = $label_

slb.=label Text = $label_ = "[Text]"

slb.set_visibility State =
  if $visible <> State:
    for S $items: S.grp_invisible = not State
    $visible = State
    $revise
  if $ui_layer: $ui_layer.set_visibility State

slb.edge_size = max $w $h $d

slb.reset_center = $center = ([$w $h $d].float*0.5).int.float

slb.box_center = [$w.float*0.5 $h.float*0.5 $d.float*0.5]


slb.get X Y Z = vxGet $handle X Y Z 

slb.set X Y Z V =
  vxSet $handle X Y Z V
  RevisionCount+


slb.getTr X Y Z = vxGetTr $handle X Y Z

slb.setTr X Y Z V = vxSetTr $handle X Y Z V

slb.getI Index = vxGetI $handle Index

slb.setI Index Color = vxSetI $handle Index Color

slb.vxSetNilAbyss Nil Abyss = vxSetNilAbyss $handle Nil Abyss

slb.set_angle Angle =
  RX,RY,RZ Angle%360.0
  if RX < 0.0: RX =  360.0 + RX
  if RY < 0.0: RY =  360.0 + RY
  if RZ < 0.0: RZ =  360.0 + RZ
  $angle =: RX RY RZ
  $revise


slb.resize W H D = //FIXME: add option to preserve the data, moving it
  if $handle: vxFree $handle
  $handle = vxNew W H D
  $w = W
  $h = H
  $d = D
  $reset_center
  $revise

slb.reinit W H D Value!No Nil!0 Abyss!0 =
  $resize W H D
  $angle =: 0.0 0.0 0.0
  $xyz =: 0.0 0.0 0.0
  $scale_box =: 1.0 1.0 1.0
  $color =: 1.0 1.0 1.0
  $scale = 1.0
  $visible = 1
  $revise
  $ui_layer = 0
  $flags = 0
  $label = "layer[$id]"
  if got Value: $clear Value
  $vxSetNilAbyss Nil Abyss

slb.turn_into_light =
  less $flags-*-SLB_LIGHT: $flags += SLB_LIGHT
  $revise

slb.turn_into_group =
  less $flags-*-SLB_GROUP:
    $flags += SLB_GROUP
    less $flags-*-SLB_INVISIBLE: $flags += SLB_INVISIBLE
    $grp_open = 1
  $revise

slb.is_group = $flags-*-SLB_GROUP

slb.grp_invisible = $flags-*-SLB_GROUP_VIS
slb.=grp_invisible V = $flags = $flags.bitSetM SLB_GROUP_VIS V

slb.grp_open = $flags-*-SLB_GROUP_OPEN
slb.=grp_open V = $flags = $flags.bitSetM SLB_GROUP_OPEN V

slb.list =
  less $id: ret $items{list}.j
  if $items.end: ret [Me] else [Me @$items{list}.j]

slb.open_parents = if $group: $group.grp_open and $group.open_parents
                   else 1

slb.depth = if $group: 1+$group.depth else 0

slb.show_bounds =
  $flags = $flags -^- SLB_SHOW_BOUNDS
  $revise

slb.swap S =
  Items $group.items
  PA Items.locate(?id><$id)
  PB Items.locate(?id><S.id)
  Items.PA = S
  Items.PB = Me
  $group.items = Items

slb.next =
  Items $group.items
  P Items.locate(?id><$id)
  if P+1 < Items.n then Items.(P+1) else 0

slb.prev =
  Items $group.items
  P Items.locate(?id><$id)
  if P-1 >> 0 then Items.(P-1) else 0

slb.move_under Slb =
  Items $group.items.skip(?id><$id)
  P Items.locate(?id><Slb.id)+1
  Items: @Items.take(P) Me @Items.drop(P)
  $group.items = Items

slb.move_out_of_group =
  Outside $group.group
  less Outside: ret
  Outside.add_item Me

label_incremented Me Delim =
  F $f
  P F.locate Delim
  if got P:
    Ds F.take(P).f
    if Ds.is_digit: ret "[F.drop(P+1).f][Delim][Ds.int+1]"
  "[Me][Delim]1"

slb.clone Name!No =
  C slb $w $h $d App!$app
  C.xyz = $xyz
  C.center = $center
  C.angle = $angle
  C.color = $color
  C.scale = $scale
  C.scale_box = $scale_box
  C.visible = $visible
  C.flags = $flags
  C.label = if got Name: Name else label_incremented $label _
  vxFree C.handle
  H vxClone $handle
  C.handle = H //apparetly there is some code generator bug
               //because `C.handle = vxClone $handle` crashes setting
               //handle to incorrect address. Adding `say C.handle` hides it
  C.set_clay_color $clay_color
  $group.add_item C
  C.move_under Me
  if $is_group: for S $items:
    SC if $app: $app.clone S else $clone S
    C.add_item SC
  C

slb.free =
  when $id >< -1: bad "slab freed multime times."
  when $group: $group.remove_item Me
  $group = 0
  for S $items: if $app: $app.free_layer S else S.free
  $id = -1
  vxFree $handle

slb.clear Color =
  vxClear $handle Color
  $revise


slb.bounding_box =
  MinXYZ $xyz-$center
  MaxXYZ MinXYZ + [$w $h $d].float
  MinXYZ,MaxXYZ

slb.orient =
  Center    $center
  CX,CY,CZ  Center
  X,Y,Z     $xyz
  AX,AY,AZ  -$angle
  SX,SY,SZ  $scale_box*$scale //scale of the slab along x,y,z
  R,G,B     $color
  vxOrient  $handle $flags SX SY SZ CX CY CZ AX AY AZ X Y Z R G B




slb.margins = vxMargins($handle).parse.0.group(3)

slb.expand_canvas2 BMin BMax =
  X,Y,Z BMin
  NW,NH,ND BMax-BMin
  when not X and not Y and not Z and NW><$w and NH><$h and ND><$d: ret 0
  when NW>MaxDim or NH>MaxDim or ND>MaxDim: ret 0
  New vxCopyBox $handle X Y Z X+NW Y+NH Z+ND
  vxFree $handle
  $handle =  New
  $w =  NW
  $h =  NH
  $d =  ND
  $revise
  1

slb.expand_to_include XYZ = //expanding set
  WHD    [$w $h $d]
  BMin   v3min [0 0 0] XYZ
  BMax   v3max WHD XYZ+[1 1 1]
  Delta  BMax-WHD + BMin
  Dir    Delta.any(?<0)
  when Dir: XYZ -= WHD
  less $expand_canvas2(BMin BMax): ret   0
  when Dir: $center = $center - Delta.float
  1

slb.eset XYZ Color = //expanding set
  P XYZ - $center.int
  $expand_to_include XYZ
  P += $center.int
  $set @P Color

slb.expand_canvas BMin BMax =
  BMax -= $center.int //hack in case BMin does the resize
  when BMin.0<0 or BMin.1<0 or BMin.2<0: $eset BMin SLB_VOID
  BMax += $center.int
  when BMax.0>>$w or BMax.1>>$h or BMax.2>>$d: $eset BMax SLB_VOID

slb.brush_cut Brush Flags = //boolean operation with a brush
  $orient
  Brush.orient
  vxCut $handle Brush.handle Flags
  $revise

slb.brush_paste Brush = //boolean operation with a brush
  $orient
  Brush.orient
  BMin,BMax vxSamplePastedAABB($handle Brush.handle).parse.0.group(3)
  $expand_canvas(BMin.round.int BMax.round.int)
  $orient
  Brush.orient
  vxPaste $handle Brush.handle 0
  $revise

slb.compact = vxCompact $handle

slb.fill_interior =
  vxFillInterior $handle
  $revise

slb.clay_color = vxGetClayColor $handle

slb.set_clay_color Color =
  vxSetClayColor $handle Color
  $revise

ActToIdTbl paint!0  npaint!1  cut!2  bevel!3  grow!4

slb.lock_mesh = less $flags-*-SLB_MESH_LOCK:
  $flags += SLB_MESH_LOCK
  vxMeshLock $handle 1

slb.unlock_mesh = when $flags-*-SLB_MESH_LOCK:
  vxMeshLock $handle 0
  $flags -= SLB_MESH_LOCK


//invert=0 cuts out transparent mask parts
//invert=1 cuts out non-transparent mask parts
slb.texture BrAct Mask Invert!1 Soft!0 Lock!0 Margin!1.732 =
  $orient
  Mask.pixels //ensure it is ready
  if Lock: $lock_mesh
  ActId ActToIdTbl.BrAct.@0
  $set_softness Soft
  vxPaintMargin Margin
  vxTexture $handle Mask.handle ActId Invert
  $revise
  0


//environment map style texturing
slb.env_texture Gfx MapBase MapType =
  Flags 0
  if MapType><normal: Flags += 0x01
  if MapType><cylinder: Flags += 0x100
  vxEnvTexture $handle Gfx.handle Flags
  $revise

slb.carve_heightmap Gfx =
  vxCarveHeightmap $handle Gfx.handle
  $revise


slb.crop_to_content =
  [X0 Y0 Z0],[X1 Y1 Z1] $margins
  if X0><0 and Y0><0 and Z0><0 and X1><$w and Y1><$h and Z1><$d: ret  
  C vxCopyBox $handle X0 Y0 Z0 X1 Y1 Z1
  vxFree $handle
  $handle =  C
  NW X1-X0
  NH Y1-Y0
  ND Z1-Z0
  //NC ([$w $h $d] - [NW NH ND]).float/2.0 //new center
  PC $center.deep_copy
  $w = NW
  $h = NH
  $d = ND
  $reset_center
  $center = $center + (PC-$center) - [X0 Y0 Z0].float
  $revise

ShadingMethods flat!0 voxels!1 cubes!2 zbuffer!3 nbuffer!4 test!5 cubic_voxels!6

ShadingFlags
  smooth   ! SLB_SHD_SMOOTH
  lowfi    ! SLB_SHD_LOWFI
  glow     ! SLB_SHD_GLOW
  pillow   ! SLB_SHD_PILLOW
  zcue     ! SLB_SHD_ZCUE    //apply depth buffer cueing to shading


DefaultLight 0

camera.default_light Intensity =
  less DefaultLight:
    DefaultLight = slb 2 2 2
    DefaultLight.id = -1
    DefaultLight.flags += SLB_LIGHT
    DefaultLight.flags += SLB_INVISIBLE
  O $origin
  //light in the upper right corner of screen
  DefaultLight.xyz = O + [1000.0 1000.0 -1000.0].mm($view_matrix.zip)
  A $angle
  //DefaultLight.angle =: -A.0 A.1 A.2
  DefaultLight.color = [1.0 1.0 1.0]*Intensity
  DefaultLight


camera.render_scene FB Slabs BGColor!0xB000B0 Skybox!0 Sample!0
                    Flags![smooth] =
  Shading $shading
  when Shading><default: Shading = \voxels
  when Sample: Shading = \flat
  Shd ShadingMethods.Shading
  when no Shd: bad "unknown shading method: [Shading]"
  for F Flags:
    V ShadingFlags.F
    when got V: Shd += V
  LightIntensity 0.5
  AmbientColor: 1.0 1.0 1.0
  AL AmbientColor*(1.0 - LightIntensity)
  less Slabs.any((?flags-*-SLB_LIGHT)):
    Slabs =: $default_light(LightIntensity) @Slabs
  vxBegin
  if Skybox: Shd = Shd + SLB_SHD_SKYBOX_BG
  vxShader Shd
  vxAmbient BGColor AL.0 AL.1 AL.2
  for Slb Slabs:
    Slb.orient
    vxSlab Slb.handle
  vxEnd
  when Sample: ret: vxRenderSample Sample.0 Sample.1
  vxRender FB.handle
  /*for Slb Slabs:
    less Slb.visible: pass
    Cs vxSampleWorldAABB Slb.handle
    Cs Cs.parse.0.group{3}
    say Cs.0,Cs.7-Cs.0*/
  for Slb Slabs:
    less Slb.flags-*-SLB_SHOW_BOUNDS: pass
    Cs vxSampleAABB Slb.handle
    Cs Cs.parse.0.group(3)
    Ps: Cs.0 Cs.1 Cs.3 Cs.2 Cs.0 Cs.4 Cs.5 Cs.7 Cs.6 Cs.4
    PP Ps.head
    for P Ps.tail
      FB.line(0x00FF00 P.int PP.int)
      PP =  P
    for A,B [[Cs.1 Cs.5] [Cs.3 Cs.7] [Cs.2 Cs.6]]
      FB.line(0x00FF00 A.int B.int)

slb.sample_xy X Y =
  $orient
  S vxSampleRay $handle X Y
  if S <> void:
    S = S.parse.0
    //VX,VY,VZ,VC S.int
    //say _ VX VY VZ _
    //R vxProjectionSample Slb.handle $fb.handle VX VY VZ
    //say "  truth: [$mice_x],[$mice_y]"
  S

slb.render FB CBuf ZBuf =
  $sample CBuf ZBuf
  $shade FB CBuf ZBuf $ambient

slb.save Filename =
  vxSetInfo $handle "Made with VoxPie"
  vxSave Filename $handle

slb.load Filename =
  when $handle: vxFree $handle
  $handle =  vxLoad Filename
  $w =  vxW $handle
  $h =  vxH $handle
  $d =  vxD $handle
  $reset_center
  $revise

slb.voxel_import Filename ClayColor!0xB000B0 =
  if Filename.url.2><png:
    $import_png Filename
    ret
  when $handle: vxFree $handle
  $handle = vxImport Filename
  $w = vxW $handle
  $h = vxH $handle
  $d = vxD $handle
  $reset_center
  $set_clay_color ClayColor
  $revise


slb.radial_cut Hrz Vrt Corona =
  vxRadialCut $handle Hrz.handle Vrt.handle Corona.handle
  $revise

slb.recolor Src Dst =
  vxRecolor $handle Src Dst
  $revise

slb.set_softness Value =
  vxSetSoftness $handle Value

slb.smooth_normals Amount =
  vxSmoothNormals $handle Amount
  $revise

slb.scaled Src =
  vxScaled $handle Src.handle
  $revise

slb.import_ply Filename =
  vxImportPly $handle Filename
  $revise

slb.import_obj Filename =
  vxImportObj $handle Filename
  $revise

slb.import Filename =
  Path,Name,Ext Filename.url
  TFN "[Path][Name].png"
  TDiff 0
  if TFN.exists:
    TDiff =  gfx TFN
    vxSetImportTexture 0 TDiff.handle
  case Ext:
    ply = $import_ply Filename
    obj = $import_obj Filename
  when TDiff: TDiff.free
  $crop_to_content
  $center = $box_center

slb.export_obj Filename Algorithm = vxExportObj Filename $handle Algorithm

slb.export_csv Filename = vxExportCSV Filename $handle

slb.get_xy_plane Z =
  G gfx $w $h
  for Y $h:
    YY $h-Y-1
    for X $w: G.set X YY: $get X Y Z
  G

slb.set_xy_plane Z G =
  Hndl $handle
  for Y $h:
    YY $h-Y-1
    for X $w: vxSet Hndl X Y Z: G.get X YY
  G

slb.export_png Filename =
  Path,Name,Ext Filename.url
  for Z $d:
    G $get_xy_plane Z
    G.save "[Path][Name]["[Z]".pad(-4 0)].[Ext]"
    G.free

slb.import_png Filename =
  G gfx Filename
  W G.w
  H G.h
  G.free
  D 0
  Path,Name,Ext Filename.url
  Index Name.f.takeIf(?is_digit).f
  Name Name.f.dropIf(?is_digit).f
  P Index.n
  I Index.int
  while 1:
    F "[Path][Name]["[I+D]".pad(-P 0)].[Ext]" 
    less F.exists: done
    D+
  $resize W H D
  Z 0
  while 1:
    N "[Name]["[I]".pad(-P 0)].[Ext]"
    F "[Path][N]" 
    less F.exists: done
    G gfx F
    $set_xy_plane Z+ G
    G.free
    I+
  $compact
  $revise
  $smooth_normals 2

slb.draw_ball VXYZ BrushSize Color Margin!10,10,10 =
  Slb Me
  if BrushSize > 48: BrushSize =  48
  if BrushSize><0:
    Slb.eset VXYZ Color
    ret  
  BMin: Slb.w-1 Slb.h-1 Slb.d-1
  BMax: 0 0 0
  for X BrushSize*2: for Y BrushSize*2: for Z BrushSize*2:
    DXYZ: X-BrushSize Y-BrushSize Z-BrushSize
    when DXYZ.float.abs<BrushSize.float:
      BMin = v3min BMin VXYZ+DXYZ
      BMax = v3max BMax VXYZ+DXYZ
  VXYZ -= Slb.center.int
  if BMin.0<0 or BMin.1<0 or BMin.2<0:
    //pre-expand to avoid multiple expansions
    Slb.eset BMin-Margin SLB_VOID
  if BMax.0>>Slb.w or BMax.1>>Slb.h or BMax.2>>Slb.d:
    Slb.eset BMax+Margin SLB_VOID
  VXYZ += Slb.center.int
  when BrushSize><1:
    for D [[0 0 0] [1 0 0] [0 1 0] [0 0 1] [-1 0 0] [0 -1 0] [0 0 -1]]:
      P VXYZ+D
      Slb.set @P Color
    ret  
  for X BrushSize*2: for Y BrushSize*2: for Z BrushSize*2:
    D: X-BrushSize Y-BrushSize Z-BrushSize
    when D.float.abs<BrushSize.float:
      P VXYZ+D
      Slb.set @P Color

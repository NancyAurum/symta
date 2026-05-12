use rgb cls ui store m hsv cm3d
export run_ui_isotest


cls isoview.WGT()
  WH!No
  :
  view_origin //x,y origin
  blit_origin
  =

Map rows:
  1 1 1 1 1 1 1 1
  1 0 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1

MapW 8
MapH 8
Recolors: 0x4b0f76 0x410062 0x360058 0x290850
ColorA Recolors{C=C^rgb2hsv(:H S V = hsv2rgb 0.5 S V)}
ColorB Recolors{C=C^rgb2hsv(:H S V = hsv2rgb 1.0 S V)}

#if 1
TileW 64
TileH 32
TileL 32
@siteToView X Y =
  VX (X*TileW - Y*TileW)/2 //+ $rect.2/2-TileW
  VY (X*TileH + Y*TileH)/2
  VX,VY
#else
TileW 64
TileH 47
TileL 32
@siteToView X Y =
  VX X*TileW + if Y-*-1: TileW/2 else 0
  VY Y*TileH - TileL*Y
  VX,VY
#endif

@draw FB =
  Tile get_pic "tlx"
  Tile.recolorable_xs Recolors
  times Y MapH:
    times X MapW:
      VX,VY $siteToView X Y      
      if Map.Y.X:
        Tile.recolor_xs if (X-^-Y)-*-1 then ColorA else ColorB
        FB.blit VX VY Tile
      "[X],[Y]".draw FB VX+TileW/2-8 VY+8 16 Color!RGB_WHITE

  FAngle uim.frame%360
  FB.blit 128 64 get_pic(cube)
  FB.isobox 128 64+9+5 45 40 45 [30.0+0.5 45.0 FAngle]*DEG
  //FB.save "C:/Users/nangl/Downloads/test.png"
  //halt


TestUI bg Add!:rowz:
  isoview

run_ui_isotest = uim 640 480 Title!"Symta UI Example 2" TestUI

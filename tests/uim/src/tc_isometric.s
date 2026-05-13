// Same isometric scaffold as examples/31-isometric, exercised here
// to guard against gfx-primitive regressions (triangle, line) and to
// cover a custom `WGT` subclass with its own update/draw.

use cls uim rgb gfx

export tc_isometric

Map rows:
  1 1 1 1 1 1 1 1
  1 0 1 1 1 1 1 1
  1 1 1 1 0 1 1 1
  1 1 1 1 1 1 1 1
  1 1 0 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1
  1 1 1 1 1 1 1 1

MapW 8
MapH 8
HW 32
HH 16

ColorA 0x3a3a5a
ColorB 0x2e2e4a
ColorBlock 0x141420
ColorEdge 0x101018

cellToView CX CY OX OY =
  OX + (CX - CY) * HW,  OY + (CX + CY) * HH

drawTile FB CX CY OX OY Color =
  PX,PY cellToView CX CY OX OY
  Top    PX, PY-HH
  Right  PX+HW, PY
  Bot    PX, PY+HH
  Left   PX-HW, PY
  FB.triangle Color Top Right Bot
  FB.triangle Color Top Bot Left
  FB.line ColorEdge Top  Right
  FB.line ColorEdge Right Bot
  FB.line ColorEdge Bot   Left
  FB.line ColorEdge Left  Top

cls iso.WGT
  :
  =

@draw FB =
  X,Y,W,H $rect
  OX X + W/2
  OY Y + HH*2
  FB.rectangle 0x101018 1 X Y W H
  times CY MapH:
    times CX MapW:
      Walkable Map.CY.CX
      Color if: not Walkable          = ColorBlock
                (CX-^-CY) -*- 1 >< 0 = ColorA
                1                     = ColorB
      drawTile FB CX CY OX OY Color

tc_isometric W H =
  TestUI bg 0x000010 Add!:rowz:
    iso X!0 Y!0 W!1.0 H!1.0
    ftx O!abs X!8 Y!8 color!RGB_WHITE "iso widget at [W]x[H]"
  uim W H Title!"UIM test: iso" TestUI

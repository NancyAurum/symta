// 31-isometric -- a tiny isometric scaffold
//
// Roughly 80 lines of Symta that draws an 8x8 isometric grid,
// highlights the tile under the mouse, and pulses a marker on a
// fixed cell. Useful as a starting skeleton for a turn-based
// strategy / tactics game.
//
// What you should look at first:
//   * cellToView / viewToCell -- the diamond projection and its
//     inverse. Everything else is bookkeeping.
//   * cls iso.WGT -- a custom widget that draws into its own rect
//     and answers mouse hits via the inverse projection.
//   * uim.frame -- monotonically increasing frame counter; we
//     mod-divide it to drive a 1-second pulse.
//
// Run:   symta examples/31-isometric && examples/31-isometric/go.exe
// Headless screenshot:
//   examples/31-isometric/go.exe --screenshot=iso.png --screenshot-frame=90
//
// To exit, close the window or click the Exit button.

use cls uim rgb gfx

// ------------------------------------------------------------------ map

// 8x8 with three blocking cells (0) so the grid isn't just a plane.
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

// Tile half-extents in screen space. Classic 2:1 diamond ratio --
// every cell becomes a parallelogram rotated 45 degrees.
HW 32
HH 16

// Pulse target: the cell that throbs every frame.
PulseX 4
PulseY 4

// Cell colors. The two checker shades plus a "blocked" shade.
ColorA 0x3a3a5a
ColorB 0x2e2e4a
ColorBlock 0x141420
ColorHover 0xffdd66
ColorPulse 0xff5050
ColorEdge 0x101018

// (CX,CY) -> screen-space (PX,PY), relative to the widget's origin.
cellToView CX CY OX OY =
  PX OX + (CX - CY) * HW
  PY OY + (CX + CY) * HH
  PX,PY

// Inverse: pixel under the mouse -> nearest integer (CX,CY).
// Derived by solving cellToView for CX, CY:
//   PX-OX = (CX-CY)*HW  =>  (PX-OX)/HW = CX-CY
//   PY-OY = (CX+CY)*HH  =>  (PY-OY)/HH = CX+CY
viewToCell PX PY OX OY =
  DX (PX - OX).float / HW
  DY (PY - OY).float / HH
  CX ((DX + DY) * 0.5 + 0.5).int
  CY ((DY - DX) * 0.5 + 0.5).int
  CX,CY

drawTile FB CX CY OX OY Color =
  PX,PY cellToView CX CY OX OY
  // Four corners of the diamond: top, right, bottom, left.
  Top    PX,    PY-HH
  Right  PX+HW, PY
  Bot    PX,    PY+HH
  Left   PX-HW, PY
  FB.triangle Color Top Right Bot
  FB.triangle Color Top Bot Left
  FB.line ColorEdge Top  Right
  FB.line ColorEdge Right Bot
  FB.line ColorEdge Bot   Left
  FB.line ColorEdge Left  Top


// ------------------------------------------------------------------ widget

cls iso.WGT
  :
  hover_x! -1
  hover_y! -1
  =

@update =
  // Translate the global mouse position into widget-local pixels
  // and resolve it to a cell index.
  MX,MY uim.mice_xy
  X,Y,W,H $rect
  // The grid is centered horizontally and shifted down a bit so
  // both `(0,0)` and `(MapW-1, MapH-1)` fit comfortably.
  OX X + W/2
  OY Y + HH*2
  CX,CY viewToCell MX MY OX OY
  $hover_x = if CX < 0 or CX >> MapW-1: -1 else CX
  $hover_y = if CY < 0 or CY >> MapH-1: -1 else CY

@draw FB =
  X,Y,W,H $rect
  OX X + W/2
  OY Y + HH*2
  // Background fill so we can see the widget's bounds.
  FB.rectangle 0x101018 1 X Y W H

  // The pulse goes 0..1..0 over 60 frames (1 second @ 60 FPS).
  // `(F%60 - 30)` is -30..29; squared and normalized gives a smooth bell.
  F uim.frame
  T (F % 60 - 30).float
  Pulse 1.0 - (T*T) / 900.0

  HX $hover_x
  HY $hover_y
  PX $PulseX
  PY $PulseY

  times CY MapH:
    times CX MapW:
      Walkable Map.CY.CX
      // Layer order: base tile, then optional hover tint, then pulse.
      BaseColor if: not Walkable           = ColorBlock
                    (CX-^-CY) -*- 1 >< 0 = ColorA
                    1                      = ColorB
      Color if: CX >< HX and CY >< HY                       = ColorHover
                CX >< PX and CY >< PY and Pulse > 0.0       = ColorPulse
                1                                           = BaseColor
      drawTile FB CX CY OX OY Color


// ------------------------------------------------------------------ scene

TestUI bg 0x000010 Add!:rowz:
  iso id!iso X!0 Y!0 W!1.0 H!1.0
  // HUD overlay -- one line of status text in the top-left.
  ftx O!abs X!8 Y!8 color!RGB_WHITE
      | => if 'iso'.hover_x < 0
              then "iso 31 -- move the mouse over a tile"
              else "hovering [`iso`.hover_x], [`iso`.hover_y]"
  // Bottom-right exit handle.
  btn O!abs X!-80 Y!-32 W!64 H!24 "Exit"| => uim.exit

uim 640 480 Title!"Symta -- Isometric Skeleton" TestUI

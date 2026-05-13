// Headless tests of the gfx C library, run via Symta against the
// pre-built ffi/gfx.ffi. Each test produces a deterministic PNG.
//
// Alpha convention reminder: in this codebase, the high byte of a pixel is
// 0x00 for fully opaque and 0xFF for fully transparent. So `0x000000` is
// opaque black, `0x00FF0000` is opaque red, `0x80FF0000` is 50%-alpha red,
// and `0xFFxxxxxx` is fully transparent. PNG save inverts before writing,
// so the viewer sees the conventional opaque-at-FF orientation.

use gfx harness

// ---- pure primitives -----------------------------------------------------

register \clear_solid: =>
  G gfx 32 32
  G.clear 0x112233  //opaque navy
  G

register \rect_filled: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.rectangle 0x0000FF 1 4 4 24 24
  G

register \rect_outline: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.rectangle 0x000000 0 2 2 28 28
  G

register \rect_fat: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.fat_rect 3 0x0000FF 4 4 24 24
  G

register \line_diag: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.line 0x000000 [2 2] [29 29]
  G.line 0x000000 [29 2] [2 29]
  G

register \line_axis: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.line 0xFF0000 [4 16] [28 16]
  G.line 0x0000FF [16 4] [16 28]
  G

register \circle_filled: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.circle 0x0000FF 1 [16 16] 12
  G

register \circle_outline: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.circle 0x000000 0 [16 16] 12
  G

register \triangle_filled: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.triangle 0x008000 [16 4] [4 28] [28 28]
  G

// ---- blit variants -------------------------------------------------------

// 16x16 source with a coloured cross on a fully-transparent background
build_source =
  S gfx 16 16
  S.clear 0xFF000000 //transparent
  S.rectangle 0xFF0000 1 0 6 16 4 //opaque red horizontal bar
  S.rectangle 0x0000FF 1 6 0 4 16 //opaque blue vertical bar
  S

register \blit_basic: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 8 8 S
  S.free
  G

register \blit_flip_x: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 8 8 S.flop
  S.free
  G

register \blit_flip_y: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 8 8 S.flip
  S.free
  G

register \blit_alpha: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S gfx 16 16
  S.clear 0x80FF0000 //~50% red over the white background
  G.blit 8 8 S
  S.free
  G

register \blit_fade: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 8 8 (S.fade 0x40)
  S.free
  G

register \blit_bright: =>
  G gfx 32 32
  G.clear 0x808080
  S gfx 16 16
  S.clear 0x404040
  G.blit 8 8 (S.brighten 60)
  S.free
  G

register \blit_oob_neg: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit -4 -4 S
  S.free
  G

register \blit_oob_pos: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 24 24 S
  S.free
  G

register \blit_oob_both: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit -8 24 S
  G.blit 24 -8 S
  S.free
  G

register \blit_subrect: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S build_source
  G.blit 8 8 (S.rect 8 0 8 16)
  S.free
  G

// ---- RLE -----------------------------------------------------------------

register \rle_blit_basic: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S gfx 24 24 //RLE has a 24-pixel-row minimum
  S.clear 0xFF000000
  S.rectangle 0xFF0000 1 0 8 24 8
  S.rectangle 0x0000FF 1 8 0 8 24
  S.rle
  G.blit 4 4 S
  S.free
  G

register \rle_blit_oob: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  S gfx 24 24
  S.clear 0xFF000000
  S.rectangle 0xFF0000 1 0 8 24 8
  S.rectangle 0x0000FF 1 8 0 8 24
  S.rle
  G.blit -8 -8 S
  S.free
  G

// ---- scaling -------------------------------------------------------------

register \scale_nearest_up: =>
  S gfx 8 8
  S.clear 0xFFFFFF
  S.rectangle 0x0000FF 1 2 2 4 4
  G gfx 32 32
  G.scaled S
  S.free
  G

register \scale_smooth_up: =>
  S gfx 8 8
  S.clear 0xFFFFFF
  S.rectangle 0x0000FF 1 2 2 4 4
  G gfx 32 32
  G.scaled_smooth S
  S.free
  G

register \scale_nearest_down: =>
  S gfx 64 64
  S.clear 0xFFFFFF
  S.rectangle 0x0000FF 1 16 16 32 32
  G gfx 16 16
  G.scaled S
  S.free
  G

// ---- scissor -------------------------------------------------------------

register \scissor_blit: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  G.scissor 4 4 16 16
  S build_source
  G.blit 8 8 S
  S.free
  G.pop_scissor
  G

// ---- gamma-corrected blend ----------------------------------------------

register \gblend_alpha: =>
  G gfx 32 32
  G.gamma 1.0
  G.clear 0xFFFFFF
  S gfx 16 16
  S.gamma 1.0
  S.clear 0x80FF0000
  G.blit 8 8 S
  S.free
  G

// ---- recolor -------------------------------------------------------------

register \recolor_xs: =>
  // make_recolor_map masks pixels with 0xFFFFFF before matching,
  // so the source-marker list must be alpha-free.
  S gfx 16 16
  S.clear 0xFF000000 //transparent bg
  S.rectangle 0xFF0000 1 0 0 8 16 //opaque red left half
  S.rectangle 0x00FF00 1 8 0 8 16 //opaque green right half
  S.recolorable_xs [0xFF0000 0x00FF00]
  S.recolor_xs [0x0000FF 0xFFFF00] //blue/yellow
  G gfx 32 32
  G.clear 0x000000
  G.blit 8 8 S
  S.free
  G

// ---- combined / regression ----------------------------------------------

register \stack_drawing: =>
  G gfx 64 64
  G.clear 0x202020
  G.rectangle 0x0000FF 1 4 4 56 24
  G.rectangle 0xFF0000 0 4 30 56 30
  G.line 0xFFFF00 [4 4] [60 60]
  G.circle 0xFFFFFF 0 [32 16] 8
  G.triangle 0x00FFFF [32 40] [12 60] [52 60]
  G

register \cut_subgfx: =>
  S gfx 32 32
  S.clear 0xFFFFFF
  S.rectangle 0xFF0000 1 0 0 16 16
  S.rectangle 0x0000FF 1 16 0 16 16
  S.rectangle 0x00FF00 1 0 16 16 16
  S.rectangle 0x000000 1 16 16 16 16
  G S.cut 8 8 16 16
  S.free
  G

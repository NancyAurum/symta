// 15-gfx -- 2D graphics from Symta
//
// `use gfx` exposes the `gfx` type. A `gfx` is a CPU-side pixel
// buffer with primitives (line, circle, rect, blit, blur, ...) and
// PNG load/save. The actual rendering is done by the C plugin in
// `c/gfx/`, packaged as `ffi/gfx.ffi`.
//
// Run:  symta examples/15-gfx && examples/15-gfx/go.exe

use gfx rgb

// Allocate a 256x256 RGBA canvas.
G gfx 256 256
G.clear RGB_BLACK

// Diagonal line.
G.line RGB_WHITE [0 0] [G.w-1 G.h-1]

// Filled red circle in the centre.
G.circle 0xFFE05050 1 [128 128] 60

// Outlined cyan rectangle.
G.rectangle 0xFF80E0FF 0 16 16 96 64

// A small triangle and a quad in different corners.
G.triangle 0xFFFFE060 [200 30] [240 30] [220 80]
G.quad [10 200] [60 210] [55 245] [12 240]

// Plot a sine wave in green.
GreenC 0xFF60D080
times X 256:
  Y 128 + (40.0 * (X.float / 8.0).sin).int
  G.set X Y GreenC

// Save the canvas to PNG.
Out "examples/15-gfx/out.png"
G.save Out
say "wrote [Out] ([G.w]x[G.h])"

// Composite-blit demo: scale a smaller copy onto a larger one.
T gfx 64 64
T.clear 0xFF101080
T.circle 0xFFFFE0E0 1 [32 32] 20
G2 gfx 64 64
G2.scaled_smooth T

Out2 "examples/15-gfx/scaled.png"
G2.save Out2
say "wrote [Out2] ([G2.w]x[G2.h])"

// Free GPU/CPU resources explicitly (the GC will also do this on
// finalisation, but explicit release is useful in tight loops).
G.free
T.free
G2.free

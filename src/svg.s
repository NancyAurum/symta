use gfx
export gfx_svg

ffi_begin macro svg
ffi svg_load_: svg_load.ptr Filename.text
ffi svg_free_: svg_free.void Svg.ptr
ffi svg_render_: svg_render.ptr Svg.ptr W.int H.int

gfx_svg W H SVGPath =
  SVG svg_load_ SVGPath
  GfxHandle svg_render_ SVG W H
  svg_free_ SVG
  G gfx
  G.set_handle GfxHandle
  G

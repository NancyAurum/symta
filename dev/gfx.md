# Symta GFX Packge Reference


Table of Contents
------------------------------
- What is GFX?
- Methods

What is GFX?
------------------------------
GFX is a bitmap object, that supports both indexed and RGB colors. It has usual functions acting on it, like resizing and blitting with zbuffer, as well as primitive drawing routines, producing lines, circles, and triangles.

Methods
------------------------------

gfx.blit{X Y Src} - blits Src image to this image at X,Y

gfx.margins - finds image marings (empty pixels around image). Use for sprite sheet creation.

gfx.cmap - 256 colors color map

gfx.rect{X Y W H} -  sets rect for the next blitting. After which it gets reset to whole image again. Useful for croping out separate frames from sprite sheet.

gfx.flip - next blit will reverse image around Y-axis

gfx.flop - next blit will reverse image around X-axis

gfx.recolor{Map} - sets map for image recoloring

gfx.alpha{Amount} - next blit will draw image transparent

gfx.z{Z} - sets z-value of this image for z-buffer

gfx.cut{X Y W H} - returns cropped copy of the image.



recolor_example = 
| BG = gfx "/Users/macbook/Documents/GitHub/symta/ffi/gfx/test/bg1.png"
| FG = gfx "/Users/macbook/Documents/GitHub/symta/ffi/gfx/test/fg.png"
| FG.recolorable{#db0000 4}.recolor{#4040db 1.5}
| BG.blit{200 180 FG}
| gui BG


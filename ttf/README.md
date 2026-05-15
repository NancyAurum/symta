# symta/ttf/

Compiler-supplied default fonts. The `ffi_begin macro ttf` macro
(see [`../src/macro.s`](../src/macro.s)) auto-stages the files
listed in `TTFBundle` from here into a project's `ttf/` directory
the first time `font.s` is compiled into it — but only if the
project doesn't already have a `ttf/` of its own.

The "do nothing if `ttf/` already exists" rule means a project
that wants its own type setup (the SoM game ships its preferred
Sarabun face under `game/ttf/`) keeps full control. Drop a `ttf/`
folder into your project root with the fonts you want and nothing
gets clobbered.

## What's in here

| File             | License | Purpose                                      |
|------------------|---------|----------------------------------------------|
| inter.ttf        | OFL 1.1 | Modern UI sans-serif; the default for fresh projects. |
| inter_license.txt | OFL 1.1 | Upstream license — must travel with the font under OFL §2. |

[Inter](https://rsms.me/inter/) by Rasmus Andersson, ~400 KB.
Hinted, screen-optimised, very legible at small sizes — a good
match for the minimalist widget defaults in
[`../src/uimgen.s`](../src/uimgen.s).

## Picking a project default

The default font NAME (which `get_font No` resolves to) is held
in `DefaultFontName` in [`../src/font.s`](../src/font.s).  To
override per-project, call `set_default_font` early in your
project's entry-point module:

```symta
use font
default_font_name = \sarabun   // or any font available on the
                                // search path: <project>/ttf/,
                                // c:/windows/fonts/, …
```

The SoM game does exactly this in `game/src/go.s`.

## Swapping the compiler-supplied default

If you want something other than Inter as the out-of-box default
for fresh projects:

1. Drop the TTF in here (e.g. `noto-sans.ttf`).
2. Add its filename to `TTFBundle` in `../src/macro.s`.
3. Change `DefaultFontName \inter` in `../src/font.s` to the
   matching name (without `.ttf`).
4. Rebuild the bootstrap (touch `../src/macro.s`, run
   `symta pkg/symta .` from symta/, tolerating the documented
   go.s bootstrap failure — macro.sbc / font.sbc get refreshed
   regardless).

## Why ship a font at all

Symta's font rendering goes through `ttf_load` (FreeType) on a
real `.ttf` file. There is no "default fallback to a built-in
glyph table". Without a bundled font, every fresh project would
have to bring its own — same leaky abstraction as the SDL DLLs
were before
[commit 731c53c](https://github.com/NancySadkov/som/commit/731c53c)
and the widget pictograms were before
[commit 550b719](https://github.com/NancySadkov/som/commit/550b719).
Bundling one ~400 KB OFL font here trades a small repo cost for
a "`use uim` Just Works" out-of-box experience.

export 'rgb' 'rgba' 'unrgb' 'unrgba' rgb_s_dummy

#:rgb

rgb R G B = form: R*0x10000 + G*0x100 + B
rgba R G B A = form: A*0x1000000 + R*0x10000 + G*0x100 + B

unrgb Color R G B =
| C 'C'.rand
| form: @
  | C Color
  | B C-*-0xFF
  | C = C/0x100
  | G C-*-0xFF
  | C = C/0x100
  | R C-*-0xFF

unrgba Color R G B A =
  C 'C'.rand
  form: @
  | C Color
  | B C-*-0xFF
  | C = C/0x100
  | G C-*-0xFF
  | C = C/0x100
  | R C-*-0xFF
  | C = C/0x100
  | A C-*-0xFF

int.unrgb =
  C Me
  B C-*-0xFF
  C = C/0x100
  G C-*-0xFF
  C = C/0x100
  R C-*-0xFF
  R,G,B

int.unrgba =
  C Me
  B C-*-0xFF
  C = C/0x100
  G C-*-0xFF
  C = C/0x100
  R C-*-0xFF
  C = C/0x100
  A C-*-0xFF
  R,G,B,A

list.rgb = Me.0.int*0x10000 + Me.1.int*0x100 + Me.2.int

int.rgb_mul V = @rgb Me.unrgb*V


ColorTable:
  RGB_BLACK     black
  RGB_NIGHT     night
  RGB_WINE      wine
  RGB_BROWN     brown
  RGB_COCONUT   coconut
  RGB_ORANGE    orange
  RGB_TAN       tan
  RGB_GOLD      gold
  RGB_YELLOW    yellow
  RGB_KIWI      kiwi
  RGB_GREEN     green
  RGB_EMERALD   emerald
  RGB_OLIVE     olive
  RGB_CAMO      camo
  RGB_ONYX      onyx
  RGB_NAVY      navy
  RGB_AGAVE     agave
  RGB_ROYAL     royal
  RGB_BLUE      blue
  RGB_CYAN      cyan
  RGB_ICE       ice
  RGB_WHITE     white
  RGB_SMOKE     smoke
  RGB_DIM       dim
  RGB_GRAY      gray
  RGB_VIOLET    violet
  RGB_RED       red
  RGB_PINK      pink
  RGB_CANDY     candy
  RGB_MOSS      moss
  RGB_BISTRE    bistre
  RGB_DARK      dark

ColorTable = ColorTable.group^2{f}.t

//FIXME: implement parsing of 0xRRGGBB and #RRGGBB colors
text.as_rgb = ColorTable.Me


rgb_s_dummy =


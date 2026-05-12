use gfx cache sml store rgb
export ttf get_font

ffi_begin macro ttf
ffi ttf_load.ptr Filename.text
ffi ttf_render.ptr Font.ptr Code.int Height.float RGB.u4


// Per-ttf-instance two-level glyph cache:
//   $glyphs : table keyed by $pfx (style-fingerprint string) -> inner table
//   inner   : table keyed by Char -> gfx
// `set_style` switches `$cur_glyphs` to the inner table for the current
// style, allocating it on first sight. `get` then does two hash lookups
// (one to find the inner table, one for the glyph) instead of building
// a fresh interpolated string key on every call.
type ttf Path: pfont!0 src!Path pfx!"" style!0 glyphs!(!) cur_glyphs!0 =
  $stylize 16 0xFFFFFF 0 0

ttf.stylize Size Color Bold Italic =
  Size = Size.float
  Style: Size Color Bold Italic
  $set_style Style


ttf.set_style Style =
  $style = Style
  $pfx = "ch[[$src.url.1 Style]]"
  have $glyphs.($pfx): !
  $cur_glyphs = $glyphs.($pfx)


ttf.get Char =
  Inner $cur_glyphs
  have Inner.Char:
    Size,Color,Bold,Italic $style
    Code Char.code
    if Size < 8.0: //too small
      Code = 0x20
      Size = 8.0
    less $pfont:
      TTF ttf_load $src
      $pfont = TTF
    if Code >< 0x20:
      G $get('-').copy
      G.clear RGB_NONE
      ret G
    G gfx
    if Italic:
      Q 4
      H ttf_render $pfont Code Size*Q Color
      G.set_handle H
      SkewG (G.h+3)/4
      XY (G.xy - [SkewG 0] - [0 Q-1])/Q
      T G.skewed SkewG
      G.free
      G = T.downbox Q
      T.free
      G.xy = XY
    else
      H ttf_render $pfont Code Size Color
      G.set_handle H
    if Bold:
      T gfx G.w+1 G.h+1
      //T gfx G.w+0 G.h+0
      T.xy = G.xy
      G.xy = 0,0
      T.clear RGB_NONE
      //prx T.blit @0 G | 0 0; 1 0
      prx T.blit @0 G | 1 0; 0 1; 1 1
      G.set_handle T.handle
    G


ttf.width FH Text =
  TW,TH Text.draw 0 0 0 FH test!1 font!Me
  TW

//FIXME: there should be a way to free ttf fonts.
//FIXME: use time stamps and a single cache scheme for all needs
FontPaths ["[main_path]ttf/" "c:/windows/fonts/"]
Fonts!
get_font N =
  if no N: N = \sarabun
  have Fonts.N:
    FP N
    less FP.exists:
      P FontPaths.find("[?][N].ttf".exists)
      if no P: bad "couldn't locate the `[N]` font"
      FP = "[P][N].ttf"
    ttf FP



//FIXME words exceeding `Wrap` should be cut into parts
//if WordW > Wrap: ...

text.draw @As =
  Me.sml.draw @As

DCall 0

#UNLIM 9000

//draw text onto a gfx G
//if G is 0, then does a test run, calculating result text dimensions
list.draw G X Y
          FH //text height
          Wrap!No //text width
          Font!No //font name
          Color!RGB_BLACK Bold!0 Shadow!No //font appearance
          Range!No //Y range start and height for printing only parts of text
          RangeX!No
          Pick!No //instead of drawing, return character position at pick
          Cursor!No
          Base!0
  =
  //less FH: bad "list.draw: zero FH" //should never happen
  DCall+
  less Font.is_ttf: Font = get_font Font
  FH2 FH*3/4 
  Y += FH2
  BXY X,Y
  Test not G
  if Color.is_text: Color = Color.as_rgb
  Alpha (Color -*- 0xFF000000) ->- 24
  Color Color-*-0xFFFFFF
  Cursor = if no Cursor or Test: 0 else Cursor+1
  Pick = if no Pick: 0 else | if Pick.is_int: Pick+1 else Pick
  if Pick.is_list and Pick.0 << X and Pick.1 << Y: ret 0
  //cursor X,Y
  if Wrap.is_fn: Wrap = Wrap()
  if no Wrap: Wrap = UNLIM
  CurX No
  CurY No
  Italic 0
  restyle = Font.stylize FH Color Bold Italic
  restyle
  SpaceW Font.get(' ').w
  RowH FH
  ItemCnt 0
  RowItemCnt 0
  MaxW 0
  ItemsO Me
  Items ItemsO
  ON ItemsO.n
  XS,XL if got RangeX: RangeX else 0,UNLIM
  X -= XS
  YS,YL if got Range: Range else 0,UNLIM
  CY Y-YS
  RE Y+YL
  Wrap += X
  CX X
  SY Y - FH2
  Kids:
  Ks:
  Kid if Ks.end: 0 else pop Ks
  KXS,KYS,KXE,KYE if Kid: Kid else 0,0,0,0
  Stack:
  Underline 0
  new_line =
    MaxW = max MaxW CX-X
    CX = X
    CY += RowH
    RowItemCnt = 0
    RowH = FH
    _label kid_again
    if CY > KYE+FH and Kid:
      Kid = if Ks.end: 0 else pop Ks
      Kid(:[&KXS &KYS &KXE &KYE])
      _goto kid_again
  draw_underline W =
    UG Font.get '_' //FIXME: this is one huge hack!!!
    UGW UG.w
    UY CY-2
    UX CX-W/3
    EX UX + W + 4
    Step UGW-3
    Count 0
    while UX < EX-W/4:
      NextUX UX + Step
      if NextUX > EX:
        UX -= NextUX-EX
        NextUX += UNLIM
      G.blit UX UY UG
      UX = NextUX
  Item 0
  PrevItem 0
  GlyphW 0
  space =
    less Test:
      if Underline:
        draw_underline SpaceW
    CX += SpaceW
    RowItemCnt+
  till Items.end:
    if CY>>RE: less Test: done
    if Pick:
      if Pick.is_int:
        if Pick << ON-Items.n+1: ret CX,CY-FH
      else if CX-GlyphW/2 > Pick.0 and CY >> Pick.1: ret ON-Items.n-1
    PrevItem = Item
    Item = pop Items
    if Cursor:
      Index ON-Items.n - 1
      if Cursor-1 << Index:
        CurX = CX
        CurY = CY
        Cursor = 0
    _label process_word
    if ' ' >< Item:
      if RowItemCnt: space
      pass
    GlyphW = 0
    if not Item.is_text:
      case Item:
        [n] = new_line
        [s] = space
        [u] = Underline+
              push u Stack
        [b] =
          Bold+
          push b Stack
          restyle
        [i] =
          Italic+
          push i Stack
          restyle
        [C<1.is_int] =
          push color,Color Stack
          Color = C
          restyle
        [['%' C]] =
          push color,Color Stack
          Color = C.as_rgb
          restyle
        [t N] = CX = X + N*SpaceW //tabulate to N spaces
        [['/' Id] x!0 y!0 w!0 h!0] =
          Wgt Id.q
          if got Wgt:
            if Base:
              if Wgt.base:
                if Wgt.base.id <> Base.id:
                  Wgt.unkid
                  Base.add Wgt
              else Base.add Wgt
            Wgt.renew_tagfrm //tell UIM we are still displaying this widget
            Wgt.o = \base
            WW,HH Wgt.rect[2:4]
            when CX+WW > Wrap and RowItemCnt:
              new_line
              if CY>>RE: less Test: done
            XX,YY [CX CY]-BXY + [X Y]
            if Wgt.xy_<>[XX YY]:
              Wgt.xy = [XX YY]
              Wgt.clear_rect_cache_r
            KXS = XX + BXY.0
            KYS = YY + BXY.1
            KXE = KXS + WW + W
            KYE = KYS + HH - FH2 + H
            Kid =: KXS KYS KXE KYE
            push Kid Kids
            CX += WW+W+X
          RowItemCnt+
        No =
          case Stack^pop:
            [color C] =
              Color = C
              restyle
            u = Underline-
            b =
              Bold-
              restyle
            i =
              Italic-
              restyle
        [pic Name x!0 y!0 w!No h!FH] =
          Pic get_pic Name w!W h!H
          ItemW Pic.w
          PH Pic.h
          AddY -FH2+Y
          RowH = max RowH PH+AddY
          _label again2
          if Kid:
            if KYS << CY and CY < KYE+RowH:
              if KXS << CX and CX < KXE: CX = KXE
              if CX < KXE and CX+ItemW >> KXS: CX = KXE
              RowItemCnt+
          when CX+ItemW > Wrap and RowItemCnt:
            new_line
            if CY>>RE: less Test: done
            _goto again2
          less Test: when CY>>SY:
            G.blit CX+X CY+AddY Pic
          CX += ItemW
          RowItemCnt+
      if Cursor:
        Index ON-Items.n-1
        if Cursor-1 << Index:
          CurX = CX
          CurY = CY
          Cursor = 0
    else
      WordW 0
      Word: Item
      while 1:
        case Items:
          [Chr < -' ' < 1.is_text @Next] =
            push Chr Word
            Items = Next
          _ = done
      Glyphs Word.f.l{C = C,|Font.get C}
      for C,Glyph Glyphs:
        W Glyph.w + Glyph.xy.0
        WordW += W
      _label again
      if Kid:
        if KYS << CY and CY < KYE+RowH:
          if KXS << CX and CX < KXE: CX = KXE
          if CX < KXE and CX+WordW >> KXS: CX = KXE
          RowItemCnt+
      when CX+WordW > Wrap and RowItemCnt:
        new_line
        if CY>>RE: less Test: done
        _goto again
      ChI 0
      for C,Glyph Glyphs:
        GlyphW = Glyph.w + Glyph.xy.0
        less Test: when CY>>SY:
          if Cursor:
            Index ON-Items.n-Word.n + ChI
            if Cursor-1 << Index:
              CurX = CX
              CurY = CY
              Cursor = 0
            ChI+
          when got Shadow:
            SColor Color
            Color = Shadow
            restyle
            SGlyph Font.get C
            Color = SColor
            restyle
            when Alpha: SGlyph.fade Alpha
            G.blit CX-1 CY+1 SGlyph
          when Alpha: Glyph.fade Alpha
          G.blit CX CY Glyph
          if Underline: draw_underline GlyphW
        CX += GlyphW
        if Pick:
          if Pick.is_int:
            if Pick << ON-Items.n-Word.n+ChI+1: ret CX-GlyphW,CY-FH
          else if CX-GlyphW/2 > Pick.0 and CY >> Pick.1:
                  ret ON-Items.n-Word.n + ChI
          ChI+
      RowItemCnt+
  if Pick:
    if Pick.is_int: ret CX,CY
    ret ItemsO.n
  if Cursor:
    CurX = CX
    CurY = CY
  less Test: when got CurX:
    HH FH2
    YY CurY-HH+2
    G.rectangle 0x000000 1 CurX   YY 1 HH
    G.rectangle 0xFFFFFF 1 CurX+1 YY 1 HH
  less MaxW: MaxW = CX - X
  new_line
  MaxH CY-Y
  for KX,KY,KEX,KEY Kids: //ensure text W,H encompass widgets
    MaxW = max MaxW KEX-X
    MaxH = max MaxH KEY
  MaxW = min XL MaxW+2 //+2 accomodates for cursor
  MaxW,MaxH

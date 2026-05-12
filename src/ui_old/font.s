use gfx fxn ui/data

Fonts!

type font Gs W H: glyphs!Gs widths!W height!H rc_bright!1.0

font.as_text = "#(font)"

get_font N = have Fonts.N:
| S  get_spr "font_[N]"
| [W H EX]   S.0^|F => [F.w F.h F.xy.0]
| FrameList   map I S.nframes: S.I
| RecolorBase   No
| RecolorRange   0
| RecolorBright   No
| SInfo   S.info
| when got SInfo:
  | FrameList = FrameList.wout SInfo.spacepos.@0
  | when got SInfo.recolor
  | RC   SInfo.recolor
  | RecolorBase =  RC.0
  | RecolorRange =  RC.1
  | RecolorBright =  RC.2
| Glyphs   FrameList(F<[X Y W H].margins=>| F.xy =  0,0
                                          | F.cut(X 0 W F.h))
| Ws   Glyphs([X Y W H].margins=>X+W-EX).list
| Ws.0 =  W/2
| when got RecolorBase:
  | Glyphs(?recolorable(RecolorBase RecolorRange))
| Glyphs(?rle)
| F   font Glyphs.list Ws H
| when got RecolorBright: F.rc_bright =  RecolorBright
| F

font.width Line =
| Ws   $widths
| CodePoint   ' '.code
| R   Line.n
| InTag   0
| for C Line:
  | when C><'<':
    | InTag++
    | pass
    when C><'>':
    | InTag--
    | pass
  | when InTag: pass
  | fxn: R += Ws.(C.code-CodePoint)
| R

font.draw G X Y Text bold/0 alpha/0 color/No shadow/No =
| Ls   Text.lines
| Ws   $widths
| Gs   $glyphs
| H   $height
| CodePoint   ' '.code
| CY   Y
| when Alpha.is_float: Alpha =  @int Alpha*255.0
| RCB   $rc_bright
| CurX   No
| CurY   No
| fxn: for L Ls:
  | CX X
  | InTag 0
  | TagCs:
  | for C L:
    | when C><'<':
      | InTag++
      | pass
    | when C><'>':
      | InTag--
      | less InTag:
        | Tag TagCs.flip.text
        | TagCs =  []
        | when Tag><"cur": when got Color:
          | CurX =  CX
          | CurY =  CY
      | pass
    | when InTag:
      | push C TagCs
      | pass
    | I C.code-CodePoint
    | W Ws.I
    | Glyph Gs.I
    | when got Shadow:
      | when Alpha: Glyph.fade(Alpha)
      | Glyph.recolor(Shadow 123.0)
      | G.blit(CX-1 CY+1 Glyph)
    | when got Color:
      | Glyph.recolor(Color RCB)
    | when Alpha: Glyph.fade(Alpha)
    | G.blit(CX CY Glyph)
    | when Bold:
      | when Alpha: Glyph.fade(Alpha)
      | if Bold><3 then G.blit(CX+1 CY-1 Glyph)
        else if Bold><1 then G.blit(CX+1 CY Glyph)
        else G.blit(CX CY-1 Glyph)
    | CX += W+1
  | CY += H
| when got CurX:
  | G.rectangle(Color 1 CurX CurY 1 H)
  | G.rectangle(0x000000 1 CurX+1 CurY 1 H)

font.format MaxLineWidth Text =
| SpaceWidth   $width ' '
| Words  Text.replace('\n' ' ').split(' ')
| Line   []
| Lines   []
| LineWidth   0
| till Words.end:
  | Word  pop Words
  | if Word >< '': pass
  | if Word.n and Word.0><'<':
    | when Word >< '<sp>'
      | Word =  ''
    | when Word >< '<br>'
      | push Line Lines
      | Line =  []
      | LineWidth =  0
      | pass
  | LT Word.locate '<'
  | GT Word.locate '>'
  | if got LT and got GT and LT < GT:
     if LT <> 0 or GT <> Word.n-1:
       W0 Word.take LT
       W1 Word.drop LT
       W2 W1.drop GT-LT+1
       W1 = W1.take GT-LT+1
       Word = W0
       push W2 Words
       push W1 Words
       if Word><'': pass
  | Width $width Word
  | less Line.end: Width += SpaceWidth
  | when LineWidth+Width > MaxLineWidth: less Line.end:
    | push Line Lines
    | Line =  []
    | LineWidth =  0
  | push Word Line
  | LineWidth += Width
| less Line.end: push Line Lines
| Lines(?flip.text(' ')).flip.text('\n')

main.font Name = get_font Name

LastFont   0
LastFontName   0

gfx.text X Y Text font/medium center/No alpha/0 bold/0 wrap/No
         color/No shadow/No =
| when Font<>LastFontName:
  | LastFontName =  Font
  | LastFont =  get_font Font
| F   LastFont
| when got Center:
  | CW,CH   Center
  | when got CW:
    | FW   F.width(Text)
    | X += (CW-FW)/2
  | when got CH:
    | FH   F.height
    | Y += (CH-FH)/2
| when got Wrap: Text =  F.format(Wrap Text)
| F.draw(Me X Y Text alpha!Alpha bold!Bold color!Color shadow!Shadow)
| Me

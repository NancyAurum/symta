GInput | No
GOutput | No

/*
type token Type Val Src P:
  type!Type
  value!Val
  src!Src   //source in the file
  parsed!P  //parsed expression associated with the token
  pchar     //prefix char
*/

token Type Val Src P = tok_ Type Val Src.0 Src.1 Src.2 P

_.is_token = 0
tok.is_token = 1

tok.src =: $row $col $orig
tok.=src S =
| $row = S.0
| $col = S.1
| $orig = S.2

//tok.as_text = "#[$value]"

tok.retok NewType = token NewType NewType $src 0

token_is What O = O.is_token and O.type >< What

add_bars Xs = //add `|`-bars to the toplevel
| Ys:
| First 1
| while not Xs.end:
  | X pop Xs
  | [Row Col Orig] X.src
  | S X.type
  | when (Col >< 0 or First) and S <> `|` and S <> `then` and S <> `else`
         and S <> `elif`:
    | push (token '|' '|' [Row Col-1 Orig] 0) Ys 
    | First =  0
  | push X Ys
| Ys.f

parser_error Cause Tok =
| [Row Col Orig] Tok.src
| bad "[Orig]:[Row],[Col]: [Cause] [Tok.value or 'eof']"

expect What Head =
| less GInput.n: parser_error "missing [What] for" Head
| Tok GInput.0
| less Tok^token_is(What): parser_error "missing [What] for" Head
| pop GInput

is_next What =
| when GInput.end: ret   0
| GInput.0^token_is(What)

parse_if Sym =
  OCol Sym.src.1
  if is_next `:`:
    T expect `:` Sym
    X parse_offside 0 0 Sym.src.0 OCol
    ret:: Sym X.0
  Cnd Then No
  RawHead:
  till GInput.end or is_next `:` or is_next `then`:
    Tok pop GInput
    push Tok RawHead
  RawHead = RawHead.f
  Cnd let GInput RawHead: parse_xs 1
  if is_next `then`:
    T expect `then` Sym
    Then = parse_offside 0 0 T.src.0 OCol
  else
    less is_next `:`: parser_error "missing `:` for" Sym
    T expect `:` Sym
    Then = parse_offside 0 0 T.src.0 OCol
  Else:
  if is_next `elif`:
    T expect `elif` Sym
    GInput =: T.retok(`else`) T.retok(`if`) @GInput
  if is_next `else`:
    T expect `else` Sym
    Else = parse_offside 0 0 T.src.0 OCol
  :Sym Cnd Then Else

parse_bar H =
| C H.src.1
| Zs:
| while not GInput.end:
  | Ys:
  | while not GInput.end and GInput.0.src.1 > C: push GInput^pop Ys
  | push Ys.f^parse_tokens Zs
  | when GInput.end: ret   [H @Zs.f]
  | X GInput.0
  | less X^token_is('|') and X.src.1 >< C: ret   [H @Zs.f]
  | pop GInput


parse_table Xs =
| As:
| Key No
| Val:
| Cnt 0
| push_kv =
    less Cnt: ret
    if Val.end: Val = [1]
    Val = Val.f
    if Val.n >< 1: Val = Val.0
    push [Key Val] As
| for X Xs:
    if X(:`!` _) then
    | push_kv
    | Cnt+
    | Key = X.1
    | Val =:
    | case Key [`!` K]:
      | push `[]` Val
      | Key = K
    else
    | push X Val
| push_kv
| As.f

UnaryOps: `+` `-` `*` `/` `%` `++` `--` `<` `>` `<<` `>>` `<>` `><` `-+` `-*`
UnaryOps = UnaryOps.bag

parse_term =
| when GInput.end: ret   0
| Tok pop GInput
| when Tok.parsed: ret Tok
| V Tok.value
| P case Tok.type
    escape+symbol+text | ret   Tok
    splice |: (token symbol `"` Tok.src 0)
              @V^parse_tokens{(1.any(?(:'~'+'?'))).value
                                  =:(token '\\' '\\' Q.src 0) Q}
    int
      | if V.n>2 and V.1 >< 'x':
          if V.0><'0': V.drop(2).int(16)
          else V.drop(2).int(V.0.int(10))
        else V.int
    hex | V.tail.int(16)
    bin | V.drop(2).int(2)
    void | No
    `()` | parse_tokens V
    `[]` | [(token symbol `[]` Tok.src 0) @V^parse_tokens]
    `{`+`{}` | [(token symbol `{` Tok.src 0) @V^parse_tokens]
    `@{}`
      | Ts parse_strip: parse_tokens V
      | @t: parse_table: parse_strip Ts
    `${}`
      | Ts parse_strip: parse_tokens V
      | Cls No
      | case Ts [&Cls<keyword? @&Ts]:
      | NewFn No.new_fn_
      | KVs parse_table: parse_strip Ts
      | if no NewFn: ['${}' @KVs]
        else
        | R NewFn(@KVs)
        | if got Cls:
          | F No.get_cls_cvt_ Cls
          | if no F: parser_error "missing cls for" Tok.value.0
          | R = F(R.id)
        | R 
    `|` | ret   Tok^parse_bar
    `if` | ret   Tok^parse_if
    Type | if UnaryOps.has Type: ret Tok^parse_unary
         | push Tok GInput
         | ret   0
| Tok.parsed =  [P]
| Tok


DelimOps: `:` `=` `<=` `=>` `if` `then` `else` `elif` ` =` `-=` `*=` `/=` `%=`
DelimOps = DelimOps.bag
is_delim X = X.is_token and DelimOps.has X.type

OpsT: `+` `-` `*` `/` `%` `^` `.` `->` `|` ';' `,` `:` `=` `=>`
      `<=` `+=` `-=` `*=` `/=` `%=`
      `!` `-+-` `-^-` `-*-` `-<-` `->-` `^^` `..` `++` `--`
      `><` `<>` `<` `>` `<<` `>>`
      `\\` `$` `@` `&`

OpsT = OpsT.bag

list.ops_lookup_ Item = Me.any Item

tbl.ops_lookup_ Item = Me.has Item

parse_op Ops =
| if GInput.end: ret 0
| T GInput.0
| less T.is_token: ret 0
| less Ops.ops_lookup_ T.type: ret 0
| pop GInput

ParenChars ['_' '`' '?' '~' `)` `}` `]` '"' "'"].bag

is_anchor_char C = C.is_alpha or C.is_digit or ParenChars.has C

SufUnaryS: '+' '-' '*' '/' '%' '<' '>' '<<' '>>'

SufUnaryL: '{}' '()' '[]' '[' '{' @SufUnaryS
SufBinaryL: `.` `^` `->`
SufUnary SufUnaryL.bag
SufBinary SufBinaryL.bag
SufOps [@SufBinaryL @SufUnaryL].bag
SufUnaryS = SufUnaryS.bag

binary_loop Ops Down E =
| O | parse_op Ops or ret E
| B | Down()
| NE if B: [O E B]
     else | O.value = "[O.value]_" //no right operand for O
          | [O E]
| binary_loop Ops Down NE

parse_binary Down Ops = binary_loop Ops Down: Down() or ret 0

DollarList [`$` unary `\\` `@` `&` `@@`].bag

parse_dollar =
| O | parse_op DollarList or ret (parse_term)
| when O^token_is(unary): ret O^parse_unary
| [O (parse_dollar or parser_error "no operand for" O)]

DisjointChars [',' ':' '<' '>' '<<' '>>'].bag

next_is_disjoint O =
  if GInput.end: ret 1
  N GInput.0
  if N.row <> O.row: ret 1
  if N.col <> O.col + O.value.n: ret 1
  if DisjointChars.has N.type: ret 1
  0

parse_suf_unary Down E O =
| less SufUnary.has O.type: ret 0
| case O.type
  '()'+'[]'+'['+'{}'+'{'
    | Valid 0
    | when O.pchar:
      | if is_anchor_char O.pchar:
        | Valid = 1
        | if O.type >< '[]': O.type = '['
    | less Valid:
      | if O.type >< '{}':  O.type = '{'
      | push O GInput
      | ret [E]
  (SufUnaryS.has ?)
    | Valid 0
    | when O.pchar:
      | if is_anchor_char O.pchar and next_is_disjoint O:
        | Valid = 1
        | less O.value.~><_: O.value = "[O.value]_"
    | less Valid:
      | push O GInput
      | ret [E]
    | ret:: (parse_suf_loop Down [O E])
| As parse_tokens O.value
| As = if got As.find(&is_delim) then [As] else As //allows Xs.map(X=>...)
| O.parsed = [O.type]
|: (parse_suf_loop Down [O E @As])


parse_suf_op =
| if GInput.end: ret 0
| less SufOps.has GInput.0.type: ret 0
| pop GInput

parse_suf_loop Down E =
| O | parse_suf_op or ret E
| R parse_suf_unary Down E O
| if R: ret R.0
| B | Down()
| less B:
  | if O^token_is('.') or O^token_is('^'): less GInput.end:
    | T GInput.0
    | if T.is_token and got OpsT.(T.value):
      | TV T.value
      | if TV><'!':
          T.type = ':'
          T.value = ':'
      | if T.value><`:`:
        | R let GOutput [T]
          | parse_delim 0
          | GOutput.f
        | R.1 = T.retok TV
        | ret: parse_suf_loop Down [O E R]
      | pop GInput
      | T.type = \symbol
      | if T.value><`=`: less GInput.end:
        | K GInput.0
        | if K.is_token and K.value.is_keyword:
          | pop GInput
          | T.value = "[T.value][K.value]"
      | ret [O E T]
  | parser_error "no right operand for" O
| less O^token_is('.') and E^token_is(int) and B^token_is(int):
  | ret: parse_suf_loop Down [O E B]
| V "[E.value].[B.value]"
| F token float V E.src [V.flt]
| ret: parse_suf_loop Down F


parse_suf = parse_suf_loop &parse_dollar: parse_dollar() or ret 0

is_eol T = GInput.end or GInput.0.row-T.row or GInput.0.col-T.col > T.value.n
parse_unary H =
| if is_eol H: ret [nullary_ H]
| A parse_prefix
| less A: ret [nullary_ H]
| if H.value<>'-': ret [H A]
| less A^token_is(int) or A^token_is(hex) or A^token_is(float): ret [H A]
| token A.type "[H.value][A.value]" H.src [-A.parsed.0]
PrefixList [unary `\\` `@` `&` `@@`].bag
parse_prefix =
| O | parse_op PrefixList or ret (parse_suf)
| when O^token_is(unary): ret O^parse_unary
| Pref parse_prefix
| less Pref: case GInput [T<token? @_]:
    if SufBinary.has T.value: Pref =: nullary_ /GInput
| if Pref: [O Pref] else [O]
parse_pow = parse_binary &parse_prefix [`^^`]
parse_mul = parse_binary &parse_pow [`*` `/` `%`]
parse_add = parse_binary &parse_mul [`+` `-`]
parse_sufs = parse_binary &parse_add [`..`]
parse_b_shift = parse_binary &parse_sufs [`-<-` `->-`]
parse_b_and = parse_binary &parse_b_shift [`-*-`]
parse_b_xor = parse_binary &parse_b_and [`-+-` `-^-`]
parse_comma = parse_binary  &parse_b_xor [`,`]

BoolList [`><` `<>` `<` `>` `<<` `>>`].bag
parse_bool = parse_binary &parse_comma BoolList

parse_blogic = parse_binary &parse_bool [`&&` `||`]

parse_excl_loop Ops E =
| O | parse_op Ops or ret E
| parse_excl_loop Ops [O E]

parse_exclamation =
| less GInput.end: when GInput.0^token_is(`!`): ret  : pop GInput
| parse_excl_loop [`!`]: parse_blogic or ret   0

CndList: `if` `then` `else` `elif`

parse_logic =
| O | parse_op [`and` `or`] or ret   (parse_exclamation)
| GOutput =  GOutput.f
| P GInput.locate(&is_delim) //hack LL(1) to speed-up parsing
| Tok | got P and GInput.P
| when no P or got CndList.locate(X => Tok^token_is(X)):
  | GOutput =  [(parse_xs 0) GOutput O]
  | ret   0
| R GInput.take(P)
| GInput =  GInput.drop(P)
| GOutput =  if Tok^token_is(`:`)
             then [[O GOutput.tail R^parse_tokens] GOutput.head]
             else [[O GOutput R^parse_tokens]]
| No

is_var_tok X =
  less X.is_token: ret 0
  V X.value
  V.is_text and not V.is_keyword

parse_offside Type ExpectEOL ORow OCol =
| when GInput.end: ret   []
| K GInput.0
| while K.is_list and not K.end: K = K.0
| less K.is_token: ret  : parse_xs 0
| Src K.src
| LinesHack 0
| less ORow < Src.0:
  | when ExpectEOL: ret  : parse_xs 0
  | LinesHack = 1
| when K.value><`|` or K.value><`:`: ret  : parse_xs 0
| SCol Src.1
| when got OCol and SCol << OCol: ret  : parse_xs 0
| SInput GInput
| H parse_exclamation
| GInput = SInput
| case H [X Y]: if X^token_is(`!`): less Y^is_var_tok: ret: parse_xs 0
| Zs:
| Ys:
| BSrc: Src.0 Src.1-1 Src.2
| till GInput.end:
  | X GInput.0
  | when X.is_token:
    | Col X.src.1
    | when Col < SCol: done
    | V X.value
    | when Type >< 'if' and V >< `then`: done
    | when Col >< SCol and not Ys.end
           and V <> `|`  and V <> `else` and V <> `elif` and V <> `then`:
      | less Type >< 'if': 
        | Bar token `|` `|` BSrc 0
        | push [Bar @Ys.f] Zs
        | Ys =:
  | pop GInput
  | push X Ys
| Bar token `|` `|` BSrc 0
| push [Bar @Ys.f] Zs
| when LinesHack and Zs.n << 1:
  | GInput = SInput
  | ret  : parse_xs 0
| Input Zs.f.j
| R parse_tokens Input
| R.f

tok.as_text = "#[$value]"

DelimList [`:` `=` `<=` `=>` `+=` `-=` `*=` `/=` `%=`].bag
parse_delim DelimCtx =
| O | parse_op DelimList or ret   (parse_logic)
| Pref if GOutput.n > 0 then GOutput.f else []
| OCol No
| ExpectEOL 0
| when O.value><`:`:
  | ExpectEOL = 1
  | case Pref [X@_]
    | when X.is_token and X.value.is_keyword: ExpectEOL = 0
| Xs parse_offside 0 ExpectEOL O.src.0 No
| GOutput =: Xs Pref O
| No

parse_semicolon =
| P GInput.locate(X => X^token_is('|') or X^token_is(';'))
| M when got P: GInput.P
| when no P or M^token_is(`|`): ret   0
| L parse_tokens GInput.take(P)
| R parse_tokens GInput.drop(P+1)
| GInput =  []
| GOutput =  if R.n and R.0^token_is(';')
             then [@R.tail.f L M]
             else [R L M]
| No

parse_xs DelimCtx =
| let GOutput []
  | parse_semicolon
  | named loop // FIXME: implement unwind_protect
    | while 1:
      | X | parse_delim DelimCtx or ret loop GOutput.f
      | when got X: push X GOutput

parse_tokens Input =
| let GInput Input
  | Xs parse_xs 0
  | less GInput.end:
      Tok pop GInput
      if no OpsT.(Tok.type): parser_error "unexpected" Tok
      N token symbol "(nullary_ [Tok])" Tok.src [[nullary_ Tok]]
      GInput =: N @GInput
      Xs =: @Xs @parse_tokens(GInput)
  | Xs

parse_strip X =
| if X.is_token
  then | P X.parsed
       | R if P then parse_strip P.0 else X.value
       | R
  else if X.is_list
  then | less X.n: ret   X
       | Head X.head
       | Meta when Head.is_token: Head.src
       | Ys map V X: parse_strip V
       | when got Meta and Meta.2 <> '<none>': Ys =  meta Ys Meta
       | Ys
  else X


normalize_folder F =
| when F >< '': ret   './'
| if F.~ >< '/' then F else "[F]/"

//FIXME: eval.s/produce_sbc could use this function too
resolve_src_path Name Folders =
| for Folder Folders
  | SrcFile "[Folder][Name].s"
  | when SrcFile.exists: ret SrcFile
  | Donor "[Folder][Name].sbc"
  | when Donor.exists: ret Donor
| No

lexical_macro_expand SrcFile Text LexP =
| [Name Paths] LexP
| [Uses Exps HSz Row] parse_module_hdr_ Text
| Hdr Text.take(HSz)
| Body Text.drop(HSz)
| SrcFolder normalize_folder SrcFile.url.0
| Incs:
| NameFolder Name.url.0
| SrcFolder SrcFile.url.0
| if SrcFolder.url.0.split^'/'(:@_ ui ''): Uses =: @Uses cls uim 
| for U Uses:
  | D U
  | when NameFolder<>'' and "[SrcFolder][D].s".exists: D = "[NameFolder][D]"
  | SrcPath resolve_src_path D Paths
  | less got SrcPath: bad "couldn't find [U] for [SrcFile]"
  | [P F @_] SrcPath.url
  | HF "[P][F].h"
  | less HF.exists: pass
  | Incs =: @Incs U
| Incs = Incs{Inc => "#:[Inc]\n"}
| Text [@Hdr @Incs "#line [Row+1] \"[SrcFile]\"\n" @Body].text
| ncm_process_ SrcFile Text Paths.l

//lexP provides paths for the lexical macro-processor
//if macroexpansion without the paths is needed, just pass []
text.parse Src!'<none>' LexP!No =
| Text Me
| when got LexP: Text = lexical_macro_expand Src Text LexP
| R parse_strip: parse_tokens: add_bars_c_: Text.tokenize(Src)
| if R.end then [[]] else R.0.tail

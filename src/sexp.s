//stripped down parser which reads only the core lisp SEXP syntax

// is_token_ defaults to 0 for everything but actual tokens. Lists,
// texts, integers etc. all answer 0; only `tok` (the lexer token
// type) overrides to 1. Without this fallback, `parse_strip` in
// this file calling X.is_token_ on a list raises "no method".
_.is_token_ = 0
tok.is_token_ = 1

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

token_is What O = O.is_token_ and O.type >< What

parser_error Cause Tok =
| [Row Col Orig] Tok.src
| bad "[Orig]:[Row],[Col]: [Cause] [Tok.value or 'eof']"

parse_unary H =
| A parse_dot
| less A^token_is(int) or A^token_is(hex) or A^token_is(float): ret   [H A]
| token A.type "[H.value][A.value]" H.src [-A.parsed.0]

parse_table Xs =
| As:
| Key No
| Val:
| Cnt 0
| push_kv = if Cnt: push [Key Val.f.0] As
| for X Xs:
    if X(:[`!` _]) then
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

parse_term =
| when GInput.end: ret   0
| Tok pop GInput
//| when Tok.parsed: ret Tok
| V Tok.value
| P case Tok.type
    escape+symbol+text | ret   Tok
    `()` | parse_tokens V
    int
      | if V.n>2 and V.1 >< 'x':
          if V.0><'0': V.drop(2).int(16)
          else V.drop(2).int(V.0.int(10))
        else V.int
    hex | V.tail.int(16)
    bin | V.drop(2).int(2)
    void | No
    unary | ret Tok^parse_unary
    `-` | ret   Tok^parse_unary
    `@{}`
      | Ts parse_strip: parse_tokens2 V
      | @t: parse_table: parse_strip Ts
    Else | push Tok GInput
         | ret   0
| Tok.parsed =  [P]
| Tok

parse_dot =
| E | parse_term or ret 0
| if GInput.end: ret E
| if GInput.0.type <> '.': ret E
| O pop GInput
| B parse_term
| less B: parser_error "no right operand for" O
| less O^token_is('.') and E^token_is(int) and B^token_is(int):
  | parser_error "Invalid float" O
| V "[E.value].[B.value]"
| token float V E.src [V.flt]



parse_xs DelimCtx =
| let GOutput []
  | named loop // FIXME: implement unwind_protect
    | while 1:
      | X | parse_dot or ret loop GOutput.f
      | when got X: push X GOutput

parse_tokens Input =
| let GInput Input
  | Xs parse_xs 0
  | less GInput.end: parser_error "unexpected" GInput.0
  | Xs

//these are here just to parse the @{} tables
parse_op Ops =
| if GInput.end: ret 0
| less Ops.any GInput.0.type: ret 0
| pop GInput
parse_suffix_loop Ops E =
| O | parse_op Ops or ret   E
| parse_suffix_loop Ops [O E]
parse_exclamation =
| less GInput.end: when GInput.0^token_is(`!`): ret: pop GInput
| parse_suffix_loop [`!`]: parse_dot or ret 0
parse_xs2 DelimCtx =
| let GOutput []
  | named loop // FIXME: implement unwind_protect
    | while 1:
      | X | parse_exclamation or ret loop GOutput.f
      | when got X: push X GOutput
parse_tokens2 Input =
| let GInput Input
  | Xs parse_xs2 0
  | less GInput.end: parser_error "unexpected" GInput.0
  | Xs


parse_strip X =
| if X.is_token_
  then | P X.parsed
       | R if P then parse_strip P.0 else X.value
       | R
  else if X.is_list
  then | less X.n: ret   X
       | Head X.head
       | Meta when Head.is_token_: Head.src
       | Ys map V X: parse_strip V
       //| when got Meta and Meta.2 <> '<none>': Ys =  meta Ys Meta
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
| for U Uses:
  | D U
  | when NameFolder<>'' and "[SrcFolder][D].s".exists: D = "[NameFolder][D]"
  | SrcPath resolve_src_path D Paths
  | less got SrcPath: bad "couldn't find [U]"
  | [P F @_] SrcPath.url
  | HF "[P][F].h"
  | less HF.exists: pass
  | Incs =: @Incs U
| Incs = Incs{"#:[?]\n"}
| Text [@Hdr @Incs "#line [Row+1] \"[SrcFile]\"\n" @Body].text
| ncm_process_ SrcFile Text Paths.l

//lexP provides paths for the lexical macro-processor
//if macroexpansion without the paths is needed, just pass []
text.sexp Src!'<none>' LexP!No List!0 =
| Text Me
| when got LexP: Text = lexical_macro_expand Src Text LexP
| R parse_strip: parse_tokens: Text.tokenize(Src)
| if List: ret R
| if R.end: No else R.0

use eval

Root main_path
Args main_args

print_usage =
  say 'Usage: symta [OPTION] <SRC_DIR> [DST_DIR]'
  say '       symta [OPTION] <SRC_DIR/main.s> [DST_DIR]'
  say 'Produce DST_DIR/sbc and DST_DIR/run from SRC_DIR/src'
  say 'If DST_DIR is unspecified, SRC_DIR is used instead'
  say ''
  say 'OPTIONS:'
  say '       -r <root_dir>   search root_dir for compiler files'
  say '       -v              print version'
  say '       -h              print this help'
  say '       -e <expr>       evaluates expression'
  say '       -f <file>       evaluates a file'

print_version =
  say "Symta v[rt_get version]"
  say "Copyright (C) 2022-2026 Nancy Sadkov"
  say "See LICENSE for copying conditions"

is_var_sym X = X.is_text and not X.is_keyword

interpreter Root =
| print_version
| say "Type `usage` to list command line arguments."
| Env!
| Env.'Env_' = Env
| Env.'Uses_' =: core_ rt_
| Env.'Last_' = No
| Done 0
| till Done:
  | say_ '> '
  | Expr btrap: => get_line().parse.0
  | if Expr.is_bterror: say Expr.text
    else case Expr
    [exit+quit] | Done = 1
    [usage] | print_usage
    [use @Xs] | Env.'Uses_' = [@Xs @Env.'Uses_'].uniq
    [eval_file Filename]
      | Val eval_file Filename RootFolder!Root
      | if Val.is_bterror: say Val.text
    [`=` [Var<1^is_var_sym] Expr]
      | Val eval Expr Env!Env RootFolder!Root
      | if Val.is_bterror: say Val.text
        else Env.Var = Val
    [`=` [Var<1.is_keyword @Args] Expr]
      | L: `=>` Args Expr
      | Val eval L Env!Env RootFolder!Root
      | if Val.is_bterror: say Val.text
        else Env.Var = Val
    [type Name @Fields]
      | L: `|` Expr [`&` Name]
      | Val eval L Env!Env RootFolder!Root
      | if Val.is_bterror: say Val.text
        else Env.Name = Val
    [type Name @Fields]
      | L: `|` Expr [`&` Name]
      | Val eval L Env!Env RootFolder!Root
      | if Val.is_bterror: say Val.text
        else Env.Name = Val
    [`:` [type Name @Fields] @_]
      | L: `|` Expr [`&` Name]
      | Val eval L Env!Env RootFolder!Root
      | if Val.is_bterror: say Val.text
        else Env.Name = Val
    Expr
      | Val eval ['|' Expr] Env!Env RootFolder!Root
      | if Val.is_text: Val = [Val].as_text.tail.lead
      | if got Val: say Val(1.is_bterror=Val.text)

case Args [@_ '-h'+'--help' @_]
| print_usage
| halt

case Args [@_ '-v'+'--version' @_]
| print_version
| halt


case Args ['-r' UserRoot @Xs]
| Root = UserRoot
| Args = Xs

Root = Root{'\\'='/'}
less Root.~ >< '/': Root = "[Root]/"
less "[Root]sbc/compiler.sbc".exists: bad "Missing [Root]sbc/compiler.sbc"

//less "[Root]src/compiler.s".exists: bad "Missing [Root]src/compiler.s"
//less "[Root]runtime/symta.h".exists: bad "Missing [Root]runtime/symta.h"

case Args [@_ '-e'+'--eval' Expr @_]
| Val eval ['|' Expr.parse.0] RootFolder!Root
| if Val.is_bterror: say Val.text else say Val
| halt

//FIXME: there should be a builtin for setting a return value
//       which will be passed to OS on exit (either normal or by halt)
case Args [@_ '-f'+'--file' Filename @_]
| Val eval_file Filename RootFolder!Root
| if Val.is_bterror: say Val.text
| halt


SrcDir DstDir No

case Args
  [Src Dst] | SrcDir = Src; DstDir = Dst
  [Src] | SrcDir = Src; DstDir = Src
  Else | interpreter Root

when got SrcDir:
| SrcDir = SrcDir{'\\'='/'}
| DstDir = DstDir{'\\'='/'}
| build Root SrcDir dst!DstDir

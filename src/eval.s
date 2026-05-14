use compiler macro
export build eval eval_file

GRootFolder No   //compiler root folder
GBuildFolder No  //project build folder
GSrcFolders GSbcFolder  No
GHeaderTimestamp GMacros GCompiledModules No
GShowInfo 0

read_normalized Text Filename Name!No =
| LexP if got Name: [Name GSrcFolders] else No
| Text.parse(Src!Filename LexP!LexP)

load_symta_file Name Filename =
  read_normalized Filename.getText Filename Name!Name

load_macros Library = Library^load_sbc.keep(X => X.1.is_macro)

// FIXME: do caching, since many SBCs tend to be shared
sbc_exports LibName =
  if LibName >< rt_: ret (builtins_){?0}
  SbcFile "[GSbcFolder][LibName].sbc"
  less SbcFile.exists: bad "no [SbcFile]"
  SrcFile,DepsT,ExportT sbc_metadata_ SbcFile
  Es ExportT.parse.0
  when Es.end: Es = [@Es 'Dummy'.rand] //FIXME: kludge to force file load
  Es

// check if Dependent file is up to date with Source file
newerThan Source Dependent =
| DependentTime Dependent.time
| Source.time << DependentTime and GHeaderTimestamp << DependentTime

copy_runtime Dst =
| Src "[GRootFolder]symta"
| when rt_get windows:
  | Src =  "[Src].exe"
  | Dst =  "[Dst].exe"
| less Dst.exists:
  | fs_copy Src Dst
  | fs_chmod 755 Dst

add_imports Expr Deps =
  less Deps.n: ret Expr
  [[_fn (map D Deps D.1) Expr]
   @(map D Deps [_import [_quote D.0] [_quote D.1]])]

module_folders = [GRootFolder GBuildFolder GSrcFolders]

compile_expr Name SrcFile Dst Expr Uses![rt_ core_] DoExport!1 =
  SrcFolder SrcFile.url.0
  if SrcFolder.split^'/'(:@_ ui ''): Uses =: @Uses cls uim //UI code
  Expr = Expr(:[use @Us] @Xs = | Uses =: @Uses @Us; Xs)
  Uses = Uses{(norm_dep ?)}
  Uses = Uses.skip(X => Name >< X).uniq
  Deps Uses.skip(rt_) //skip builtin modules
  Uses Uses.keep(rt_)
  SbcPrefSz GSbcFolder.n
  NameFolder Name.url.0
  Deps = map D Deps:
    when NameFolder<>'' and "[SrcFolder][D].s".exists: D = "[NameFolder][D]"
    R produce_sbc D
    when no R: bad "couldnt compile [D].s"
    R.drop(SbcPrefSz)
  Uses =: @Uses @Deps
  when GShowInfo: say "compiling [Name]..."
  Imports (map U Uses: map E U^sbc_exports: [U E]).j
  Macros Imports.skip(X => X.1.is_text).map(X => X.0)
         .uniq.skip(X => X><macro) // keep macros (without macro.s)
  Imports Imports.keep(X => X.1.is_text) // skip macros
  Ms [GMacros @(map M Macros "[GSbcFolder][M]"^load_macros)].j
  Expr = if DoExport: [`|` @Expr [export_]].l
         else [`|` @Expr].l
  ExprWithDeps add_imports Expr Imports
  [ExpandedExpr Exports] macroexpand ExprWithDeps Ms.t
                                     &produce_sbc &module_folders
  Ssa produce_ssa ExpandedExpr
  ExportsList 0
  DepsList 0
  when got Dst:
    DepsList = Deps
    ExportsList = map E Exports: case E:
      [`\\` X] = "\\[X.as_text]"
      X = "[X.as_text]"
  SIFText ssa_to_sif Ssa DepsList ExportsList
  when no Dst: ret SIFText
  DstSBC "[Dst].sbc"
  DstFolder DstSBC.url.0
  less DstFolder.folder: DstFolder.mkpath
  Result sif_to_sbc_ DstSBC SIFText
  less DstSBC.exists:
    bad "Compilation failed[if got Result then ": [Result]" else ""]"
  Deps


norm_dep D = case D
  [`/` A B] | "[norm_dep A]/[norm_dep B]"
  Else | D            

produce_sbc Name =
| R GCompiledModules.Name
| when got R: ret R
| DstFile "[GSbcFolder][Name]"
| GCompiledModules.Name = DstFile
| SbcFile "[DstFile].sbc"
| for Folder GSrcFolders
  | SrcFile "[Folder][Name].s"
  | SrcExists SrcFile.exists
  | less SrcExists: less SbcFile.exists:
    | Donor "[Folder][Name].sbc"
    | when Donor.exists: fs_copy Donor SbcFile
  | when SbcFile.exists and (not SrcExists or SbcFile^newerThan(SrcFile)):
    | OSrcFile,DepsT,ExportT sbc_metadata_ SbcFile
    | Deps read_normalized(DepsT SbcFile).0{(norm_dep ?)}
    | CompiledDeps map D Deps:
      | produce_sbc D
    | when  SbcFile^newerThan(SrcFile)
         and CompiledDeps.all(X => got X and SbcFile^newerThan("[X].sbc")):
      | R = DstFile
      | pass //up to date
  | less SrcExists: pass //nothing to compile
  | Expr load_symta_file Name SrcFile
  | compile_expr Name SrcFile DstFile Expr
  | ret DstFile
| R

build_entry Entry =
| let GMacros 'macro'^load_macros
  | DstFile produce_sbc Entry
  | when no DstFile: bad "cant compile [Entry]"
  | DstFile

normalize_folder F =
| when F >< '': ret './'
| if F.~ >< '/' then F else "[F]/"

build RootFolder SrcFolder dst/0 =
| Entry "go"
| SrcPref "src/"
| DstFolder | Dst or SrcFolder
| when DstFolder.file: DstFolder =  DstFolder.url.0
| when SrcFolder.file:
  | SrcFolder =  SrcFolder.url.0
  | SrcPref =  ""
| RootFolder =  normalize_folder RootFolder
| SrcFolder =  normalize_folder SrcFolder
| DstFolder =  normalize_folder DstFolder
| when DstFolder.0 <> '/' and DstFolder.1 <> ':':
  | DstFolder =  "[get_work_folder]/[DstFolder]"
| let GRootFolder RootFolder
      GBuildFolder DstFolder
      GSbcFolder "[DstFolder]sbc/"
      GSrcFolders ["[SrcFolder][SrcPref]" "[GRootFolder]src/"]
      GHeaderTimestamp "[GRootFolder]/runtime/symta.h".time
      GShowInfo 1
      GCompiledModules (!)
  | less "[GSrcFolders.0][Entry].s".exists:
    | bad "Missing [GSrcFolders.0][Entry].s"
  | "[GSbcFolder]/ffi".mkpath
  | add_sbc_folder GSbcFolder
  | RuntimePath "[DstFolder]go"
  | copy_runtime RuntimePath
  | build_entry Entry
  | RuntimePath //unix RuntimePath //"[RuntimePath] ':[GSbcFolder]'"

default_root =
  Root main_path(){'\\'='/'}
  less Root.~ >< '/': Root = "[Root]/"
  CSbc "[Root]sbc/compiler.sbc"
  less CSbc.exists: bad "Missing [CSbc]"
  Root

//required since we want `Last_` preserved.
#ALT_NO (ref_ 14 1)

eval Expr Env!No RootFolder!No BuildFolder!No =
| when no Env: //FIXME: Env should be registered with runtime
  | Env =  No!ALT_NO 'Uses_'![core_ rt_] 'Last_'!No
  | Env.'Env_' = Env
| when no RootFolder: RootFolder = default_root
| when no BuildFolder: BuildFolder = GBuildFolder //reuse the parent's one
| when no BuildFolder: BuildFolder = "[RootFolder]tmp/"
| let GRootFolder RootFolder
      GMacros 'macro'^load_macros
      GSrcFolders ["[GRootFolder]src/"]
      GHeaderTimestamp "[GRootFolder]/runtime/symta.h".time
      GBuildFolder BuildFolder
      GSbcFolder "[BuildFolder]sbc/"
      GCompiledModules (!)
      GShowInfo 0
  | Entry @rand tmp
  | Vars map [K V] Env K
  | Values map [K V] Env V
  | Expr =  [_fn Vars
              ['|' ['=' ['Last_'] Expr]
                   @(map V Vars
                     | K ['\\' V]
                     | S if V.is_keyword: [`&` V] else V
                     | ['=' [['.' 'Env_' K]] S])
                   0]]
  | Expr = [[use @Env.'Uses_'] Expr]
  | SIFText btrap: => compile_expr Entry 'no_src' No Expr DoExport!0
  | when SIFText.is_bterror: ret SIFText
  | Fn sif_eval_ SIFText
  | R btrap: => Values.apply Fn
  | when R.is_bterror: ret R
  | Env.'Last_'

eval_file SrcFile RootFolder!No BuildFolder!No SrcFolders![] Uses![rt_ core_] =
| when no RootFolder: RootFolder = default_root
| when no BuildFolder: BuildFolder = GBuildFolder //reuse the parent's one
| when no BuildFolder: BuildFolder = "[RootFolder]tmp/"
| less SrcFile.exists: ret: bterror eval_file "File doesn't exist"
| let GRootFolder RootFolder
      GMacros 'macro'^load_macros
      GSrcFolders ["[GRootFolder]src/" @SrcFolders]
      GHeaderTimestamp "[GRootFolder]/runtime/symta.h".time
      GBuildFolder BuildFolder
      GSbcFolder "[BuildFolder]sbc/"
      GCompiledModules (!)
      GShowInfo 0
  | Entry @rand tmp
  | Expr btrap: => load_symta_file No SrcFile
  | when Expr.is_bterror: ret Expr
  | SIFText btrap: => compile_expr Entry 'no_src' No Expr Uses!Uses DoExport!0
  | when SIFText.is_bterror: ret SIFText
  | btrap: => | sif_eval_ SIFText

list.eval Env!No RootFolder!No BuildFolder!No =
  eval Me Env!Env RootFolder!RootFolder BuildFolder!BuildFolder

//allow stuff like "1+2".eval
text.eval Env!No RootFolder!No BuildFolder!No =
  eval Me.parse.0 Env!Env RootFolder!RootFolder BuildFolder!BuildFolder

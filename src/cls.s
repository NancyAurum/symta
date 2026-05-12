export new part cls_stats cls_parts 'cls' cls_tick id
       each_ each_tag_ 'each' cls_prune
       cls_tbl_ 'cls_tbl'
       cls_tbl_set_ 'cls_tbl_set'
       cls_get_ cls_set_
       'cph' cls_run_phase

/*
CLS CoLumnar Store (aka Symta's take on ECS).

"Any sufficiently complicated LISP program is going to contain a slow implementation of half of Prolog" - Norvig's Corollary to
Greenspun's Tenth Law of Programming

ECS runs queries over id (primary key) joined components (tables).
SQL does the same.

Backward chaining allows for implicit (deduced) components.

Consider syllogism:
Socrates is a man. (entity Socrates has component man)
All men are mortal. (system adds component mortal to every entity with man)
Therefore Socrates is a man. (result of system's work)

But the point is that we can run all-are system on demand.
Starting from the goal question is(Socrates mortal)
There we do backchaining from the allare system's facts table.
Basically we treat the system as an entity itself.
I.e. the above allare system is a set of entities with component `is`.
Now we've re-invented Prolog on top of an ECS engine. As they said, 

This framework introduces a flexible object system over the database-tables

Instead of the classic ECS entity-component-system we use id-part-react
Rationale:
 - "id" is shorter than "entity".
 - "part" is shorter than "component".
   part tables map ids to parts
   parts are rigged onto ids.
 - "react" is how part reacts to different triggers.
    It is shorter than "system" and better represents the reactive nature
    of the ECS idea. 
 - allows differentiating from other ECS frameworks
Referencing a part prefixed with '_' creates it
otherwise error is thrown is the part doesn't exist

CLS tries its best integrating with Symta, so stuff like `wgt{rect}` works as expected (for every widget retrieves its rect).


What people struggle accepting about ECS is that code processing ids must be
per component/component-group. For example,
- Geometry system requires id to have xyz part
- Physics system also requires velocity and accel vectors.
- Visual system uses both geometry and physics in addition to a shape part
  speed will determine how fast the shape animation plays.
One can create three classes for working with each side of the id.
These classes also provide their own implementation for each method.
In case of this framework, we call them classifiers.

Implementation notes:
- `cls` types are just a way to [temporarily] associate entity with an interface
  it has no normal fields, but instead uses the meta field.

TODO:
- All entities have flags and yet most have different sets of flags
  Maybe we can keep a generic bit field map together
  with some configuration structure describing what flags each
  bit represents.
- ECS query: find entities for a set of components
- Prefabs - premade clonalbe entities
- If each entity stores a list of all tables it uses, we can
  free it much quicker.
- To avoid id explosion, GC can replace the freed entities with a nul-entity.
  Then the freed id is placed onto the free list, which could be reused.
  That guarantees the number of ids stays below a specific base
  and we can use them as direct indicies into a table.
- System's phase determines in what order system in run.
  Basically determines what triggers the entity's reaction.
- `cls` should provide a way to declare and del sections of fields
- ECS should be integrated into runtime, so `id` would be a runtime
  type, while its methods would be provided by runtime
- A way to join two separately created entitites:
  parts of one entity should override the parts of another,
  while being removed from the donor

- Code like `id.__ Method Args =` could be greatly optimized
  Have `index_dispatch_` method
*/


TickN 0 //current step

TblsLst:
Tbls!

IdTag intern_ id
type id

id.id = stub_ Me &gid_

no.as_id = No
int.as_id = ref_ IdTag Me

cls_stats = nitems_total!(topId_) nparts!TblsLst.n


cls_parts = Tbls

part Name =
  Table (!)
  push Table TblsLst
  Tbls.Name = Table
  Table

cls_tbl_ Name = //gets or creates a table
  Table Tbls.Name
  if no Table: Table = part Name
  Table

cls_tbl_set_ Name NewTable = //gets or creates a table
  Table Tbls.Name
  if no Table: Table = part Name
  Tbls.Name = NewTable
  No

VoidIds!

id.void = VoidIds.(_gid Me) = 1

//FIXME: allow excluding some of the larger tables
//FIXME: only do cls_prune when the number of VoidIds is large enough 
//cls_prune works by NSteps,
//once NSteps are complete, PrunedIds are set to VoidIds
//and VoidIds are cleared
cls_prune Step NSteps =
  Count 0
  for Table TblsLst:
    Freed:
    for K,V Table:
      Count+
      if got VoidIds.K: push K Freed
    for Id Freed: Table.del Id
  VoidIds = !
  say cls_prune Count

new @UseTbls =
  Me ref_ IdTag (newId_)
  for UP UseTbls: $rig UP
  Me

no.new_fn_ = &new

id.rig PartInfo =
  Name,Value if PartInfo.is_list: PartInfo else PartInfo,No
  Table cls_tbl_ Name
  if got Value: Table.(_gid Me) = Value

each_ PartName =
  Table Tbls.PartName
  if no Table: ret []
  gid_refs_ Table IdTag //Table{(ref_ IdTag ?0)}

each_tag_ Tag PartName =
  Table Tbls.PartName
  if no Table: ret []
  gid_refs_ Table Tag //Table{(ref_ IdTag ?0)}


cls_tbl @Args =
  case Args:
    [['.' TagName PartName]] =
      if PartName.is_keyword: PartName = "[TagName]_[PartName]" //prefix it
      form: cls_tbl_ PartName
    [PartName] = form: cls_tbl_ PartName
    Else = bad "Bad args [Args]"

cls_tbl_set @Args =
  case Args:
    [['.' TagName PartName] Table] =
      if PartName.is_keyword: PartName = "[TagName]_[PartName]" //prefix it
      form: cls_tbl_set_ PartName Table
    [PartName Table] = form: cls_tbl_set_ PartName Table
    Else = bad "Bad args [Args]"


/*******************************************************************************
Produces a list of all entities having some component.

Usage:
  each(component) //raw ids for that component
  each(typename.component) //prefixes the component with typename
  each(typename.'component') //don't prefix the component with typename
  each(typename.Component) //variable component name

*******************************************************************************/
each @Args = //list of all items using this part
  case Args:
    [['.' TagName PartName]] =
      if PartName.is_keyword: PartName = "[TagName]_[PartName]" //prefix it
      form: each_tag_ (_tag (_data TagName)) PartName
    [PartName] = form: each_ PartName
    Else = bad "Bad args [Args]"

id.parts = //list all item parts
  Id _gid Me
  @skip No: map Name,Table Tbls: if Table.has Id: Name

finalize_parts Me Tables =
  Id _gid Me
  for Table Tables: if got Table and Table.has Id: Table.del Id


id.del @Names = finalize_parts Me Names{Tbls.?}

cls_tick =
  bad "FIXME: cls_tick"
  for Name,Table Tbls: Table.upd()(Table)
  TickN+

id.got Part = //checks if item has part Part
  Table Tbls.Part
  if no Table: ret 0
  Table.got (_gid Me)


id.__ Method Args =
  MN Method^_method_name
  if _gt Args.n 1:
    if MN.0 <> '=':
      F Me.MN
      less F.is_fn: bad "method `id.[MN]` is undefined"
      ret: F(@Args)
    Me.(MN.tail) = Args.1
  else Me.MN

id.`.` Name =
  Table Tbls.Name
  if got Table: Table.(_gid Me) else No
  

id.`=` Name Value =
  Table cls_tbl_ Name
  Table.(_gid Me) = Value

cls_get_ Id Name =
  Table Tbls.Name
  if got Table: Table.Id else No

cls_set_ Id Name Value =
  Table cls_tbl_ Name
  Table.Id = Value

id.ser = "$[$t.as_text.drop^1]" //serialize

id.as_text = "$<[$ub.'cls']:[_gid Me]>"

id.ub = ref_ IdTag (_gid Me) //unbox

list.clone = dup I $n $I

_.clone = Me

//implement deep_clone, which would allow cloning the component items
//Just ensure it detects loops and respects the clone method
//we need a special "being-cloned" table

id.clone =
  C ref_ IdTag (newId_)
  for K,V $xs:
    less V.is_item: V = V.clone
    C.K = V
  C

id.xs = $parts{?,$?}

id.t = $parts{?,$?}.t


ClsCvts!
no.set_cls_cvt_ Name F = ClsCvts.Name = F

id.cls =
  Cvt ClsCvts.($ub.'cls')
  if no Cvt: bad "No `cls` for [Me]"
  Cvt((_gid Me))

id.trycls =
  Cvt ClsCvts.($ub.'cls')
  if no Cvt: ret Me
  Cvt((_gid Me))

id.init_id = ref_ (_tag Me) (newId_)

cls_tbl_ cls //ensure the cls table always exists

id.init_cls Value = //set value only if it isn't set
  Id _gid Me
  less Id:
    Me = $init_id
    Id = _gid Me
  Table Tbls.'cls'
  if no Table.Id: Table.Id = Value
  Me

id.init_cls_nocvt =
  Id _gid Me
  less Id: Me = $init_id
  Me


is_constant Expr =
  if Expr.is_int: ret 1
  if Expr.is_float: ret 1
  if Expr.is_fixtext: ret Expr.is_keyword
  if Expr.is_keyword: ret 1
  if imm_ Expr: ret 1 //immediate, like No
  case Expr:
    ['\\' X] = 1
    ['$' X<0.is_keyword] = 1
    [[Expr]<E] = is_constant E
    ['[]'+'+'+'-'+'*'+'/'+'%' @Xs] = Xs.all(&is_constant)
    Else = 0


/*******************************************************************************

Column-oriented Struct (aka CLasSifier)

Usage:
  cls tname.parent(@ParentArgs)! @`prefix` @Params @Args
    :
    @Fields
    =
    @InitCode

!                      - Provide `.tname` method

@`prefix`              - Picks custom prefix for the database columns,
                         associated with this type. Default is `tname_`
                         Can be ``



@Params:               - Params to the cls
  \nocvt               - Dont init the `cls` column, which allows
                         converting the unknown `id` entities to this type
  \clearable           - Provides the No.cls_clear_tname method,
                         which clears all tables associated with this type

@Args:
  UpperCaseName        - Normal argument name, usable Fields values and Initcode
  lowerCaseName        - Creates a field and initalizes with that arg
  Name!Value           - Keyword argument with a default value

@Fields:
  name                 - Unitialized field, which defaults to No
  name!!Default        - Unitialized field, which defaults to Default
  name!Value           - Initialized field. Value could reference @Args
  'name'!Value         - Creates a column prefix_name, but doesn't generate
                         the cls.name method.
                         Symta's version of C++ private data fields

*******************************************************************************/
cls Name @Fs =
  OName Name
  NeedsTypePredicate Name(0:['!' &Name]) //wants a type predicate?
  Parent \id
  ParentArgs:
  Name(:['()' &Name @&ParentArgs])
  Name(:['.' &Name &Parent])
  Sink No
  case Parent:
    [`$` Part] =
      Sink = Part
      Parent = \id
      Name(:['.' &Name &Parent])
  Pfx "[Name]_" //part prefix
  case Fs [`@` &Pfx],@R: Fs = R //cls Name@UserPfx @Fs
  Params nocvt!0 clearable!0
  while 1:
    case Fs:
      [`\\` Param] @_ =
        case Param:
          [`()` Param @Args] =
            case Param:
              [`.` Param Suf] = Params."[Param].[Suf]" = Args
              Else = Params.Param = Args
          Else = Params.Param = 1
      Else = done
    Fs = Fs.tail
  Body:
  AFs: //auto initialized fields
  case Fs [@FsP [[`|` @L [`=` [] []] @R ]]]: Fs =: @FsP [`=` L.j [`|` @R]]
  case Fs [@FsP [`=` &AFs &Body]]: Fs = FsP
  case Fs [@FsP &Body<[`|` @_]]: Fs = FsP
  //normalize into [Name Value PrefixedName]
  // If Name is `_`, dont produce a database column, only a method
  AFs = AFs{['!' ['@' ['.' PN N]]] V =: N V PN
           ;['!' ['@' PN]] V =: _ V PN
           ;['!' ['!' N]] V =: N ['!' V] "[Pfx][N]"
           ;['!' N] V =: N V "[Pfx][N]"
           ; N =: N No "[Pfx][N]"}
  if NeedsTypePredicate: push [_ 1 Name] AFs //if Object.widget: ...
  //push [_ Name cls]  AFs
  FNs: //field names
  PFNs: //typenanme-prefixed field names
  KVs: //keyworded args default value
  As: //arguments
  FAs: //field arguments
  till Fs.end:
    KV No //keyword value
    case Fs [[`!` _] [`!` _]@_]+[[`!` _]]: Fs =: Fs.head No @Fs.tail
    case Fs [[`!` F] D @R]:
      KV =: D
      Fs =: F @R
    push KV KVs
    AN /Fs
    A AN.title
    push A As
    if AN.is_keyword:
      push AN FNs
      push A FAs
      PAN "[Pfx][AN]"
      Fs(:[`@` &PAN],@&Fs)
      push PAN PFNs
  FNs,FAs,As,KVs,PFNs [FNs FAs As KVs PFNs]{f}
  KAs [As KVs].zip{A,[D] =: ['!' A] D; A,_=:A}.j //args with keywords
  Fields: @[FNs PFNs].zip{?0,?1,No} @AFs.skip(?0><_){?0,?2,?1}
  ParentRig []
  ParentDel []
  Me \Me
  Id form: _gid Me
  if Parent<>id:
    PR form: Me.$("rig_[Parent]") $@ParentArgs
    push PR ParentRig
    PD form: Me.$("del_[Parent]") 
    push PD ParentDel
  InitedAFs AFs.keep(?1(1:No+['!' V]=0))
  InitCls if Params.nocvt: form (Me = $init_cls_nocvt)
          else form (Me = $init_cls Name)
  Hdr1 form: `|` (cls_set_ Id cls Name)
                $@([PFNs FAs].zip{PFN,A = form: $rig PFN,A})
                $@(InitedAFs{N,V,PN = form: $rig PN,V})
  Hdr form: InitCls
            $@ParentRig
            $@([PFNs FAs].zip{PFN,A = form: $rig PFN,A})
            $@(InitedAFs{N,V,PN = form: $rig PN,V})
  Type form: type Name.Parent $@(KAs): = `|` $@Hdr Body
  Rig1 form: id.$("rig1_[Name]") $@(KAs) = $@Hdr1 Body Me
  Rig form: id.$("rig_[Name]") $@(KAs) = `|` $@Hdr Body Me
  IdS "Id".rand
  Del1F form: `|` (IdS Id) $@(Fields{FN,PFN,V = form: $"T_[PFN]_".del IdS})
  Del1 form: id.$("del1_[Name]") = Del1F
  //Del1 form: id.$("del1_[Name]") = $del $@(Fields{?1})
  Del form: id.$("del_[Name]") = `|` $$("del1_[Name]") $@ParentDel
  AsText form: Name.ser_ = "${[\Name] [Me.ub.t.as_text.drop^2]}"
  //`cls`-types don't have the `__` method
  //Instead they explicitly define accessors for the used fields.
  FAs:
  TblNos:
  for [FN PFN V] Fields:
    SetPrologue 0
    GetPrologue 0
    Acc Params."acc.[FN]"([Name] = Name)
    if got Acc: FN = Acc
    case V:
      ['!' X] =
        case X [['$' SE] @&X]: //user wants custom setter prologue
          SetPrologue = SE
          case X [['$' GE] @&X]: GetPrologue = GE
          X = X.~
          V =: '!' X
        if X^is_constant:
          case X ['$' E]: X = E
          push PFN,X TblNos
          V = No
    FG case V: //field getter: replacing missing elements by the default value
        ['!' X] =
          case GetPrologue:
            ['=' [Var] PBody] =
              form: Name.FN =
                Var (gid_get_ $"T_[PFN]_" Me).@X
                PBody
            Else = form: Name.FN = (gid_get_ $"T_[PFN]_" Me).@X
        Else = case GetPrologue:
                 ['=' [Var] PBody] =
                   form: Name.FN =
                     Var (gid_get_ $"T_[PFN]_" Me)
                     PBody
                 Else = form: Name.FN = cls_get_stub_ Me $"T_[PFN]_"
    push FG FAs
    FSN "=[FN]"
    FS case V: //field setter: assigning default value removes the element
        ['!' V] =
          case SetPrologue:
            ['=' [Var] PBody] =
              form: Name.FSN Var =
                ~V PBody
                if ~V><V: ($"T_[PFN]_").del Id
                else gid_set_ $"T_[PFN]_" Me ~V
            Else =
              form: Name.FSN ~V =
                if ~V><V: ($"T_[PFN]_").del Id
                else gid_set_ $"T_[PFN]_" Me ~V
        Else =
          case SetPrologue:
            ['=' [Var] PBody] =
              form: Name.FSN Var = gid_set_ $"T_[PFN]_" Me PBody
            Else = form: Name.FSN ~V = cls_set_stub_ Me ~V $"T_[PFN]_"
    push FS FAs
  if got Sink:
    S form: Name.__ ~Method ~Args = |~Args.0 = $(\Me).Sink; ~Args.apply_method ~Method
    push S FAs
  AsType form: id.$("[Name]_") = ref_ (_tag (_data Name)) Id
  SetCvt form: No.set_cls_cvt_ Name: ~Id => ref_ (_tag (_data Name)) ~Id
  push AsType FAs
  push SetCvt FAs
  FTbls map [FN PFN V] Fields: //field tables
    form: $"T_[PFN]_" cls_tbl_ PFN
  SetNos map PFN,NoVal TblNos: //set default value for the tables
    form: $"T_[PFN]_".setNo NoVal
  Clearer:
  if Params.clearable:
    CTbls map [FN PFN V] Fields: form: $"T_[PFN]_".clear
    Clear form: no.$("cls_clear_[Name]") = `;` $@CTbls
    Clearer =: Clear
  FieldNames Fields{['\\' ?1]}
  FldLst form: no.$("cls_[Name]_fields_") = `[]` $@FieldNames
  form @(`|` $@FTbls $@SetNos Type Rig1 Rig Del1 Del AsText $@FAs $@Clearer
             $FldLst)


//We need singleton entity for updates inside phases
type lone.id
Lone ref_ (_tag (lone)) (newId_)
no.lone = Lone


Phases!

no.cls_add_phase Phase Tag TagName PartName Method =
  Phases.Phase =: @Phases.Phase.@[] Tag,PartName,Method,TagName


// The `cph Phase -Method` means that we delete the part from further updates

//FIXME: allow cph phase_name --Method
//       where `--` means decrement the partname until it reaches 0
//       and then delete it
//       That will simplify `cph anim unit.'suffers'` and similar
//       actions
cph Phase Method @Args Body = //columnar phase
  TagName PartName 0
  Tag No
  DoDel 0

  FName 0
  case Method ['/' M Name]:
    Method = M
    FName = Name

  case Method ['-' &Method]: DoDel = 1
    
  case Method:
    ['.' &TagName ['\\' &PartName]] =
      Tag = form: _tag (_data TagName)
    ['.' &TagName &PartName] =
      Tag = form: _tag (_data TagName)
      PartName = "[TagName]_[PartName]"
    [&PartName < 1.is_text] =
    Else = bad "cph bad method [Method]"
  less FName: FName = PartName.rand
  if DoDel: Body = [';' (form: $del PartName) Body]
  case Args [Arg @As]: //user wants us to bind that variable before deling it?
    Args = As
    Body = [';' (form: Arg (`.` $\Me (`\\` PartName))) Body]
  Init 0
  if TagName><lone:
    Init = form: No.lone.(`\\` PartName) = 1
  R form:
      $@Init
      FName $\Me = Body
      No.cls_add_phase Phase Tag TagName \PartName &FName
  form: `@` $@R

cls_run_phase Phase =
  for Tag,PartName,Method,TagName Phases.Phase.@[]:
    if got Tag:
      for E each_tag_(Tag PartName): Method(E)
    else
      for E each_(PartName): Method(E)



/*


// EXAMPLE:
cls unit@`` \nocvt: //@`` means dont prefix fields with `unit_`
                    //\nocvt means id to unit conversion method is missing
                    //       reducing parts count
                    //!! provides a default value other than No
                    //! just sets the value at init, while default remains No
  //Core Data
  msps!!$mold.msps //mold supplied parts
  isps!![] //instance supplied parts

  type         //textual type id
  title!!$class_name.title  //proper unit type title shown to player
  icon!!dummy  //icon for this unit


*/


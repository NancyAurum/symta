export say bad new_macro meta methods
       rand_get rand_set rand_push rand_pop zip setters_ getters_ atan2
       btland btjump bterror btrap `..`

_.new_fn_ = No

_.free =

//No acts as an identity element in arithmetics
no.`+` B = B
no.`-` B = B
no.`*` B = B
no.`++` = 1
no.`--` = -1

float.`++` = Me + 1.0
float.`--` = Me - 1.0


no.'<' B =
  if B.is_int: ret B < 0
  0
no.'>' B =
  if B.is_int: ret B > 0
  0
no.`<<` B =
  if B.is_int: ret B << 0
  0
no.`>>` B =
  if B.is_int: ret B >> 0
  0

_.bool = 1
int.bool = if Me: 1 else 0
float.bool = Me <> 0.0
text.bool = if Me.end: 0 else 1
no.bool = 0

text.`+` B = "[Me][B]"

_._ = [Me]
list._ = Me
_list_._ = Me
_empty_._ = Me
hard_list._ = Me

_.`><` B = same Me B
_.`<>` B = not Me >< B
_.`<<` B = not B < Me
_.`>` B = B < Me
_.`>>` B = not Me < B

_.is_int = 0
int.is_int = 1

_.is_float = 0
float.is_float = 1

_.is_fn = 0
fn.is_fn = 1

_.is_list = 0
list.is_list = 1

_.is_text = 0
text.is_text = 1

_.is_fixtext = 0
_fixtext_.is_fixtext = 1

//_.is_hard_list = 0
//list.is_hard_list = 1

_.is_table = 0

_.copy = Me
list.copy = map X Me X

_.deep_copy = Me
list.deep_copy = map X Me X.deep_copy

methods Object = Object^methods_.t

_.`()` A = A.Me
fn.`()` @As = As.apply(Me)

_.`{}` F = $map(F)

int.sign = if Me < 0 then -1
           else if Me > 0 then 1
           else 0

float.sign = if Me < 0.0 then -1.0
             else if Me > 0.0 then 1.0
             else 0.0

list.sign = map X Me X.sign

int.abs = _abs Me

float.abs = _abs Me

float.log2 = $log/2.0.log

list.metric =
  R 0.0
  map X Me: R += @float X*X
  R

list.abs =
  R 0.0
  map X Me: R += @float X*X
  R.sqrt

list.normalize = Me / $abs

list.orth = [$1 -$0].normalize //perpendicular vector to a given

list.loop I =
| S $n
| if I<0 then Me.(S - I%S) else Me.(I%S)

list.neg = dup I $n -$I
list.`+` Ys = dup I $n $I+Ys.I
list.`-` Ys = dup I $n $I-Ys.I
list.`*` Ys =
   if Ys.is_list: dup I $n $I*Ys.I
   else map X Me: X*Ys
list.`/` A = map X Me: X/A
list.`%` A = map X Me: X%A
list.float = map X Me: X.float
list.int = map X Me: X.int
list.round = Me{?round}

zip Xs Ys =
| Xs Xs.l
| Ys Ys.l
| Sz min Xs.n Ys.n
| dup I Sz: Xs.I,Ys.I

list.zip = dup I Me.0.n: map Xs Me Xs.I

text.`<` B =
| less B.is_text: bad "cant compare string `[Me]` with [B]"
| AS $n
| BS B.n
| if AS < BS
  then | times I AS:
         | AC $I.code
         | BC B.I.code
         | when AC <> BC: ret   AC < BC
       | 1
  else | times I BS:
         | AC $I.code
         | BC B.I.code
         | when AC <> BC: ret   AC < BC
       | 0

text.`*` N = @text: dup N: Me

text.is_upcase =
| times I $n:
  | C $I.code
  | when C < 'A'.code or 'Z'.code < C: ret   0
| 1

text.is_downcase =
| times I $n:
  | C $I.code
  | when C < 'a'.code or 'z'.code < C: ret   0
| 1

text.is_digit =
| times I $n:
  | C $I.code
  | when C < '0'.code or '9'.code < C: ret   0
| 1

text.is_alpha =
| times I $n:
  | C $I.code
  | when C < 'a'.code or 'z'.code < C:
    | when C < 'A'.code or 'Z'.code < C:
      | ret   0
| 1

//shorthands to make text matching easier
text.is_d = $is_digit
text.is_a = $is_alpha

text.has F = bad "text.has is obsolete"

/*
text.has F =
| if F.is_fn then
  | times I $n: when F($I): ret   1
  else
  | times I $n: when $I >< F: ret   1
| 0
*/

text.u = //upcase text
| Ys map Char $l
  | C Char.code
  | if C < 'a'.code or 'z'.code < C then Char
    else (C - 'a'.code + 'A'.code).char
| Ys.text

text.d = //downcase text
| Ys map Char $l
  | C Char.code
  | if C < 'A'.code or 'Z'.code < C then Char
    else (C - 'A'.code + 'a'.code).char
| Ys.text


text.title =
| less $n: ret   Me
| if $0.is_upcase then Me else "[$0.u][$tail]"

_.is_keyword = 0
text.is_keyword = not: $n and $0.is_upcase

text.trim s/' ' i/0 l/1 r/1 =
| Xs $l
| when L: 
  | It case Xs [$S@Zs] Zs
  | while It: Xs =  It
| when R
  | Xs Xs.f
  | It case Xs [$S@Zs] Zs
  | while It: Xs =  It
  | Xs =  Xs.f
| Xs.text

text.begin T =
  if T.is_text: ret $upto(T.n) >< T
  T.any: X => $upto(X.n) >< X

int.map F = dup I Me: F(I)

int.keep F = $l.keep(F)
int.skip F = $l.skip(F)

list.i = dup I $n: [I Me^pop]

text.i = $l.i

list.`.` K =
| times I K: Me =  $tail
| $head

list.del K = [@$take(K) @$drop(K+1)]
list.insert K V = [@$take(K) V @$drop(K)]
list.change K V = [@$take(K) V @$drop(K+1)]

text.del K = [@$take(K) @$drop(K+1)].text
text.insert K V = [@$take(K) V @$drop(K)].text
text.change K V = [@$take(K) V @$drop(K+1)].text

`..` X N = dup N X

list.n =
| S 0
| till $end
  | Me =  $tail
  | S+
| S

list.end = _eq $n 0

text.end = _eq $n 0


bytes.bytes = Me

list.bytes =
| N $n
| as Ys N.bytes: times I N: Ys.I =  pop Me

list.utf8 = $bytes.utf8

list.head = $0

list.tail = $l.tail

no.'*head' = No
list.'*head' = if $end: No else $head
text.'*head' = if $end: No else $head
list.`++` = if $end: No else $tail

list.`><` B =
| less B.is_list: ret   0
| till $end or B.end: less Me^pop >< B^pop: ret   0
| $end and B.end

hard_list.`><` B =
| less B.is_list: ret   0 //FIXME: cons_list B will be O(n^2) slow
| N $n
| when _ne N B.n: ret   0
| times I N: less $I >< B.I: ret   0
| 1

_list_.`><` B =
  less B.is_list: ret   0 //FIXME: cons_list B will be O(n^2) slow
  when _ne (_tag B) 9: B = B.l //convert B to _list_
  N $n
  when _ne N B.n: ret   0
  times I N: less (_lget Me I) >< (_lget B I): ret   0
  1

list.`<` Xs =
| A B 0
| times I $n:
  | A =  $I
  | B =  Xs.I
  | when A <> B: ret   A < B
| ret   A < B

list.`>` Xs =
| A B 0
| times I $n:
  | A =  $I
  | B =  Xs.I
  | when A <> B: ret   A > B
| ret   A > B

list.`<<` Xs =
| A B 0
| times I $n:
  | A =  $I
  | B =  Xs.I
  | when A <> B: ret   A << B
| ret   A << B

list.`>>` Xs =
| A B 0
| times I $n:
  | A =  $I
  | B =  Xs.I
  | when A <> B: ret   A >> B
| ret   A >> B

list.f =
| N $n
| Ys dup N
| while N
  | N-
  | Ys.N =  pop Me
| Ys

hard_list.f =
| N $n
| dup N
  | N-
  | $N

text.f = $l.f.text


text.cnt C = $l.cnt^C


list.map F = dup $n: F(Me^pop)
hard_list.map F = dup I $n: F($I)
_list_.map F = dup I $n: F(|_lget Me I)
text.map F = $l.map(F)

list.`^^` X = map Y Me Y^^X

list.fold Run F =
| for X Me: Run = F(Run X)
| Run

list.e F = till $end: F(Me^pop)
hard_list.e F = times I $n: F($I)

list.z =
| S 0
| till $end: S += pop Me
| S

hard_list.z =
| S 0
| times I $n: S += $I
| S

list.cnt F =
| C 0
| if F.is_fn
  then till $end: when F(Me^pop): C+
  else till $end: when F><Me^pop: C+
| C

hard_list.cnt F =
| C I 0
| if F.is_fn
  then times I $n: when F($I): C+
  else times I $n: when F><$I: C+
| C

list.keep F =
| Ys:
| if F.is_fn
  then for X Me: when F(X): Ys =  [X@Ys]
  else for X Me: when F >< X: Ys =  [X@Ys]
| Ys.f

list.skip F =
| Ys:
| if F.is_fn
  then for X Me: less F(X): Ys =  [X@Ys]
  else for X Me: less F >< X: Ys =  [X@Ys]
| Ys.f

list.j =
| Rs dup $map(?n).z
| I 0
| for Ys Me: for Y Ys: Rs.(I+) =  Y
| Rs


_list_.l = Me
_bytes_.l = dup I $n $I
text.l = dup I $n $I
list.l =
| N $n
| Ys dup N
| times I N: Ys.I =  pop Me
| Ys
int.l = dup I Me: I //iota operator

list.apply F = $l.apply(F)
list.apply_method F = $l.apply_method(F)

list.text @As =
| R $l
| if As.n then R.text(As.0) else R.text

text.text = Me

list.x @As = Me{as_text}.text(@As)

list.split S =
| F if S.is_fn then S else X => S >< X
| Ys:
| P $locate(F)
| while got P:
  | Ys =  [$take(P)@Ys]
  | Me =  $drop(P+1)
  | P =  $locate(F)
| [Me@Ys].f

text.split F = $l.split(F).map(X=>X.text)

text.all F = $l.all(F)
text.any F = $l.any(F)

text.rows = $split '\n'

text.get = get_file_ Me
text.getText = get_text_file_ Me
text.lines = $getText.rows
text.set Value =
| if Value.is_text then set_text_file_ Me Value else set_file_ Me Value.bytes
| 0
text.exists = file_exists_ Me
text.time = file_time_ Me
text.mkpath = mkpath_ Me


text.paths @As =
| Path if '/' >< $~ then Me else "[Me]/"
| Xs if As.n then $items(all) else $items
| map X Xs "[Path][X]"

text.urls = Me.paths{url}

text.folders = Me.items{url}.keep(?(:_ '' '')){?0.lead}

text.url =
| Name Ext ""
| Xs $l.f
| Sep Xs.locate(?><'/')
| Dot Xs.locate(?><'.')
| when got Dot and (no Sep or Dot < Sep):
  | Ext =  Xs.take(Dot).f.text
  | Xs =  Xs.drop(Dot+1)
  | when got Sep: Sep -= Dot+1
| Folder Name No
| if got Sep
  then | Folder =  "[Xs.drop(Sep+1).f.text]/"
       | Name =  Xs.take(Sep).f.text
  else | Folder =  ''
       | Name =  Xs.f.text
| [Folder Name Ext]

list.unurl =
| [Folder Name Ext] Me
| when Ext <> '': Ext =  ".[Ext]"
| "[Folder][Name][Ext]"

list.take N = dup N: Me^pop
hard_list.take N = dup I N $I

list.drop N =
| times I N Me^pop
| Me

hard_list.drop S = dup $n-S: Me.|S+

text.drop S = $l.drop(S).text
text.take S = $l.take(S).text
text.~ = $($n-1)
text.head = $0
text.tail = $drop(1)
text.lead = $take($n-1)

text.inc Val = Me.l("[Me]0":
                      @T '-' D<+d? = if T.end: "[T.text][-D.text.int+Val]"
                                     else "[T.text]-[D.text.int+Val]"
                      @T D<+d? = "[T.text][D.text.int+Val]")

text.`++` = $inc 1
text.`--` = $inc -1

list.~ = $($n-1)
list.suf X = [@Me X]
list.lead = $take($n-1)

// take up to N elements
list.upto N = $take(|max 0: min $n N)
text.upto N = $take(|max 0: min $n N)

text.`.!` I = if I < $n and I >> 0: Me.I

// drop up to N elements
list.wout N = $drop(|max 0: min $n N)
text.wout N = $drop(|max 0: min $n N)

list.cut P S = $drop(P).take(S)
text.cut P S = $drop(P).take(S)

list.slice B E S =
  Xs $l
  N Xs.n
  if B < 0:
    B = N+B
    if B < 0: B = 0
  else if B > N: B = N
  if E < 0:
    E = N+E
    if E < 0: E = 0
  else if E > N: E = N
  if B < E:
    N (E-B+S-1)/S
    B -= S
    dup N: Xs.|B += S
  else
    N (B-E+S-1)/S
    E += S*N
    dup N: Xs.|E -= S


text.slice B E S = $l.slice(B E S).text

int.bes E S = //implementation of [B:E:S]
  B Me
  if B<E:
    N (E-B+S)/S
    B -= S
    dup N: B += S
  else
    N (B-E+S)/S
    E += S*N
    dup N: E -= S

text.bes E S = [Me.code:E.code:S]{char}

text.keep Item = $l.keep(Item).text
text.skip Item = $l.skip(Item).text

list.takeIf F =
  R:
  while not $end and F(Me.0): push Me^pop R
  R

list.dropIf F =
  while not $end and F(Me.0): Me^pop
  Me

text.dropIf F = $l.dropIf(F).text
text.takeIf F = $l.takeIf(F).text

_.rmap F = F(Me)
list.rmap F = map X Me X.rmap(F)

list.infix Item = // intersperse from Haskell
| N $n*2-1
| if N < 0 then [] else dup I N: if I%2 then Item else Me^pop

list.locate F =
| less F.is_fn: F = (X => F >< X)
| for (I 0; not $end; I+): when F(Me^pop): ret   I

hard_list.locate F =
| if F.is_fn then times I $n: when F($I): ret   I
  else times I $n: when F >< $I: ret   I

text.locate F =
| if F.is_fn then times I $n: when F($I): ret   I
  else times I $n: when F >< $I: ret   I

list.find F =
| if F.is_fn
  then for (I 0; not $end; I+):
  | It Me^pop; when F(It): ret   It
  else for (I 0; not $end; I+):
  | It Me^pop
  | when F><It: ret   It

hard_list.find F =
| if F.is_fn
  then | times I $n:
         | It $I
         | when F(It): ret   It
  else | times I $n:
         | It $I
         | when F><It: ret   It

_list_.find F =
| if F.is_fn
  then | times I Me^_size:
         | It _lget Me I
         | when F(It): ret   It
  else | times I Me^_size:
         | It _lget Me I
         | when F><It: ret   It


list.group N =
| Y Ys []
| I 0
| till $end
  | push Me^pop Y
  | I+
  | when I >< N
    | push Y.f Ys
    | Y =  []
    | I =  0
| when Y.n: push Y.f Ys
| Ys.f

text.group N = $l.group(N){?text}

list.all F =
| if F.is_fn then for X Me: less F(X): ret   0
  else for X Me: less F >< X: ret   0
| 1

list.any F =
| if F.is_fn then for X Me: when F(X): ret   1
  else for X Me: when F >< X: ret   1
| 0

list.max =
| when $end: ret   No
| M $head
| for X Me: when X > M: M =  X
| M

list.min =
| when $end: ret   No
| M $head
| for X Me: when X < M: M =  X
| M

list.maxBy F =
| when $end: ret   No
| M $head
| FM F(M)
| for X Me:
  | FX F(X)
  | when FX > FM:
    | M =  X
    | FM =  FX
| M

list.minBy F =
| when $end: ret   No
| M $head
| FM F(M)
| for X Me:
  | FX F(X)
  | when FX < FM:
    | M =  X
    | FM =  FX
| M

//list.has Item = got $find(Item)
list.has Item = bad "list.has is obsolete" 

HexChars '0123456789ABCDEF'

int.x =
| less Me: ret   '0'
| Cs:
| S ''
| when Me < 0
  | S =  '-'
  | Me =  -Me
| while Me > 0
  | Cs =  [HexChars.(Me%16) @Cs]
  | Me /= 16
| [S@Cs].text

_.as_text = "#([Me^typename] [Me^gid_.x])"

no.as_text = 'No'

int.as_text =
| less Me: ret   '0'
| Cs:
| S ''
| when Me < 0
  | S =  '-'
  | Me =  -Me
| while Me > 0
  | Cs =  [HexChars.(Me%10) @Cs]
  | Me /= 10
| [S@Cs].text

plain_char C =
| N C.code
| if   ('a'.code << N and N << 'z'.code)
    or ('A'.code << N and N << 'Z'.code)
    or ('0'.code << N and N << '9'.code)
    or '_'.code >< N
  then 1
  else 0

text.as_text =
| less $n: ret '``'
| case Me 'if'+'then'+'else'+'or'+'and': ret "`[Me]`"
| Cs:
| Q $0.is_digit
| for C Me
  | less plain_char C: Q =  1
  | when '`' >< C: C =  '\\`'
  | when '\\' >< C: C =  '\\\\'
  | push C Cs
| if Q then ['`' @['`' @Cs].f].text else Me

list.as_text = "([(map X Me X.as_text).text(' ')])"

_.textify_ = $as_text
text.textify_ = Me

say @As = say_ "[As{"[?]"}.text(' ')]\n"

bad @Text = rterr_ "[Text.text ' ']"

tbl.__ Method Args =
| if _gt Args.n 1
  then Args.0.(Method^_method_name.tail) =  Args.1 // strip `assign indicator`
  else Me.(Method^_method_name)
tbl.map F = $l.map(F)
tbl.as_text = "@{[$l{"[?0.as_text]![?1.as_text]"}.text(' ')]}"
tbl.copy = $l.t
tbl.deep_copy = $l.t
tbl.s @As = $l.s @As

list.t =
| T!
| for [K V] Me: T.K =  V
| T

list.bag = Me{?,1}.t

list.uniq =
| Seen!
| $skip(X => got Seen.X or (Seen.X = 1) and 0)

text.pad Count Item =
  X "[Item]"
  when X.n > 1: bad "pad item: [X]"
  N  +Count - $n
  when N < 0: bad "text is larger than [Count.abs]: '[Me]'"
  Pad  @text: dup N X
  if Count < 0: "[Pad][Me]" else "[Me][Pad]"

list.pad Count Item =
  N  +Count - $n
  when N < 0: bad "list is larger than [Count.abs]: '[Me]'"
  Pad dup N Item
  if Count < 0: [@Pad @Me] else [@Me @Pad]


int.digits @Base =
  B if Base.end: 10 else Base.head
  Ys:
  while Me > 0:
    push Me%B Ys
    Me /= B
  Ys.l

list.digits @Base =
  B if Base.end: 10 else Base.head
  R 0
  for X Me:
    R *= B
    R += X
  R

type macro @new_macro N E: name!N expander!E

// `meta A B` -- attach a meta value B (typically a source-position
// triple) to A.  For heap-allocated A we stash (A -> B) in the
// runtime's weak hashtable via `meta_attach_` and return A
// unchanged; consumers still read it back via A.meta_ (which goes
// through `meta_lookup_`).  This means AST nodes coming out of the
// parser are plain lists -- no wrapper struct intercepting method
// dispatch -- which is what unblocks `text.parse` from switching
// to `parse_strip_c_` (see docs/reader-consolidation.md).
//
// For immediates (int, fixtext, No, T_TAG, ...) we fall back to
// the legacy `meta_wrapper` type: immediates have no GC identity
// to key on in the weak table.  The parser doesn't attach meta to
// immediates today, but other consumers (e.g. ui_old's widget
// rects) might.
//
//~ means meta_wrapper doesn't inherit from _, so all methods route
//  through __ -- preserving the "forwarding wrapper" semantics for
//  the immediate case.
// `meta A B` -- attach a meta value B (typically a source-position
// triple) to A.  For heap-allocated A we stash (A -> B) in the
// runtime's weak hashtable via `meta_attach_` and return A
// unchanged; consumers still read it back via A.meta_ (which goes
// through `meta_lookup_`).  This means AST nodes coming out of the
// parser are plain lists -- no wrapper struct intercepting method
// dispatch -- which is what unblocks `text.parse` from switching
// to `parse_strip_c_` (see docs/reader-consolidation.md).
//
// For immediates (int, fixtext, No, T_TAG, ...) we fall back to
// the legacy `meta_wrapper` type: immediates have no GC identity
// to key on in the weak table.  The parser doesn't attach meta to
// immediates today, but other consumers (e.g. ui_old's widget
// rects) might.
//
//~ means meta_wrapper doesn't inherit from _, so all methods route
//  through __ -- preserving the "forwarding wrapper" semantics for
//  the immediate case.
// `meta A B` -- attach a meta value B (typically a source-position
// triple) to A.  For heap-allocated A we stash (A -> B) in the
// runtime's weak hashtable via `meta_attach_` and return A
// unchanged; consumers read it back via `A.meta_` which goes
// through `meta_lookup_`.  AST nodes coming out of the parser stay
// as plain lists -- no wrapper struct intercepting method dispatch
// -- which is what unblocks `text.parse` from switching to
// `parse_strip_c_` (see docs/reader-consolidation.md).
//
// For immediates (int, fixtext, No, T_TAG, ...) we fall back to
// the legacy `meta_wrapper` type: immediates have no GC identity
// so they can't be keyed in a weak table.  The parser doesn't
// attach meta to immediates today, but other consumers (e.g.
// ui_old widgets) might.
//
//~ means meta_wrapper doesn't inherit from _; non-field method
//  calls go through __ which forwards to the wrapped object.
type meta_wrapper.~ O M: object_!O imm_meta_!M
meta_wrapper.__ Method Args =
  Args.0 = $object_
  Args.apply_method(Method)
meta_wrapper.is_meta = 1
meta_wrapper.meta_ = | $imm_meta_

_.meta_ = | meta_lookup_ Me
_.is_meta = | meta_present_ Me

meta A B =
| if imm_ A then meta_wrapper A B else (meta_attach_ A B; A)


LCG_Seed  No
LCG_M     2147483647
LCG_M_F   LCG_M.float
LCG_A     16807
LCG_B     0

lcg_init Seed =
| LCG_Seed =  Seed
| 10.rand
| No

int.rand =
| LCG_Seed =  (LCG_Seed*LCG_A + LCG_B) % LCG_M
| @int: @round: LCG_Seed.float*$float/LCG_M_F

rand_get = LCG_Seed
rand_set V = LCG_Seed =  V

LCG_Stack    dup 32 0
LCG_StackTop 0

rand_push NewSeed =
| LCG_Stack.LCG_StackTop =  LCG_Seed
| LCG_StackTop += 1
| LCG_Seed =  NewSeed

rand_pop =
| R LCG_Seed
| LCG_StackTop -= 1
| LCG_Seed =  LCG_Stack.LCG_StackTop
| R

float.rand =
| LCG_Seed =  (LCG_Seed*LCG_A + LCG_B) % LCG_M
| LCG_Seed.float/LCG_M_F*Me

int.randr B =
  A Me
  B = B.int
  if B < A: swap A B
  (B-A).rand + A

float.randr B =
  A Me
  B = B.float
  if B < A: swap A B
  (B-A).rand + A

int.chnc = 100.rand << Me

float.chnc = 100.0.rand << Me

list.rand = $(@rand $n-1)

list.outcome = $find(s~+=?0; p~^(Me{?0}.z*)<<s~).1


GGensymCount 0
text.rand = "[Me]__[GGensymCount+]"

lcg_init: time

list.shuffle =
  Xs $copy
  N Xs.n
  while N > 1:
    N-
    R N.rand
    X Xs.R
    Xs.R =  Xs.N
    Xs.N =  X
  Xs


IValue  0
IParent 1
ILeft   2
IRight  3

merge H1 H2 =
| less H1: ret   H2
| less H2: ret   H1
| when H2.IValue < H1.IValue:
  | T H1
  | H1 =  H2
  | H2 =  T
| if 1.rand
  then | H1.ILeft =  merge H1.ILeft H2
       | when H1.ILeft: H1.ILeft.IParent =  H1
  else | H1.IRight =  merge H1.IRight H2
       | when H1.IRight: H1.IRight.IParent =  H1
| H1

sort_asc Xs =
| Root 0
| for X Xs
  | Root =  merge [X 0 0 0] Root
  | Root.IParent =  0
| dup Xs.n
  | V Root.IValue
  | Root =  merge Root.ILeft Root.IRight
  | V

list.s @As = 
| F No
| case As
  [A] | F =  A
  [] | ret  : sort_asc Me
  Else | bad "list.s: invalid number of arguments"
| merge H1 H2 =
  | less H1: ret   H2
  | less H2: ret   H1
  | when F(H2.IValue H1.IValue):
    | T H1
    | H1 =  H2
    | H2 =  T
  | if 1.rand
    then | H1.ILeft =  merge H1.ILeft H2
         | when H1.ILeft: H1.ILeft.IParent =  H1
    else | H1.IRight =  merge H1.IRight H2
         | when H1.IRight: H1.IRight.IParent =  H1
  | H1
| Root 0
| for X Me
  | Root =  merge [X 0 0 0] Root
  | Root.IParent =  0
| dup $n
  | V Root.IValue
  | Root =  merge Root.ILeft Root.IRight
  | V
  

list.sortBy F =
  Xs if F.is_int: Me{X=X.F,X} else Me{X=F(X),X}
  Xs.s(?0 < ??0){1}



//= parse text as integer; an optional argument provides Radix
text.int @Radix =
| Rdx 10
| when Radix.n: Rdx =  Radix.0
| T Me.u
| N $n
| R I 0
| Sign if $I >< '-'
       then | I+
            | -1
       else 1
| Base '0'.code
| AlphaBase 'A'.code - 10
| for (; I < N; I+)
  | C T.I.code
  | V if '0'.code << C and C << '9'.code then C - Base else C - AlphaBase
  | R =  R*Rdx + V
| R*Sign

//tries parsing as int, or returns Default
text.try_int Default = if $n>0 and $all(?is_digit): $int else Default


list.u4  = $3*0x1000000 + $2*0x10000 + $1*0x100 + $0
list.u4b = $0*0x1000000 + $1*0x10000 + $2*0x100 + $3
list.s4  = as R $u4: when R-*-0x80000000: R-=0x100000000
list.s4b = as R $u4b: when R-*-0x80000000: R-=0x100000000

list.u2 = $1*0x100 + $0
list.u2b = $0*0x100 + $1
list.s2 = as R $u2: when R-*-0x8000: R-=0x10000
list.s2b = as R $u2b: when R-*-0x8000: R-=0x10000

int.u4 = [Me%256 Me/0x100%256 Me/0x10000%256 Me/0x1000000%256]
int.u4b = [Me/0x1000000%256 Me/0x10000%256 Me/0x100%256 Me%256]
int.s4 =
| when Me < 0: Me += 0x100000000
| [Me%256 Me/0x100%256 Me/0x10000%256 Me/0x1000000%256]
int.s4b =
| when Me < 0: Me += 0x100000000
| [Me/0x1000000%256 Me/0x10000%256 Me/0x100%256 Me%256]

int.u2 = [Me%256 Me/0x100%256]
int.u2b = [Me/0x100%256 Me%256]
int.s2 = 
| when Me < 0: Me += 0x10000
| [Me%256 Me/0x100%256]
int.s2b =
| when Me < 0: Me += 0x10000
| [Me/0x100%256 Me%256]

int.bit N = _and 1: _shr Me N
int.bitSet N Bit = _xor Me (_xor (_shl Bit N) (_and Me (_shl 1 N)))

int.invert = 0xFFFFFFFFFFFFFFF -^- Me

int.bitSetM Mask V = if V:
                       if Me -*- Mask: Me
                       else Me -+- Mask
                     else
                       if Me -*- Mask: Me -*- Mask.invert
                       else Me

int.in Start End = Start << Me and Me < End
list.in [RX RY RW RH] = $0.in(RX RX+RW) and $1.in(RY RY+RH)

int.clip A B = if Me < A then A
               else if Me > B then B
               else Me

float.clip A B = if Me < A then A
                 else if Me > B then B
                 else Me

atan2 X Y =
| PI 3.141592653589793 //FIXME: use a macro
| R if X > 0.0 then @atan Y/X
    else if Y > 0.0 then
      if X < 0.0 then PI + (Y/X).atan
      else PI/2.0
    else if Y < 0.0 then
      if X < 0.0 then -PI + (Y/X).atan
      else PI*6.0/4.0
    else if X < 0.0 then PI
    else 0.0
| when R < 0.0: R += 2.0*PI
| R


list.<= Src = times I $n: $I = Src.I

_list_.<= Src = times I Me^_size: _lset Me I Src.I

list.div F =
  No //hack!!! otherwise R! wont be processed correctly
  R!
  for X Me:
    K F(X)
    Xs R.K
    R.K = if got Xs: [X@Xs] else [X]
  R{[?0 ?1.f]}.t

list.rip F = //rip list in two parts
| As $keep(F)
| Bs $skip(F)
| Bs,As

list.hash =
| H 0
| for X Me: H =  (H -<- 1) -^- X.hash
| if H<0 then -H else H

list.clear Value = times I $n: Me.I =  Value

type iter base p
iter.end = $base.n><$p
iter.`++` = $base.($p+)
iter.`--` = $base.($p-)
iter.head = $base.$p
iter.`.` N = $base.($p+N)
iter.`=` N V = $base.($p+N) =  V
iter.`+` N = iter $base $p+N

list.iter = iter $l 0

SetterCache!
GetterCache!

setters_ O =
| T typename O
| Ss SetterCache.T
| when no Ss:
  | Ss =  O^methods_.keep(?0.0 >< '='){[?0.tail ?1]}.t
  | SetterCache.T =  Ss
| Ss

getters_ O =
| T typename O
| Gs GetterCache.T
| when no Gs:
  | Setters setters_ O
  | Gs =  O^methods_.keep(K,V => got Setters.K).t
  | GetterCache.T =  Gs
| Gs

_.getter = getters_ Me

_.setter = setters_ Me


//backtracking landing place
//very fragile, and will crash and call any function other than F
btland F =
| K _setjmp
| if _lget K 0: F(| _lget K 1) else _lget K 1

//jumps to previously created backtrack landing
btjump Land Value = _longjmp Land Value

type bterror type text

btrap F = btland: K =>
| PrevDEH get_deh
| set_deh: Type Msg =>
  | set_deh PrevDEH
  | btjump K: bterror reader_error Msg
| R F()
| set_deh PrevDEH
| R
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
    Then = parse_offside 'if' 0 T.src.0 OCol
  else
    less is_next `:`: parser_error "missing `:` for" Sym
    T expect `:` Sym
    Then = parse_offside 'if' 0 T.src.0 OCol
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
    | when Type >< 'if' and Col >< SCol
           and (V >< `then` or V >< `else` or V >< `elif`)
           and (Ys.end or not Ys.~.is_token or Ys.~.value <> `if`): done
    | when Col >< SCol and not Ys.end
           and V <> `|`  and V <> `else` and V <> `elif` and V <> `then`:
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
| R parse_strip_c_: parse_tokens_c_: add_bars_c_: Text.tokenize(Src)
| if R.end then [[]] else R.0.tail

// `text.sexp`: a simpler entry point than `text.parse` that
// skips the `add_bars_c_` toplevel-wrapping pass.  Used by
// places that want to parse one expression (or a small list
// of them) without the `|`-block structure -- e.g. the UIM
// event-stream and the show-wh tag in uim.s.  Returns the
// first expression by default; pass `list!1` to get the full
// list of parsed expressions.
text.sexp Src!'<none>' LexP!No List!0 =
| Text Me
| when got LexP: Text = lexical_macro_expand Src Text LexP
| R parse_strip_c_: parse_tokens_c_: Text.tokenize(Src)
| if List: ret R
| if R.end: No else R.0

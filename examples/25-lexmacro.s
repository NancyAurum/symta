// 25-lexmacro.s -- the lexical macro processor (NCM)
//
// Symta has TWO macro systems. The Lisp-style one (example 12)
// runs after parsing and rewrites AST -- it sees structured forms
// and produces structured forms.
//
// NCM ("New C Macroprocessor", `runtime/ncm.h`) runs BEFORE parsing
// and rewrites raw text. It's a more capable cousin of the C
// preprocessor and is what powers `#:include` directives,
// per-module `.h` constant headers, and the `text.sexp` lexical-
// macro hook.
//
// User code reaches it via the `ncm_process_ Filename Text Paths`
// builtin. NCM's macro character is `#`.
//
// Run:  symta -f examples/25-lexmacro.s


// ----------------------------------------------------------------
// 1. Plain constants. `#NAME body` defines a macro; mentioning
//    NAME later substitutes its body.
// ----------------------------------------------------------------
Src1 '
#WIDTH  640
#HEIGHT 480

The window is WIDTH x HEIGHT pixels.
That is WIDTH*HEIGHT total.
'
say '--- plain constants ---'
say (ncm_process_ '<demo>' Src1 [])


// ----------------------------------------------------------------
// 2. Compile-time arithmetic. `#[expr]` evaluates an integer
//    expression at expansion time.
//    Operators: + - * / %, unary - + ~ !, & | ^,
//    && ||, == != < > <= >=, parentheses, hex literals,
//    plus the builtin `log2(x)`.
// ----------------------------------------------------------------
Src2 '
#A 12
#B 30

#[A * B + 4]      = compile-time arithmetic
#[A & 0xF]        = bitwise AND/OR/XOR
#[1 + 2*3]        = precedence (* binds tighter than +)
#[log2(1024)]     = log2 builtin
#[A > B]          = comparison (1 if true, 0 if false)
'
say ""
say '--- compile-time #[expr] ---'
say (ncm_process_ '<demo>' Src2 [])


// ----------------------------------------------------------------
// 3. Function-style macros: `#name(arg1,arg2,...) body`.
//    Arguments are comma-separated. Bodies may reference other
//    macros, which expand recursively.
// ----------------------------------------------------------------
Src3 '
#sq(X) ((X)*(X))

#area(W,H) sq(W) + sq(H)

a := sq(7)
b := area(3,4)
c := area(2+1, 4)            // inner expressions OK if you parenthesize
'
say ""
say '--- fn-style macros ---'
say (ncm_process_ '<demo>' Src3 [])


// ----------------------------------------------------------------
// 4. Optional arguments with defaults. Trailing args after an
//    `=Default` may be omitted at the call site.
// ----------------------------------------------------------------
Src4 '
#greet(Name=World) hello, Name!
#log(Msg, Level=info) [Level] Msg

a := greet()
b := greet(Symta)
c := log("started")
d := log("boom", error)
'
say ""
say '--- optional args ---'
say (ncm_process_ '<demo>' Src4 [])


// ----------------------------------------------------------------
// 5. Variadic arguments. `[Xs]` captures everything after
//    previous formal as a single text blob.
// ----------------------------------------------------------------
Src5 '
#all([Xs]) [Xs]
#cmd(Name, [Args]) Name Args

a := all(1, 2, 3, 4)
b := cmd(printf, "value=%d\\n", v)
'
say ""
say '--- variadic ---'
say (ncm_process_ '<demo>' Src5 [])


// ----------------------------------------------------------------
// 6. Body arguments. A `{Body}` formal binds a `{ ... }` block
//    that follows the call. Lets you write your own control
//    structures that take an inline statement block.
//    (NCM emits #line directives the C preprocessor recognises;
//    they're noise in this demo.)
// ----------------------------------------------------------------
Src6 '
#unless(Cnd, {Body}) if (!(Cnd)) { Body }

unless(ready) { wait(); retry(); }
'
say ""
say '--- body args ---'
say (ncm_process_ '<demo>' Src6 [])


// ----------------------------------------------------------------
// 7. Conditional inclusion. `#if` / `#elif` / `#else` / `#endif`
//    work like C, including the conditions on `#expr` evaluation.
//    `#undef Name` removes a macro.
// ----------------------------------------------------------------
Src7 '
#DEBUG 1
#OPT_LEVEL 2

#if DEBUG
slow_path_with_assertions();
#elif OPT_LEVEL == 3
fast_path();
#else
default_path();
#endif

#undef DEBUG
#if DEBUG
should not appear
#else
debug now disabled
#endif
'
say ""
say '--- conditionals ---'
say (ncm_process_ '<demo>' Src7 [])


// ----------------------------------------------------------------
// 8. Inline shell-style format with `#("fmt" args...)`. Useful
//    for code generation -- emit the right hex constant,
//    zero-padded id, etc, at compile time.
// ----------------------------------------------------------------
// Inside `#("fmt" args...)` use `#[expr]` to inject a number,
// `#<NAME>` to inject a macro body verbatim. Plain text args
// (like `foo`) come through unchanged.
Src8 '
#R 255
#G 160
#B 64

color = #("0x%02X%02X%02X" #[R] #[G] #[B]);
id    = #("ent_%05d" 42);
left  = #("%-10s" foo);
'
say ""
say '--- format with #(...) ---'
say (ncm_process_ '<demo>' Src8 [])


// ----------------------------------------------------------------
// 9. Escaping `#`. `##` emits a literal `#` (not a macro).
//    Inside a quoted string, `#` is also untouched.
// ----------------------------------------------------------------
Src9 '
#X 42
1) X is here -> X
2) ## stays literal
3) "X is also literal inside quotes"
'
say ""
say '--- escaping ---'
say (ncm_process_ '<demo>' Src9 [])


// ----------------------------------------------------------------
// 10. NCM is what processes Symta module headers (`.h` files)
//     when you do `use rgb` and `rgb.h` provides constants like
//     `#RGB_BLACK 0x000000`. The same engine, exposed as a builtin
//     so any text-template task you have -- code generation,
//     templated SQL, build configs -- can ride on it.
// ----------------------------------------------------------------
Src10 '
#:rgb           // pulls src/rgb.h (defines RGB_BLACK, RGB_WHITE, ...)

bg = RGB_BLACK;
fg = RGB_WHITE;
'
say ""
say '--- file include (uses src/rgb.h) ---'
// the third arg to ncm_process_ is the include search path
SearchPaths ['src/']
say (ncm_process_ '<demo>' Src10 SearchPaths)


// ----------------------------------------------------------------
// 11. NCM regression probes -- the three operators below all had
//     latent off-by-one parser bugs that produced silently-wrong
//     values.  Pinned here so a future ncm.h refactor can't
//     re-break them.
// ----------------------------------------------------------------
//
// NCM-1: `<<` and `>>` shifts.  Used to drop the second `<` /
//        `>`, parse the operand from the dropped char, fail, and
//        return 0 -- producing `A<<2` = A unchanged or 0
//        depending on follow-on parser state.
//
// NCM-2: `&&` and `||` logic.  Used to be eaten as bitwise `&`/`|`
//        by `eval_bitwise`, so `#[1 && 1]` returned 0.
//
// NCM-3: `cmd_fmt` numeric args.  `#("%d" 0xFF)` used to read
//        only the leading `0` because the decimal parser stopped
//        at `x`.  Now `0x` / `0X` triggers base-16 parsing.
Src11 '
#A 5
#B 3

shifts:   #[A<<2]   #[A<<B]   #[100>>2]
logic:    #[1 && 1]   #[1 && 0]   #[0 || 1]   #[0 || 0]
chained:  #[1 && 1 && 1]   #[0 && 1]
bitwise:  #[3 & 5]   #[3 | 5]   #[3 ^ 5]
hex-X:    #("0x%08X" 0xFEEDFACE)
hex-x:    #("0x%08x" 0xFEEDFACE)
hex-dec:  #("%d" 0xFF)   #("%d" -0x10)
'
say ""
say '--- NCM-1/2/3 regression probes ---'
say (ncm_process_ '<demo>' Src11 [])

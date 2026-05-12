// reader.c -- C port of symta/src/reader.s.
//
// Token-list -> AST parser. Tokens come from tokenize.c (already C),
// the AST shape matches what compiler.s / macro.s consume.
//
// Migration strategy (see issues.md B8):
//   * Each public symbol of reader.s is mirrored by a C entry
//     declared in reader.h and exposed as a builtin from bltin.c.
//   * reader.s is patched to dispatch through those builtins so the
//     `text.parse` entrypoint keeps working.
//
// Token layout (T_TOK objects, 7 slots):
//   0 type   1 value   2 pchar   3 row   4 col   5 orig   6 parsed
//
// All operations run with GC disabled so we can hold raw `dyn`
// pointers across allocations without worrying about relocation.

#include "common.h"
#include "ng.h"
#include "reader.h"

// ====================================================================
// Token introspection.
// ====================================================================

static inline int is_tok(dyn o)     { return O_TAG(o) == T_TOK; }
static inline int is_list(dyn o)    { return O_TAG(o) == T_LIST; }

static inline dyn tok_type(dyn o)   { return LGET(o, 0); }
static inline dyn tok_value(dyn o)  { return LGET(o, 1); }
static inline dyn tok_pchar(dyn o)  { return LGET(o, 2); }
static inline dyn tok_row(dyn o)    { return LGET(o, 3); }
static inline dyn tok_col(dyn o)    { return LGET(o, 4); }
static inline dyn tok_orig(dyn o)   { return LGET(o, 5); }
static inline dyn tok_parsed(dyn o) { return LGET(o, 6); }

static inline int text_eq_c(dyn a, dyn b) {
  if (a == b) return 1;
  if (IS_BIGTEXT(a) && IS_BIGTEXT(b)) return texts_equal(a, b);
  return 0;
}

static inline int token_is(dyn o, dyn what) {
  if (!is_tok(o)) return 0;
  return text_eq_c(tok_type(o), what);
}

// ====================================================================
// Interned keyword bag.
// Filled lazily on first use; everything in here is a fixtext or bigtext
// so `text_eq_c` is fast.
// ====================================================================

#define KW_LIST \
  X(pipe,      "|"   ) X(then,    "then" ) X(else_,   "else" ) \
  X(elif,      "elif") X(if_,     "if"   ) X(symbol,  "symbol") \
  X(escape,    "escape") X(text,   "text"  ) X(splice, "splice") \
  X(int_,      "int" ) X(hex,     "hex"  ) X(bin,     "bin"  ) \
  X(void_,     "void") X(parens,  "()"   ) X(brackets,"[]"   ) \
  X(curly,     "{}"  ) X(curlyL,  "{"    ) X(table,   "@{}"  ) \
  X(object,    "${}" ) X(unary,   "unary") X(bracketsL,"["  ) \
  X(dot,       "."   ) X(caret,   "^"    ) X(arrow,   "->"  ) \
  X(eq,        "="   ) X(le,      "<="   ) X(plus,    "+"   ) \
  X(minus,     "-"   ) X(star,    "*"    ) X(slash,   "/"   ) \
  X(pct,       "%"   ) X(pow,     "^^"   ) X(range,   ".."  ) \
  X(shl,       "-<-" ) X(shr,     "->-"  ) X(band,    "-*-" ) \
  X(bor,       "-+-" ) X(bxor,    "-^-"  ) X(comma,   ","   ) \
  X(eqeq,      "><"  ) X(neq,     "<>"   ) X(lt,      "<"   ) \
  X(gt,        ">"   ) X(lte,     "<<"   ) X(gte,     ">>"  ) \
  X(and_,      "&&"  ) X(or_,     "||"   ) X(bang,    "!"   ) \
  X(arrow_eq,  "=>"  ) X(at,      "@"    ) X(amp,     "&"   ) \
  X(atat,      "@@"  ) X(plus_eq, "+="   ) X(minus_eq,"-="  ) \
  X(star_eq,   "*="  ) X(slash_eq,"/="   ) X(pct_eq,  "%="  ) \
  X(plusplus,  "++"  ) X(minusminus,"--" ) X(quotebs, "\\"  ) \
  X(dollar,    "$"   ) X(semi,    ";"    ) X(colon,   ":"   ) \
  X(at_curly,  "@{"  ) X(amp_amp, "^^"   )                    \
  X(none,      "<none>")

#define X(slot,_lit) static dyn KW_##slot;
KW_LIST
#undef X

static int kw_initted = 0;
static void kw_init(void) {
  if (kw_initted) return;
#define X(slot,lit) TEXT(KW_##slot, lit);
  KW_LIST
#undef X
  kw_initted = 1;
}

// Membership tests via switch -- compact and avoids hash overhead.

// reader.s DelimList: [`:` `=` `<=` `=>` `+=` `-=` `*=` `/=` `%=`]
static int is_delim_op(dyn t) {
  return text_eq_c(t, KW_colon) || text_eq_c(t, KW_eq) ||
         text_eq_c(t, KW_le)    || text_eq_c(t, KW_arrow_eq) ||
         text_eq_c(t, KW_plus_eq) || text_eq_c(t, KW_minus_eq) ||
         text_eq_c(t, KW_star_eq) || text_eq_c(t, KW_slash_eq) ||
         text_eq_c(t, KW_pct_eq);
}

// reader.s DelimOps (parse_delim "is_delim" filter):
// `:` `=` `<=` `=>` `if` `then` `else` `elif` ` =` `-=` `*=` `/=` `%=`
// (note: there's a `+=` typo in original Symta as ` =` -- preserve)
static int is_delim_filter(dyn t) {
  if (!is_tok(t)) return 0;
  dyn ty = tok_type(t);
  return text_eq_c(ty, KW_colon) || text_eq_c(ty, KW_eq) ||
         text_eq_c(ty, KW_le)    || text_eq_c(ty, KW_arrow_eq) ||
         text_eq_c(ty, KW_if_)   || text_eq_c(ty, KW_then) ||
         text_eq_c(ty, KW_else_) || text_eq_c(ty, KW_elif) ||
         text_eq_c(ty, KW_plus_eq) || text_eq_c(ty, KW_minus_eq) ||
         text_eq_c(ty, KW_star_eq) || text_eq_c(ty, KW_slash_eq) ||
         text_eq_c(ty, KW_pct_eq);
}

// reader.s OpsT.
static int is_opt(dyn t) {
  return text_eq_c(t, KW_plus) || text_eq_c(t, KW_minus) ||
         text_eq_c(t, KW_star) || text_eq_c(t, KW_slash) ||
         text_eq_c(t, KW_pct)  || text_eq_c(t, KW_caret) ||
         text_eq_c(t, KW_dot)  || text_eq_c(t, KW_arrow) ||
         text_eq_c(t, KW_pipe) || text_eq_c(t, KW_semi)  ||
         text_eq_c(t, KW_comma)|| text_eq_c(t, KW_colon) ||
         text_eq_c(t, KW_eq)   || text_eq_c(t, KW_arrow_eq) ||
         text_eq_c(t, KW_le)   || text_eq_c(t, KW_plus_eq) ||
         text_eq_c(t, KW_minus_eq) || text_eq_c(t, KW_star_eq) ||
         text_eq_c(t, KW_slash_eq) || text_eq_c(t, KW_pct_eq) ||
         text_eq_c(t, KW_bang) || text_eq_c(t, KW_bor) ||
         text_eq_c(t, KW_bxor) || text_eq_c(t, KW_band) ||
         text_eq_c(t, KW_shl)  || text_eq_c(t, KW_shr) ||
         text_eq_c(t, KW_amp_amp) || text_eq_c(t, KW_range) ||
         text_eq_c(t, KW_plusplus) || text_eq_c(t, KW_minusminus) ||
         text_eq_c(t, KW_eqeq) || text_eq_c(t, KW_neq) ||
         text_eq_c(t, KW_lt)   || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte)  || text_eq_c(t, KW_gte) ||
         text_eq_c(t, KW_quotebs) || text_eq_c(t, KW_dollar) ||
         text_eq_c(t, KW_at) || text_eq_c(t, KW_amp);
}

// reader.s UnaryOps:
//   [`+` `-` `*` `/` `%` `++` `--` `<` `>` `<<` `>>` `<>` `><` `-+` `-*`]
// (note: these are the OPERATOR types -- the token types, not values --
// that can appear in unary position. tok.type==`unary` already covers
// the prefix case from tokenize.c.)
static int is_unary_op_type(dyn t) {
  return text_eq_c(t, KW_plus) || text_eq_c(t, KW_minus) ||
         text_eq_c(t, KW_star) || text_eq_c(t, KW_slash) ||
         text_eq_c(t, KW_pct)  || text_eq_c(t, KW_plusplus) ||
         text_eq_c(t, KW_minusminus) ||
         text_eq_c(t, KW_lt)   || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte)  || text_eq_c(t, KW_gte) ||
         text_eq_c(t, KW_neq)  || text_eq_c(t, KW_eqeq);
  // `-+` `-*` aren't in our kw list but unused too.
}

static int is_suf_unary(dyn t) {
  // SufUnaryL: '{}' '()' '[]' '[' '{' '+' '-' '*' '/' '%' '<' '>' '<<' '>>'
  return text_eq_c(t, KW_curly) || text_eq_c(t, KW_parens) ||
         text_eq_c(t, KW_brackets) || text_eq_c(t, KW_bracketsL) ||
         text_eq_c(t, KW_curlyL) || text_eq_c(t, KW_plus) ||
         text_eq_c(t, KW_minus) || text_eq_c(t, KW_star) ||
         text_eq_c(t, KW_slash) || text_eq_c(t, KW_pct) ||
         text_eq_c(t, KW_lt) || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte) || text_eq_c(t, KW_gte);
}

static int is_suf_unary_s(dyn t) {
  return text_eq_c(t, KW_plus) || text_eq_c(t, KW_minus) ||
         text_eq_c(t, KW_star) || text_eq_c(t, KW_slash) ||
         text_eq_c(t, KW_pct)  ||
         text_eq_c(t, KW_lt) || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte) || text_eq_c(t, KW_gte);
}

static int is_suf_binary(dyn t) {
  return text_eq_c(t, KW_dot) || text_eq_c(t, KW_caret) ||
         text_eq_c(t, KW_arrow);
}

static int is_suf_op(dyn t) {
  return is_suf_binary(t) || is_suf_unary(t);
}

static int is_dollar_op(dyn t) {
  // DollarList: [`$` unary `\\` `@` `&` `@@`]
  return text_eq_c(t, KW_dollar) || text_eq_c(t, KW_unary) ||
         text_eq_c(t, KW_quotebs) || text_eq_c(t, KW_at) ||
         text_eq_c(t, KW_amp) || text_eq_c(t, KW_atat);
}

static int is_prefix_op(dyn t) {
  // PrefixList: [unary `\\` `@` `&` `@@`]
  return text_eq_c(t, KW_unary) || text_eq_c(t, KW_quotebs) ||
         text_eq_c(t, KW_at) || text_eq_c(t, KW_amp) ||
         text_eq_c(t, KW_atat);
}

static int is_bool_op(dyn t) {
  // BoolList: [`><` `<>` `<` `>` `<<` `>>`]
  return text_eq_c(t, KW_eqeq) || text_eq_c(t, KW_neq) ||
         text_eq_c(t, KW_lt) || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte) || text_eq_c(t, KW_gte);
}

// DisjointChars: [',' ':' '<' '>' '<<' '>>']
static int is_disjoint_char(dyn t) {
  return text_eq_c(t, KW_comma) || text_eq_c(t, KW_colon) ||
         text_eq_c(t, KW_lt) || text_eq_c(t, KW_gt) ||
         text_eq_c(t, KW_lte) || text_eq_c(t, KW_gte);
}

// ParenChars: ['_' '`' '?' '~' `)` `}` `]` '"' "'"]
static int is_paren_char(char c) {
  return c=='_' || c=='`' || c=='?' || c=='~' || c==')' ||
         c=='}' || c==']' || c=='"' || c=='\'';
}

static int is_anchor_char_c(dyn pchar) {
  // pchar is a 1-char text (or No/0). text.tokenize emits via code2text.
  if (!pchar || pchar == No) return 0;
  char buf[8] = {0};
  decode_text(buf, pchar);
  char c = buf[0];
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || is_paren_char(c);
}

// ====================================================================
// Parser state.
//
// `gi` is the input list; we read forwards from gi[gi_pos]. To "push
// back" a token onto the input, we put it into the `pushback` array
// (LIFO). p_pop reads from pushback first.
//
// `go` is the GOutput stack used by parse_xs/parse_delim/parse_logic.
// It's stretchy.
// ====================================================================

typedef struct {
  dyn gi;          // input list (immutable)
  uint64_t gi_n;   // LIST_SIZE(gi)
  uint64_t gi_pos; // next index to read
  dyn *pushback;   // stretchy LIFO of pushed-back tokens
  dyn *go;         // output stack
} pstate_t;

static void p_init(pstate_t *p, dyn input) {
  p->gi = input;
  p->gi_n = is_list(input) ? LIST_SIZE(input) : 0;
  p->gi_pos = 0;
  p->pushback = 0;
  p->go = 0;
}

static void p_free(pstate_t *p) {
  if (p->pushback) arrfree(p->pushback);
  if (p->go) arrfree(p->go);
  p->pushback = 0;
  p->go = 0;
}

static inline int p_end(pstate_t *p) {
  return arrlen(p->pushback) == 0 && p->gi_pos >= p->gi_n;
}

static inline dyn p_peek(pstate_t *p) {
  int pn = arrlen(p->pushback);
  if (pn > 0) return p->pushback[pn - 1];
  return LGET(p->gi, p->gi_pos);
}

static inline dyn p_pop(pstate_t *p) {
  int pn = arrlen(p->pushback);
  if (pn > 0) { dyn t = p->pushback[pn - 1]; arrpop(p->pushback); return t; }
  dyn t = LGET(p->gi, p->gi_pos);
  p->gi_pos++;
  return t;
}

static inline void p_push_back(pstate_t *p, dyn t) {
  arrput(p->pushback, t);
}

// ====================================================================
// add_bars -- mirror of reader.s:30-42.
// Used both at toplevel (text.parse calls it) and as a building block.
// ====================================================================

// Build a `|` separator token mirroring [Row Col Orig].
// token_() leaves slot 2 (pchar) and slot 6 (parsed) uninitialised;
// must zero them or downstream `Tok.parsed` reads garbage.
static dyn make_bar(dyn row, dyn col, dyn orig) {
  dyn t = token(KW_pipe, KW_pipe, row, col, orig);
  LGET(t, 2) = 0;
  LGET(t, 6) = 0;
  return t;
}

dyn reader_add_bars(dyn xs) {
  GC_DISABLE();
  kw_init();
  uint64_t n = LIST_SIZE(xs);
  dyn *out = 0;
  int first = 1;
  for (uint64_t i = 0; i < n; i++) {
    dyn x = LGET(xs, i);
    if (is_tok(x)) {
      dyn type = tok_type(x);
      int is_continuation =
            text_eq_c(type, KW_pipe) || text_eq_c(type, KW_then) ||
            text_eq_c(type, KW_else_) || text_eq_c(type, KW_elif);
      int col = (int)(int64_t)UNFXN(tok_col(x));
      if ((col == 0 || first) && !is_continuation) {
        dyn col_m1; LDFXN(col_m1, col - 1);
        dyn bar = make_bar(tok_row(x), col_m1, tok_orig(x));
        arrput(out, bar);
        first = 0;
      }
    }
    arrput(out, x);
  }
  int outn = arrlen(out);
  dyn r;
  LIST(r, outn);
  for (int i = 0; i < outn; i++) LGET(r, i) = out[i];
  arrfree(out);
  GC_ENABLE();
  return r;
}

// ====================================================================
// parse_strip -- mirror of reader.s:460-472 (no meta wrapping).
// ====================================================================

dyn reader_parse_strip(dyn x) {
  if (is_tok(x)) {
    dyn parsed = tok_parsed(x);
    if (parsed) return reader_parse_strip(LGET(parsed, 0));
    return tok_value(x);
  }
  if (!is_list(x)) return x;
  uint64_t n = LIST_SIZE(x);
  if (n == 0) return x;

  GC_DISABLE();
  dyn out; LIST(out, n);
  for (uint64_t i = 0; i < n; i++) {
    LGET(out, i) = reader_parse_strip(LGET(x, i));
  }
  GC_ENABLE();
  return out;
}

// Forward declarations (mutually recursive).
static dyn parse_term(pstate_t *p);
static dyn parse_dollar(pstate_t *p);
static dyn parse_suf(pstate_t *p);
static dyn parse_suf_loop(pstate_t *p, dyn (*down)(pstate_t*), dyn e);
static dyn parse_prefix(pstate_t *p);
static dyn parse_pow(pstate_t *p);
static dyn parse_mul(pstate_t *p);
static dyn parse_add(pstate_t *p);
static dyn parse_sufs(pstate_t *p);
static dyn parse_b_shift(pstate_t *p);
static dyn parse_b_and(pstate_t *p);
static dyn parse_b_xor(pstate_t *p);
static dyn parse_comma(pstate_t *p);
static dyn parse_bool(pstate_t *p);
static dyn parse_blogic(pstate_t *p);
static dyn parse_exclamation(pstate_t *p);
static dyn parse_logic(pstate_t *p);
static dyn parse_offside(pstate_t *p, dyn type, int expect_eol,
                          int orow, dyn ocol);
static dyn parse_if(pstate_t *p, dyn sym);
static dyn parse_bar(pstate_t *p, dyn h);
static dyn parse_table(dyn xs);
static dyn parse_delim(pstate_t *p, int delim_ctx);
static dyn parse_xs(pstate_t *p, int delim_ctx);
static int parse_semicolon(pstate_t *p);
static dyn parse_tokens_inner(dyn input);

// ====================================================================
// Generic builders.
// ====================================================================

static dyn make_list_n(int n, dyn *xs) {
  dyn r;
  LIST(r, n);
  for (int i = 0; i < n; i++) LGET(r, i) = xs[i];
  return r;
}

static dyn make_list_from_arr(dyn *arr) {
  return make_list_n(arrlen(arr), arr);
}

// ====================================================================
// parser_error -- emits a positioned rterr similar to reader.s:44.
// ====================================================================

static void parser_error(const char *cause, dyn tok) {
  if (is_tok(tok)) {
    int row = (int)UNFXN(tok_row(tok));
    int col = (int)UNFXN(tok_col(tok));
    char *orig = "<none>";
    if (IS_BIGTEXT(tok_orig(tok))) orig = text_to_cstring(tok_orig(tok));
    dyn tv = tok_value(tok);
    char *vs = "eof";
    if (tv) vs = print_object(tv);
    rterr("%s:%d,%d: %s `%s`", orig, row, col, cause, vs);
  } else {
    rterr("<?>: %s `%s`", cause, tok ? print_object(tok) : "eof");
  }
}

// ====================================================================
// parse_op -- consume a token if its .type is in the predicate set.
// ====================================================================

typedef int (*op_pred_t)(dyn);

static dyn parse_op(pstate_t *p, op_pred_t pred) {
  if (p_end(p)) return 0;
  dyn t = p_peek(p);
  if (!is_tok(t)) return 0;
  if (!pred(tok_type(t))) return 0;
  return p_pop(p);
}

// ====================================================================
// Backwards-compatible builders for tokens we synthesize.
// `parsed` (slot 6) must always be 0 unless we have a parsed cache.
// ====================================================================

static dyn mk_token(dyn type, dyn val, dyn row, dyn col, dyn orig,
                    dyn parsed) {
  dyn t = token(type, val, row, col, orig);
  LGET(t, 2) = 0;       // pchar
  LGET(t, 6) = parsed;
  return t;
}

// retok: reuse src of an existing token, change type+value.
static dyn retok(dyn old, dyn new_type) {
  return mk_token(new_type, new_type,
                  tok_row(old), tok_col(old), tok_orig(old), 0);
}

// ====================================================================
// parse_term -- reader.s:128-172.
//
// Dispatches on the token type. Returns a parsed-expr or 0 if no
// match (in which case the token is pushed back).
// ====================================================================

static dyn parse_term(pstate_t *p) {
  if (p_end(p)) return 0;
  dyn tok = p_pop(p);

  // Cached parsed form?
  if (tok_parsed(tok)) return tok;

  dyn val = tok_value(tok);
  dyn type = tok_type(tok);
  dyn parsed = 0;  // the inner parsed-expression list
  // `parsed` doubles as a "got something to cache" sentinel. But
  // for an integer literal of 0, FXN(0) == 0 which collides with
  // the "no parse" value. Track presence explicitly.
  int have_parsed = 0;

  if (text_eq_c(type, KW_escape) || text_eq_c(type, KW_symbol) ||
      text_eq_c(type, KW_text)) {
    return tok;
  }
  if (text_eq_c(type, KW_int_)) {
    int n = IS_BIGTEXT(val) ? BIGTEXT_SIZE(val) : (IS_FIXTEXT(val) ? fixtext_size(val) : 0);
    char *s = text_to_cstring(val);
    long long v;
    if (n > 2 && s[1] == 'x') {
      // 0x... or Nx... (radix from first char)
      if (s[0] == '0') v = strtoll(s + 2, 0, 16);
      else {
        int radix = (int)(s[0] - '0');
        v = strtoll(s + 2, 0, radix);
      }
    } else {
      v = strtoll(s, 0, 10);
    }
    LDFXN(parsed, v);
    have_parsed = 1;
  } else if (text_eq_c(type, KW_hex)) {
    char *s = text_to_cstring(val);
    long long v = strtoll(s + 1, 0, 16);
    LDFXN(parsed, v);
    have_parsed = 1;
  } else if (text_eq_c(type, KW_bin)) {
    char *s = text_to_cstring(val);
    long long v = strtoll(s + 2, 0, 2);
    LDFXN(parsed, v);
    have_parsed = 1;
  } else if (text_eq_c(type, KW_void_)) {
    parsed = 0;  // No
    have_parsed = 1;
  } else if (text_eq_c(type, KW_parens)) {
    parsed = parse_tokens_inner(val);
    have_parsed = 1;
  } else if (text_eq_c(type, KW_brackets)) {
    // `[]` -- [symbol-`[]` @parsed_inner]
    dyn inner = parse_tokens_inner(val);
    uint64_t in = LIST_SIZE(inner);
    dyn head = mk_token(KW_symbol, KW_brackets,
                        tok_row(tok), tok_col(tok), tok_orig(tok), 0);
    dyn r; LIST(r, in + 1);
    LGET(r, 0) = head;
    for (uint64_t i = 0; i < in; i++) LGET(r, i + 1) = LGET(inner, i);
    parsed = r;
    have_parsed = 1;
  } else if (text_eq_c(type, KW_curlyL) || text_eq_c(type, KW_curly)) {
    // `{` / `{}` -- [symbol-`{` @parsed_inner]
    dyn inner = parse_tokens_inner(val);
    uint64_t in = LIST_SIZE(inner);
    dyn head = mk_token(KW_symbol, KW_curlyL,
                        tok_row(tok), tok_col(tok), tok_orig(tok), 0);
    dyn r; LIST(r, in + 1);
    LGET(r, 0) = head;
    for (uint64_t i = 0; i < in; i++) LGET(r, i + 1) = LGET(inner, i);
    parsed = r;
    have_parsed = 1;
  } else if (text_eq_c(type, KW_splice)) {
    // String splice -- value is a list of `symbol`/`()` tokens.
    // Output:
    //   [(symbol `"` Tok.src 0) @parsed_inner-with-quoting]
    // where each parsed item gets a preceding `\\` token if it
    // contains `~` or `?` markers (we skip that nicety for now;
    // game code rarely uses tilde/question markers inside splices).
    dyn inner = parse_tokens_inner(val);
    uint64_t in = LIST_SIZE(inner);
    static dyn quote_text = 0;
    if (!quote_text) TEXT(quote_text, "\"");
    dyn head = mk_token(KW_symbol, quote_text,
                        tok_row(tok), tok_col(tok), tok_orig(tok), 0);
    dyn r; LIST(r, in + 1);
    LGET(r, 0) = head;
    for (uint64_t i = 0; i < in; i++) LGET(r, i + 1) = LGET(inner, i);
    parsed = r;
    have_parsed = 1;
  } else if (text_eq_c(type, KW_table)) {
    // `@{}` -- table literal. parse_strip + parse_table + spread.
    dyn ts_raw = parse_tokens_inner(val);
    dyn ts = reader_parse_strip(ts_raw);
    dyn kvs = reader_parse_table(ts);
    static dyn t_at_brace = 0;
    if (!t_at_brace) TEXT(t_at_brace, "@t");
    uint64_t kvn = LIST_SIZE(kvs);
    dyn r; LIST(r, kvn + 1);
    LGET(r, 0) = t_at_brace;
    for (uint64_t i = 0; i < kvn; i++) LGET(r, i + 1) = LGET(kvs, i);
    parsed = r;
    have_parsed = 1;
  } else if (text_eq_c(type, KW_object)) {
    // `${}` -- object literal. Complex: needs `new_fn_` + class
    // conversion. Bail by emitting a placeholder; downstream
    // macroexpand will likely fail on this but better than silent
    // wrong AST.
    dyn ts_raw = parse_tokens_inner(val);
    dyn ts = reader_parse_strip(ts_raw);
    dyn kvs = reader_parse_table(ts);
    static dyn t_dollar_brace = 0;
    if (!t_dollar_brace) TEXT(t_dollar_brace, "${}");
    uint64_t kvn = LIST_SIZE(kvs);
    dyn r; LIST(r, kvn + 1);
    LGET(r, 0) = t_dollar_brace;
    for (uint64_t i = 0; i < kvn; i++) LGET(r, i + 1) = LGET(kvs, i);
    parsed = r;
    have_parsed = 1;
  } else if (text_eq_c(type, KW_pipe)) {
    return parse_bar(p, tok);  // already has a parsed-expr structure
  } else if (text_eq_c(type, KW_if_)) {
    return parse_if(p, tok);
  } else {
    // splice, @{}, ${}, unary, and operator types fall through here.
    // For the unhandled "complex" cases (splice strings, @{}, ${}),
    // bail and let Symta-side parse_term handle them.
    // For unary/operator types, parse_unary handling.
    if (is_unary_op_type(type) ||
        (text_eq_c(type, KW_unary))) {
      // parse_unary handles unary tokens.
      // Implementation inline: reader.s:310 parse_unary
      // | if is_eol H: ret [nullary_ H]
      // | A parse_prefix
      // | less A: ret [nullary_ H]
      // | if H.value<>'-': ret [H A]
      // | less A^token_is(int) or hex or float: ret [H A]
      // | token A.type "[H.value][A.value]" H.src [-A.parsed.0]
      dyn h = tok;
      // is_eol: next is on a different row, or col != H.col + value.n.
      int is_eol = 0;
      if (p_end(p)) is_eol = 1;
      else {
        dyn nx = p_peek(p);
        if (!is_tok(nx)) is_eol = 1;
        else {
          int nrow = (int)UNFXN(tok_row(nx));
          int hrow = (int)UNFXN(tok_row(h));
          int ncol = (int)UNFXN(tok_col(nx));
          int hcol = (int)UNFXN(tok_col(h));
          int hvn = IS_BIGTEXT(tok_value(h)) ?
                    BIGTEXT_SIZE(tok_value(h)) :
                    (IS_FIXTEXT(tok_value(h)) ? fixtext_size(tok_value(h)) : 0);
          if (nrow != hrow || ncol != hcol + hvn) is_eol = 1;
        }
      }
      static dyn nullary_sym = 0;
      if (!nullary_sym) TEXT(nullary_sym, "nullary_");
      if (is_eol) {
        dyn r; LIST(r, 2);
        LGET(r, 0) = nullary_sym;
        LGET(r, 1) = h;
        // Cache on tok.parsed.
        LGET(tok, 6) = (r ? ({ dyn p; LIST(p, 1); LGET(p, 0) = r; p; }) : 0);
        return tok;
      }
      dyn a = parse_prefix(p);
      if (!a) {
        dyn r; LIST(r, 2);
        LGET(r, 0) = nullary_sym;
        LGET(r, 1) = h;
        dyn pp; LIST(pp, 1); LGET(pp, 0) = r;
        LGET(tok, 6) = pp;
        return tok;
      }
      // Default: [H A]
      dyn r; LIST(r, 2);
      LGET(r, 0) = h;
      LGET(r, 1) = a;
      dyn pp; LIST(pp, 1); LGET(pp, 0) = r;
      LGET(tok, 6) = pp;
      return tok;
    }
    // Not a known type -- push back and return 0.
    p_push_back(p, tok);
    return 0;
  }

  // Cache parsed on tok.parsed = [parsed]. Use the have_parsed
  // flag rather than `if (parsed)` because for integer/void/etc.
  // a legitimate parsed value can be 0 (FXN(0) == 0).
  if (have_parsed) {
    dyn pp; LIST(pp, 1); LGET(pp, 0) = parsed;
    LGET(tok, 6) = pp;
  }
  return tok;
}

// ====================================================================
// parse_dollar -- reader.s:223-226.
// ====================================================================

static dyn parse_dollar(pstate_t *p) {
  dyn o = parse_op(p, is_dollar_op);
  if (!o) return parse_term(p);
  // is unary? -> parse_unary path (treats o as the unary op token).
  if (token_is(o, KW_unary)) {
    // parse_unary inlined.
    return parse_term((p_push_back(p, o), p));  // re-enter via parse_term
  }
  dyn rhs = parse_dollar(p);
  if (!rhs) {
    parser_error("no operand for", o);
  }
  dyn r; LIST(r, 2); LGET(r, 0) = o; LGET(r, 1) = rhs;
  return r;
}

// ====================================================================
// parse_suf_loop / parse_suf -- reader.s:267-307.
// Implementation NOTE: this section in reader.s is the most intricate
// part of the parser (handles `.`, `^`, `->`, `[...]`, `{...}`, etc.
// postfix forms). We port it faithfully but with explicit state.
// ====================================================================

static dyn parse_suf_op(pstate_t *p) {
  return parse_op(p, is_suf_op);
}

// Implements parse_suf_unary's logic for invocation-style postfixes
// like `[]`, `()`, `{}`, `[`, `{` plus the SufUnaryS operator-as-suffix.
// Returns 0 if no match (and pushes O back); otherwise wraps E.
static dyn parse_suf_unary(pstate_t *p, dyn (*down)(pstate_t*),
                            dyn e, dyn o) {
  if (!is_suf_unary(tok_type(o))) return 0;
  dyn type = tok_type(o);
  if (text_eq_c(type, KW_parens) || text_eq_c(type, KW_brackets) ||
      text_eq_c(type, KW_bracketsL) || text_eq_c(type, KW_curly) ||
      text_eq_c(type, KW_curlyL)) {
    int valid = 0;
    dyn pc = tok_pchar(o);
    if (pc && is_anchor_char_c(pc)) {
      valid = 1;
      if (text_eq_c(type, KW_brackets)) {
        LGET(o, 0) = KW_bracketsL;  // type = `[`
        type = KW_bracketsL;
      }
    }
    if (!valid) {
      if (text_eq_c(type, KW_curly)) {
        LGET(o, 0) = KW_curlyL;
      }
      p_push_back(p, o);
      dyn r; LIST(r, 1); LGET(r, 0) = e;
      return r;
    }
    // valid suffix
  } else if (is_suf_unary_s(type)) {
    int valid = 0;
    dyn pc = tok_pchar(o);
    if (pc && is_anchor_char_c(pc)) {
      // next_is_disjoint check
      int disjoint = 1;
      if (!p_end(p)) {
        dyn nx = p_peek(p);
        if (is_tok(nx)) {
          int nrow = (int)UNFXN(tok_row(nx));
          int orow = (int)UNFXN(tok_row(o));
          int ncol = (int)UNFXN(tok_col(nx));
          int ocol = (int)UNFXN(tok_col(o));
          int ovn = IS_BIGTEXT(tok_value(o)) ?
                    BIGTEXT_SIZE(tok_value(o)) :
                    (IS_FIXTEXT(tok_value(o)) ? fixtext_size(tok_value(o)) : 0);
          if (nrow == orow && ncol == ocol + ovn &&
              !is_disjoint_char(tok_type(nx))) {
            disjoint = 0;
          }
        }
      }
      if (disjoint) {
        valid = 1;
        // append `_` to o.value if not already ending in `_`
        dyn v = tok_value(o);
        char *s = text_to_cstring(v);
        int sl = (int)strlen(s);
        if (sl == 0 || s[sl-1] != '_') {
          char *buf = (char*)malloc(sl + 2);
          memcpy(buf, s, sl);
          buf[sl] = '_';
          buf[sl+1] = 0;
          dyn nv; TEXT(nv, buf);
          LGET(o, 1) = nv;
          free(buf);
        }
      }
    }
    if (!valid) {
      p_push_back(p, o);
      dyn r; LIST(r, 1); LGET(r, 0) = e;
      return r;
    }
    // ret:: (parse_suf_loop Down [O E]) -- wrap in 1-element list.
    dyn pair; LIST(pair, 2); LGET(pair, 0) = o; LGET(pair, 1) = e;
    dyn inner_result = parse_suf_loop(p, down, pair);
    dyn wrap; LIST(wrap, 1); LGET(wrap, 0) = inner_result;
    return wrap;
  } else {
    // Other SufUnary type not in '[' '(' '{' or SufUnaryS.
  }

  // The "valid" case for brackets/parens/curly group: parse the inner.
  dyn val = tok_value(o);
  dyn as = parse_tokens_inner(val);
  uint64_t an = LIST_SIZE(as);
  int has_delim = 0;
  for (uint64_t i = 0; i < an; i++) {
    if (is_delim_filter(LGET(as, i))) { has_delim = 1; break; }
  }
  if (has_delim) {
    dyn wrapper; LIST(wrapper, 1); LGET(wrapper, 0) = as;
    as = wrapper;
    an = 1;
  }
  dyn ot_pair; LIST(ot_pair, 1); LGET(ot_pair, 0) = tok_type(o);
  LGET(o, 6) = ot_pair;
  // [O E @as]
  dyn full; LIST(full, an + 2);
  LGET(full, 0) = o;
  LGET(full, 1) = e;
  for (uint64_t i = 0; i < an; i++) LGET(full, i + 2) = LGET(as, i);
  // ret:: parse_suf_loop ... -- wrap result in 1-element list.
  dyn inner_result = parse_suf_loop(p, down, full);
  dyn wrap; LIST(wrap, 1); LGET(wrap, 0) = inner_result;
  return wrap;
}

static dyn parse_suf_loop(pstate_t *p, dyn (*down)(pstate_t*), dyn e) {
  // Mirror of reader.s:272-304. NOT iterative -- the recursive case
  // is inside parse_suf_unary or in the binary-suffix fallback.
  //
  //   parse_suf_loop Down E =
  //   | O | parse_suf_op or ret E
  //   | R parse_suf_unary Down E O
  //   | if R: ret R.0           // R is always a single-element list
  //                             // wrapping the final value
  //   | B | Down()
  //   | less B:
  //     | ... `.` / `^` method-name idiom or parser_error ...
  //   | less O^token_is('.') and E^token_is(int) and B^token_is(int):
  //     | ret: parse_suf_loop Down [O E B]
  //   | V "[E.value].[B.value]"
  //   | F token float V E.src [V.flt]
  //   | ret: parse_suf_loop Down F
  dyn o = parse_suf_op(p);
  if (!o) return e;
  dyn r = parse_suf_unary(p, down, e, o);
  if (r) return LGET(r, 0);
  // parse_suf_unary returned 0 -- O is a binary suffix (`.` / `^` / `->`).
  dyn b = down(p);
  if (!b) {
    if (token_is(o, KW_dot) || token_is(o, KW_caret)) {
      if (!p_end(p)) {
        dyn t = p_peek(p);
        if (is_tok(t) && is_opt(tok_value(t))) {
          // method-name idiom: X.+  -> [O E T-as-symbol]
          (void)p_pop(p);
          LGET(t, 0) = KW_symbol;
          dyn r2; LIST(r2, 3);
          LGET(r2, 0) = o; LGET(r2, 1) = e; LGET(r2, 2) = t;
          return r2;
        }
      }
    }
    parser_error("no right operand for", o);
    return 0;
  }
  // X.Y where both are ints -> float token, then continue with that.
  if (token_is(o, KW_dot) && token_is(e, KW_int_) && token_is(b, KW_int_)) {
    // text_to_cstring uses a shared static buffer (api.sbuf), so a
    // second call clobbers the first. Snapshot the first into a
    // local before reading the second.
    char *raw1 = text_to_cstring(tok_value(e));
    int el = (int)strlen(raw1);
    char es_buf[64];
    if (el >= (int)sizeof(es_buf)) el = (int)sizeof(es_buf) - 1;
    memcpy(es_buf, raw1, el);
    es_buf[el] = 0;
    char *es = es_buf;
    char *bs = text_to_cstring(tok_value(b));
    int bl = (int)strlen(bs);
    char *buf = (char*)malloc(el + bl + 2);
    memcpy(buf, es, el);
    buf[el] = '.';
    memcpy(buf + el + 1, bs, bl);
    buf[el + bl + 1] = 0;
    dyn v; TEXT(v, buf);
    static dyn t_float = 0;
    if (!t_float) TEXT(t_float, "float");
    // Symta: `F token float V E.src [V.flt]` -- pre-fills the
    // parsed slot with [V.flt] so parse_strip yields the float
    // immediately rather than treating "3.14" as a symbol.
    double dv = atof(buf);
    dyn flt;
    LDFLT(flt, dv);
    dyn parsed_list; LIST(parsed_list, 1);
    LGET(parsed_list, 0) = flt;
    dyn ftok = mk_token(t_float, v, tok_row(e), tok_col(e),
                         tok_orig(e), parsed_list);
    free(buf);
    return parse_suf_loop(p, down, ftok);
  }
  dyn r2; LIST(r2, 3);
  LGET(r2, 0) = o; LGET(r2, 1) = e; LGET(r2, 2) = b;
  return parse_suf_loop(p, down, r2);
}

static dyn parse_suf(pstate_t *p) {
  dyn d = parse_dollar(p);
  if (!d) return 0;
  return parse_suf_loop(p, parse_dollar, d);
}

// ====================================================================
// parse_prefix / parse_unary stub (parse_unary handled inline in
// parse_term's unary branch).
// ====================================================================

static dyn parse_prefix(pstate_t *p) {
  dyn o = parse_op(p, is_prefix_op);
  if (!o) return parse_suf(p);
  // unary -> parse_term re-entry via push back
  if (token_is(o, KW_unary)) {
    p_push_back(p, o);
    return parse_term(p);  // parse_term will detect unary and chain
  }
  dyn pref = parse_prefix(p);
  if (!pref) {
    // Check for SufBinary following on this row
    if (!p_end(p)) {
      dyn t = p_peek(p);
      if (is_tok(t) && is_suf_binary(tok_value(t))) {
        // nullary_ at end of GInput
        static dyn nullary_ = 0;
        if (!nullary_) TEXT(nullary_, "nullary_");
        dyn r; LIST(r, 2); LGET(r, 0) = nullary_; LGET(r, 1) = t;
        pref = r;
      }
    }
  }
  if (pref) {
    dyn r; LIST(r, 2); LGET(r, 0) = o; LGET(r, 1) = pref;
    return r;
  }
  dyn r; LIST(r, 1); LGET(r, 0) = o;
  return r;
}

// ====================================================================
// Precedence ladder helper.
// ====================================================================

static dyn binary_loop(pstate_t *p, op_pred_t ops_pred,
                       dyn (*down)(pstate_t*), dyn e) {
  for (;;) {
    dyn o = parse_op(p, ops_pred);
    if (!o) return e;
    dyn b = down(p);
    dyn ne;
    if (b) {
      LIST(ne, 3); LGET(ne, 0) = o; LGET(ne, 1) = e; LGET(ne, 2) = b;
    } else {
      // O.value = O.value + "_"
      dyn v = tok_value(o);
      char *s = text_to_cstring(v);
      int sl = (int)strlen(s);
      char *buf = (char*)malloc(sl + 2);
      memcpy(buf, s, sl);
      buf[sl] = '_';
      buf[sl+1] = 0;
      dyn nv; TEXT(nv, buf);
      LGET(o, 1) = nv;
      free(buf);
      LIST(ne, 2); LGET(ne, 0) = o; LGET(ne, 1) = e;
    }
    e = ne;
  }
}

static inline dyn parse_binary(pstate_t *p, op_pred_t ops_pred,
                                dyn (*down)(pstate_t*)) {
  dyn e = down(p);
  if (!e) return 0;
  return binary_loop(p, ops_pred, down, e);
}

static int op_pow(dyn t)    { return text_eq_c(t, KW_pow); }
static int op_mul(dyn t)    { return text_eq_c(t, KW_star) || text_eq_c(t, KW_slash) || text_eq_c(t, KW_pct); }
static int op_add(dyn t)    { return text_eq_c(t, KW_plus) || text_eq_c(t, KW_minus); }
static int op_sufs(dyn t)   { return text_eq_c(t, KW_range); }
static int op_bshift(dyn t) { return text_eq_c(t, KW_shl) || text_eq_c(t, KW_shr); }
static int op_band(dyn t)   { return text_eq_c(t, KW_band); }
static int op_bxor(dyn t)   { return text_eq_c(t, KW_bor) || text_eq_c(t, KW_bxor); }
static int op_comma(dyn t)  { return text_eq_c(t, KW_comma); }
static int op_blogic(dyn t) { return text_eq_c(t, KW_and_) || text_eq_c(t, KW_or_); }
static int op_excl(dyn t)   { return text_eq_c(t, KW_bang); }

static dyn parse_pow(pstate_t *p)       { return parse_binary(p, op_pow, parse_prefix); }
static dyn parse_mul(pstate_t *p)       { return parse_binary(p, op_mul, parse_pow); }
static dyn parse_add(pstate_t *p)       { return parse_binary(p, op_add, parse_mul); }
static dyn parse_sufs(pstate_t *p)      { return parse_binary(p, op_sufs, parse_add); }
static dyn parse_b_shift(pstate_t *p)   { return parse_binary(p, op_bshift, parse_sufs); }
static dyn parse_b_and(pstate_t *p)     { return parse_binary(p, op_band, parse_b_shift); }
static dyn parse_b_xor(pstate_t *p)     { return parse_binary(p, op_bxor, parse_b_and); }
static dyn parse_comma(pstate_t *p)     { return parse_binary(p, op_comma, parse_b_xor); }
static dyn parse_bool(pstate_t *p)      { return parse_binary(p, is_bool_op, parse_comma); }
static dyn parse_blogic(pstate_t *p)    { return parse_binary(p, op_blogic, parse_bool); }

static dyn parse_exclamation(pstate_t *p) {
  // Symta's parse_excl_loop: left-folds `!` without consuming a
  // right operand, and WITHOUT appending `_` to the op value:
  //   parse_excl_loop Ops E =
  //     | O | parse_op Ops or ret E
  //     | parse_excl_loop Ops [O E]
  // Different from binary_loop -- this preserves `Env!` as
  // `(`!` Env)` instead of `(`!_` Env)`. The empty-table macro
  // looks for the `!` keyword specifically.
  if (!p_end(p)) {
    dyn t = p_peek(p);
    if (is_tok(t) && text_eq_c(tok_type(t), KW_bang)) {
      return p_pop(p);
    }
  }
  dyn e = parse_blogic(p);
  if (!e) return 0;
  for (;;) {
    dyn o = parse_op(p, op_excl);
    if (!o) return e;
    // No right operand fetched -- just left-fold [O E].
    dyn ne; LIST(ne, 2); LGET(ne, 0) = o; LGET(ne, 1) = e;
    e = ne;
  }
}

// ====================================================================
// parse_offside / parse_if / parse_bar / parse_logic / parse_delim
//
// These are sufficiently complex that we delegate back to Symta for
// now. Once the inner ladder is verified, port them in a follow-up.
//
// To keep parse_term/parse_dollar/parse_suf working through these
// branches, parse_if and parse_bar must produce valid AST. We do
// the minimal structural work here.
// ====================================================================

// Build a synthetic call out to Symta. Not implemented in this
// initial cut; we expect text.parse to keep using the Symta-side
// parser until these functions are complete.
//
// As a placeholder, parse_if/parse_bar return 0 -- the caller's
// parse_term then returns 0, signalling no-match and parsing continues
// or errors. We never actually reach parse_term's `|` or `if` cases
// when text.parse is using the Symta path; this code only runs once
// we flip the switch in reader.s.

// ====================================================================
// parse_bar -- reader.s:87-97.
//
//   parse_bar H =
//   | C H.src.1
//   | Zs:
//   | while not GInput.end:
//     | Ys:
//     | while not GInput.end and GInput.0.src.1 > C: push GInput^pop Ys
//     | push Ys.f^parse_tokens Zs
//     | when GInput.end: ret   [H @Zs.f]
//     | X GInput.0
//     | less X^token_is('|') and X.src.1 >< C: ret   [H @Zs.f]
//     | pop GInput
//
// Gathers indented sub-blocks separated by `|` tokens at column C
// (the column of the leading `|`). Each block is parsed by recursing
// into parse_tokens_inner. Returns [H @blocks].
// ====================================================================

static dyn parse_bar(pstate_t *p, dyn h) {
  int c = (int)UNFXN(tok_col(h));
  dyn *zs = 0;
  for (;;) {
    if (p_end(p)) break;
    // Collect Ys = tokens with col > c (the block body).
    dyn *ys = 0;
    while (!p_end(p)) {
      dyn t = p_peek(p);
      if (!is_tok(t)) break;
      int tc = (int)UNFXN(tok_col(t));
      if (tc <= c) break;
      arrput(ys, p_pop(p));
    }
    dyn yslist;
    int yn = arrlen(ys);
    LIST(yslist, yn);
    for (int i = 0; i < yn; i++) LGET(yslist, i) = ys[i];
    arrfree(ys);
    dyn parsed = parse_tokens_inner(yslist);
    arrput(zs, parsed);
    if (p_end(p)) break;
    dyn x = p_peek(p);
    if (!is_tok(x)) break;
    if (!text_eq_c(tok_type(x), KW_pipe)) break;
    if ((int)UNFXN(tok_col(x)) != c) break;
    (void)p_pop(p);  // consume the next `|`
  }
  int zn = arrlen(zs);
  dyn r;
  LIST(r, zn + 1);
  LGET(r, 0) = h;
  for (int i = 0; i < zn; i++) LGET(r, i + 1) = zs[i];
  arrfree(zs);
  return r;
}

// ====================================================================
// parse_offside -- reader.s:369-411.
// Indentation-sensitive block parser. Collects same-col-or-deeper
// tokens, splitting at same-col occurrences (auto-bar) into separate
// statements, then parse_tokens on the result.
// ====================================================================

static int is_var_tok(dyn x) {
  if (!is_tok(x)) return 0;
  dyn v = tok_value(x);
  return IS_BIGTEXT(v) && !IS_FIXTEXT(v);
  // Symta's is_var_tok is "is_text and not is_keyword" -- bigtexts
  // are not keywords (only fixtexts are).
}

static dyn parse_offside(pstate_t *p, dyn type, int expect_eol,
                          int orow, dyn ocol) {
  if (p_end(p)) {
    dyn empty;
    LIST(empty, 0);
    return empty;
  }
  // Symta's `ret : parse_xs 0` returns parse_xs result directly --
  // the `:` is part of `ret`'s colon-line syntax (`ret: X` means
  // `return X`), not a list constructor. parse_if's `ret:: Sym X.0`
  // takes the FIRST element of the parse_xs result, which is the
  // operator symbol for delim-headed forms.
  #define RET_XS_WRAPPED() return parse_xs(p, 0)
  dyn k = p_peek(p);
  while (is_list(k) && LIST_SIZE(k) > 0) k = LGET(k, 0);
  if (!is_tok(k)) RET_XS_WRAPPED();
  int k_row = (int)UNFXN(tok_row(k));
  int k_col = (int)UNFXN(tok_col(k));
  int lines_hack = 0;
  if (!(orow < k_row)) {
    if (expect_eol) RET_XS_WRAPPED();
    lines_hack = 1;
  }
  if (text_eq_c(tok_value(k), KW_pipe) ||
      text_eq_c(tok_value(k), KW_colon)) {
    RET_XS_WRAPPED();
  }
  int s_col = k_col;
  if (ocol != No && (int)UNFXN(ocol) >= s_col) {
    RET_XS_WRAPPED();
  }
  // Snapshot input position for potential rollback.
  uint64_t s_pos = p->gi_pos;
  int s_pn = arrlen(p->pushback);
  dyn h = parse_exclamation(p);
  // Rollback (Symta restores GInput regardless of H's value).
  p->gi_pos = s_pos;
  while (arrlen(p->pushback) > s_pn) arrpop(p->pushback);
  // case H [X Y]: if X^token_is(`!`): less Y^is_var_tok: ret: parse_xs 0
  if (h && is_list(h) && LIST_SIZE(h) == 2) {
    dyn x = LGET(h, 0);
    dyn y = LGET(h, 1);
    if (is_tok(x) && text_eq_c(tok_type(x), KW_bang)) {
      if (!is_var_tok(y)) RET_XS_WRAPPED();
    }
  }
  dyn *zs = 0;
  dyn *ys = 0;
  // BSrc: [Src.row Src.col-1 Src.orig]
  dyn bsrc_row = tok_row(k);
  dyn bsrc_col; LDFXN(bsrc_col, k_col - 1);
  dyn bsrc_orig = tok_orig(k);
  while (!p_end(p)) {
    dyn x = p_peek(p);
    if (is_tok(x)) {
      int col = (int)UNFXN(tok_col(x));
      if (col < s_col) break;
      dyn v = tok_value(x);
      if (text_eq_c(type, KW_if_) && text_eq_c(v, KW_then)) break;
      if (col == s_col && arrlen(ys) > 0 &&
          !text_eq_c(v, KW_pipe) && !text_eq_c(v, KW_else_) &&
          !text_eq_c(v, KW_elif) && !text_eq_c(v, KW_then)) {
        // less Type >< 'if': auto-bar insert.
        // (We preserve the original Symta behaviour exactly, even
        // though the broader B7 discussion would prefer different
        // semantics. Don't touch without flipping B7.)
        if (!text_eq_c(type, KW_if_)) {
          dyn bar = make_bar(bsrc_row, bsrc_col, bsrc_orig);
          int yn = arrlen(ys);
          dyn block;
          LIST(block, yn + 1);
          LGET(block, 0) = bar;
          for (int i = 0; i < yn; i++) LGET(block, i + 1) = ys[i];
          arrput(zs, block);
          arrfree(ys); ys = 0;
        }
      }
    }
    arrput(ys, p_pop(p));
  }
  {
    dyn bar = make_bar(bsrc_row, bsrc_col, bsrc_orig);
    int yn = arrlen(ys);
    dyn block;
    LIST(block, yn + 1);
    LGET(block, 0) = bar;
    for (int i = 0; i < yn; i++) LGET(block, i + 1) = ys[i];
    arrput(zs, block);
    arrfree(ys); ys = 0;
  }
  if (lines_hack && arrlen(zs) <= 1) {
    p->gi_pos = s_pos;
    while (arrlen(p->pushback) > s_pn) arrpop(p->pushback);
    arrfree(zs);
    RET_XS_WRAPPED();
  }
  // Input = Zs.f.j -- forward, joined (each block is a list of toks)
  dyn *flat = 0;
  for (int i = 0; i < arrlen(zs); i++) {
    dyn block = zs[i];
    int bn = LIST_SIZE(block);
    for (int j = 0; j < bn; j++) arrput(flat, LGET(block, j));
  }
  int fn = arrlen(flat);
  dyn input;
  LIST(input, fn);
  for (int i = 0; i < fn; i++) LGET(input, i) = flat[i];
  arrfree(flat);
  arrfree(zs);
  dyn r = parse_tokens_inner(input);
  // .f -- reverse (Symta returns R.f at end of parse_offside).
  // We return r as-is here; the caller (parse_delim) expects the
  // forward form. The Symta `.f` was needed because parse_xs
  // accumulated via push (reverse-insertion); our parse_xs uses
  // arrput so no reverse needed.
  return r;
}

// ====================================================================
// parse_if -- reader.s:58-85.
// ====================================================================

static int is_next(pstate_t *p, dyn type) {
  if (p_end(p)) return 0;
  dyn t = p_peek(p);
  return is_tok(t) && text_eq_c(tok_type(t), type);
}

static dyn parse_if(pstate_t *p, dyn sym) {
  int o_col = (int)UNFXN(tok_col(sym));
  dyn ocol_dyn; LDFXN(ocol_dyn, o_col);
  if (is_next(p, KW_colon)) {
    dyn t = p_pop(p);
    if (!text_eq_c(tok_type(t), KW_colon)) {
      parser_error("missing `:` for", sym);
      return 0;
    }
    dyn x = parse_offside(p, 0, 0, (int)UNFXN(tok_row(sym)), ocol_dyn);
    // ret:: Sym X.0
    if (is_list(x) && LIST_SIZE(x) > 0) {
      dyn r; LIST(r, 2);
      LGET(r, 0) = sym;
      LGET(r, 1) = LGET(x, 0);
      return r;
    }
    dyn r; LIST(r, 1);
    LGET(r, 0) = sym;
    return r;
  }
  // gather RawHead = tokens until end / : / then
  dyn *raw = 0;
  while (!p_end(p)) {
    if (is_next(p, KW_colon) || is_next(p, KW_then)) break;
    arrput(raw, p_pop(p));
  }
  dyn raw_list;
  int rn = arrlen(raw);
  LIST(raw_list, rn);
  for (int i = 0; i < rn; i++) LGET(raw_list, i) = raw[i];
  arrfree(raw);
  // Cnd = let GInput RawHead: parse_xs 1
  dyn cnd;
  {
    pstate_t inner;
    p_init(&inner, raw_list);
    cnd = parse_xs(&inner, 1);
    p_free(&inner);
  }
  // Symta keeps Cnd as the parse_xs list (e.g. `(X)`) -- don't .0
  // extract or we lose the wrapping.
  dyn then_, else_;
  if (is_next(p, KW_then)) {
    dyn t = p_pop(p);
    then_ = parse_offside(p, 0, 0, (int)UNFXN(tok_row(t)), ocol_dyn);
  } else {
    if (!is_next(p, KW_colon)) {
      parser_error("missing `:` for", sym);
      return 0;
    }
    dyn t = p_pop(p);
    then_ = parse_offside(p, 0, 0, (int)UNFXN(tok_row(t)), ocol_dyn);
  }
  // `Else: ` -- Symta initialises Else to empty list, not 0.
  LIST(else_, 0);
  if (is_next(p, KW_elif)) {
    dyn t = p_pop(p);
    // GInput =: T.retok(`else`) T.retok(`if`) @GInput
    dyn else_tok = retok(t, KW_else_);
    dyn if_tok = retok(t, KW_if_);
    p_push_back(p, if_tok);
    p_push_back(p, else_tok);
  }
  if (is_next(p, KW_else_)) {
    dyn t = p_pop(p);
    else_ = parse_offside(p, 0, 0, (int)UNFXN(tok_row(t)), ocol_dyn);
  }
  // `:Sym Cnd Then Else` -- 4-element list. Cnd/Then/Else stay
  // as-is (lists or whatever the inner parsers produced).
  dyn r;
  LIST(r, 4);
  LGET(r, 0) = sym;
  LGET(r, 1) = cnd;
  LGET(r, 2) = then_ ? then_ : ({ dyn e; LIST(e, 0); e; });
  LGET(r, 3) = else_;
  return r;
}

// ====================================================================
// parse_logic / parse_delim / parse_semicolon / parse_xs / parse_tokens
// ====================================================================

// parse_logic w/o the LL(1) hack -- straight delegation.
// The hack accelerates `case` etc. but isn't needed for correctness.
static dyn parse_logic(pstate_t *p) {
  // First try and/or
  static op_pred_t logic_pred = 0;
  // op_blogic handles `&&` `||`; logic-level handles `and` `or`
  // We need a predicate for `and` / `or`.
  static int logic_initted = 0;
  static dyn t_and = 0, t_or = 0;
  if (!logic_initted) {
    TEXT(t_and, "and");
    TEXT(t_or, "or");
    logic_initted = 1;
  }
  (void)logic_pred;  // we inline the predicate here
  // Parse exclamation first
  dyn e = parse_exclamation(p);
  if (!e) return 0;
  // Look for `and` / `or`
  for (;;) {
    if (p_end(p)) return e;
    dyn t = p_peek(p);
    if (!is_tok(t)) return e;
    dyn ty = tok_type(t);
    if (!text_eq_c(ty, t_and) && !text_eq_c(ty, t_or)) return e;
    dyn o = p_pop(p);
    dyn rhs = parse_exclamation(p);
    if (!rhs) {
      parser_error("no right operand for", o);
      return 0;
    }
    dyn ne; LIST(ne, 3);
    LGET(ne, 0) = o; LGET(ne, 1) = e; LGET(ne, 2) = rhs;
    e = ne;
  }
}

static dyn parse_delim(pstate_t *p, int delim_ctx) {
  (void)delim_ctx;
  dyn o = parse_op(p, is_delim_op);
  if (!o) return parse_logic(p);
  // Pref = GOutput.f -- in Symta the output stack so far gets
  // included as the LHS of the delim form. We use p->go.
  int gn = arrlen(p->go);
  dyn pref;
  if (gn > 0) {
    LIST(pref, gn);
    for (int i = 0; i < gn; i++) LGET(pref, i) = p->go[i];
    arrsetlen(p->go, 0);
  } else {
    LIST(pref, 0);
  }
  int expect_eol = 0;
  if (text_eq_c(tok_value(o), KW_colon)) {
    expect_eol = 1;
    // case Pref [X@_]: when X.is_token and X.value.is_keyword: ExpectEOL = 0
    if (LIST_SIZE(pref) > 0) {
      dyn x = LGET(pref, 0);
      if (is_tok(x) && IS_FIXTEXT(tok_value(x))) {
        expect_eol = 0;
      }
    }
  }
  dyn xs = parse_offside(p, 0, expect_eol,
                          (int)UNFXN(tok_row(o)), No);
  // Symta: `GOutput =: Xs Pref O` sets GOutput = [Xs Pref O], and
  // then parse_xs returns GOutput.f = [O Pref Xs]. We're using
  // arrput (insertion order, no reverse in parse_xs), so put the
  // elements directly in the FINAL desired order [O Pref Xs].
  arrsetlen(p->go, 0);
  arrput(p->go, o);
  arrput(p->go, pref);
  arrput(p->go, xs);
  return 0;
}

static int parse_semicolon(pstate_t *p) {
  // Find the first `|` or `;` token in remaining input.
  static dyn t_semi = 0;
  if (!t_semi) TEXT(t_semi, ";");
  int found_at = -1;
  dyn found_tok = 0;
  // Look in pushback (reverse order) then gi.
  int pn = arrlen(p->pushback);
  for (int i = pn - 1; i >= 0; i--) {
    dyn t = p->pushback[i];
    if (is_tok(t)) {
      dyn ty = tok_type(t);
      if (text_eq_c(ty, KW_pipe) || text_eq_c(ty, t_semi)) {
        found_at = -1 - i;  // negative encoding for pushback index
        found_tok = t;
        break;
      }
    }
  }
  if (!found_tok) {
    for (uint64_t i = p->gi_pos; i < p->gi_n; i++) {
      dyn t = LGET(p->gi, i);
      if (is_tok(t)) {
        dyn ty = tok_type(t);
        if (text_eq_c(ty, KW_pipe) || text_eq_c(ty, t_semi)) {
          found_at = (int)(i - p->gi_pos);
          found_tok = t;
          break;
        }
      }
    }
  }
  if (!found_tok) return 0;
  // If it's a `|`, do nothing (Symta returns 0).
  if (text_eq_c(tok_type(found_tok), KW_pipe)) return 0;
  // Otherwise it's `;` -- split.
  // L = parse_tokens(GInput.take(P))
  // R = parse_tokens(GInput.drop(P+1))
  // GInput = []
  // GOutput = if R.0^token_is(`;`) then [@R.tail.f L M] else [R L M]
  // where M is the `;` token.
  dyn left_list, right_list;
  {
    dyn *left = 0;
    for (int i = 0; i < found_at; i++) {
      arrput(left, p_pop(p));
    }
    int ln = arrlen(left);
    LIST(left_list, ln);
    for (int i = 0; i < ln; i++) LGET(left_list, i) = left[i];
    arrfree(left);
  }
  // Consume the `;`
  if (found_at >= 0) {
    (void)p_pop(p);  // drop the `;`
  }
  {
    // Consume rest into right_list.
    dyn *right = 0;
    while (!p_end(p)) arrput(right, p_pop(p));
    int rn = arrlen(right);
    LIST(right_list, rn);
    for (int i = 0; i < rn; i++) LGET(right_list, i) = right[i];
    arrfree(right);
  }
  dyn l = parse_tokens_inner(left_list);
  dyn r = parse_tokens_inner(right_list);
  arrsetlen(p->go, 0);
  // Check if R.0^token_is(`;`)
  if (is_list(r) && LIST_SIZE(r) > 0 &&
      is_tok(LGET(r, 0)) && text_eq_c(tok_type(LGET(r, 0)), t_semi)) {
    // [@R.tail.f L M] -- push in reverse, then L, then M
    dyn r_tail; uint64_t rn = LIST_SIZE(r);
    for (uint64_t i = rn; i > 1; i--) arrput(p->go, LGET(r, i - 1));
    arrput(p->go, l);
    arrput(p->go, found_tok);
    (void)r_tail;
  } else {
    // [R L M]
    arrput(p->go, r);
    arrput(p->go, l);
    arrput(p->go, found_tok);
  }
  return 0;
}

static dyn parse_xs(pstate_t *p, int delim_ctx) {
  // let GOutput []
  //   parse_semicolon
  //   named loop:
  //     while 1:
  //       X | parse_delim DelimCtx or ret loop GOutput.f
  //       when got X: push X GOutput
  //
  // We use a per-call output stack.
  dyn *saved_go = p->go;
  p->go = 0;
  parse_semicolon(p);
  for (;;) {
    dyn x = parse_delim(p, delim_ctx);
    if (!x) {
      // parse_delim returned 0 (it pushed to GOutput already and finished
      // a single delim), or parse_logic returned 0. Either way, break.
      break;
    }
    arrput(p->go, x);
  }
  // GOutput.f -- forward order. Our arrput is already forward.
  int gn = arrlen(p->go);
  dyn out;
  LIST(out, gn);
  for (int i = 0; i < gn; i++) LGET(out, i) = p->go[i];
  arrfree(p->go);
  p->go = saved_go;
  return out;
}

static dyn parse_tokens_inner(dyn input) {
  pstate_t p;
  p_init(&p, input);
  dyn xs = parse_xs(&p, 0);
  if (!p_end(&p)) {
    dyn tok = p_pop(&p);
    if (is_tok(tok) && is_opt(tok_value(tok))) {
      // Recovery: synthesize a nullary token and re-parse.
      // (parse_tokens line 449-457 fallback.) For now we just error.
    }
    parser_error("unexpected", tok);
  }
  p_free(&p);
  return xs;
}

// WORK-IN-PROGRESS. Not wired into reader.s text.parse yet -- the
// scaffold (parser state, op tables, precedence ladder, parse_term
// for literals/lists, parse_suf chain) is in place but parse_if,
// parse_bar, parse_offside, parse_logic's LL(1) hack, parse_delim's
// offside coupling, and parse_xs's named-loop semantics are stubs
// that error out. Calling this on real input will segfault or hit
// an "not yet ported" error. See issues.md B8 for what's left.
//
// Kept reachable as `parse_tokens_c_` builtin only so test harnesses
// can probe it; the production text.parse still uses Symta.
dyn reader_parse_tokens(dyn input) {
  GC_DISABLE();
  kw_init();
  dyn r = parse_tokens_inner(input);
  GC_ENABLE();
  return r;
}

// ====================================================================
// parse_table -- reader.s:100-123.
// Walks a STRIPPED list of items where each `[`!` K V]` form pairs
// a key with a value, returning [[K V] ...] for the table builder.
// ====================================================================

dyn reader_parse_table(dyn xs) {
  static dyn t_bang = 0;
  if (!t_bang) TEXT(t_bang, "!");

  GC_DISABLE();
  dyn *as = 0;
  dyn key = 0;
  dyn *val = 0;
  int cnt = 0;
  uint64_t n = is_list(xs) ? LIST_SIZE(xs) : 0;

  for (uint64_t i = 0; i <= n; i++) {
    int flush = 0;
    int start_kv = 0;
    dyn x = 0;
    if (i == n) flush = 1;
    else {
      x = LGET(xs, i);
      // Check for [`!` K @rest] form
      if (is_list(x) && LIST_SIZE(x) >= 2 &&
          text_eq_c(LGET(x, 0), t_bang)) {
        flush = 1;
        start_kv = 1;
      }
    }

    if (flush && cnt) {
      // push_kv:
      int vn = arrlen(val);
      dyn vlist;
      if (vn == 0) {
        // Val = [1]
        LIST(vlist, 1);
        dyn one; LDFXN(one, 1);
        LGET(vlist, 0) = one;
      } else if (vn == 1) {
        // Val.0
        vlist = val[0];
      } else {
        LIST(vlist, vn);
        for (int j = 0; j < vn; j++) LGET(vlist, j) = val[j];
      }
      dyn pair; LIST(pair, 2);
      LGET(pair, 0) = key;
      LGET(pair, 1) = vlist;
      arrput(as, pair);
      arrfree(val); val = 0;
    }

    if (start_kv) {
      cnt++;
      key = LGET(x, 1);
      // case Key [`!` K]: push `[]` Val; Key = K
      if (is_list(key) && LIST_SIZE(key) == 2 &&
          text_eq_c(LGET(key, 0), t_bang)) {
        static dyn t_brackets = 0;
        if (!t_brackets) TEXT(t_brackets, "[]");
        arrput(val, t_brackets);
        key = LGET(key, 1);
      }
    } else if (!flush) {
      arrput(val, x);
    }
  }

  dyn r = make_list_from_arr(as);
  arrfree(as);
  if (val) arrfree(val);
  GC_ENABLE();
  return r;
}

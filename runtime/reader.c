// reader.c -- C port of symta/src/reader.s.
//
// Token-list -> AST parser. Tokens come from tokenize.c (already C),
// the AST shape matches what compiler.s / macro.s consume.
//
// Migration strategy (see issues.md B8):
//   * Each public symbol of reader.s is mirrored by a C entry
//     declared in reader.h and exposed as a builtin from bltin.c
//     (e.g. `add_bars_c_`).
//   * reader.s is patched to dispatch through those builtins so the
//     `text.parse` entrypoint and method-style calls keep working.
//   * Once the suite is green, the Symta implementation can be
//     deleted.
//
// Token layout (T_TOK objects, 7 slots):
//   0 type   1 value   2 pchar   3 row   4 col   5 orig   6 parsed

#include "common.h"
#include "ng.h"
#include "reader.h"

// ----------------------------------------------------------------------
// Token introspection helpers.
// ----------------------------------------------------------------------

static inline int is_tok(dyn o)     { return O_TAG(o) == T_TOK; }

static inline dyn tok_type(dyn o)   { return LGET(o, 0); }
static inline dyn tok_value(dyn o)  { return LGET(o, 1); }
static inline dyn tok_row(dyn o)    { return LGET(o, 3); }
static inline dyn tok_col(dyn o)    { return LGET(o, 4); }
static inline dyn tok_orig(dyn o)   { return LGET(o, 5); }

// Token types from tokenize.c are interned fixtexts most of the
// time, but user-supplied keywords may be bigtexts; cover both.
static inline int text_eq_c(dyn a, dyn b) {
  if (a == b) return 1;
  if (IS_BIGTEXT(a) && IS_BIGTEXT(b)) return texts_equal(a, b);
  return 0;
}

// Build a `|` separator token mirroring [Row Col Orig].
// token_() leaves slot 2 (pchar) and slot 6 (parsed) uninitialised --
// gc_alloc returns dirty memory. The Symta-side `add_bars` passes
// parsed=0 explicitly, so do the same here, plus set pchar to 0.
// If we leave parsed as garbage, parse_term short-circuits on
// `when Tok.parsed: ret Tok` and the parser fails downstream.
static dyn make_bar(dyn row, dyn col, dyn orig) {
  static dyn pipe_text = 0;
  if (!pipe_text) { TEXT(pipe_text, "|"); }
  dyn t = token(pipe_text, pipe_text, row, col, orig);
  LGET(t, 2) = 0;  // pchar
  LGET(t, 6) = 0;  // parsed
  return t;
}

// ----------------------------------------------------------------------
// add_bars -- mirror of reader.s:30-42.
// ----------------------------------------------------------------------

dyn reader_add_bars(dyn xs) {
  static dyn t_pipe, t_then, t_else, t_elif;
  if (!t_pipe) {
    TEXT(t_pipe, "|");
    TEXT(t_then, "then");
    TEXT(t_else, "else");
    TEXT(t_elif, "elif");
  }

  GC_DISABLE();
  uint64_t n = LIST_SIZE(xs);
  dyn *out = 0;
  int first = 1;
  for (uint64_t i = 0; i < n; i++) {
    dyn x = LGET(xs, i);
    if (is_tok(x)) {
      dyn type = tok_type(x);
      int is_continuation =
            text_eq_c(type, t_pipe)
         || text_eq_c(type, t_then)
         || text_eq_c(type, t_else)
         || text_eq_c(type, t_elif);
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

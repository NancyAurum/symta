# Reader consolidation: weak-meta + C reader switchover

Plan for retiring the Symta-side parser (`src/core_.s` lines ~1132–1604)
in favour of the C reader (`runtime/reader.c`), addressing the two
long-standing blockers (READER-2 + READER-3) that previous attempts
have hit.

## Why now

`runtime/reader.c` is ~1982 lines.  Today only ~35 of those lines
(`reader_add_bars`, used by `text.parse`) run during a bootstrap.
The rest is shadow code — built, kept compiling, validated for parity
by `tests/runtime/parser-compare.s` (20/20), but never actually called
in production.  It exists specifically to replace the Symta-side
parser; what's blocking the switch is the `meta` source-position
wrapping, which the C `parse_strip_c_` can't currently produce.

## Why meta is the blocker

Today `meta A B` is implemented as a wrapper type:

```symta
type meta.~ O M: object_!O meta_!M
meta.__ Method Args = Args.0 = $object_; Args.apply_method(Method)
```

`parse_strip` calls `meta Ys [row col file]` on every non-empty list
whose head is a token, so every meta-bearing AST node is a 2-field
`meta` struct that forwards method dispatch through `__`.  The whole
codebase (compiler, macroexpand, error reporter) reads through that
forwarding transparently.

The C reader **cannot** allocate this wrapper without either calling
back into Symta or duplicating the meta-type constructor in C, both
of which are messy.  The pragmatic fix is to invert the
representation:

- For **heap-allocated** objects (lists, tags, closures, …) — store
  meta in a **weak hashtable** keyed by object identity.  The AST
  node stays a plain list; nothing wraps it.
- For **immediate** values (int, fixtext, No, T_TAG, …) — keep the
  wrapper type, because immediates have no GC identity and a single
  `5` could legitimately appear at many source positions with
  different metas.  (In practice the parser doesn't attach meta to
  immediates today, so this case is mostly theoretical, but it
  preserves semantics.)

After this change `parse_strip_c_` can attach meta with a single C
call (`meta_set_(obj, src)`), and Symta's `parse_strip` does the
exact same thing — producing byte-identical output across the two
paths, which is what unblocks the switchover.

## Phase 1 — weak hashtable type in C

New runtime type `T_WH` (`wh_t`), backed by `dh.h`:

```c
typedef struct wh_t {
  /* placed by the GC into the heap block so the table moves with
   * its generation, like other heap objects */
  uint32_t age;       /* O_AGE bit-shared with other heap objects */
  uint32_t cap;       /* slot count (power of 2) */
  uint32_t used;      /* live entries */
  wh_slot_t *slots;   /* {key:dyn, val:dyn, hash:u32} packed, malloc'd */
} wh_t;
```

Builtins:
- `weak_table_` → fresh empty table
- `wh.set Key Val` → add/update
- `wh.get Key` → value or `No`
- `wh.has Key` → 0/1
- `wh.del Key`
- `wh.n` → live entry count

GC integration in `gc.c`:
- Each `hg_t` keeps a list of weak tables alive in that generation
  (`weak_tables` field next to `finalizers`, `magnets`).
- After the marking phase, before generation cleanup, walk each
  weak table's slots:
  - If `O_AGE(key) == GC_MOVED` → key still reachable; forward both
    key and val via `GC_REC`, keep entry.
  - Else → drop entry (decrement `used`, mark slot empty).
- Robin-Hood layout means deletion just leaves a tombstone; lookup
  remains correct.  Compact on grow.

## Phase 2 — `meta_get` / `meta_set_` builtins

A single global weak table for parser meta (call it
`api.meta_table`), allocated at runtime startup.  Two builtins:

- `meta_set_ Obj Src` — stores `Src` keyed by `Obj`'s identity.  For
  immediates falls back to constructing a `meta` wrapper (Phase 4
  handles this via the Symta side).
- `meta_get Obj` — looks up; returns `Src` or `No`.

These are leaf C functions, no allocation on hit path beyond the
table grow.

## Phase 3 — Symta-side `meta` becomes a function

Replace:

```symta
type meta.~ O M: object_!O meta_!M
_.meta_ = No
meta.__ Method Args = Args.0 = $object_; Args.apply_method(Method)
```

with:

```symta
// For heap objs: stash (Obj -> Src) in the global table; return Obj
// unchanged.  For immediates: wrap in the old type so .meta_ still
// resolves through __ forwarding -- but the parser never hits this
// path (no token's parsed value is an immediate in a context where
// meta gets attached).
type meta_wrapper.~ O M: object_!O meta_!M
meta_wrapper.__ Method Args = Args.0 = $object_; Args.apply_method(Method)

meta Obj Src =
| if Obj.has_identity_                  // builtin: true for heap objs
  then meta_set_ Obj Src; Obj
  else meta_wrapper Obj Src

// The legacy `.meta_` getter now consults the table first, then
// falls back to the wrapper field.  Compiler / macro / ui_old can
// keep their existing `.meta_` reads unchanged.
_.meta_ = meta_get Me
meta_wrapper.meta_ = $meta_   // wrapper still has its own meta_ field

// `is_meta` is true for either the wrapper OR a plain object that
// has an entry in the table.
_.is_meta = if Me.is_meta_wrapper then 1 else got (meta_get Me)
```

This keeps the existing consumers (`compiler.s:300`, `compiler.s:343`,
`compiler.s:595`, `compiler.s:620`, `macro.s:1666`, `macro.s:1697`,
`macro.s:1700`, `macro.s:2033`, `src/ui_old/m.s` — all reading
`.meta_` or testing `.is_meta`) working with no source change.

## Phase 4 — `parse_strip` (Symta) attaches meta through the new API

Mechanical:

```symta
parse_strip X =
| if X.is_token
  then | P X.parsed
       | if P then parse_strip P.0 else X.value
  else if X.is_list
  then | less X.n: ret   X
       | Head X.head
       | Meta when Head.is_token: Head.src
       | Ys map V X: parse_strip V
       | when got Meta and Meta.2 <> '<none>': meta_set_ Ys Meta
       | Ys                                    // ← no wrapper!
  else X
```

Run `tests/runtime/lineno-check.sh` (5 cases) to confirm stack
traces still resolve line numbers correctly.  Bootstrap drift must
stay at fixed point.

## Phase 5 — `parse_strip_c_` does the same

In `runtime/reader.c`, `reader_parse_strip` returns the same plain
list and calls a new `meta_set_c_(obj, src)` helper before returning
the list.

## Phase 6 — flip the switch in `text.parse`

```symta
text.parse Src!'<none>' LexP!No =
| Text Me
| when got LexP: Text = lexical_macro_expand Src Text LexP
| R parse_strip_c_: parse_tokens_c_: add_bars_c_: Text.tokenize(Src)
| if R.end then [[]] else R.0.tail
```

Now the AST that comes back is structurally identical to what the
Symta-side path produced (the meta table being a side effect).

## Phase 7 — delete dead code

Remove from `src/core_.s` lines 1161–1604 (the parser block) and
the now-unused helpers, per the May 2026 audit:
`add_bars`, `parser_error`, `expect`, `is_next`, `parse_if`,
`parse_bar`, `parse_table`, `UnaryOps`, `parse_term`, `DelimOps`,
`is_delim`, `OpsT`, `list.ops_lookup_`, `tbl.ops_lookup_`,
`parse_op`, `ParenChars`, `is_anchor_char`, `SufUnary*`, `SufOps`,
`binary_loop`, `parse_binary`, `DollarList`, `parse_dollar`,
`DisjointChars`, `next_is_disjoint`, `parse_suf_*`, `parse_suf`,
`is_eol`, `parse_unary`, `PrefixList`, `parse_prefix`, `parse_pow`,
`parse_mul`, `parse_add`, `parse_sufs`, `parse_b_shift`,
`parse_b_and`, `parse_b_xor`, `parse_comma`, `BoolList`,
`parse_bool`, `parse_blogic`, `parse_excl_loop`, `parse_exclamation`,
`CndList`, `parse_logic`, `is_var_tok`, `parse_offside`, `DelimList`,
`parse_delim`, `parse_semicolon`, `parse_xs`, `parse_tokens`,
`parse_strip`, plus `tok.retok` and `token_is` and the `token`
constructor.

Keep: `_.is_token`, `tok.is_token`, `tok.src`, `tok.=src`,
`tok.as_text`, `normalize_folder`, `resolve_src_path`,
`lexical_macro_expand`, `text.parse`, `text.sexp`.

## Test gates between phases

| After phase | Required to pass |
|---|---|
| 1 | `make runtime`, no Symta changes yet — runtime still bootstraps. |
| 2 | `meta_set_` / `meta_get` callable; smoke test from REPL. |
| 3 | All Symta sources unchanged (still use wrapper); no behavioural change. |
| 4 | `lineno-check.sh` 5/5; drift PASS; full sweep green. |
| 5 | `parser-compare.s` still 20/20. |
| 6 | Drift PASS; **all 13 sweep lines green**.  Crash from READER-3 reproduction script (`/tmp/test_build` with `04-lists.s`) must succeed. |
| 7 | All sweep lines green; code shrinks ~440 lines from `core_.s`. |

## Phase boundaries are commit boundaries

Each phase is a single commit.  Phases 1–3 are independently
useful (weak tables are a generally-useful runtime type) and can
land as a stack without changing the parser's behaviour at all.
Phases 4–7 are the actual switchover and can be reverted as a
group if something downstream depends on the wrapper representation
in a way the audit missed.

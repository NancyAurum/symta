# Reader consolidation: how the Symta-side parser was retired

Done as of commit 2129767.  Historical record of how `src/core_.s`'s
~448-line Symta-side parser block was retired in favour of
`runtime/reader.c`'s C implementation, plus the false start that
got reverted.

## Where things landed

`text.parse` runs the entire pipeline through C now:

```symta
text.parse Src!'<none>' LexP!No =
| Text Me
| when got LexP: Text = lexical_macro_expand Src Text LexP
| R parse_strip_c_: parse_tokens_c_: add_bars_c_: Text.tokenize(Src)
| if R.end then [[]] else R.0.tail
```

Each list whose head was a token with a real source position comes
back wrapped in the `type meta.~ O M: object_!O meta_!M` Symta type.
The wrapping is done by `reader_parse_strip` in C — it interns the
`meta` type tag once at the C level, then `OBJECT(w, meta_tag, 2)`
allocates a fresh 2-slot heap struct holding (stripped_list, src)
directly.  `find_meta_tag()` returns -1 if `type meta` hasn't been
registered yet, in which case the wrapper is skipped — that path
exists so transitional bootstraps (whose SBCs only know the prior
name) can run.

The Symta-side parser block in `core_.s` (lines 1161–1604 in the
pre-consolidation source: `parse_term`, `parse_dollar`,
`parse_offside`, `parse_if`, `parse_xs`, `parse_tokens`,
`parse_strip`, plus ~30 helpers and operator-bag definitions) is
gone.  `core_.s` retains:

- `_.is_token = 0` / `tok.is_token = 1` — token discriminator
- `tok.src =: $row $col $orig` and the setter — field accessors
- `tok.as_text` — pretty-printer
- the `type token` doc comment — spec for the C side
- `type meta.~ O M: object_!O meta_!M` — the wrapper type itself,
  `meta.__` forwarder, `_.meta_ = No` default

`runtime/reader.c` exposes four builtins
(`add_bars_c_`, `parse_tokens_c_`, `parse_strip_c_`, `parse_table_c_`).
`text.parse` calls the first three.

## False start: the weak hashtable

The first attempt (Phases 1–6 in the original plan, commits
`3229299` through `07b10dc`) replaced the wrapper type with a
**weak hashtable** keyed by GC identity.  `parse_strip_c_` did
`meta_set_(list, src)`, and `_.meta_` looked the value up via
`meta_get_`.  The idea: AST nodes stay as plain lists, no struct
intercepting method dispatch; the per-node lookup overhead would
be small because `ih.h` does Murmur3 + Robin-Hood probing in
~50 ns.

It worked functionally — full sweep green, drift PASS — but the
game benchmark cold compile blew up from 51-54 s baseline to
**90-120 s**.  The compiler / macroexpander read `.meta_` and
`.is_meta` on every AST node, and even a fast hashtable lookup
multiplied by tens of millions of accesses is much slower than a
field load from a 2-slot struct.  Direct C-side method dispatch
(commit `785d890`) clawed back ~25 % of that but still left a
1.7× regression vs baseline.

So the diagnostic Nancy asked for: revert to wrapper-meta and see
where the bench lands.

## Result of the diagnostic

Wrapper-meta + C-side allocation: **38-40 s cold**, ~25 % faster
than the pre-consolidation Symta-side parser.  The C reader is
genuinely faster; the regression was entirely from `_.meta_`
hashtable lookups in the hot path.  Hence this is the design we
keep.

| Snapshot | Cold | Warm |
|---|---|---|
| Pre-consolidation (Symta parser, wrapper meta) at `0b9e2c4` | 51-54 s | 0.15 s |
| Phase 5 first measurement (C reader, weak-table meta, Symta-side wrapper getter) | 120 s | 0.23 s |
| Phase-5b (C reader, weak-table meta, C-side direct dispatch) at `785d890` | 93-95 s | 0.14 s |
| **Current** (C reader, wrapper meta, C-side allocation) at `2129767` | **38-40 s** | 0.24 s |

## Files removed during the revert (commit 2129767)

- `runtime/meta_table.h`
- `runtime/meta_table.c`
- `runtime/gc.c` weak-meta GC sweep block
- `runtime/bltin.c` `meta_attach_` / `meta_lookup_` / `meta_present_`
  builtins and their B() entries
- `runtime/main.c` `add_method_r` redefinition special-case
- `runtime/symta.h` `api.m_meta_under_` / `api.m_is_meta` fields
- Makefiles' `meta_table.c` entries

## Why the wrapper isn't a method-dispatch problem

The original concern with wrapper-meta — that every method call
on a wrapped AST node has to forward through `meta.__` — is real
but cheap.  `__` does:

```symta
meta.__ Method Args =
  Args.0 = $object_
  Args.apply_method(Method)
```

Args.0 is replaced with the wrapped object_, then `apply_method`
dispatches.  One extra MCALL per access through a wrapped node.
The compiler+macroexpander do this constantly, but each
"forwarding tax" call is faster than a hashtable lookup because
the method dispatch path is already heavily optimised (RT-7's
mcache, RT-5's inlined hook).

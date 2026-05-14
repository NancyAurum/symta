# NCM — Lexical Macro Processor

NCM ("Nancy's C Macros") is Symta's text-rewriting macro
processor.  Source: [`runtime/ncm.h`](../runtime/ncm.h).

It runs **before** the parser and produces text.  Symta's
*other* macro system (Lisp-style, AST-rewriting) is described
in `examples/12-macro.s` and runs after parsing — that one
sees structured forms.  NCM sees raw bytes.

Reached from Symta code via the
`ncm_process_ Filename Text Paths` builtin.  Also runs
automatically on module headers (`*.h`) imported with
`use <name>` (e.g. `use rgb` pulls `src/rgb.h` through NCM
before treating its body as Symta source).

Macro character: `#`.

A worked tour lives in [`examples/25-lexmacro.s`](../examples/25-lexmacro.s)
+ its golden output.  This doc is the compact reference.

---

## Directives

### Conditional inclusion

| Form | Meaning |
|------|---------|
| `#if expr`    | If `expr` (an `#[expr]`-evaluated integer) is non-zero, emit the following block; else skip. |
| `#elif expr`  | Like `#if`, after a preceding `#if` whose condition was false. |
| `#else`       | Emit if all preceding `#if`/`#elif`s in the chain failed. |
| `#endif`      | Close an `#if` chain. |

The condition is evaluated with the same expression
language as `#[expr]` (see below).

### Macro lifecycle

| Form | Meaning |
|------|---------|
| `#NAME body`               | Define `NAME` to expand to `body`. |
| `#NAME(arg1, arg2) body`   | Define a function-style macro. |
| `#NAME(arg=default) body`  | Trailing args with `=default` may be omitted at the call site. |
| `#NAME([Xs]) body`         | Variadic — `Xs` captures everything after previous formal as one text blob. |
| `#NAME({Body}) body`       | Body arg — the next `{ ... }` block after the call binds to `Body`. |
| `#undef NAME`              | Remove `NAME` from the macro table. |

Macro bodies may reference other macros, which expand
recursively.  Set `#:#` immediately before a definition to
opt the body out of recursive expansion (rare; useful when a
body should pass through to a downstream macro processor
unchanged).

### File inclusion

| Form | Meaning |
|------|---------|
| `#:NAME`         | Search the include paths (third arg to `ncm_process_`) for `NAME.h` and splice its expansion in here. |
| `#:include NAME` | (currently passes through — use `#:NAME` instead) |
| `#once`          | If this file has been included more than once, skip to end.  Standard header-guard idiom. |
| `#multi`         | Counterpart to `#once` — explicitly opt **out** of the "skip duplicate includes" default for `.h` files. |
| `#line N "FILE"` | Passes through verbatim (NCM doesn't process these; the downstream C preprocessor / Symta line tracker consumes them). |

### Expression evaluation

| Form | Meaning |
|------|---------|
| `#[expr]`        | Evaluate `expr` as an integer and splice the decimal text in. |
| `#("fmt" args)`  | printf-style format.  See "Format directives" below. |
| `#<NAME>`        | Force expansion of `NAME` even where it would otherwise be ambiguous (e.g. concatenating onto a longer identifier). |

### Escaping

| Form | Meaning |
|------|---------|
| `##`             | Emit a literal `#` (does not start a directive). |
| `#\NAME`         | Pass `#NAME` through literally (for downstream macro processors). |
| Inside `"..."`   | `#` is left alone in quoted strings. |

### Scope

| Form | Meaning |
|------|---------|
| `#{ ... }` | Open a fresh macro scope.  Definitions inside don't leak; the parent's definitions are visible.  Useful for hygienic local helpers. |

---

## `#[expr]` expression language

Integers; operator precedence from tightest to loosest:

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| highest    | `( )`, `NAME`, `0x...`, `log2(x)`, decimal literal | n/a |
|            | `- + ~ !` (unary)               | right |
|            | `* / %`                         | left |
|            | `+ -`                           | left |
|            | `<< >>`                         | left |
|            | `< > <= >=`                     | non-assoc (chain → first comparison wins) |
|            | `== !=`                         | left |
|            | `& \| ^`                        | left |
| lowest     | `&& \|\|`                       | left |

Notes:

- All arithmetic is 32-bit signed (`S32`).
- Comparison results are 0/1.
- Logic operators `&&`/`||` are short-circuit and return 0/1.
- `log2(x)` returns ⌊log₂(x)⌋, useful for `#[log2(1024)]` = 10.
- Identifiers in the expression are expanded as macros first;
  `#A 3 ... #[A*A]` evaluates to 9.

Historical landmines (now fixed; pinned by `examples/25-lexmacro.s`):

- **NCM-1** (May 2026): `<<` and `>>` used to drop one
  character before parsing the operand, so `#[5<<2]`
  returned a silently-wrong value.
- **NCM-2** (May 2026): `&&` and `||` used to be eaten by
  `eval_bitwise` as if they were bitwise `&`/`|`, so
  `#[1 && 1]` returned 0.

If a new operator is added, mirror the pop-then-peek-then-
`unpop_char` pattern used by `eval_cmp` (`<<` / `>>`) and the
re-fixed `eval_bitwise` (`&&` / `||`) so multi-char operators
don't get split mid-parse.

---

## Format directives — `#("fmt" args...)`

A printf-like formatter, run at expansion time.  Args are
plain text tokens after the format string.

Supported directives:

| Spec | Meaning |
|------|---------|
| `%d` | Decimal integer.  Arg may be a bare decimal, `+N`, `-N`, or `0xHEX`. |
| `%x` | Hex integer, **upper-case** output. |
| `%X` | Hex integer, **lower-case** output. |
| `%s` | String (any text arg). |
| `%%` | Literal `%`. |

Flags between `%` and the type:

| Flag | Meaning |
|------|---------|
| `0`        | Pad with zeros. |
| `+`        | Always emit sign on positive numbers. |
| `-`        | Left-justify. |
| `?C`       | Pad with character `C`. |
| width      | Decimal number — pad/truncate to this width. |

Numeric args accept `0x` / `0X` prefix for hex input (added
in NCM-3, May 2026 — before that, `#("%d" 0xFF)` parsed only
the leading `0`).

A note on the **case inversion** (`%x` → upper, `%X` →
lower): this is intentionally inverted from C's `printf`
convention.  The reasoning is buried in the original NCM
prototype; current code matches the convention and existing
golden files rely on it (see `examples/.expected/25-lexmacro.out`
where `0x%02X%02X%02X` outputs lowercase `0xffa040`).  Don't
"fix" it without an audit of every NCM-using header.

---

## Symbol expansion `#<NAME>`

Plain identifier `NAME` is expanded only when followed by a
non-identifier character.  When you need to splice a macro
body into an identifier — e.g. building a class-method name
`xxx_NAME_yyy` where `NAME` is a macro — use `#<NAME>` to
force expansion:

```
#TYPE int

   xxx_TYPE_yyy           // NOT expanded -- looks like a single identifier
   xxx_#<TYPE>_yyy        // expands to xxx_int_yyy
```

---

## Where the dispatcher lives

| Source line | What |
|-------------|------|
| `runtime/ncm.h::process_file`    | The main directive loop.  Each `#X` is dispatched from here. |
| `runtime/ncm.h::process_scope`   | Body-scope expansion (called by `#if`, `#{...}`, etc.). |
| `runtime/ncm.h::eval_expr`       | Top of the `#[expr]` parser.  Calls `eval_logic`. |
| `runtime/ncm.h::eval_logic`      | `&& \|\|` |
| `runtime/ncm.h::eval_bitwise`    | `& \| ^` |
| `runtime/ncm.h::eval_cmp`        | `< > <= >= == !=` |
| `runtime/ncm.h::eval_shift`      | `<< >>` |
| `runtime/ncm.h::eval_add`        | `+ -` |
| `runtime/ncm.h::eval_mul`        | `* / %` |
| `runtime/ncm.h::eval_unary`      | `- + ~ !` |
| `runtime/ncm.h::eval_atom`       | identifier, literal, `( )`, `log2(...)` |
| `runtime/ncm.h::cmd_fmt`         | `#("fmt" ...)` formatter |
| `runtime/ncm.h::expand_file2`    | `#:NAME` include resolver |
| `runtime/ncm.h::macroexpand2`    | The recursive macro-body expander |
| `runtime/ncm.h::add_macro`       | Macro-table insert (called for every `#NAME body`) |

For a tour-by-example see [`examples/25-lexmacro.s`](../examples/25-lexmacro.s);
for the full regression matrix see the new section 11 in the
same file (NCM-1 / NCM-2 / NCM-3 probes).

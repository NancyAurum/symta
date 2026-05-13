# dev/

Scratch space for Symta development: raw notebooks, language-design
ideas, the long-form tutorial draft, and editor-tooling assets.
Nothing here is required to build or run Symta — it's the
"my-engineer-notebook" half of the repo.

If you're trying to *use* Symta, see [`../README.md`](../README.md);
if you're trying to *build* it, see [`../BUILDING.md`](../BUILDING.md);
if you want the high-level "how it works", see
[`../architecture.md`](../architecture.md); if you want the open
punch-list, see [`../TODO.md`](../TODO.md).

---

## Living documents

| File | What it's for |
|------|---------------|
| [`todo.txt`](todo.txt) | Raw stream-of-consciousness notebook; precursor to items that graduate into [`../TODO.md`](../TODO.md). |
| [`todo-spell-of-mastery.txt`](todo-spell-of-mastery.txt) | Friction list collected while building SoM on top of Symta — the ergonomic gaps a real project exposed. Items here feed the **R-4** entry in [`../TODO.md`](../TODO.md). |

## Long-form notes (still load-bearing)

| File | Topic |
|------|-------|
| [`sbe.txt`](sbe.txt) | "Learn Symta by Example" — the long-form tutorial that the `examples/*.s` files are the executable companion to. ~2,700 lines. Kept in dev/ until it's polished enough to publish at the top level. |
| [`cls.txt`](cls.txt), [`cls-gc.txt`](cls-gc.txt) | Why the Symta ECS (`cls` / `dsm` / IPS) is shaped the way it is, and how entity lifetime interacts with the generational GC. Background for understanding the adaptive-map design in `runtime/am.h`. |
| [`ideas.txt`](ideas.txt) | Language-design ideas that haven't crystallised into TODO items yet. Brainstorm, not commitments. |

## Assets

| File | Use |
|------|-----|
| [`logo.jpg`](logo.jpg), `logo.xcf.zip` | Project logo + the editable GIMP source. |
| [`flt32bit.svg`](flt32bit.svg), [`flt64bit.svg`](flt64bit.svg) | IEEE-754 layout diagrams used in talks / docs. |
| [`npp_symta.xml`](npp_symta.xml) | Notepad++ language definition file for `.s` files. Import via Settings → Style Configurator. |
| [`BookmarksInOpenDocsList.py`](BookmarksInOpenDocsList.py) | Notepad++ PythonScript companion: lists every bookmarked line across open docs in a clickable view. |

---

## What used to live here (and where it went)

Files that were checked in earlier but no longer match the project:

- `dev.txt` — old gotchas (strict aliasing, gdb tips), plus a
  draft of a 5-bit-per-char fixtext encoding that was never
  built. The gdb tips have moved into [`../BUILDING.md`](../BUILDING.md);
  the gotchas are now either fixed or documented in the
  architecture doc. The fixtext sketch was never integrated and
  the current short-text representation does what we need.
  **Dropped.**
- `nib.txt` — speculative tape-VM brainfuck variant with
  xor-shift instruction hashing. Never built. The actual VM is
  the well-understood threaded interpreter in
  [`../runtime/sbc.c`](../runtime/sbc.c). **Dropped.**
- `compression.txt` — five-line sketch. Now superseded by the
  full implementation in [`../examples/26-huffman.s`](../examples/26-huffman.s).
  **Dropped.**
- `string-encoding.txt` — bit-packing C snippets from an
  exploratory phase. The encoding sketch never landed in
  production text handling. **Dropped.**
- `gfx.md` — TOC stub that was never completed. The actual
  `gfx` surface is documented inline in
  [`../src/gfx.s`](../src/gfx.s) and [`../src/store.s`](../src/store.s),
  which is where developers will look anyway. **Dropped.**
- `ui_isotest.s` — moved to [`../examples/31-isometric/`](../examples/31-isometric/)
  and turned into a proper tutorial example, also exercised by
  the UIM regression suite.
- `ui_test1.s` / `ui_test2.s` / `ui_test3.s` — removed; the
  public-facing UI showcase is [`../examples/16-ui/`](../examples/16-ui/)
  and the regression cases live under [`../tests/uim/src/`](../tests/uim/src/).

If you're looking for one of those dropped files in a historical
context, `git log -- symta/dev/<filename>` will find the last
revision before removal.

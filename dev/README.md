# dev/

Scratch space for Symta development: design notes, raw todos, tooling
helpers, and assets that don't belong in the user-facing parts of the
project. Nothing here is required to build or run Symta — it's the
"my-engineer-notebook" half of the repo.

If you're trying to *use* Symta, see [`../README.md`](../README.md);
if you're trying to *build* it, see [`../BUILDING.md`](../BUILDING.md);
if you want the high-level "how it works", see
[`../architecture.md`](../architecture.md).

---

## Living documents

| File | What it's for |
|------|---------------|
| [`TODO.md`](TODO.md) | Curated punch-list with concrete fixes per item (P0..P3, effort tag). The one to read first. |
| [`todo.txt`](todo.txt) | Raw stream-of-consciousness notebook; precursor to items that graduate into `TODO.md`. |
| [`todo-spell-of-mastery.txt`](todo-spell-of-mastery.txt) | Friction list collected while building SoM on top of Symta — the ergonomic gaps a real project exposed. |

## Design notes (historical, but still load-bearing)

| File | Topic |
|------|-------|
| [`cls.txt`](cls.txt), [`cls-gc.txt`](cls-gc.txt) | Why the Symta ECS (`cls` / `dsm` / IPS) is shaped the way it is, and how entity lifetime interacts with the generational GC. |
| [`nib.txt`](nib.txt) | Sketch ideas for a tape-VM variant (xor-shift instruction hashing, brainfuck-style addressing); never built, kept as a brainstorm. |
| [`string-encoding.txt`](string-encoding.txt) | Bit-packing helpers for the variable-width string representation. |
| [`compression.txt`](compression.txt) | Five-line sketch of the huffman-with-rare-char escape used in `26-huffman.s`. |
| [`dev.txt`](dev.txt) | Cross-cutting gotchas (strict aliasing, alignment, etc.) hit during runtime development. |
| [`ideas.txt`](ideas.txt) | Language-design ideas that haven't crystallized into RFCs yet. |
| [`gfx.md`](gfx.md) | Stub for a `gfx` package reference. Incomplete; expand if you touch the package. |

## Assets

| File | Use |
|------|-----|
| [`logo.jpg`](logo.jpg), `logo.xcf.zip` | Project logo + the editable GIMP source. |
| [`flt32bit.svg`](flt32bit.svg), [`flt64bit.svg`](flt64bit.svg) | IEEE-754 layout diagrams used in talks / docs. |
| [`npp_symta.xml`](npp_symta.xml) | Notepad++ language definition file for `.s` files. Import via Settings → Style Configurator. |
| [`BookmarksInOpenDocsList.py`](BookmarksInOpenDocsList.py) | Notepad++ PythonScript companion: lists every bookmarked line across open docs in a clickable view. |

## Tutorial draft

| File | Status |
|------|--------|
| [`sbe.txt`](sbe.txt) | "Learn Symta by Example" — the long-form tutorial that the `examples/*.s` files are the executable companion to. Kept in dev/ until it's polished enough to publish at the top level. |

---

## What used to live here (and where it went)

- `ui_isotest.s` — moved to [`../examples/31-isometric/`](../examples/31-isometric/)
  and turned into a proper tutorial example (small isometric game scaffold,
  also exercised by the UIM regression test suite).
- `ui_test1.s` / `ui_test2.s` / `ui_test3.s` — removed; the upstream
  copies live under [`../../game/src/`](../../game/src/) (in the SoM
  game tree), and the public-facing UI showcase is
  [`../examples/16-ui/`](../examples/16-ui/) which descends from
  `ui_test1.s` with comments added.

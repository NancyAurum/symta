# dev/

Long-form notes and editor-tooling assets that don't ship with a
binary but support the day-to-day work of developing Symta.
Nothing here is required to build or run Symta — it's the
"my-engineer-notebook" half of the repo.

If you're trying to *use* Symta, see [`../README.md`](../README.md);
if you're trying to *build* it, see [`../BUILDING.md`](../BUILDING.md);
if you want the high-level "how it works", see
[`../architecture.md`](../architecture.md); for the milestone
roadmap, see [`../TODO.md`](../TODO.md).

---

## Long-form notes (still load-bearing)

| File | Topic |
|------|-------|
| [`sbe.txt`](sbe.txt) | "Learn Symta by Example" — the long-form tutorial that the `examples/*.s` files are the executable companion to. ~2,700 lines. Kept in dev/ until it's polished enough to publish at the top level. |
| [`ncm.md`](ncm.md) | NCM ("New C Macroprocessor") — the lexer-stage macro pre-processor.  Design notes + reference. |
| [`cls.txt`](cls.txt), [`cls-gc.txt`](cls-gc.txt) | Why the Symta ECS (`cls` / `dsm` / IPS) is shaped the way it is, and how entity lifetime interacts with the generational GC. Background for understanding the adaptive-map design in `runtime/am.h`. |

## Bench / profiling

| File | Use |
|------|-----|
| [`bench-setup-windows.md`](bench-setup-windows.md), [`bench-setup-windows.ps1`](bench-setup-windows.ps1), [`bench-admin.bat`](bench-admin.bat) | Stable-clock setup for repeatable wall-time measurements on Windows: disables Turbo Boost, pins to high-performance plan, sets process priority.  Used when collecting numbers for compile-time / runtime regressions. |

## Assets

| File | Use |
|------|-----|
| [`logo.jpg`](logo.jpg), `logo.xcf.zip` | Project logo + the editable GIMP source. |
| [`flt32bit.svg`](flt32bit.svg), [`flt64bit.svg`](flt64bit.svg) | IEEE-754 layout diagrams used in talks / docs. |
| [`npp_symta.xml`](npp_symta.xml) | Notepad++ language definition file for `.s` files. Import via Settings → Style Configurator. |

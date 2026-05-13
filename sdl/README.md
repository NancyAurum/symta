# symta/sdl/

Runtime DLLs that have to sit next to a project's `go.exe` for the
`ui` FFI plugin to load. They are checked-in (vs built from source)
because:

- They are stock upstream binaries — exactly what you'd download
  from libsdl.org, with no SoM-side patches.
- Building SDL + SDL_mixer + their dependency tree (libvorbis,
  libogg, libFLAC, libmikmod, libmodplug, smpeg2) on Windows from
  scratch is a multi-hour exercise; not worth re-doing on every
  fresh clone.
- They live here once (~4 MB) instead of being duplicated into
  every example/test directory that uses `uim`.

## How they get staged

When a project's source uses the `ui` plugin (typically via
`use uim`), the `ffi_begin` macro in
[`src/macro.s`](../src/macro.s) detects this and copies every file
listed in `SDLBundle` from `symta/sdl/` into the project's build
directory (next to `go.exe`).

The copy is mtime-aware: the same `copy_ffi` helper used to stage
`ui.ffi` itself is reused, so subsequent compiles skip the copy
unless the source DLL is newer.

## When this needs touching

- **SDL version bump**: drop the new DLLs in here, run a smoke
  test on `examples/16-ui`.
- **New sidecar DLL**: append its filename to the `SDLBundle`
  list in `src/macro.s` (one source of truth).
- **Renderer swap** (away from SDL): delete the relevant DLLs and
  the `SDLBundle` entry; the macro auto-skips missing files.

## Provenance

Bundled with SDL2 2.0.x / SDL2_mixer 2.0.x release zips for
mingw-w64. No modifications.

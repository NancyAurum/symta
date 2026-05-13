# tests/gfx — headless gfx-FFI golden-image suite

Pure-Symta regression suite for the C `gfx` blitter (compiled via
`c/gfx/` into `ffi/gfx.ffi`). Each test builds a small image
programmatically, saves it to `out/<name>.png`, and exact-byte-diffs
against `golden/<name>.png`. No window, no SDL — runs anywhere the
runtime builds.

Sibling suites under `symta/tests/`:

- `tokenizer/`, `reader/`, `macros/` — language layers
- `runtime/`, `compiler/` — end-to-end + .sbc byte-equality
- `uim/` — widget rendering + synthetic interaction (needs SDL)
- `gfx/` (this) — blitter unit tests (no SDL)
- `bootstrap/` — 5-stage self-compile drift check


## Running

From the symta/ root:

```sh
make test-gfx          # build + run
bash tests/gfx/run.sh  # same, without invoking make
```

Or by hand:

```sh
cd tests/gfx && ../../symta.exe . && ./go.exe
```

Output:

- `out/<name>.png` for every test (always written)
- `Summary: 29 passed, 0 failed, 0 new`
- exit code 0 if everything passed, non-zero on any failure


## Promoting a new baseline

On first run for a new test, the harness reports `NEW` and writes the
PNG to `out/<name>.png`. **Inspect it visually**, then copy it into
`golden/<name>.png` to promote.

```sh
cp out/<name>.png       golden/<name>.png   # promote one
cp out/*.png            golden/             # promote all
bash run.sh --update                         # alias for the bulk copy
```

The diff is exact (zero tolerance). Test cases pick colours that hit
the most-trodden blitter paths: alpha blend, RLE, recolor, scissor,
gamma-corrected blend, scale, etc.


## Adding a test

Edit `src/cases.s`:

```symta
register \my_test: =>
  G gfx 32 32
  G.clear 0xFFFFFF
  // build the gfx however you like; return it.
  G
```

Recompile, run, inspect `out/my_test.png`, promote when satisfied.


## Alpha convention reminder

In this codebase the high byte of a pixel is **inverted alpha**:
`0x00` = fully opaque, `0xFF` = fully transparent. PNG save flips it
before writing, so a viewer sees the conventional opaque-at-FF
orientation, but code values use the inverted form. Source colours
for `blit` need to be opaque (high byte = `0x00`) to actually appear.

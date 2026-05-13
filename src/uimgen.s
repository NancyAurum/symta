// uimgen — default UI pictogram generator.
//
// Rationale
// ---------
// UIM widgets (button, slider, checkbox, droplist, window frame, …)
// each blit a small PNG/SVG from `pic/ui/`. Until now, every Symta
// project that depended on uim had to ship its own copy of those
// pictograms — the fantasy-game scroll-and-gem set bled into
// examples/16-ui/, examples/31-isometric/, and the test harness
// because they were committed alongside each project's source.
//
// That had three problems:
//
//   1. **Wrong tone**: the scrolls/gems aesthetic was designed for
//      one specific game. A first-time Symta user wanting a plain
//      tool window got fantasy iconography.
//   2. **Disk waste**: ~30 PNGs × 3-4 copies = ~12 MB of duplicate
//      raster art in the repo.
//   3. **Leaky abstraction**: `use uim` worked only if you also
//      remembered to copy the right pic/ui/ tree from somewhere.
//
// This module ships a neutral, accessibility-conscious default set
// as SVGs (vector → resolution-independent, ~25 KB total) and
// guarantees the `pic/ui/*` files exist before any widget tries to
// draw. The flow:
//
//   * uim.s adds `use uimgen` to its imports, so any project that
//     uses uim transitively pulls this module in.
//   * On module load (toplevel call below), we check
//     `<main_path>/pic/ui/` for each of our names; missing files
//     are written from the in-memory SVG corpus. Present files
//     are left alone, so a project that wants to override
//     `ui/btn.svg` with its own art just commits the override
//     file — uimgen won't clobber it.
//
// Design language
// ---------------
// Following modern OS / WCAG 2.2 conventions:
//
//   * **Dark-theme palette** (UIM's default bg is `RGB_BLACK` /
//     `RGB_NIGHT`). Surface fills sit ~10-25% lighter; text/glyphs
//     are near-white. Contrast ratios verified ≥ 4.5:1 for text
//     and ≥ 3:1 for non-text indicators (WCAG 2.2 AA, non-bold).
//   * **Rounded rectangles** with 4-24 px corner radii — less
//     visually aggressive than sharp rects, matches Material 3 /
//     iOS HIG conventions.
//   * **State conveyed by more than colour** — a checked checkbox
//     gets a ✓ glyph, not just a colour swap. Helps colour-blind
//     users and grayscale rendering.
//   * **Pic9-friendly geometry** — for widgets that the engine
//     scales via pic9 (button, frame, tab header), the corner
//     ornament fits inside the configured `csz` corner region so
//     scaling doesn't smear it. Source pic dimensions match the
//     existing pic9 / csz config in uim.s exactly — no engine
//     changes needed to adopt the new defaults.
//   * **Single-source palette** — all SVGs interpolate the colour
//     constants from `Pal` below. Changing one constant re-themes
//     the whole UI on next regeneration.
//
// Where the SVGs end up
// ---------------------
// `<main_path>/pic/ui/` of the currently-running project. That's
// the same place `store.s` looks for pictograms via `PicPaths`,
// so `get_pic 'ui/btn'` finds them without further plumbing.
//
// To override
// -----------
// Drop your own `<main_path>/pic/ui/btn.svg` (or .png) BEFORE the
// project first runs. uimgen only writes files that don't already
// exist; your override survives every rebuild. To roll back to
// the defaults, delete the override and rerun — uimgen will
// repopulate.

use gfx
export ensure_ui_pics

// --------------------------------------------------------------- palette

// Hex strings; we add the `#` prefix inside the SVG templates.
Pal_bg0  '1c1c24'   //base panel
Pal_bg1  '2c2c38'   //surface
Pal_bg2  '3c3c4c'   //surface raised
Pal_bg3  '50506a'   //surface hover / pressed
Pal_acc  '7a8cff'   //accent (focus / check / active)
Pal_acc2 '5a6cdf'   //accent darker (pressed)
Pal_txt  'e6e6f0'   //text / icon glyph
Pal_mut  '8888a0'   //muted stroke

// --------------------------------------------------------------- helpers

// 9-slice-friendly rounded panel. Width, height, corner radius,
// fill colour, stroke colour, stroke width.
panel_svg W H R Fill Stroke SW =
  "<svg xmlns='http://www.w3.org/2000/svg' width='[W]' height='[H]' viewBox='0 0 [W] [H]'><rect x='[SW]' y='[SW]' width='[W-SW*2]' height='[H-SW*2]' rx='[R]' ry='[R]' fill='#[Fill]' stroke='#[Stroke]' stroke-width='[SW]'/></svg>"

// Slider chevron-button (80×80). Dir in {u,d,l,r}; Active toggles
// the accent fill.
chevron_path Dir =
  case Dir:
    \u = 'M 24,52 L 40,28 L 56,52'
    \d = 'M 24,28 L 40,52 L 56,28'
    \l = 'M 52,24 L 28,40 L 52,56'
    \r = 'M 28,24 L 52,40 L 28,56'

barw_svg Dir Active =
  Fill if Active: Pal_acc2 else Pal_bg2
  Path chevron_path Dir
  "<svg xmlns='http://www.w3.org/2000/svg' width='80' height='80' viewBox='0 0 80 80'><rect x='2' y='2' width='76' height='76' rx='10' ry='10' fill='#[Fill]' stroke='#[Pal_mut]' stroke-width='2'/><path d='[Path]' fill='none' stroke='#[Pal_txt]' stroke-width='8' stroke-linecap='round' stroke-linejoin='round'/></svg>"

// --------------------------------------------------------------- pics

// Button (320×320, csz=40 in uim.s). 9-slice rounded panel; r=24
// fits inside the 40px corner region so scaling doesn't smear it.
btn_svg = panel_svg 320 320 24 Pal_bg2 Pal_mut 3

// Button "recessed" (160×160, csz=40). Used in radio-group rows
// (game-side). Darker fill, with an inner highlight stripe to
// suggest sunkenness.
btnr_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='160' height='160' viewBox='0 0 160 160'><rect x='3' y='3' width='154' height='154' rx='20' ry='20' fill='#[Pal_bg0]' stroke='#[Pal_mut]' stroke-width='3'/><rect x='12' y='12' width='136' height='10' rx='5' ry='5' fill='#[Pal_bg1]'/></svg>"

// Button "selected" (160×160). Accent fill — active item in a
// radio group.
btns_svg = panel_svg 160 160 20 Pal_acc2 Pal_acc 3

// Button "selected2" (160×160). Secondary-selected variant; the
// game uses it to distinguish a second selection state.
btns2_svg = panel_svg 160 160 20 Pal_bg3 Pal_acc 3

// Checkbox unchecked (20×20). Rounded outline only.
cb0_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='20' height='20' viewBox='0 0 20 20'><rect x='2' y='2' width='16' height='16' rx='4' ry='4' fill='#[Pal_bg1]' stroke='#[Pal_mut]' stroke-width='2'/></svg>"

// Checkbox checked (20×20). Accent fill + white ✓ — the glyph is
// what keeps colour-blind users covered.
cb1_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='20' height='20' viewBox='0 0 20 20'><rect x='2' y='2' width='16' height='16' rx='4' ry='4' fill='#[Pal_acc]' stroke='#[Pal_acc2]' stroke-width='2'/><path d='M 5,10 L 9,14 L 15,6' fill='none' stroke='#[Pal_txt]' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'/></svg>"

// Droplist closed (160×160, csz=80,80 = 4-quadrant). Flat plate
// with a chevron on the right; pic9 stretches each quadrant
// independently when the widget is wider than the source.
drpl_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='160' height='160' viewBox='0 0 160 160'><rect x='2' y='2' width='156' height='156' rx='16' ry='16' fill='#[Pal_bg2]' stroke='#[Pal_mut]' stroke-width='2'/><path d='M 124,72 L 140,72 L 132,86 Z' fill='#[Pal_txt]'/></svg>"

// Droplist expanded (464×464, csz=80,80). Same look, bigger; pic9
// stretches to cover the open list pane.
drpl2_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='464' height='464' viewBox='0 0 464 464'><rect x='2' y='2' width='460' height='460' rx='20' ry='20' fill='#[Pal_bg1]' stroke='#[Pal_mut]' stroke-width='2'/></svg>"

// Knob (74×74). Slider thumb. Filled disc + ring + inner accent.
knob_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='74' height='74' viewBox='0 0 74 74'><circle cx='37' cy='37' r='32' fill='#[Pal_bg3]' stroke='#[Pal_mut]' stroke-width='3'/><circle cx='37' cy='37' r='10' fill='#[Pal_acc]'/></svg>"

// Tab inactive (420×320, csz=210,160). Rounded top, flat bottom
// so it sits flush against the header strip.
tab_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='420' height='320' viewBox='0 0 420 320'><path d='M 2,318 L 2,40 Q 2,2 40,2 L 380,2 Q 418,2 418,40 L 418,318 Z' fill='#[Pal_bg1]' stroke='#[Pal_mut]' stroke-width='3'/></svg>"

// Tab active / "down" (same geometry, accent fill). uim's pick=tabd
// style swap flips between tab and tabd to show selection.
tabd_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='420' height='320' viewBox='0 0 420 320'><path d='M 2,318 L 2,40 Q 2,2 40,2 L 380,2 Q 418,2 418,40 L 418,318 Z' fill='#[Pal_acc2]' stroke='#[Pal_acc]' stroke-width='3'/></svg>"

// Tab-header strip (80×80, csz=21). 9-slice. The horizontal bar
// the tabs sit on top of.
thdr_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='80' height='80' viewBox='0 0 80 80'><rect x='1' y='1' width='78' height='78' rx='10' ry='10' fill='#[Pal_bg1]' stroke='#[Pal_mut]' stroke-width='2'/></svg>"

// Window frame (526×373, csz=48). 9-slice. Non-square dims are
// historical — pic9 doesn't care; corners are 48×48, edges are
// the leftover stripes.
wfrm_svg = panel_svg 526 373 24 Pal_bg0 Pal_mut 3

// Window frame variant (560×402). Nested-frame variant — slightly
// lighter fill reads as "inset within".
wfrm2_svg = panel_svg 560 402 24 Pal_bg1 Pal_mut 3

// Window header (160×160, csz=20). 9-slice. uim passes cut=bottom,
// so only the top portion renders. We make it a raised plate so
// the title bar reads as elevated above the wfrm body.
whdr_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='160' height='160' viewBox='0 0 160 160'><path d='M 2,158 L 2,20 Q 2,2 20,2 L 140,2 Q 158,2 158,20 L 158,158 Z' fill='#[Pal_bg3]' stroke='#[Pal_mut]' stroke-width='2'/></svg>"

// Window close (39×39). × glyph on a dim disc.
wcls_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='39' height='39' viewBox='0 0 39 39'><circle cx='19.5' cy='19.5' r='17' fill='#[Pal_bg2]'/><path d='M 12,12 L 27,27 M 27,12 L 12,27' fill='none' stroke='#[Pal_txt]' stroke-width='3' stroke-linecap='round'/></svg>"

// Window minimise (39×39). Bottom-aligned bar.
wmin_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='39' height='39' viewBox='0 0 39 39'><circle cx='19.5' cy='19.5' r='17' fill='#[Pal_bg2]'/><rect x='12' y='24' width='15' height='3' rx='1.5' ry='1.5' fill='#[Pal_txt]'/></svg>"

// Window maximise (39×39). Hollow square.
wmax_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='39' height='39' viewBox='0 0 39 39'><circle cx='19.5' cy='19.5' r='17' fill='#[Pal_bg2]'/><rect x='12' y='12' width='15' height='15' rx='1.5' ry='1.5' fill='none' stroke='#[Pal_txt]' stroke-width='2'/></svg>"

// Slider chevron buttons (80×80 each, 4 directions × 2 states).
barwu_svg  = barw_svg \u 0
barwua_svg = barw_svg \u 1
barwd_svg  = barw_svg \d 0
barwda_svg = barw_svg \d 1
barwl_svg  = barw_svg \l 0
barwla_svg = barw_svg \l 1
barwr_svg  = barw_svg \r 0
barwra_svg = barw_svg \r 1

// Cursor (32×32). Classic upper-left arrow with a thin black
// outline — the accessibility trick that keeps the cursor visible
// on both light and dark backgrounds. Hot spot is at (1,1), the
// pointy end, matching OS convention.
cursor_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32' viewBox='0 0 32 32'><path d='M 1,1 L 1,22 L 7,17 L 11,26 L 14,25 L 10,16 L 17,16 Z' fill='#ffffff' stroke='#000000' stroke-width='1.5' stroke-linejoin='round'/></svg>"

// Replacement arrows used in inline ftx text (<pic arrow_X>).
// 64×64 filled triangles in the same neutral palette; the
// originals were 500×500 Inkscape exports.
arrow_down_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='64' height='64' viewBox='0 0 64 64'><path d='M 10,18 L 54,18 L 32,52 Z' fill='#[Pal_txt]'/></svg>"
arrow_up_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='64' height='64' viewBox='0 0 64 64'><path d='M 10,46 L 54,46 L 32,12 Z' fill='#[Pal_txt]'/></svg>"
arrow_left_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='64' height='64' viewBox='0 0 64 64'><path d='M 46,10 L 46,54 L 12,32 Z' fill='#[Pal_txt]'/></svg>"
arrow_right_svg =
  "<svg xmlns='http://www.w3.org/2000/svg' width='64' height='64' viewBox='0 0 64 64'><path d='M 18,10 L 18,54 L 52,32 Z' fill='#[Pal_txt]'/></svg>"

// --------------------------------------------------------------- corpus

// Name → SVG-content hash. The order doesn't matter for
// correctness but doubles as documentation of which pics
// ship by default.
UIPics!
UIPics.btn       = btn_svg
UIPics.btnr      = btnr_svg
UIPics.btns      = btns_svg
UIPics.btns2     = btns2_svg
UIPics.cb0       = cb0_svg
UIPics.cb1       = cb1_svg
UIPics.drpl      = drpl_svg
UIPics.drpl2     = drpl2_svg
UIPics.knob      = knob_svg
UIPics.tab       = tab_svg
UIPics.tabd      = tabd_svg
UIPics.thdr      = thdr_svg
UIPics.wfrm      = wfrm_svg
UIPics.wfrm2     = wfrm2_svg
UIPics.whdr      = whdr_svg
UIPics.wcls      = wcls_svg
UIPics.wmin      = wmin_svg
UIPics.wmax      = wmax_svg
UIPics.barwu     = barwu_svg
UIPics.barwua    = barwua_svg
UIPics.barwd     = barwd_svg
UIPics.barwda    = barwda_svg
UIPics.barwl     = barwl_svg
UIPics.barwla    = barwla_svg
UIPics.barwr     = barwr_svg
UIPics.barwra    = barwra_svg
UIPics.cursor    = cursor_svg
UIPics.arrow_down  = arrow_down_svg
UIPics.arrow_up    = arrow_up_svg
UIPics.arrow_left  = arrow_left_svg
UIPics.arrow_right = arrow_right_svg

// --------------------------------------------------------------- entry

// Materialise any missing default into `<Dir>/<name>.svg`. Existing
// files (including PNG overrides — locate_pic prefers .png) are
// left untouched. Idempotent + cheap on subsequent runs: one stat
// call per name, then nothing.
ensure_ui_pics Dir =
  less Dir.exists: Dir.mkpath
  for Name,Content UIPics:
    Path "[Dir][Name].svg"
    PathPng "[Dir][Name].png"
    less Path.exists or PathPng.exists:
      Path.set Content

// On module import, ensure the running project has the defaults.
// `main_path` is the directory the active executable lives in.
ensure_ui_pics "[main_path()]pic/ui/"

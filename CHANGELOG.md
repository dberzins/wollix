# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.6.0] - 2026-06-28

### Changed (Breaking) — v0.6 coordinated API group (V05 R9b)

One release, four changes. Renamed fields keep a deprecated alias of the same
storage (an anonymous-union member) for **one minor version after 0.6**, so
existing designated initializers compile unchanged; migrate before 0.7.

- **`wlx_widget` / `wlx_separator`: `.color` -> `.back_color`.** The fill
  field now matches every other widget. Migration: rename the initializer;
  `.color` still compiles via the deprecated alias.
- **`wlx_slider`: `.show_label` -> `.show_value`.** The field always toggled
  the numeric value readout, never the label text; the name now says so.
  Migration: rename the initializer; `.show_label` still compiles via the
  deprecated alias.
- **`wlx_panel`: `border_width` unset sentinel is `-1` + theme inheritance.**
  An unset panel border now inherits `theme->border_width` (and a `{0}`
  `border_color` inherits `theme->border`), matching widget resolution -
  previously a panel could never inherit the theme border. All bundled theme
  presets ship `border_width = 0`, so default rendering is unchanged with
  stock themes; under a custom theme with a non-zero `border_width`, panels
  that relied on default-no-border must now pass an explicit
  `.border_width = 0`.
- **`wlx_inputbox` returns *changed*, not *focused*.** The return value is now
  "the buffer text was mutated this frame" (insert or delete; cursor-only
  movement does not count), which is what callers almost always want. Focus
  state - the old return value - is available through the new
  `.out_focused = &flag` out-param. Migration:
  `bool focused = wlx_inputbox(...)` becomes
  `bool focused; wlx_inputbox(..., .out_focused = &focused)`.

### Added
- **Interactive containers (`interact` / `interact_out`).** Container begin opts
  (`WLX_Layout_Opt`, grids, and `WLX_Panel_Opt` via `WLX_CONTAINER_DECOR_FIELDS`)
  gain `uint32_t interact` (a `WLX_Interact_Flags` mask; `0` = non-interactive,
  the default and zero-overhead) and `WLX_Interaction *interact_out` (optional).
  When `interact != 0`, `wlx_layout_begin` resolves the interaction on the
  container rect (folding in the container's scope id) **before** recording its
  chrome, writes the raw `WLX_Interaction` through `interact_out`, and applies
  hover-variant colors with replace semantics: new `hover_back_color`,
  `hover_border_color`, and `hover_border_color_top/right/bottom/left` twins each
  replace their base color while hovered (a `{0}` twin keeps the base). Color-only
  - there is no hover-variant width/gradient/shadow/glow; an *appearing* border is
  a constant width with a transparent base color toggled to an accent twin.
  Supported flags are `HOVER`, `CLICK`, `KEYBOARD` (`FOCUS`/`DRAG` are unsupported
  on containers; the auto-counted begins lack a stable call-site id and do not
  support `interact`). A `CLICK` container is queried before its children and
  captures the press first (first-writer), so keep interactive containers to
  composite regions of non-interactive content. Existing containers are bit-for-bit
  unchanged (`interact = 0`). Covered by `tests/test_interactive_container.c`. See
  `docs/LAYOUT_MODEL.md` § Interactive containers.
- **List clipper for virtualized rows, plus opt-in offscreen command culling.**
  `wlx_list_clipper_begin` / `wlx_list_clipper_end` (with helpers
  `wlx_list_clipper_height` and `wlx_list_clipper_item_height`) build only the
  rows of a long list that fall within the enclosing scroll panel's viewport,
  reserving the off-screen extent with empty spacers so the scrollbar stays
  correct. Fixed pitch or variable heights (a prefix-sum `item_offsets` array) are
  both supported, plus an `overscan` band. New `wlx_get_scroll_panel_offset`
  exposes the innermost panel's scroll offset to pair with
  `wlx_get_scroll_panel_viewport`. Rows outside the returned `[first, last)` are
  not produced (no ids, interactions, or persistent state that frame) - keep
  stateful or interactive widgets out of virtualized rows, or widen the range with
  `overscan`. Separately, `wlx_set_cull_offscreen(ctx, true)` (default off) makes
  the deferred recorder skip rect-bounded draw commands (rect, rect-lines, rounded
  fill/lines, vertical gradient, texture) that lie fully outside the active clip,
  trimming command-buffer size and backend replay; output is unchanged because
  only provably-invisible commands drop, and immediate mode is unaffected. Covered
  by `tests/test_list_clipper.c` and `tests/test_offscreen_cull.c`. See
  `docs/WIDGETS.md` and `docs/API_REFERENCE.md`.
- **Per-corner rounding (`rounded_corners`):** a new `int rounded_corners`
  decoration field on every bordered widget and container (`WLX_BORDER_FIELDS` /
  `WLX_CONTAINER_DECOR_FIELDS`, default `0`) selects **which** corners use the
  resolved `corner_radius` / `roundness`. It is a `WLX_Corner` bitmask
  (`WLX_CORNER_TOP_LEFT` ... `WLX_CORNER_BOTTOM_LEFT`) with the convenience masks
  `WLX_CORNERS_ALL` / `_TOP` / `_BOTTOM` / `_LEFT` / `_RIGHT`; `0` (unset) is
  treated as all four corners, so existing output is byte-for-byte unchanged.
  Corners absent from the mask are squared off by overdrawing their corner box
  with the fill color (no command-buffer or backend change). It applies to the
  **solid fill only** - gradient fills and rounded borders stay uniform; use a
  per-side straight border for an edge accent on a squared corner. Honored on the
  shared decor path (containers, grids, panels, scroll panels, and the chrome of
  `widget` / `label` / `button` / `inputbox`); the widget primitives that draw
  their own rounded shapes accept but ignore it. Covered by
  `tests/test_corner_radius.c`. See `docs/WIDGETS.md` § Per-corner rounding.
- **Scroll panel transparent background:** `WLX_Scroll_Panel_Opt` gains
  `bool transparent_background` (default `false`). When `true`, the panel draws no
  fill so the container behind it shows through - distinct from `back_color = {0}`,
  which resolves to the theme background. Existing panels are unchanged.
- **`wlx_button` per-call hover override:** `WLX_Button_Opt` gains
  `hover_brightness` (default `WLX_FLOAT_UNSET` -> `theme->hover_brightness`) and
  `hover_back_color` (default `{0}`), mirroring `wlx_toggle` / `wlx_radio` /
  `wlx_slider`. A non-zero `hover_back_color` replaces the fill while hovered;
  otherwise the brightness path applies. Inert by default, so existing buttons
  render unchanged.
- **`wlx_inputbox`: inner icon support.** `WLX_Inputbox_Opt` gains the
  image-capable fields (`texture`, `texture_src`, `texture_tint`,
  `image_placement`, `image_size`, `image_text_gap`) so a leading (`LEFT`,
  default) or trailing (`RIGHT`) icon renders **inside** the field frame instead
  of as a detached sibling widget. The icon is centered vertically independent of
  `align`, and the reserved band insets the text and caret so they never overlap
  the glyph. A zero `texture` keeps the field text-only with byte-identical
  geometry, so existing call sites are unaffected. The dashboard top-bar search
  now uses this to put the magnifier glyph inside the box (via a new
  `dashboard_icon_inputbox` demo helper).
- **Dashboard showcase ("Mechanical Glass") -- the canonical cross-backend demo
  (ADR_026):** `demos/dashboard/` is a single-translation-unit, demo-local
  dashboard that builds and runs on raylib, SDL3, and bare WASM. A shared top bar
  and sidebar route between distinct, persistent views -- Overview, Tokens,
  Components, Layouts, Theme Lab -- through a dashboard-local section dispatch
  table. The Overview shows library-honest metrics (backend FPS, and in a
  `WLX_PERF` build the draw-call count and wollix arena memory, plus desktop
  process RSS where measurable), and reference links open real repository/doc
  URLs through a per-backend open-URL hook (raylib `OpenURL`, SDL3 `SDL_OpenURL`,
  a `window.open` shim on WASM). Tokens/Components/Layouts/Theme Lab demonstrate
  the full public widget and layout surface (label, button, checkbox, toggle,
  radio, slider, progress, input, image, widget; linear/grid/flex/auto layout,
  opacity stack, ID stack, borders, scroll panel; tokens, theme presets, and a
  component-state matrix). The core (`wollix.h`) and backend headers are
  unchanged; the only host addition is an additive `window.open` shim in
  `web/wollix_wasm.js`. `make dashboard` / `dashboard_sdl3` / `dashboard_perf`
  build the desktop binaries. See
  `docs/dev/ADR_026_DASHBOARD_REPLACES_GALLERY.md` and
  `docs/DASHBOARD_DESIGN_SYSTEM.md`.
- **API hygiene, additive (V05 R9a):**
  - `WLX_SCROLL_AUTO_HEIGHT` names the `-1` auto-height sentinel for
    `wlx_scroll_panel_begin`; all in-tree call sites converted.
  - `wlx_rect_contains(WLX_Rect, float, float)` is the new canonical
    float-precision point-in-rect query; `wlx_point_in_rect` remains as an
    int-based shim, and internal hit tests now route through the new query.
  - `WLX_Panel_Opt` now embeds the full container decor set
    (`WLX_CONTAINER_DECOR_FIELDS` + `WLX_SLOT_DECOR_FIELDS`): panels gain
    `rounded_segments`, shadow/glow/gradient, and per-slot decor, forwarded to
    the panel body layout. All new knobs default to off; existing panels
    render unchanged (panel `border_width` keeps its 0-as-unset convention).
  - `wlx_split_next` now goes through `wlx_default_split_next_opt`, aligning
    it with the default-opt pattern.
  - `WLX_SHORT_NAMES` gains `image`, `slot_style`, `grid_cell_style`,
    `push_id`/`pop_id`, `push_opacity`/`pop_opacity`.
  - Docs: a "Widget Return Semantics" table in API_REFERENCE.md (clicked vs
    changed vs focused), and the unset-sentinel rule is written down next to
    `WLX_FLOAT_UNSET` in the header.
- **`WLX_HARD_ASSERT` - memory-safety guards survive release builds (V05 R1):**
  guards whose failure would corrupt memory no longer compile out under
  `NDEBUG`: scissor-stack push, the CONTENT-slot count in layout begin/end,
  persistent-state size collisions in `wlx_get_state_impl`, and every internal
  allocation result now abort with a `wollix fatal:` message instead of writing
  out of bounds or dereferencing NULL. Exception: a layout whose slot count
  exceeds `WLX_CONTENT_SLOTS_MAX` with CONTENT slots present clamps in release
  (warns on stderr and disables CONTENT tracking for that layout - CONTENT
  slots fall back to min size) rather than aborting. The assert-vs-hard-assert
  policy is documented in the header preamble (ASSERTION POLICY). Covered by
  the new `tests/test_hard_assert.c` NDEBUG death-test binary, wired into
  `make test`.
- **Layout over-allocation diagnostic (`WLX_DEBUG`):** a linear layout whose slot
  sizes resolve to a boundary past its own extent (so the trailing slot overflows
  onto its neighbor) now emits a one-time warning through the existing debug warn
  channel (`warn_count` / `warn_cb`). The warning is suppressed when the layout or
  an ancestor clips (the overflow is contained) and is deduplicated per call-site.
  Zero footprint in release builds. Covered by `tests/test_slot_redistribute.c`.
- **Absolute pixel corner radius (`corner_radius`):** an opt-in `float
  corner_radius` decoration field on every bordered widget and container
  (`WLX_BORDER_FIELDS` / `WLX_CONTAINER_DECOR_FIELDS`, default `0`) declares the
  corner radius in **pixels**, where `roundness` is a fraction of the shorter
  side. `corner_radius > 0` overrides `roundness`; it is resolved centrally at
  draw time inside `wlx_draw_box` to the fraction the backends already consume
  (`clamp(2 * corner_radius / min(w,h), 0, 1)`), so two differently sized
  elements share the same pixel corner and the recorded command still carries a
  fraction - no command-buffer or backend change. Existing callers are
  bit-for-bit unchanged (`0` = unset). v1 honors it on the decor path
  (containers, grids, panels, scroll panels, and the chrome of `widget` /
  `label` / `button` / `inputbox`); widget primitives that draw their own
  rounded shapes (`checkbox`, `slider`, `progress`, `scrollbar`, `radio`) accept
  but ignore it. See `docs/dev/ADR_025_ABSOLUTE_CORNER_RADIUS.md`.
- **First-class vertical gradient fill:** a two-stop vertical gradient is now a
  decoration field on every bordered widget and container. New
  `WLX_GRADIENT_FIELDS` (`gradient_top`, `gradient_bottom`) embed in
  `WLX_BORDER_FIELDS` and `WLX_CONTAINER_DECOR_FIELDS`; both zero-initialize to
  "disabled" (`gradient_top = {0}` is the enable gate, a zero `gradient_bottom`
  resolves to `gradient_top` for a uniform fill), so existing callers are
  bit-for-bit unchanged. `wlx_draw_box` emits the gradient **in place of** the
  solid fill, after soft effects and before borders, via a new command type
  `WLX_CMD_GRADIENT_V` (public `wlx_cmd_record_gradient_v`); the
  `wlx_draw_layout_decor` gate fires for a gradient-only container. At replay
  (and in immediate mode) the optional `WLX_Backend.draw_gradient_v` callback
  renders natively (Raylib: `DrawRectangleGradientV` sharp, rounded-band
  approximation for rounded rects) else a software fallback slices the rect into
  `max(1, rect.h / 4)` solid bands interpolating the stops. Widget-path stops
  carry the effective opacity (container stops pass through like `back_color`);
  `WLX_PERF` counts one command per element. New `wlx_color_lerp` helper; v1 is
  vertical two-stop only (ADR_023). Covered by `tests/test_gradient.c`. See
  `docs/WIDGETS.md` § Gradient fill fields.
- **Opt-in layout clipping:** `WLX_Layout_Opt` gains `.clip` (default `false`).
  When set, `wlx_layout_begin` clips its children to the post-padding content
  rect by beginning a scissor at begin and releasing it at end. The clip rect is
  intersected with the active clip before installing, so a `clip` layout inside a
  scroll panel (or another clip layout) never widens the visible region. The
  container's own chrome (background/border/glow/shadow) is recorded before the
  scissor and is never cropped; a child's effects are inside the clip and are
  cropped to the parent content rect. `wlx_panel_begin(.clip = true)` now routes
  through this shared path, and `wlx_layout_end` is the single owner of the
  scissor release (`wlx_panel_end` no longer ends the scissor itself); panel clip
  output is unchanged. Backends without `begin_scissor`/`end_scissor` no-op the
  recorded commands at replay. Grids do not expose `clip`; wrap a grid in a
  `clip` layout for cell containment.
- **Segmented progress self-containment:** `wlx_progress_cell_rect` now keeps
  every segmented cell inside its track at all widths. The inter-cell gap is
  compressed (down to 0) when the nominal gaps would fill the track, cell widths
  keep a 1px floor, and a trailing cell that still cannot fit is clamped to the
  track's right edge (collapsing to zero width) instead of drawing past it. The
  minimum segmented bar width drops to `segments` px (1px per cell, gap 0). This
  is an internal geometry fix; no public API or option change.
- **First-class glow and shadow:** a soft outer glow and a soft drop shadow are
  now decoration fields on every bordered widget and container. New
  `WLX_GLOW_FIELDS` (`glow_color`, `glow_spread`, `glow_rings`) and
  `WLX_SHADOW_FIELDS` (`shadow_color`, `shadow_offset_x`, `shadow_offset_y`,
  `shadow_blur`, `shadow_layers`) embed in `WLX_BORDER_FIELDS` and
  `WLX_CONTAINER_DECOR_FIELDS`; all fields zero-initialize to "effect disabled"
  (non-zero color is the enable gate), so existing callers are bit-for-bit
  unchanged. Each numeric knob resolves `<= 0` / `== 0` to the new
  `WLX_Theme.shadow` / `WLX_Theme.glow` sub-structs, then to a hard fallback
  (`8` px blur / `4` layers / `4` px spread / `3` rings); built-in presets need
  no edit. `wlx_draw_box` emits the effects before the element fill via two new
  command types `WLX_CMD_SHADOW` / `WLX_CMD_GLOW` (recorded with the public
  `wlx_cmd_record_shadow` / `wlx_cmd_record_glow`). At replay (and in immediate
  mode) the optional `WLX_Backend.draw_shadow` / `draw_glow` callbacks render
  natively when set, else a software fallback draws layered offset rounded rects
  / concentric expanding rounded rings; all in-tree backends use the fallback
  this cut. Effect colors flow through the same effective-opacity premultiply as
  fill/border. Emission is wired for `wlx_widget` / `wlx_label` / `wlx_button` /
  `wlx_checkbox` / `wlx_inputbox` / the scroll panel and the layout/grid
  container decor path. `wlx_slider` and `wlx_toggle` render the effect behind
  their thumb; continuous `wlx_progress` renders it behind the track and
  segmented `wlx_progress` behind the bounding run of filled cells. The
  dashboard demo's `dashboard_draw_glow` / `dashboard_draw_shadow` now delegate
  to the new recorders, and `dashboard_segmented_progress` expresses its neon
  run-glow through the `wlx_progress` glow options instead of a manual emitter.
- **`wlx_progress` segmented mode:** `WLX_Progress_Opt` gains `.segments`
  (`0` = continuous, the default; `> 0` = discrete cells) and `.segment_gap`
  (pixel gap between cells; `<= 0` resolves to the theme default). A new
  `WLX_Theme.progress.segment_gap` field supplies that default, falling back to
  `2px` when zero (built-in presets need no edit). In segmented mode the bar
  draws `round(value * segments)` filled cells in `fill_color` and the rest in
  `track_color`, with equal floor-rounded widths (the last cell absorbs the
  remainder) and transparent gaps - no continuous track box or border. The
  continuous path is unchanged for `segments == 0`. The dashboard demo's
  `dashboard_segmented_progress` helper now delegates its cells to
  `wlx_progress` (keeping its neon glow) and the demo-local
  `dashboard_segments_filled` helper is retired.
- **`wlx_label` aggregate text style and vertical metric:** `WLX_Label_Opt`
  gains an optional `.style` (`WLX_Text_Style`) and `.vertical_metric`
  (`WLX_Vertical_Metric`). When `.style.font_size > 0` the aggregate
  overrides the individual `font`/`font_size`/`spacing`/`front_color`
  fields (a fully-zero `style.color` falls back to the resolved
  `front_color`); otherwise existing per-field behavior is preserved.
  `.vertical_metric` defaults to `WLX_VMETRIC_LINE_HEIGHT` (no change);
  `WLX_VMETRIC_FONT_SIZE` centers fitted text on the font-size em box
  for consistent cap-height placement across backends whose line height
  exceeds the font size. Setting both `.style.font_size > 0` and the
  field `font_size > 0` emits a once-per-site warning under
  `WLX_DEBUG`. The dashboard demo's `dashboard_slot_text` helper now
  uses `wlx_label` with `.style` and `.vertical_metric = WLX_VMETRIC_FONT_SIZE`.
- **Dashboard demo scaffold (Stitch "Mechanical Glass"):** new `demos/dashboard/dashboard.c`
  plus demo-local `demos/dashboard/dashboard_theme.h` stub (included only by that TU,
  extending the `gallery_perf.h` local-header convention). Opens a themed Raylib window,
  clears to the Stitch dark background (`#0f1419`), and renders a placeholder label. New
  `make dashboard` target, wired into `make test-demos`. `wollix.h` and backends unchanged.
- **Vendored fonts:** Geist (Regular/Medium/SemiBold/Bold) and JetBrains Mono
  (Regular/Medium/Bold) under `demos/assets/` with their SIL Open Font License files, for the
  dashboard typography roles.
- **Dashboard token model (dark + light):** `demos/dashboard/dashboard_theme.h` now carries the
  Stitch-derived, dashboard-local design tokens as pure data - color roles + surface-container
  ramp, on-colors, status, table, glass-edge and glow colors, typography roles (family/weight
  intent + size + line-height + integer tracking), a strict 4px spacing scale, a radius scale,
  and per-effect intent - as `dashboard_tokens_dark` / `dashboard_tokens_light`. A runtime
  resolver (`dashboard_font_resolve` / `dashboard_type_font`) maps typography intent to loaded
  Geist / JetBrains Mono handles. New `tests/test_dashboard_tokens.c` token-integrity suite.
  `wollix.h` and backends remain unchanged.
- **Dashboard `WLX_Theme` mapper:** `dashboard_wlx_theme(mode, fonts)` in
  `demos/dashboard/dashboard_theme.h` builds a `WLX_Theme` from the active mode's tokens each
  frame - baseline subset (background, foreground, surface, border, accent, padding, roundness,
  font/size) plus widget overrides for input focus, slider, checkbox, toggle, radio, progress,
  and scrollbar. The dashboard demo now renders a token-driven title and the ordinary widgets
  under the mapped theme, switching dark/light with `M` (or a `light` argv). New
  `tests/test_dashboard_theme.c` mapper suite.
- **Dashboard component recipes:** `demos/dashboard/dashboard_components.h` adds dashboard-local
  components built from existing primitives + the token model - glass module with asymmetric
  edge strokes, technical primary / glass secondary buttons, technical input, status
  chip / badge / pulsing pip, dense data table (header bg, zebra, grid lines), segmented
  progress bar, navigation sidebar, and a responsive 12-to-4 column grid helper. The demo gains
  a components showcase view (`TAB` switches views; a `widgets` argv starts on the widgets view).
  New `tests/test_dashboard_components.c` suite covers the pure geometry/state helpers.
  `wollix.h` and backends remain unchanged.
- **Dashboard Overview screen (Stitch replica):** the dashboard demo's primary view is now a
  faithful rebuild of the Stitch dark "Wollix Explorer | Overview" reference - a top navigation
  bar, a left sidebar (nav links + Deploy Build + footer), and a scrollable main area with three
  hero stat cards (segmented usage bars) and a 12-column grid (Basic Widgets + Structural
  Layouts, a tabbed feature-card panel + a scanline Terminal Logs panel, Integrations + a
  gradient Visual Laboratory). Colors adapt per mode to track both the Stitch dark and light
  references (cyan vs blue accent, light-on-dark vs dark-on-light overlays, and the
  terminal/log treatment), with identical layout and content. Radii, borders, typography roles,
  and the scroll/clip regions track the references. Material-Symbols icons and the remote images
  are drawn as geometric placeholders. `TAB` switches to the ordinary-widgets mapping view; a `widgets` argv
  starts there. New `WLX_DASHBOARD_SHOT=<file>` env captures a screenshot for offline
  verification. `wollix.h` and backends remain unchanged.
- **Dashboard visual effects (primitive-only):** `demos/dashboard/dashboard_effects.h` adds
  layered-primitive approximations - outer glow / highlight, soft shadow, vertical gradient,
  scanline overlay, and a frosted backdrop-blur stand-in (darkened/desaturated translucent
  fill) - plus pure color helpers (lerp, desaturate, blur tint, glow falloff). Wired into the
  demo as module shadows, a page gradient backdrop, an active segmented-bar glow, and a
  dedicated "Visual effects" section. New `tests/test_dashboard_effects.c` suite. A per-effect
  fidelity assessment is recorded under `docs/dev/dashboard/`; true backdrop blur is deferred
  to an ADR-gated core change. `wollix.h` and backends remain unchanged.

### Changed
- **The published web demo and canonical showcase are now the dashboard
  (ADR_026).** `make wasm-site` now packages the bare-WASM **dashboard** into
  `dist/wasm-demo/` (the GitHub Pages artifact), so the demo URL is unchanged but
  serves the dashboard. The gallery keeps building and coexists: `make gallery`,
  `make gallery_sdl3`, and the new `make gallery-wasm-site` (into
  `dist/gallery-demo/`); `make dashboard-wasm-site` is now an alias of
  `make wasm-site`. `docs/DASHBOARD_DESIGN_SYSTEM.md` is now the canonical
  showcase design reference and supersedes `docs/DESIGN_SYSTEM.md`, which remains
  the reference for the core public theme contract.
- **Dashboard Overview elements are now functional, not decorative.** The Overview
  landing's previously static controls gained real, library-honest behavior (all
  demo-local; no core change): the Basic Widgets and Structural Layouts lists and the
  capability cards deep-link into the Components/Layouts sections; the top-bar search
  filters those lists in place; the "Active State"/"Preview" tabs switch and persist
  (Active State hosts live checkbox/toggle/slider/button controls); Terminal Logs now
  renders a live in-app action log (navigation, theme changes, deploy, tab switches)
  in a scroll panel, newest first with timestamps, replacing the hardcoded lines;
  Deploy Build posts a real build sequence to that log; the bell tints to the accent
  while the log holds unread entries and clears on click; Settings and "Launch Lab"
  open the Theme Lab section; and the "Visual Laboratory" panel's fictional
  physics/luminosity copy was rewritten to describe Theme Lab honestly. The identity
  avatar stays intentionally static (no fabricated account).
- **Image-capable `button` / `label` crop an oversized image to the widget instead
  of overflowing it.** Widget drawing is not scissored, so an explicit `image_size`
  (or a fixed image beside text) larger than a squeezed content rect previously drew
  past the control's edge. The image + text path now keeps the image at its requested
  size and crops it to the content rect via a new geometric blit-clip
  (`wlx_clip_textured_blit`: intersect the destination, then narrow the source
  proportionally), clipping the glyph at the edge rather than scaling it down; the
  image-only path sizes through `wlx_get_align_rect`, which already clamps to the
  content rect; an image fully outside the rect draws nothing. Covered by
  `tests/test_button_image.c`.
- **Per-side border forwarding behind one macro (V05 R7, internal):** the
  eight per-side border fields every chrome-drawing widget forwards into
  `wlx_border_sides_for_widget` now go through `WLX_BORDER_SIDES_ARGS(opt)`
  at all seven call sites; adding a per-side field is a one-macro change.
- **Min/max redistribution scratch rides the frame arena (V05 R4, internal):**
  the freeze-and-redistribute pass in the offset solver no longer heap-allocates
  its working buffers when a constrained layout exceeds `WLX_OFFSET_STACK_LIMIT`
  slots. The new context-aware entry `wlx_compute_offsets_ctx` takes one
  combined block from the frame byte-scratch arena (reclaimed by the per-frame
  reset); the ctx-less `wlx_compute_offsets` remains for standalone callers
  with a hard-checked heap fallback. A steady-state frame of a static UI now
  performs **zero** heap allocations, locked in by a new `WLX_PERF` test
  (`perf_steady_state_zero_allocations`). Offset math is unchanged.
- **Label/button content emission unified (V05 R5, internal):** the
  three-branch image/text block (measure, auto-size, layout, fit, draw) and
  the tint/gap resolver tail that were pasted into both widgets now live in
  `wlx_draw_widget_content` / `wlx_resolve_widget_content`. Rendering is
  unchanged; button text keeps line-height centering (the `.vertical_metric`
  cap-height knob remains label-only by decision).
- **Glyph + label row layout unified (V05 R6, internal):** checkbox, toggle,
  and radio share `wlx_layout_glyph_row` for the measure / combined-block /
  align / split sequence. Per-widget row quirks are preserved exactly
  (checkbox: always-reserved label padding, top-aligned glyph, label height
  extending to the content bottom, optional `full_slot_hit`; toggle/radio:
  collapsed empty-label space, vertically centered glyph).
  `WLX_CHECKBOX_LABEL_PADDING_FACTOR` was renamed to the shared
  `WLX_GLYPH_ROW_LABEL_PADDING_FACTOR` (internal constant).
- **Scratch allocation converged on the sub-arena primitives (V05 R3,
  internal):** `wlx_scratch_alloc`, `wlx_dyn_scratch_alloc`, and
  `wlx_scratch_alloc_bytes` are now thin wrappers over `wlx_sub_arena_alloc` /
  `_alloc_bytes`, restoring the alignment power-of-two and `size_t`-overflow
  asserts the byte path had dropped. A new `wlx_sub_arena_extend_to` primitive
  replaces the four hand-rolled reserve/count/high-water sequences in
  `wlx_create_layout_auto`, `wlx_create_grid_auto`, and both dynamic-growth
  paths in `wlx_get_slot_rect`; every bump of the arena invariant now lives
  inside `wlx_sub_arena_*`. No allocation-pattern or layout-behavior change.
- **Slot min/max redistribution is now the default.** When a slot clamps to its
  min or max, its surplus/deficit is redistributed to unfrozen flex/auto siblings
  so the resolved boundary stays at `total`. This changes resolved sizes only for
  layouts that use min/max-constrained slots; constraint-free layouts are
  byte-for-byte unchanged (gated on `has_constraints`). Define
  `WLX_SLOT_SINGLE_PASS_CLAMP` to opt out and restore the legacy single-pass
  clamp. The former opt-in `WLX_SLOT_MINMAX_REDISTRIBUTE` is now a deprecated
  no-op, accepted but ignored for source compatibility. Redistribution cannot
  remove a physical overflow when fixed + min sizes exceed the container; use an
  opt-in layout `.clip` to contain that. See `docs/LAYOUT_MODEL.md` § Redistribute
  Mode.
- **Dashboard chrome on native surfaces:** the dashboard demo no longer hand-places
  `wlx_cmd_record_*` chrome against parent rects. Topbar/sidebar/terminal fills, hairlines,
  and panel outlines moved onto container `.back_color` and (per-side) border decor; icon
  boxes, the terminal window dots, and the status-chip/status-pip discs moved onto
  slot-claiming `wlx_widget`s (square + `roundness = 1.0` renders true discs via the
  box->circle fallback); the badge pill is now a single `wlx_label` `.show_background`; the
  data table expresses its header/zebra fills and grid rules through per-row `.back_color`
  and per-cell borders; the status-pip halo uses first-class `.glow_*`. The unused
  `dashboard_draw_glow` / `dashboard_draw_shadow` / `dashboard_draw_frost` helpers were
  deleted (redundant with first-class glow/shadow decor and container tints); the pure color
  helpers and the scanline draw are retained. The live Overview render is unchanged within
  sub-pixel border-resolver tolerance. The visual-lab image gradient now uses the first-class
  `.gradient_top` / `.gradient_bottom` decor (see Added), so `dashboard.c` is free of raw
  `wlx_cmd_record_*` calls; the scanline overlay is the lone documented demo-local residual.

### Fixed
- **Slice-only backends pass the readiness check (V05 R8):**
  `wlx_backend_is_ready` required the legacy NUL-terminated `draw_text` /
  `measure_text` callbacks even though the slice entry points are the
  documented preferred contract, so a slice-only backend failed the readiness
  assert. Readiness now accepts either form per direction. Also fixed an
  `||`/`&&` precedence slip in `wlx_draw_text_slice`'s immediate-mode assert
  that bound the message to the wrong operand. Covered by the new
  `tests/test_slice_backend.c` suite (readiness matrix plus deferred and
  immediate rendering through the slice callback).
- **Toggle and radio honor `.wrap` (V05 R9a):** both option structs carried
  the wrap field but the draw call hardcoded no-wrap; the field is now passed
  through like checkbox. Default remains `false`, so behavior changes only
  for callers who set it.
- **One-byte out-of-bounds read on the legacy text fallback (V05 R2):**
  `wlx_span_measure_text` and `wlx_draw_text_span_immediate` probed
  `text[len] == '\0'` to skip the temporary copy; for a slice ending exactly at
  the end of an allocation that read one byte past the span. The legacy path
  (backends without `draw_text_slice` / `measure_text_slice`) now always copies
  via a new shared `wlx_cstr_tmp_begin/end` helper, which also replaces the
  three hand-rolled stack-or-heap copy blocks (measure, draw, deferred-replay
  dispatch). No in-tree backend is affected - all implement the slice
  callbacks.

## [0.5.0] - 2026-05-23

### Added
- **Image widget:** `wlx_image` with `WLX_Image_Scale` (`STRETCH`/`FIT`/`FILL`/`NONE`),
  `WLX_Image_Opt` (alignment, tint, source sub-rect, opacity), and `demos/image`.
- **Image-capable button:** `WLX_Button_Opt` gains `texture`, `texture_src`, `texture_scale`,
  `texture_tint`, `image_placement`, `image_size`, `image_text_gap`; new `WLX_Image_Placement`
  enum (`LEFT`/`RIGHT`/`TOP`/`BOTTOM`). New `demos/button_image` demo.
- **Image-capable label:** same image surface as button via `WLX_Label_Opt`. New `demos/label_image` demo.
- **Image-capable checkbox texture mode:** `WLX_Checkbox_Opt` gains `tex_checked_src`,
  `tex_unchecked_src`, `tex_checked_tint`, `tex_unchecked_tint`. Both tints fold through the
  opacity stack; texture mode now requires both state textures and defaults tint to `WLX_WHITE`.
- **Content padding for all widgets:** `WLX_CONTENT_PADDING_FIELDS` (`content_padding`,
  `content_padding_top/right/bottom/left`) added to every widget and chrome container;
  `WLX_PADDING_USE_THEME` sentinel opts into `theme->padding`. Defaults are zero so existing
  call sites are pixel-stable.
- **Disabled-state Phase 1:** `.disabled` option on `wlx_button`, `wlx_checkbox`, `wlx_inputbox`,
  `wlx_slider`, `wlx_toggle`, and `wlx_radio`; `theme->disabled_brightness` and
  `theme->disabled_opacity` knobs; `WLX_DEFAULT_DISABLED_OPACITY` constant (`0.55f`) shared
  across built-in presets. Coverage rule documented in ADR_018 and `docs/WIDGETS.md`.
- **WASM texture support:** `wlx_wasm_texture_create` / `wlx_wasm_texture_destroy` C helpers;
  JS host owns a per-texture `OffscreenCanvas` registry copied at upload time. Full RGB tint
  is supported via a source-space variant cache: primary path uses `ctx.filter feColorMatrix`;
  fallback uses Canvas 2D `multiply` compositing. Alpha applies per-draw via `ctx.globalAlpha`.
  See ADR_017 for the full contract.
- **Gallery Lucide icon atlas (32 icons, four tiers):** `demos/assets/wlx_icons.h` is now a
  576x120 white-alpha atlas at 16/24/32/48 px. `gallery_icon_src_for()` picks the sharpest tier
  at the call-site size. Source SVGs and manifest under `demos/assets/icons/`.

### Changed
- **Breaking:** `WLX_Split_Opt` and `WLX_Panel_Opt` replace `.padding*` fields with
  `.content_padding*`. `WLX_PADDING_FIELDS`, `WLX_PADDING_DEFAULTS`, and `WLX_RESOLVE_PADDING`
  macros removed. Rename `.padding` → `.content_padding` (and per-side variants) to migrate.
- **Breaking:** `pushed_id` fields on `WLX_Widget_Frame` and `WLX_Scroll_Panel_State` renamed to
  `bool pushed_scope`; `pushed_scope_id` on `WLX_Layout` renamed to `pushed_scope`.
- Separator now respects the opacity stack and `theme->opacity` (was silently ignored).
- Gallery spacing roles (`space_shell`, `space_panel`, `space_heading_*`, `gap_dense`, etc.)
  derived from `WLX_Theme.padding` instead of hard-coded literals.
- `wlx_widget_impl` draw rect no longer rounded with `ceilf`; matches all other widget paths.

### Fixed
- Fixed incorrect cast in `wlx_scroll_panel_frame_begin`.

### Internal
- `WLX_Checkbox_Opt` and `WLX_Inputbox_Opt` embed `WLX_BORDER_FIELDS`/`WLX_BORDER_DEFAULTS`
  instead of hand-rolled border fields.
- `wlx_resolve_content_rect_full` + `WLX_RESOLVE_CONTENT_RECT` collapse the common
  padding/clamp/inset three-liner used by label, button, checkbox, slider, progress, toggle, radio.
- `WLX_RESOLVE_VISUAL_STATE` macro unifies resolver tails; `wlx_color_hover_tint` centralizes
  the hover-tint gate across all interactive widgets.
- `wlx_scope_push`/`wlx_scope_pop` unify the `.id` → id-stack push/pop pattern used by every
  frame helper. Under `WLX_DEBUG`, `wlx_end` asserts the id-stack returns to zero each frame.
- `wlx_contribute_to_parent_layout` (`WLX_Parent_Contribution`) consolidates parent
  content-tracking for `wlx_widget_begin`, `wlx_layout_end`, and scroll-panel.
- `bool wrap` split from `WLX_TEXT_TYPOGRAPHY_FIELDS` into `WLX_TEXT_WRAP_FIELDS`; slider
  resolver aligned with shared `wlx_resolve_typography`/`wlx_resolve_border` helpers.

## [0.4.0] - 2026-05-09

### Added
- **Slice-aware text paths:** added public `wlx_measure_text_slice()` and
  `wlx_draw_text_slice()` helpers, optional `WLX_Backend.draw_text_slice` /
  `WLX_Backend.measure_text_slice` callbacks, and slice-first core/replay
  routing while preserving C-string compatibility.
- **Performance diagnostics:** added opt-in `WLX_PERF` core and backend
  snapshots, gallery perf builds with summaries/CSV output, public diagnostics
  docs, and regression tests.
- **Backend text caches:** added retained SDL3 `TTF_Text` caching with font
  variants, flush/lifecycle helpers, and counters; added Raylib text
  measurement caching with cap-zero disable, generation-bump clearing, and
  cache stats.
- **Gallery and design system updates:** added a grouped gallery shell,
  Overview, Semantic Tokens, Theme Lab, Brand theme preview, responsive section
  templates, refreshed gallery assets, and public design-system documentation.
- **Panel and typography options:** added panel body-frame controls on
  `WLX_Panel_Opt` and restored opt-in `spacing` across text style and widget
  typography options.

### Changed
- Reworked fitted text into a UTF-8-safe line/run model with explicit newline
  handling, shared cursor layout, fewer emitted text commands, and private
  builder refactors that keep public APIs stable.
- Updated SDL3 and Raylib text rendering paths for better measurement reuse,
  coordinate snapping, font scaling, and custom-font spacing behavior.
- Improved gallery layout behavior around section splits, theme controls,
  status chrome, semantic colors, and version display.
- Refined content sizing and vertical parent height accounting for more stable
  nested layout behavior.

### Fixed
- Fixed checkbox label width math, slider fill/roundness consistency, missing
  widget ID scopes, Theme Lab padding/preview sizing, and stale gallery version
  display.
- Fixed fitted-text and inputbox clipping so scissor scopes are installed only
  when needed and cursor clipping remains independent.
- Hardened SDL3 text cache teardown, renderer-change invalidation,
  font-variant fallback paths, and text rasterization stability.

## [0.3.0] - 2026-04-26

### Added
- Scoped container IDs: `WLX_Layout_Opt`, `WLX_Grid_Opt`, `WLX_Grid_Auto_Opt`,
  `WLX_Panel_Opt`, and `WLX_Split_Opt` now accept `const char *id`, pushing a
  scope ID for the full container body. `wlx_scroll_panel_begin` /
  `wlx_scroll_panel_end` now do the same for scroll-panel bodies. The identity
  model is now documented in `wollix.h` and `docs/API_REFERENCE.md`, including
  Widget ID / State ID / Scope ID roles and guidance on when
  `wlx_push_id` / `wlx_pop_id` is still needed.

  **Migration note:** callers that both pass scroll-panel `.id` and wrap the
  panel body in manual `wlx_push_id` / `wlx_pop_id` calls will see descendant
  state keys change because the panel scope now stays active for the full body.
- New widgets and styling APIs: added `wlx_progress`, `wlx_toggle`,
  `wlx_radio`, and `wlx_separator`; added rounded backgrounds and borders for
  widgets and containers; and added per-slot / per-grid-cell decoration plus
  one-shot style overrides.
- Theming additions: added built-in `wlx_theme_glass`, theme sub-structs for
  toggle, radio, and progress styling, advisory size/style constants, and
  `WLX_SHORT_NAMES` aliases for `progress`, `toggle`, and `radio`.
- Layout additions: added per-side padding across layouts, panels, grids,
  splits, and scroll panels, plus per-slot `gap` support.
- Backend and web additions: `WLX_Backend` now supports
  `draw_rect_rounded_lines`, plus optional `draw_circle` and `draw_ring`
  callbacks with widget-level fallbacks. Raylib and WASM expose native
  circle/ring support, SDL3 now includes rounded-shape tessellation helpers,
  and the bare-WASM backend/browser host gained optional FPS limiting.
- Demo and docs additions: added `demos/auth.c`, expanded the gallery with
  themed control sections and layout examples, and added `docs/CORE_PATTERNS_GUIDE.md`
  plus `docs/SENTINEL.md`.

### Changed
- `gallery.c` is now the main cross-backend gallery source for Raylib, SDL3,
  and WASM, with backend-specific setup moved into platform layers and the
  current demo layout retuned for a wider sidebar.
- `gallery.c` now derives shared demo chrome from a private gallery semantic
  helper layer, centralizing elevated surfaces, muted text, selection,
  border-emphasis, and status or destructive treatment without expanding the
  public `WLX_Theme` API.
- The same gallery semantic helpers now drive scroll-panel lists, theme-preview
  cards, helper metadata, and progress-state badges so the proving surfaces use
  the private token roles beyond shell chrome.
- Default rendering is now deferred through a per-frame command buffer
  replayed by `wlx_end`; use `wlx_begin_immediate` to opt out. Public examples
  and docs now consistently call `wlx_end(ctx)` before `EndDrawing()` /
  `SDL_RenderPresent(ren)`.
- `WLX_Context` per-frame storage now lives in `WLX_Arena_Pool`, with new
  `wlx_context_init` and `wlx_context_init_ex` entry points plus
  allocator-group configuration.
- Theme presets and demo styling were refreshed across dark, light, and glass
  modes, and toggle/radio label placement now respects the `align` field via
  `wlx_get_align_rect`.
- Text measurement, wrapping, and input/gallery sizing were refined for more
  stable two-pass layout behavior and cleaner demo ergonomics.

### Changed (Breaking)
- **Breaking:** `WLX_Theme.toggle` gained `track_to_height_ratio` (default 0 =
  use 2.0) and `thumb_inset_ratio` (default 0 = use 0.15). `WLX_Theme.radio`
  gained `selected_inset_ratio` (default 0 = use 0.25). `WLX_Theme` gained a
  global `min_rounded_segments` field (built-in presets set to 16) replacing
  the hardcoded `< 16 ? 16` clamp in toggle and radio impls. Custom themes
  that zero-initialize from struct literals gain the new fields at zero, which
  preserves previous visual output.
- **Breaking:** `WLX_Theme` gained a `progress` sub-struct with `track`,
  `fill`, and `track_height` fields. `wlx_resolve_opt_progress` now reads
  from `theme->progress` first and falls back to `theme->slider.track` /
  `theme->accent` only when the progress-specific fields are zero. Custom
  themes that zero-initialize the new sub-struct retain previous visual output.
  Built-in presets (`wlx_theme_dark`, `wlx_theme_light`, `wlx_theme_glass`)
  have been updated with dedicated progress colors.

### Fixed
- Fixed nested CONTENT bootstrap deadlocks by seeding unmeasured slots,
  improving intrinsic height contribution, and including child padding in
  linear parent measurements.
- Fixed overly noisy nested CONTENT debug warnings while preserving
  widget/content-slot diagnostics for real layout issues.
- Fixed gap handling so slot and grid rects no longer paint into visual gap
  space, and dynamic linear/grid paths now apply spacing consistently.
- Fixed dynamic-layout and nested-static-layout contention by moving dynamic
  offsets into a dedicated `dyn_offsets` arena.
- Fixed `wlx_widget` so opacity and hover brightness apply to border color as
  well as fill color.
- Fixed sentinel resolution for negative and float-valued option fields,
  including border width, roundness, scrollbar width, and hover-brightness
  fields.
- Fixed `WLX_Cmd_Range.parent_range_idx` back to `int` with
  `WLX_NO_RANGE == -1`, removing redundant casts and restoring the intended
  internal sentinel handling.
- Fixed WASM host input edge cases, SDL3 multisampling, a dangling-pointer bug
  in `wlx_grid_begin_impl`, content-based debug-warning deduplication, and
  assorted demo sizing issues.
- Fixed documentation drift around backend requirements, sentinel
  conventions, the identity model, and frame-loop ordering.

### Removed
- Deprecated `wlx_is_unset()` removed. Use `wlx_is_negative_unset()` for `-1`
  sentinel fields and `wlx_is_float_unset()` when other negative values are
  valid.
- **Breaking:** Removed `WLX_Grid_Cell_Opt` and `wlx_default_grid_cell_opt`.
  `row_span` and `col_span` now live on `WLX_Slot_Style_Opt` (defaults remain
  `1`), and `wlx_grid_cell()` now takes `WLX_Slot_Style_Opt` directly. Calls
  like `wlx_grid_cell(ctx, r, c, .row_span = 2, .col_span = 2)` still work,
  but named `WLX_Grid_Cell_Opt` variables must migrate to `WLX_Slot_Style_Opt`.
- Removed outdated `demos/gallery_new.c` after folding its content into the
  main gallery flow.

### Internal
- Introduced shared helper and macro layers for command recording, widget
  frames, layout/container decoration, padding, opacity, typography, border
  resolution, and box drawing.
- Unified the five layout-begin entry points behind shared frame/common
  helpers, extracted the CONTENT sizing lifecycle into named helpers,
  decomposed `wlx_scroll_panel_begin_impl`, and moved widget wrappers onto a
  balanced frame lifecycle.
- Refactored per-frame storage around `WLX_Arena_Pool`, `WLX_Sub_Arena`, a
  WASM page pool, `dyn_offsets`, and `WLX_ARENA_POOL_FIELDS(X)`, while
  switching layout internals to on-demand offset/content-height accessors.
- Added SDL3-local tessellation helpers and expanded regression coverage with
  new suites including `test_circle_dispatch`, `test_cmd_replay`,
  `test_container_scope`, `test_dyn_offsets`, `test_rounding`,
  `test_slot_style`, `test_sub_arena`, `test_text_layout`, `test_wasm_pool`,
  `test_widget_wrapper`, and `test_widgets`.

## [0.2.0] - 2026-04-07

### Removed
- **Breaking:** Removed `wlx_checkbox_tex` compatibility macro and
  `checkbox_tex` short alias. Use `wlx_checkbox` with `.tex_checked` /
  `.tex_unchecked` fields instead (drop-in replacement).

### Changed
- **Breaking:** Renamed `.boxed` to `.show_background` on `WLX_Label_Opt`,
  `WLX_Checkbox_Opt`, and `WLX_Checkbox_Tex_Opt`. Removed `.boxed` from
  `WLX_Button_Opt` entirely (it was dead code — buttons always draw their
  background). Checkbox/checkbox_tex opts gain a new `full_slot_hit` field
  (default `true`) that controls whether the full slot rect is used for
  hover/click interaction, decoupling hit-testing from the visual background.
  Migration: `.boxed = true` → `.show_background = true` on labels and
  checkboxes; remove `.boxed` from button calls.
- **Breaking:** Renamed `wlx_textbox` to `wlx_label` (and `WLX_Textbox_Opt`
  to `WLX_Label_Opt`). Search-replace `wlx_textbox` → `wlx_label` in your
  code to migrate.
- **Breaking:** Interaction IDs no longer include a per-frame sequential counter.
  IDs are now `hash(file, line) ^ id_stack_hash`, matching persistent state IDs.
  Loops that emit interactive widgets MUST use `wlx_push_id()`/`wlx_pop_id()`.
- **Breaking:** `wlx_resolve_opt_*` functions now take `const WLX_Context *`
  instead of `const WLX_Theme *` to access the context opacity stack.
- Layout dimensions switched from `int16_t` to `float` with pixel-aligned
  rounding for sub-pixel accuracy.
- Refactored `wlx_widget()` to use `WLX_Widget_Opt` struct with designated
  initializer syntax (e.g. `wlx_widget(ctx, .color = red)`), replacing the
  old positional color argument.
- Refactored state pool from a linear array to an open-addressing hashmap for
  faster persistent state lookups.
- Slider value area uses fixed width for consistent geometry across values.

### Added
- **Three-layer opacity model:** final opacity is
  `widget.opacity * theme.opacity * ctx_stack_opacity`. Each layer defaults
  to 1.0 (fully opaque) when omitted. See `docs/OPACITY.md`.
  - Per-widget `.opacity` field on all widget option structs (negative
    sentinel = unset).
  - `WLX_Theme.opacity` field for global UI transparency.
  - `wlx_push_opacity()` / `wlx_pop_opacity()` / `wlx_get_opacity()` for
    scoped region-level opacity. Nested pushes multiply automatically.
  - `wlx_resolve_opacity()` and `wlx_color_apply_opacity()` helpers.
- **Layout sizing:** `WLX_SIZES` macro and `wlx_layout_begin_s` for
  auto-counted slot size arrays. `wlx_layout_auto_slot` for dynamic
  mixed-size layouts. `WLX_SIZE_CONTENT` and `WLX_SIZE_FILL` slot size
  types for content-driven and viewport-relative sizing.
- **Compound panel widget:** `wlx_panel_begin` / `wlx_panel_end` — a
  capacity-based CONTENT layout with optional heading label. Eliminates
  manual slot counting for all-CONTENT layouts. Pre-allocates 32 CONTENT
  slots by default; unused slots contribute 0px. See `docs/WIDGETS.md`
  and `docs/API_REFERENCE.md`.
- **Compound split widget:** `wlx_split_begin` / `wlx_split_next` /
  `wlx_split_end` — a two-pane split layout with independent scroll panels.
  Replaces the common 6-pair begin/end sidebar+content pattern with 3 calls.
  Configurable pane sizes, gap, fill behavior, and per-pane background colors
  via `WLX_Split_Opt` designated initializers. See `docs/WIDGETS.md` and
  `docs/API_REFERENCE.md` for full documentation.
- **Border support:** `border_color` and `border_width` fields on widget
  option structs. Border thickness clamped to 1px minimum with alpha scaling
  for sub-pixel values.
- **Debug instrumentation:** `WLX_DEBUG` compile-time mode with
  `WLX_Debug_Companion` for content-slot oscillation detection, site-based
  warning system, and vertical bounding checks. Warns when the same
  call-site is hit multiple times per frame without `wlx_push_id()`.
- **Inputbox improvements:** `wlx_inputbox_handle_keys` for extracted key
  handling. Width guard guaranteeing minimum `font_size * 3` pixels.
  `content_padding` auto-shrink when slot constrains widget height.
- `wlx_parent_rect()` and `wlx_viewport_rect()` layout query helpers.
- Content height tracking for horizontal and vertical layouts.
- `WLX_Scratch_Bytes` byte-level scratch arena.
- Optional `.id` field on all widget option structs for explicit string-based
  identity (e.g. `.id = "my_button"`). When set, the string is hashed and
  pushed onto the ID stack automatically, giving widgets stable IDs independent
  of source location.
- `wlx_hash_string()` helper — djb2 hash for string-to-ID conversion.
- SDL3_ttf font support in the SDL3 backend (`wollix_sdl3.h`).
- Inter and Public Sans font assets in `demos/assets/`.
- New demos: `border_demo`, `opacity_demo` (three-layer model).
- Expanded `gallery.c` with all new features.
- Updated `font_demo.c` with new font showcases.
- New test suites: `test_auto_layout`, `test_split`, `test_panel`,
  `test_opacity_stack`.
- Expanded existing test suites: `test_layout_math`, `test_edge_cases`,
  `test_scroll_panel`, `test_interaction`.
- New documentation: `docs/OPACITY.md`.
- Expanded `docs/API_REFERENCE.md`, `docs/LAYOUT_MODEL.md`,
  `docs/WIDGETS.md`.
- CI: Raylib and SDL3 demo builds, sanitizer tests, `-Werror` test.

### Fixed
- `active_id_seen` tracking bug where stale active IDs could persist across
  frames when the originating widget was removed.
- Auto-height scroll panel content height double-counting regression.

## [0.1.0] - 2026-03-21

### Added
- Initial release of wollix
- Single-header immediate-mode GUI library
- Raylib backend (`wollix_raylib.h`)
- SDL3 backend (`wollix_sdl3.h`)
- Flexbox-inspired layout system (row, column, wrap)
- Grid layout support
- Scrollable panels
- Widget library: buttons, checkboxes, sliders, text input, labels
- Theming and opacity support
- UTF-8 text handling
- Demo programs and documentation

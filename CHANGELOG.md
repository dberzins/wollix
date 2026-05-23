# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

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

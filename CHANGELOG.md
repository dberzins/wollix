# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- **Disabled-state coverage rule + docs.** `wlx_widget_impl` and
  `wlx_label_impl` now route their interaction call through
  `wlx_get_interaction_for(..., false, ...)` instead of the legacy
  `wlx_get_interaction`, and use `wlx_color_hover_tint` for hover-tint;
  the call shape now matches every disabled-aware widget so a future
  `.disabled` opt-in is a one-line change. Neither option struct gains
  `.disabled` in this slice (decision documented inline and in
  `docs/dev/ADR_018_DISABLED_STATE_MODEL.md`). ADR_018 grows a "Coverage
  Rule and Matrix" section; `docs/WIDGETS.md` gains a "Disabled state"
  reference section covering the coverage matrix, the `.disabled` effect,
  theme knobs, and the "add `.disabled` to a new widget" checklist;
  `docs/API_REFERENCE.md` documents `wlx_get_interaction_for`,
  `WLX_Interaction.disabled`, `theme->disabled_brightness`, and
  `theme->disabled_opacity`.
- **Internal: split paragraph-wrap out of typography field macro and aligned
  slider with V03 resolver helpers.** `bool wrap` moved from
  `WLX_TEXT_TYPOGRAPHY_FIELDS` into a dedicated `WLX_TEXT_WRAP_FIELDS`
  alongside `WLX_TEXT_WRAP_DEFAULTS` (`.wrap = false`). Label, button,
  checkbox, inputbox, toggle, and radio option structs now embed both
  macros; their default-opt macros drop redundant `.wrap = false` lines and
  keep explicit `.wrap = true` for label/button/inputbox where that is the
  per-widget default. `WLX_Slider_Opt` adopts the slim
  `WLX_TEXT_TYPOGRAPHY_FIELDS` (no wrap, since slider renders single-line
  text), and `wlx_resolve_opt_slider` now resolves typography and border
  through the shared `wlx_resolve_typography` / `wlx_resolve_border`
  helpers and finishes with `WLX_RESOLVE_VISUAL_STATE` — matching every
  other resolver. The `min_rounded_segments` clamp,
  `thumb_hover_brightness = hover_brightness * 0.5f` derivation, and
  `track_height` / `thumb_width` theme fallbacks are preserved. Public
  designated-init surface is unchanged: `.wrap`, `.font`, `.font_size`,
  `.align`, `.spacing` continue to compile and produce byte-identical
  defaults on every widget that exposed them previously.
- **Internal: unified resolver visual-state tail and hover-tint gate.** Added
  internal `WLX_RESOLVE_VISUAL_STATE(ctx, opt_ptr, disabled, /*colors...*/)`
  macro that names the color list exactly once and folds
  `wlx_resolve_opacity_for` + `WLX_APPLY_DISABLED` + `WLX_APPLY_OPACITY` into
  a single resolver-tail call. Every `wlx_resolve_opt_*` resolver (button,
  checkbox, inputbox, slider, toggle, radio, plus the decorative label,
  progress, image, widget, scroll-panel set called with `disabled=false`)
  now ends with one macro invocation. Added internal `wlx_color_hover_tint`
  helper that centralizes the `(hover && !disabled) ? brightness : c` gate
  for button, checkbox, toggle, radio, inputbox, slider hover-tint sites
  (`wlx_widget_impl` / `wlx_label_impl` keep the unguarded brightness call
  pending the disabled-coverage decision in the next phase). No public API
  change; no observable behavior change (full test suite + demos pass).
- **Internal: converged parent content-tracking contribution into a single
  helper.** Added internal `WLX_Parent_Contribution` struct and
  `wlx_contribute_to_parent_layout`, used by `wlx_widget_begin`,
  `wlx_layout_end`, and `wlx_scroll_panel_contribute_to_parent` to update the
  parent layout's `accumulated_content_height`, per-slot CONTENT bucket, and
  per-row grid bucket through a single code path. No public API change; no
  observable behavior change (byte-identical contribution math, full test
  suite + demos pass).
- **Gallery semantic spacing:** the gallery now derives local spacing roles
  (`space_shell`, `space_panel`, `space_heading_x`, `space_heading_y`,
  `space_control_x`, `space_preview`, `gap_dense`, and `gap_section`) from
  `WLX_Theme.padding`. Headings, option labels, title/sidebar/status chrome,
  token cards, Theme Lab preset cards, and the component preview matrix now use
  those roles for consistent content inset while raw layout/default-widget
  examples keep explicit geometry. This is gallery-local only; no public theme
  fields or core widget defaults changed.
- **Unified content padding across all chrome widgets.** `WLX_Split_Opt` and
  `WLX_Panel_Opt` now embed `WLX_CONTENT_PADDING_FIELDS` (`content_padding`,
  `content_padding_top`, `content_padding_right`, `content_padding_bottom`,
  `content_padding_left`). The old `WLX_PADDING_FIELDS` / `.padding` /
  `.padding_top` / `.padding_right` / `.padding_bottom` / `.padding_left`
  fields on Split and Panel are removed — rename call sites to the
  `content_padding*` equivalents. `WLX_Inputbox_Opt.content_padding` remains
  supported (same scalar default of `10`) and gains per-side overrides
  (`content_padding_top` / `_right` / `_bottom` / `_left`). The text cursor
  rect inside the editing box is now inset by the fixed constant
  `WLX_INPUTBOX_TEXT_INSET` (5 px) instead of the computed `content_padding /
  2`; with the default `content_padding = 10` the rendered output is
  byte-identical to the previous implementation. All five chrome widgets
  (`label`, `button`, `split`, `panel`, `inputbox`) resolve padding through
  the shared `wlx_resolve_content_padding` helper and support the
  `WLX_PADDING_USE_THEME` sentinel. `WLX_PADDING_FIELDS`,
  `WLX_PADDING_DEFAULTS`, and `WLX_RESOLVE_PADDING` macros are removed.

### Added
- **Content padding extended to chrome widgets:** `wlx_checkbox`, `wlx_radio`,
  `wlx_toggle`, `wlx_slider`, and `wlx_progress` now embed
  `WLX_CONTENT_PADDING_FIELDS` (`content_padding`, `content_padding_top`,
  `content_padding_right`, `content_padding_bottom`, `content_padding_left`).
  The inset shrinks the compound content rect (glyph + label, label/track/
  value region, or the progress track) while the click/hover hit rect and
  slot contribution stay at the full widget rect; checkbox honours
  `full_slot_hit`, slider's drag hit-rect tracks the inset track region,
  and progress clamps its rendered track height to the inset height.
  Defaults resolve to zero so existing call sites are pixel-stable; pass
  `.content_padding = WLX_PADDING_USE_THEME` to opt into the theme's
  `padding` knob. Gallery demos show baseline + padded variants for each
  widget.
- **Content padding for `wlx_label` and `wlx_button`:** five new option
  fields (`content_padding`, `content_padding_top`, `content_padding_right`,
  `content_padding_bottom`, `content_padding_left`) plus a public
  `WLX_PADDING_USE_THEME` sentinel inset only the content (text + image)
  rect of a label or button. Chrome (background, border) and the click
  hit rect remain at the full widget rect, so a padded button still gets
  its full clickable area. Defaults resolve to zero so existing call
  sites keep their pre-change visuals; pass
  `.content_padding = WLX_PADDING_USE_THEME` to opt into the theme's
  `padding` value (`WLX_STYLE_CONTENT_PADDING` by default). Resolved
  insets are clamped proportionally so a content rect never has negative
  dimensions on tight slots. `demos/button` and `demos/label_image`
  showcase the new rows. No backend or theme schema changes.
- **Gallery navigation icon expansion:** the curated gallery atlas grew from
  12 to 32 icons under `demos/assets/icons/lucide-svg/`, adding `app-window`,
  `chevron-right`, `house`, `blocks`, `layout-dashboard`, `route`,
  `sliders-horizontal`, `type`, `mouse-pointer-click`, `text-cursor-input`,
  `component`, `align-horizontal-space-between`, `grid-3x3`,
  `stretch-horizontal`, `layout-template`, `scroll-text`, `layers`, `blend`,
  `square-dashed`, and `toggle-left`. Three gallery-local helpers
  (`gallery_icon_button`, `gallery_icon_label`, `gallery_panel_heading`) route
  the new icons into the title bar, theme selector, status bar, overview
  quick-link chevrons, the Welcome / Demo & Links / Theme Controls / 15
  `Options` panel headings, the `Use Custom Theme` apply action, and the
  sidebar group rows (Phase 4a). Sidebar section icons are code-complete and
  ship after the operator-gated density review. All helpers fall back to
  text-only when the atlas is unavailable or the caller passes
  `WLX_ICON_COUNT` as the icon. No public API change.
- **Gallery Lucide icon atlas:** the gallery now uses a committed 12-icon
  Lucide 1.14.0 white-alpha atlas in `demos/assets/wlx_icons.h` for
  icon-backed label, button, image, texture-checkbox, and Theme Lab
  examples. The first set is `check`, `x`, `info`, `triangle-alert`,
  `image`, `palette`, `save`, `rotate-ccw`, `settings`, `play`, `square`,
  and `square-check`. Source SVGs, manifest, and license live under
  `demos/assets/icons/`; regenerate the header with
  `bash tools/gen_icon_atlas.sh`.
- **Image-capable checkbox texture mode:** `wlx_checkbox` texture mode now
  supports per-state source rects and per-state tints through four new
  `WLX_Checkbox_Opt` fields: `tex_checked_src`, `tex_unchecked_src`,
  `tex_checked_tint`, and `tex_unchecked_tint`. Unset source rects fall back
  to the full selected texture; unset tints resolve to `WLX_WHITE`. Both
  tints participate in opacity resolution alongside the existing colors. This
  brings checkbox texture mode in line with the `wlx_label`, `wlx_button`,
  and `wlx_image` source-rect and tint contract, and enables a single shared
  icon atlas to drive both states. New `demos/checkbox_tex` rows exercise the
  shared-atlas path with semantic per-state tints.
- **Image-capable label:** `wlx_label` now supports text-only and
  text + image content modes through the existing `wlx_label(ctx, text, ...)`
  call, with image-only as a supported edge case (use `wlx_image` for pure
  image content). New `WLX_Label_Opt` fields mirror the button surface
  (`texture`, `texture_src`, `texture_scale`, `texture_tint`,
  `image_placement`, `image_size`, `image_text_gap`). No separate
  `wlx_image_label` / `wlx_label_image` function or option struct is
  introduced. Labels remain non-interactive: hover-brightness still applies
  only to the optional background when `show_background = true`, and
  texture tint folds through the opacity stack but is never modulated by
  hover. The private image content helpers used by `wlx_button` were
  renamed to widget-neutral names (`wlx_widget_has_image`,
  `wlx_widget_image_src`, `wlx_widget_auto_image_size`,
  `wlx_widget_layout_image_text`) and now back both widgets without
  option-struct coupling. New `demos/label_image` Raylib demo and a
  gallery row in the Label section exercise text-only, text + image,
  image-only, four placements, four scale modes, and a row with
  `show_background = true`.
- **Image-capable button:** `wlx_button` now supports text-only, image-only,
  and image + text content modes through new `WLX_Button_Opt` fields
  (`texture`, `texture_src`, `texture_scale`, `texture_tint`,
  `image_placement`, `image_size`, `image_text_gap`). Mode is selected by
  the inputs — there is no separate `wlx_image_button` function or option
  struct. New public `WLX_Image_Placement` enum
  (`LEFT`/`RIGHT`/`TOP`/`BOTTOM`) controls image position relative to text;
  `align` continues to position the combined block. The shared
  source/destination fitting math is now a private helper used by both
  `wlx_image` and `wlx_button`. Texture tint folds through the opacity
  stack; hover modulates only the chrome background. New
  `demos/button_image` Raylib demo and a gallery row in the Button section
  exercise all three modes, four placements, and four scale modes.
- **Image widget:** new `wlx_image` widget with `WLX_Image_Scale` enum
  (`STRETCH`, `FIT`, `FILL`, `NONE`), `WLX_Image_Opt` option struct
  (scale mode, image-content alignment, tint, source sub-rect, opacity
  folding), and a standalone `demos/image` demo exercising all four scale
  modes, alignment variants, and an opacity-fade pane. Gallery panel added
  under the Components group.
- **WASM texture support:** bare-WASM backend now provides texture
  authoring through new `create_texture` / `destroy_texture` host imports
  and `wlx_wasm_texture_create` / `wlx_wasm_texture_destroy` C helpers.
  The JavaScript host owns a per-texture `OffscreenCanvas` registry (with
  detached `<canvas>` fallback for older Safari) and copies RGBA pixel
  data out of WASM memory at upload time so memory-growth events do not
  invalidate stored textures. The WASM gallery now renders a procedural
  landscape image in all four `wlx_image` scale modes and the texture-mode
  checkbox sample, both backed by C-side RGBA generators.

### Changed
- **Gallery icon atlas rasterises four tiers (16 / 24 / 32 / 48 px).** The
  committed `demos/assets/wlx_icons.h` is now a 576x120 row-per-tier white-alpha
  atlas with per-tier source-rect metadata (`wlx_icon_tier_sizes[]`,
  `wlx_icon_tier_y[]`, `wlx_icon_rects_tiered[TIER][ICON]`). The original 1D
  `wlx_icon_rects[]` table is retained for backward compatibility and equals
  the 16 px tier. Gallery callsites pick a tier through the size-aware helper
  `gallery_icon_src_for(WLX_Icon, float target_px)`, which chooses the
  smallest tier >= `ceilf(target_px)` and falls back to 48 px for larger
  targets. Status, button (including slider extremes), checkbox, image
  preview, and Theme Lab matrix surfaces all route through the helper so
  larger gallery icons stay crisp without relying on backend interpolation.
  Regenerate with `bash tools/gen_icon_atlas.sh`.
- **Checkbox texture-mode activation now requires both state textures.**
  Texture mode previously activated whenever `tex_checked.width > 0`, so a
  checkbox with `tex_checked` set but `tex_unchecked` unset would draw an
  empty texture in the unchecked state. Texture mode now activates only when
  **both** `tex_checked` and `tex_unchecked` are drawable; if either is
  missing, both states fall back to the native indicator. This prevents
  half-configured checkboxes from silently rendering an empty unchecked
  state.
- **Checkbox texture-mode default tint is now `WLX_WHITE`.** Texture mode
  previously tinted the texture with the resolved checkbox background color
  (and hover brightness). It now resolves an unset `tex_checked_tint` or
  `tex_unchecked_tint` to `WLX_WHITE` so authored texture colors render
  predictably, matching the rest of the texture-capable widget surface.
  Hover brightness no longer modulates texture tint; hover feedback remains
  a native-mode concern. `check_color` is documented as native-mode only
  and is ignored in texture mode.
  **Migration:** callers that relied on the old background-derived tinting
  should set `.tex_checked_tint` and `.tex_unchecked_tint` explicitly to
  recover the previous tone.

### Limitations
- **WASM tint is alpha-only.** Texture tint on the bare-WASM backend
  honours the alpha channel via Canvas `globalAlpha`; non-white RGB tint
  channels are ignored and emit a one-shot console warning. Full RGB
  modulation is deferred (see ADR 014).

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

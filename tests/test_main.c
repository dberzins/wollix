// test_main.c - entry point for wollix.h test suite
// Single-TU build: all test files are #included here.

#define WLX_DEBUG
// Disable the wlx_image empty-texture assert so test_image can exercise the
// safe no-op fallback path without aborting the runner.
#define WLX_IMAGE_ASSERT_TEXTURE_VALID(tex) ((void)(tex))
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_editor.h"

#include "tests.h"
#include "test_mock_backend.h"

// Test files (pure math - no backend needed)
#include "test_layout_math.c"
#include "test_slot_redistribute.c"
#include "test_color.c"
#include "test_utf8.c"
#include "test_core_utils.c"
#include "test_sub_arena.c"

// Test files (need mock backend / WLX_Context)
#include "test_grid.c"
#include "test_interaction.c"
#include "test_scroll_panel.c"
#include "test_input.c"
#include "test_input_contract.c"
#include "test_input_selection.c"
#include "test_input_clipboard.c"
#include "test_input_modes.c"
#include "test_input_multiline.c"
#include "test_input_scroll.c"
#include "test_frame_time.c"
#include "test_focus_release.c"
#include "test_cursor_shape.c"
#include "test_tab_traversal.c"
#include "test_content_axis.c"
#include "test_overlay.c"
#include "test_dropdown.c"
#include "test_tooltip.c"
#include "test_menu.c"
#include "test_auto_layout.c"

// fuzz + edge cases
#include "test_fuzz.c"
#include "test_edge_cases.c"

// compound widgets
#include "test_split.c"
#include "test_panel.c"

// opacity
#include "test_opacity_stack.c"

// rounding, separator, layout background
#include "test_rounding.c"

// Phase 2 widgets: progress, toggle, radio
#include "test_widgets.c"

// Circle/ring dispatch routing
#include "test_circle_dispatch.c"

// Draw command replay tests
#include "test_cmd_replay.c"

// Opt-in layout clip (scissor recording, panel-clip parity, balanced nesting)
// and segmented progress cell containment. Included after test_cmd_replay.c to
// reuse the crec_* recording backend.
#include "test_layout_clip.c"

// Slot and grid cell decoration / style overrides
#include "test_slot_style.c"

// Per-side border colors and widths: resolver sentinels + all_equal, per-side
// hover/disabled tint, and widget + container command-replay (incl. uniform
// no-regression for sharp and rounded borders).
#include "test_per_side_border.c"

// Option default and sentinel contract: explicit zero, unset inheritance,
// the signed-domain brightness sentinel, request tokens, and the v0.9
// enumerated default changes.
#include "test_sentinel.c"

// Interaction-aware containers (.interact / .interact_out / hover-variant
// chrome) and the per-call wlx_button hover override.
#include "test_interactive_container.c"

// WASM clipboard transport against a test-TU host (owns the wasm header's
// first include so its cap override applies)
#include "test_wasm_clipboard.c"

// WASM page-pool allocator
#include "test_wasm_pool.c"

// dyn_offsets arena isolation
#include "test_dyn_offsets.c"

// Two-pass text layout regression (kerning mock)
#include "test_text_layout.c"

// Text spacing opt-in API: default natural spacing and explicit propagation
#include "test_text_spacing.c"

// Widget wrapper contract: id stack balance and opacity/hover-brightness coverage
#include "test_widget_wrapper.c"

// Container Scope ID isolation and no-id collision regression
#include "test_container_scope.c"

// WLX_PERF instrumentation: disabled-build coverage plus guarded histogram,
// text counter, arena high-water, timer, and immediate-mode snapshot tests.
#include "test_perf.c"

// wlx_image scale modes, alignment anchors, src defaults, tint + opacity
#include "test_image.c"

// Image-capable wlx_button: text-only regression, image-only, image + text,
// placement, scale parity, empty fallbacks, opacity, hover, slot consumption
#include "test_button_image.c"

// Image-capable wlx_label: text-only regression (no chrome by default),
// image-only edge case, image + text, placement, scale parity with wlx_image,
// empty fallbacks, opacity, hover, slot consumption, non-interactive contract
#include "test_label_image.c"

// wlx_label .style aggregate intake and .vertical_metric centering basis.
#include "test_label_style.c"

// Image-capable wlx_checkbox texture mode: per-state src and tint, white
// default tint with opacity folding, both-textures-required activation,
// fallback to native when either state texture is missing, hover and
// check_color isolation, layout slot consumption.
#include "test_checkbox_texture.c"

// Inputbox inner icon: text-only regression, leading/trailing band placement,
// vertical centering independent of opt.content_align, default/explicit tint, and the
// narrow-field clamp guard.
#include "test_inputbox_icon.c"

// Content padding on wlx_label / wlx_button: default zero, uniform,
// asymmetric per-side, per-side override, theme opt-in via
// WLX_PADDING_USE_THEME, clamp on tight rect, chrome rect unchanged,
// hit-rect unchanged, image-only inset, image+text inset, slot+content
// padding composition.
#include "test_content_padding.c"

// Cross-widget invariants for the unified WLX_CONTENT_PADDING_FIELDS shape.
#include "test_padding_alignment.c"

// Inputbox per-side migration: visual stability, theme opt-in, clamp.
#include "test_inputbox_padding.c"

// Content padding on wlx_checkbox / wlx_radio / wlx_toggle / wlx_slider /
// wlx_progress: per-widget chrome shifts in unison with the resolved
// content_rect; hit-rect on checkbox keeps using wr when full_slot_hit is
// set; progress clamps track height to the inset rect.
#include "test_missing_padding.c"

// Disabled-state model: interaction gating, hover-tint suppression,
// brightness + opacity transforms, sentinel inheritance, theme defaults,
// back-compat default behaviour.
#include "test_disabled_state.c"

// Dashboard demo token model: surface ramp monotonicity, alpha ranges,
// required-role population, typography validity, strict 4px spacing, ascending
// radius, per-effect intent, and runtime font resolution for both modes.
#include "test_dashboard_tokens.c"

// Dashboard token -> WLX_Theme mapper: baseline color subset, widget overrides,
// sentinel resolution, font resolution, and dark/light divergence.
#include "test_dashboard_theme.c"

// Dashboard component pure helpers: responsive columns, segment fill, pulse
// alpha, table row geometry, ASCII uppercasing.
#include "test_dashboard_components.c"

// Dashboard visual-effect pure helpers: color interpolation, desaturation,
// frosted blur tint, glow ring alpha falloff.
#include "test_dashboard_effects.c"

// First-class glow/shadow decorations: fallback geometry/alpha + demo parity,
// numeric-knob resolution, zero-color sentinel, opacity premultiply, render
// order, and replay dispatch (software fallback vs native callback, immediate).
#include "test_glow_shadow.c"

#include "test_gradient.c"

// Absolute pixel corner radius (corner_radius): px->fraction helper math,
// size-invariance, precedence over roundness, off-by-default byte identity,
// clamp at min/2, and container decor resolution.
#include "test_corner_radius.c"

// Slice-only backend contract: readiness with only the slice text callbacks,
// deferred replay and immediate draws routed through draw_text_slice.
#include "test_slice_backend.c"

// List clipper virtualization: fixed-pitch range/positioning, virtualized
// visible-count, and variable-height range via item_offsets.
#include "test_list_clipper.c"

// Opt-in offscreen command culling: default-off parity, drop-outside/keep-inside,
// edge-touch, excluded text, and nested-clip intersection (superset safety).
#include "test_offscreen_cull.c"

// Windowed text-build entries: build-from-offset window/tail equivalence and
// non-wrap truncate-and-continue (per-record budget, tail skip, source tiling).
#include "test_editor_pipeline.c"

// wlx_editor windowed view: line index construction + guards, anchor clamp,
// wheel consume/leave rules, exact vertical thumb, window draw at the anchor.
#include "test_editor_view.c"

// wlx_editor caret/selection/hit-test: mouse caret in scrolled windows,
// multi-click, drag auto-scroll both axes, keyboard motion incl. sticky
// column and caret-coupled paging, caret-follow, strip press exclusion.
// Included after test_editor_view.c to reuse its ev_* fixture.
#include "test_editor_caret.c"

// wlx_editor editing: typing/Enter/Tab/Backspace/Delete incl. word variants,
// clipboard cut/copy/paste, length in/out contract, rebuild-on-edit,
// revision interplay, read-only rejection, next-tab-stop geometry.
#include "test_editor_edit.c"

// Text undo journal: per-widget recording through the shared edit
// primitives (entries, removed bytes, caret pairs), staleness guard,
// password/unfocused exclusions, entry and byte caps with eviction.
#include "test_text_undo.c"

// wlx_editor wrapped mode: band-wide rows, row-space scrolling, overflow
// probe, top/bottom clamps, thumb drag to end, mode toggles, gutter rows.
// Included after test_editor_view.c to reuse its ev_* fixture.
#include "test_editor_wrap.c"

// wlx_editor retained line geometry: replay equivalence vs the measuring
// build, edit/external/environment invalidation, key shifting, LRU bounds,
// zero-measure idle frames.
#include "test_editor_geom_cache.c"

// WLX_Backend v2 contract: instance pointer, v1 shim, style transform at
// the backend boundary. Reuses the mock backend and the geometry-cache
// editor fixture (gc_*), so it follows test_editor_geom_cache.c.
#include "test_backend_contract.c"

// measure_text_advances backend callback: records, caret x, hit tests,
// selection spans, and whole-frame draw commands identical with the
// callback present vs absent over a mixed corpus, both wrap modes.
#include "test_advances_parity.c"

// No-wrap windowed horizontal origin: seam agreement across caret /
// hit-test / selection / draw on budget-deep lines, origin hysteresis,
// stitching continuity, tab restart at the origin, END and far-offset
// editing, and line-start origins for near content.
#include "test_editor_windowed_origin.c"

int main(void) {
    RUN_SUITE(layout_math);
    RUN_SUITE(slot_redistribute);
    RUN_SUITE(color);
    RUN_SUITE(utf8);
    RUN_SUITE(core_utils);
    RUN_SUITE(sub_arena);
    RUN_SUITE(grid);
    RUN_SUITE(interaction);
    RUN_SUITE(scroll_panel);
    RUN_SUITE(input);
    RUN_SUITE(input_contract);
    RUN_SUITE(input_selection);
    RUN_SUITE(input_clipboard);
    RUN_SUITE(input_modes);
    RUN_SUITE(input_multiline);
    RUN_SUITE(input_scroll);
    RUN_SUITE(auto_layout);
    RUN_SUITE(fuzz);
    RUN_SUITE(edge_cases);
    RUN_SUITE(split);
    RUN_SUITE(panel);
    RUN_SUITE(opacity_stack);
    RUN_SUITE(rounding);
    RUN_SUITE(widgets);
    RUN_SUITE(circle_dispatch);
    RUN_SUITE(cmd_replay);
    RUN_SUITE(layout_clip);
    RUN_SUITE(progress_bounds);
    RUN_SUITE(slot_style);
    RUN_SUITE(per_side_border);
    RUN_SUITE(sentinel);
    RUN_SUITE(backend_contract);
    RUN_SUITE(interactive_container);
    RUN_SUITE(wasm_clipboard);
    RUN_SUITE(wasm_pool);
    RUN_SUITE(dyn_offsets);
    RUN_SUITE(text_layout);
    RUN_SUITE(text_spacing);
    RUN_SUITE(widget_wrapper);
    RUN_SUITE(container_scope);
    RUN_SUITE(perf);
    RUN_SUITE(image);
    RUN_SUITE(button_image);
    RUN_SUITE(label_image);
    RUN_SUITE(label_style);
    RUN_SUITE(checkbox_texture);
    RUN_SUITE(inputbox_icon);
    RUN_SUITE(content_padding);
    RUN_SUITE(padding_alignment);
    RUN_SUITE(inputbox_padding);
    RUN_SUITE(missing_padding);
    RUN_SUITE(disabled_state);
    RUN_SUITE(dashboard_tokens);
    RUN_SUITE(dashboard_theme);
    RUN_SUITE(dashboard_components);
    RUN_SUITE(dashboard_effects);
    RUN_SUITE(glow_shadow);
    RUN_SUITE(gradient);
    RUN_SUITE(corner_radius);
    RUN_SUITE(slice_backend);
    RUN_SUITE(list_clipper);
    RUN_SUITE(offscreen_cull);
    RUN_SUITE(editor_pipeline);
    RUN_SUITE(editor_view);
    RUN_SUITE(editor_caret);
    RUN_SUITE(editor_edit);
    RUN_SUITE(text_undo);
    RUN_SUITE(editor_wrap);
    RUN_SUITE(editor_geom_cache);
    RUN_SUITE(advances_parity);
    RUN_SUITE(editor_windowed_origin);
    RUN_SUITE(frame_time);
    RUN_SUITE(focus_release);
    RUN_SUITE(cursor_shape);
    RUN_SUITE(tab_traversal);
    RUN_SUITE(content_axis);
    RUN_SUITE(overlay);
    RUN_SUITE(dropdown);
    RUN_SUITE(tooltip);
    RUN_SUITE(menu);
    return test_summary();
}

// test_main.c - entry point for wollix.h test suite
// Single-TU build: all test files are #included here.

#define WLX_DEBUG
// Disable the wlx_image empty-texture assert so test_image can exercise the
// safe no-op fallback path without aborting the runner.
#define WLX_IMAGE_ASSERT_TEXTURE_VALID(tex) ((void)(tex))
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#include "tests.h"
#include "test_mock_backend.h"

// Test files (pure math - no backend needed)
#include "test_layout_math.c"
#include "test_slot_redistribute.c"
#include "test_color.c"
#include "test_utf8.c"
#include "test_sub_arena.c"

// Test files (need mock backend / WLX_Context)
#include "test_grid.c"
#include "test_interaction.c"
#include "test_scroll_panel.c"
#include "test_input.c"
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

// Interaction-aware containers (.interact / .interact_out / hover-variant
// chrome) and the per-call wlx_button hover override.
#include "test_interactive_container.c"

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
// vertical centering independent of opt.align, default/explicit tint, and the
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

int main(void) {
    RUN_SUITE(layout_math);
    RUN_SUITE(slot_redistribute);
    RUN_SUITE(color);
    RUN_SUITE(utf8);
    RUN_SUITE(sub_arena);
    RUN_SUITE(grid);
    RUN_SUITE(interaction);
    RUN_SUITE(scroll_panel);
    RUN_SUITE(input);
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
    RUN_SUITE(interactive_container);
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
    return test_summary();
}

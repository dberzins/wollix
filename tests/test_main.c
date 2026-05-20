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

// Slot and grid cell decoration / style overrides
#include "test_slot_style.c"

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

// Image-capable wlx_checkbox texture mode: per-state src and tint, white
// default tint with opacity folding, both-textures-required activation,
// fallback to native when either state texture is missing, hover and
// check_color isolation, layout slot consumption.
#include "test_checkbox_texture.c"

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

int main(void) {
    RUN_SUITE(layout_math);
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
    RUN_SUITE(slot_style);
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
    RUN_SUITE(checkbox_texture);
    RUN_SUITE(content_padding);
    RUN_SUITE(padding_alignment);
    RUN_SUITE(inputbox_padding);
    RUN_SUITE(missing_padding);
    RUN_SUITE(disabled_state);
    return test_summary();
}

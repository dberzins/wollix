// test_main.c - entry point for wollix.h test suite
// Single-TU build: all test files are #included here.

#define WLX_DEBUG
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

// Widget wrapper contract: id stack balance and opacity/hover-brightness coverage
#include "test_widget_wrapper.c"

// Container Scope ID isolation and no-id collision regression
#include "test_container_scope.c"

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
    RUN_SUITE(widget_wrapper);
    RUN_SUITE(container_scope);
    return test_summary();
}

// test_main.c — entry point for wollix.h test suite
// Single-TU build: all test files are #included here.

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#include "tests.h"
#include "test_mock_backend.h"

// Test files (pure math — no backend needed)
#include "test_layout_math.c"
#include "test_color.c"
#include "test_utf8.c"

// Test files (need mock backend / WLX_Context)
#include "test_grid.c"
#include "test_interaction.c"
#include "test_scroll_panel.c"
#include "test_input.c"

// Phase 3: fuzz + edge cases
#include "test_fuzz.c"
#include "test_edge_cases.c"

int main(void) {
    RUN_SUITE(layout_math);
    RUN_SUITE(color);
    RUN_SUITE(utf8);
    RUN_SUITE(grid);
    RUN_SUITE(interaction);
    RUN_SUITE(scroll_panel);
    RUN_SUITE(input);
    RUN_SUITE(fuzz);
    RUN_SUITE(edge_cases);
    return test_summary();
}

// test_dashboard_components.c - tests for the dashboard component pure helpers.
// Included from test_main.c (single TU build). Pure geometry/state checks only;
// the rendering recipes are exercised visually by the demo.
//
// Covers: responsive column count, status-pip pulse alpha, table row rect
// geometry, and ASCII uppercasing. The segmented-bar fill rule now lives in
// the library (wlx_progress_filled_count), covered by the widgets suite.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#include "demos/dashboard/dashboard_theme.h"
#include "demos/dashboard/dashboard_effects.h"
#include "demos/dashboard/dashboard_components.h"

TEST(dashboard_grid_columns_breakpoints) {
    ASSERT_EQ_INT(12, dashboard_grid_columns(1920.0f));
    ASSERT_EQ_INT(12, dashboard_grid_columns(1200.0f));
    ASSERT_EQ_INT(8,  dashboard_grid_columns(1199.0f));
    ASSERT_EQ_INT(8,  dashboard_grid_columns(720.0f));
    ASSERT_EQ_INT(4,  dashboard_grid_columns(719.0f));
    ASSERT_EQ_INT(4,  dashboard_grid_columns(0.0f));
}

TEST(dashboard_pulse_alpha_bounds) {
    // Triangle wave: trough is 40% of base, peak (at 0.5) is 100% of base.
    ASSERT_EQ_INT(40,  dashboard_pulse_alpha(100, 0.0f));
    ASSERT_EQ_INT(100, dashboard_pulse_alpha(100, 0.5f));
    ASSERT_EQ_INT(70,  dashboard_pulse_alpha(100, 0.25f));
    // The wave wraps on the integer part.
    ASSERT_EQ_INT(40,  dashboard_pulse_alpha(100, 1.0f));
    ASSERT_EQ_INT(100, dashboard_pulse_alpha(100, 2.5f));
    // Result always within [0.4*base, base].
    for (int i = 0; i <= 20; i++) {
        uint8_t a = dashboard_pulse_alpha(200, (float)i * 0.1f);
        ASSERT_TRUE(a >= 80);
        ASSERT_TRUE(a <= 200);
    }
}

TEST(dashboard_table_row_rect_layout) {
    WLX_Rect table = { 10.0f, 20.0f, 300.0f, 200.0f };
    WLX_Rect header = dashboard_table_row_rect(table, 0, 28.0f, 26.0f);
    ASSERT_EQ_RECT(header, wlx_rect(10.0f, 20.0f, 300.0f, 28.0f), 0.01f);
    WLX_Rect row1 = dashboard_table_row_rect(table, 1, 28.0f, 26.0f);
    ASSERT_EQ_RECT(row1, wlx_rect(10.0f, 48.0f, 300.0f, 26.0f), 0.01f);
    WLX_Rect row3 = dashboard_table_row_rect(table, 3, 28.0f, 26.0f);
    ASSERT_EQ_RECT(row3, wlx_rect(10.0f, 100.0f, 300.0f, 26.0f), 0.01f);
}

TEST(dashboard_upper_ascii) {
    char buf[16];
    ASSERT_EQ_STR("OK", dashboard_upper("ok", buf, sizeof(buf)));
    ASSERT_EQ_STR("MIX3-X", dashboard_upper("mix3-x", buf, sizeof(buf)));
    ASSERT_EQ_STR("", dashboard_upper("", buf, sizeof(buf)));
    ASSERT_EQ_STR("", dashboard_upper(NULL, buf, sizeof(buf)));
    // Truncation always leaves a NUL terminator.
    char small[4];
    ASSERT_EQ_STR("ABC", dashboard_upper("abcdef", small, sizeof(small)));
}

SUITE(dashboard_components) {
    RUN_TEST(dashboard_grid_columns_breakpoints);
    RUN_TEST(dashboard_pulse_alpha_bounds);
    RUN_TEST(dashboard_table_row_rect_layout);
    RUN_TEST(dashboard_upper_ascii);
}

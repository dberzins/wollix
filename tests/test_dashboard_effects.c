// test_dashboard_effects.c - tests for the dashboard visual-effect pure helpers.
// Included from test_main.c (single TU build). Pure color math only; the drawn
// approximations are exercised visually by the demo.
//
// Covers: u8 and color interpolation, desaturation, the frosted blur tint, and
// glow ring alpha falloff.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#include "demos/dashboard/dashboard_effects.h"

TEST(dashboard_lerp_u8_endpoints_and_mid) {
    ASSERT_EQ_INT(0,   dashboard_lerp_u8(0, 100, 0.0f));
    ASSERT_EQ_INT(100, dashboard_lerp_u8(0, 100, 1.0f));
    ASSERT_EQ_INT(50,  dashboard_lerp_u8(100, 0, 0.5f));
    ASSERT_EQ_INT(128, dashboard_lerp_u8(0, 255, 0.5f));
}

TEST(dashboard_color_lerp_channels) {
    WLX_Color a = WLX_RGBA(0, 0, 0, 0);
    WLX_Color b = WLX_RGBA(200, 100, 40, 255);
    ASSERT_EQ_COLOR(dashboard_color_lerp(a, b, 0.0f), a);
    ASSERT_EQ_COLOR(dashboard_color_lerp(a, b, 1.0f), b);
    ASSERT_EQ_COLOR(dashboard_color_lerp(a, b, 0.5f), WLX_RGBA(100, 50, 20, 128));
    // Out-of-range t clamps.
    ASSERT_EQ_COLOR(dashboard_color_lerp(a, b, 2.0f), b);
    ASSERT_EQ_COLOR(dashboard_color_lerp(a, b, -1.0f), a);
}

TEST(dashboard_desaturate_to_grey) {
    WLX_Color red = WLX_RGBA(255, 0, 0, 200);
    // amount 0 leaves the color untouched.
    ASSERT_EQ_COLOR(dashboard_desaturate(red, 0.0f), red);
    // amount 1 collapses to luma grey; alpha is preserved.
    WLX_Color grey = dashboard_desaturate(red, 1.0f);
    ASSERT_EQ_INT(grey.r, grey.g);
    ASSERT_EQ_INT(grey.g, grey.b);
    ASSERT_EQ_INT(76, grey.r);
    ASSERT_EQ_INT(200, grey.a);
}

TEST(dashboard_blur_tint_darkens_and_sets_alpha) {
    WLX_Color base = WLX_RGBA(200, 100, 40, 255);
    // No darken / no desaturate: only alpha changes.
    ASSERT_EQ_COLOR(dashboard_blur_tint(base, 0.0f, 0.0f, 128), WLX_RGBA(200, 100, 40, 128));
    // Darken 0.5 scales rgb by 0.5; alpha set explicitly.
    ASSERT_EQ_COLOR(dashboard_blur_tint(base, 0.5f, 0.0f, 128), WLX_RGBA(100, 50, 20, 128));
}

TEST(dashboard_glow_ring_alpha_falloff) {
    // Innermost ring is strongest, outermost faintest.
    ASSERT_EQ_INT(200, dashboard_glow_ring_alpha(0, 4, 200));
    ASSERT_EQ_INT(50,  dashboard_glow_ring_alpha(3, 4, 200));
    ASSERT_TRUE(dashboard_glow_ring_alpha(0, 4, 200) > dashboard_glow_ring_alpha(3, 4, 200));
    // Index clamps; degenerate ring count yields zero.
    ASSERT_EQ_INT(200, dashboard_glow_ring_alpha(-1, 4, 200));
    ASSERT_EQ_INT(50,  dashboard_glow_ring_alpha(9, 4, 200));
    ASSERT_EQ_INT(0,   dashboard_glow_ring_alpha(0, 0, 200));
}

SUITE(dashboard_effects) {
    RUN_TEST(dashboard_lerp_u8_endpoints_and_mid);
    RUN_TEST(dashboard_color_lerp_channels);
    RUN_TEST(dashboard_desaturate_to_grey);
    RUN_TEST(dashboard_blur_tint_darkens_and_sets_alpha);
    RUN_TEST(dashboard_glow_ring_alpha_falloff);
}

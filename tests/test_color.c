// test_color.c — tests for wlx_clamp_u8, wlx_color_brightness,
// wlx_color_apply_opacity, wlx_color_is_zero.
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// wlx_clamp_u8
// ============================================================================

TEST(clamp_u8_in_range) {
    ASSERT_EQ_INT(wlx_clamp_u8(0),   0);
    ASSERT_EQ_INT(wlx_clamp_u8(128), 128);
    ASSERT_EQ_INT(wlx_clamp_u8(255), 255);
}

TEST(clamp_u8_below) {
    ASSERT_EQ_INT(wlx_clamp_u8(-1),  0);
    ASSERT_EQ_INT(wlx_clamp_u8(-100), 0);
}

TEST(clamp_u8_above) {
    ASSERT_EQ_INT(wlx_clamp_u8(256), 255);
    ASSERT_EQ_INT(wlx_clamp_u8(1000), 255);
}

// ============================================================================
// wlx_color_is_zero
// ============================================================================

TEST(color_is_zero_true) {
    ASSERT_TRUE(wlx_color_is_zero((WLX_Color){0, 0, 0, 0}));
}

TEST(color_is_zero_false) {
    ASSERT_FALSE(wlx_color_is_zero((WLX_Color){1, 0, 0, 0}));
    ASSERT_FALSE(wlx_color_is_zero((WLX_Color){0, 0, 0, 1}));
    ASSERT_FALSE(wlx_color_is_zero((WLX_Color){255, 255, 255, 255}));
}

// ============================================================================
// wlx_color_brightness
// ============================================================================

TEST(brightness_zero_factor) {
    // factor=0 → unchanged
    WLX_Color c = {100, 150, 200, 255};
    WLX_Color r = wlx_color_brightness(c, 0.0f);
    ASSERT_EQ_COLOR(r, c);
}

TEST(brightness_positive) {
    // factor > 0 → moves toward 255 (white)
    WLX_Color c = {100, 100, 100, 200};
    WLX_Color r = wlx_color_brightness(c, 0.5f);
    // r = 100 + (255-100)*0.5 = 100 + 77.5 = 178 (rounded)
    ASSERT_EQ_INT(r.r, 178);
    ASSERT_EQ_INT(r.g, 178);
    ASSERT_EQ_INT(r.b, 178);
    ASSERT_EQ_INT(r.a, 200);  // alpha unchanged
}

TEST(brightness_negative) {
    // factor < 0 → moves toward 0 (black)
    WLX_Color c = {200, 200, 200, 255};
    WLX_Color r = wlx_color_brightness(c, -0.5f);
    // r = 200 * (1 + (-0.5)) = 200 * 0.5 = 100
    ASSERT_EQ_INT(r.r, 100);
    ASSERT_EQ_INT(r.g, 100);
    ASSERT_EQ_INT(r.b, 100);
    ASSERT_EQ_INT(r.a, 255);
}

TEST(brightness_full_positive) {
    WLX_Color c = {100, 100, 100, 255};
    WLX_Color r = wlx_color_brightness(c, 1.0f);
    // r = 100 + (255-100)*1.0 = 255
    ASSERT_EQ_INT(r.r, 255);
    ASSERT_EQ_INT(r.g, 255);
    ASSERT_EQ_INT(r.b, 255);
}

TEST(brightness_full_negative) {
    WLX_Color c = {100, 100, 100, 255};
    WLX_Color r = wlx_color_brightness(c, -1.0f);
    // r = 100 * (1 + (-1)) = 0
    ASSERT_EQ_INT(r.r, 0);
    ASSERT_EQ_INT(r.g, 0);
    ASSERT_EQ_INT(r.b, 0);
}

TEST(brightness_clamp_high) {
    // factor > 1.0 clamped to 1.0
    WLX_Color c = {100, 100, 100, 255};
    WLX_Color r = wlx_color_brightness(c, 5.0f);
    WLX_Color expected = wlx_color_brightness(c, 1.0f);
    ASSERT_EQ_COLOR(r, expected);
}

TEST(brightness_clamp_low) {
    // factor < -1.0 clamped to -1.0
    WLX_Color c = {100, 100, 100, 255};
    WLX_Color r = wlx_color_brightness(c, -3.0f);
    WLX_Color expected = wlx_color_brightness(c, -1.0f);
    ASSERT_EQ_COLOR(r, expected);
}

TEST(brightness_preserves_alpha) {
    WLX_Color c = {100, 150, 200, 42};
    WLX_Color r = wlx_color_brightness(c, 0.3f);
    ASSERT_EQ_INT(r.a, 42);
}

// ============================================================================
// wlx_color_apply_opacity
// ============================================================================

TEST(opacity_full) {
    WLX_Color c = {100, 150, 200, 255};
    WLX_Color r = wlx_color_apply_opacity(c, 1.0f);
    ASSERT_EQ_COLOR(r, c);
}

TEST(opacity_zero) {
    WLX_Color c = {100, 150, 200, 255};
    WLX_Color r = wlx_color_apply_opacity(c, 0.0f);
    ASSERT_EQ_INT(r.r, 100);
    ASSERT_EQ_INT(r.g, 150);
    ASSERT_EQ_INT(r.b, 200);
    ASSERT_EQ_INT(r.a, 0);
}

TEST(opacity_half) {
    WLX_Color c = {100, 150, 200, 200};
    WLX_Color r = wlx_color_apply_opacity(c, 0.5f);
    // alpha = round(200 * 0.5) = 100
    ASSERT_EQ_INT(r.a, 100);
    // RGB unchanged
    ASSERT_EQ_INT(r.r, 100);
    ASSERT_EQ_INT(r.g, 150);
    ASSERT_EQ_INT(r.b, 200);
}

TEST(opacity_above_one) {
    // opacity >= 1.0 → unchanged
    WLX_Color c = {10, 20, 30, 128};
    WLX_Color r = wlx_color_apply_opacity(c, 2.0f);
    ASSERT_EQ_COLOR(r, c);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(color) {
    // wlx_clamp_u8
    RUN_TEST(clamp_u8_in_range);
    RUN_TEST(clamp_u8_below);
    RUN_TEST(clamp_u8_above);

    // wlx_color_is_zero
    RUN_TEST(color_is_zero_true);
    RUN_TEST(color_is_zero_false);

    // wlx_color_brightness
    RUN_TEST(brightness_zero_factor);
    RUN_TEST(brightness_positive);
    RUN_TEST(brightness_negative);
    RUN_TEST(brightness_full_positive);
    RUN_TEST(brightness_full_negative);
    RUN_TEST(brightness_clamp_high);
    RUN_TEST(brightness_clamp_low);
    RUN_TEST(brightness_preserves_alpha);

    // wlx_color_apply_opacity
    RUN_TEST(opacity_full);
    RUN_TEST(opacity_zero);
    RUN_TEST(opacity_half);
    RUN_TEST(opacity_above_one);
}

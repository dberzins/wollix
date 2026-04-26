// test_edge_cases.c - edge-case tests for layout math, geometry, and color
// Covers: zero-size slots, negative values, very large layouts, degenerate rects.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#define EDGE_EPS 0.01f

// ============================================================================
// wlx_compute_offsets edge cases
// ============================================================================

TEST(edge_offsets_zero_total) {
    // total=0 -> all offsets should be 0
    float off[4];
    wlx_compute_offsets(off, 3, 0.0f, 0.0f, NULL, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[3], 0.0f, EDGE_EPS);
}

TEST(edge_offsets_zero_total_with_sizes) {
    // total=0 but slots request space -> offsets clamp, no crash
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
    float off[3];
    wlx_compute_offsets(off, 2, 0.0f, 0.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    // PX(100) still gets 100, flex gets 0 (remaining is 0)
    ASSERT_EQ_F(off[1], 100.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 100.0f, EDGE_EPS);
}

TEST(edge_offsets_all_zero_px) {
    // All PX(0) -> all offsets at 0
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(0), WLX_SLOT_PX(0), WLX_SLOT_PX(0) };
    float off[4];
    wlx_compute_offsets(off, 3, 500.0f, 500.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[3], 0.0f, EDGE_EPS);
}

TEST(edge_offsets_px_exceeds_total) {
    // Pixel slots sum to more than total -> no crash, offsets go beyond total
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(300), WLX_SLOT_PX(300) };
    float off[3];
    wlx_compute_offsets(off, 2, 100.0f, 100.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 300.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 600.0f, EDGE_EPS);
    // Offsets still monotonic even though they exceed total
    ASSERT_TRUE(off[1] >= off[0]);
    ASSERT_TRUE(off[2] >= off[1]);
}

TEST(edge_offsets_pct_exceeds_100) {
    // Percentages sum > 100% -> no crash, slots stretch beyond total
    WLX_Slot_Size sizes[] = { WLX_SLOT_PCT(80), WLX_SLOT_PCT(80) };
    float off[3];
    wlx_compute_offsets(off, 2, 100.0f, 100.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 80.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 160.0f, EDGE_EPS);
}

TEST(edge_offsets_flex_zero_weight) {
    // FLEX(0) -> should get 0 size, not crash/divide-by-zero
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX(0), WLX_SLOT_FLEX(1) };
    float off[3];
    wlx_compute_offsets(off, 2, 100.0f, 100.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    // FLEX(0) gets 0, FLEX(1) gets all 100
    ASSERT_EQ_F(off[1], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 100.0f, EDGE_EPS);
}

TEST(edge_offsets_all_flex_zero) {
    // All FLEX(0) -> total_weight=0, no crash, all get 0
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX(0), WLX_SLOT_FLEX(0) };
    float off[3];
    wlx_compute_offsets(off, 2, 200.0f, 200.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 0.0f, EDGE_EPS);
}

TEST(edge_offsets_very_large_total) {
    // Very large total -> no overflow, slots divide correctly
    float off[3];
    wlx_compute_offsets(off, 2, 1000000.0f, 1000000.0f, NULL, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, 1.0f);
    ASSERT_EQ_F(off[1], 500000.0f, 1.0f);
    ASSERT_EQ_F(off[2], 1000000.0f, 1.0f);
}

TEST(edge_offsets_many_slots) {
    // Many slots (16) with NULL sizes -> equal split works
    float off[17];
    wlx_compute_offsets(off, 16, 1600.0f, 1600.0f, NULL, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[16], 1600.0f, EDGE_EPS);
    for (int i = 0; i < 16; i++) {
        float slot = off[i + 1] - off[i];
        ASSERT_EQ_F(slot, 100.0f, EDGE_EPS);
    }
}

TEST(edge_offsets_min_larger_than_slot) {
    // Min is larger than what the slot would normally get
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_FLEX_MIN(1, 300),  // normally 50px in 100px total, but min=300
    };
    float off[2];
    wlx_compute_offsets(off, 1, 100.0f, 100.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    // Min forces to 300 even though total is only 100
    ASSERT_EQ_F(off[1], 300.0f, EDGE_EPS);
}

TEST(edge_offsets_max_smaller_than_total) {
    // Max constrains the slot even though there's plenty of space
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX_MAX(1, 50) };
    float off[2];
    wlx_compute_offsets(off, 1, 1000.0f, 1000.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 50.0f, EDGE_EPS);
}

TEST(edge_offsets_min_equals_max) {
    // min == max -> slot locked to that exact size
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX_MINMAX(1, 100, 100) };
    float off[2];
    wlx_compute_offsets(off, 1, 500.0f, 500.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 100.0f, EDGE_EPS);
}

TEST(edge_offsets_single_auto) {
    // Single AUTO slot -> gets all the space
    WLX_Slot_Size sizes[] = { WLX_SLOT_AUTO };
    float off[2];
    wlx_compute_offsets(off, 1, 500.0f, 500.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 500.0f, EDGE_EPS);
}

TEST(edge_offsets_mixed_all_types) {
    // One of each type in the same layout
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PX(50),     // 50
        WLX_SLOT_PCT(25),    // 25% of 400 = 100
        WLX_SLOT_FLEX(1),    // remaining = 400 - 50 - 100 = 250, flex=1 of total weight 2 -> 125
        WLX_SLOT_AUTO,       // auto(1) of total weight 2 -> 125
    };
    float off[5];
    wlx_compute_offsets(off, 4, 400.0f, 400.0f, sizes, 0.0f);
    ASSERT_EQ_F(off[0], 0.0f, EDGE_EPS);
    ASSERT_EQ_F(off[1], 50.0f, EDGE_EPS);
    ASSERT_EQ_F(off[2], 150.0f, EDGE_EPS);  // 50 + 100
    ASSERT_EQ_F(off[3], 275.0f, EDGE_EPS);  // 150 + 125
    ASSERT_EQ_F(off[4], 400.0f, EDGE_EPS);  // 275 + 125
}

// ============================================================================
// wlx_rect_inset edge cases
// ============================================================================

TEST(edge_inset_very_large) {
    WLX_Rect r = wlx_rect(10, 20, 100, 80);
    // Inset larger than half either dimension -> w/h clamped to 0
    WLX_Rect result = wlx_rect_inset(r, 200);
    ASSERT_EQ_F(result.w, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(result.h, 0.0f, EDGE_EPS);
}

TEST(edge_inset_exactly_half) {
    // Inset = exactly half of smaller dimension
    WLX_Rect r = wlx_rect(0, 0, 100, 60);
    WLX_Rect result = wlx_rect_inset(r, 30);
    ASSERT_EQ_F(result.x, 30.0f, EDGE_EPS);
    ASSERT_EQ_F(result.y, 30.0f, EDGE_EPS);
    ASSERT_EQ_F(result.w, 40.0f, EDGE_EPS);  // 100 - 60
    ASSERT_EQ_F(result.h, 0.0f, EDGE_EPS);   // 60 - 60
}

TEST(edge_inset_negative_value) {
    // Negative inset -> should return original rect unchanged
    WLX_Rect r = wlx_rect(10, 20, 100, 80);
    WLX_Rect result = wlx_rect_inset(r, -10);
    ASSERT_EQ_RECT(result, r, EDGE_EPS);
}

// ============================================================================
// wlx_rect_intersect edge cases
// ============================================================================

TEST(edge_intersect_zero_size_rect) {
    WLX_Rect a = wlx_rect(10, 10, 0, 0);  // zero-size
    WLX_Rect b = wlx_rect(5, 5, 100, 100);
    WLX_Rect result = wlx_rect_intersect(a, b);
    ASSERT_EQ_F(result.w, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(result.h, 0.0f, EDGE_EPS);
}

TEST(edge_intersect_identical) {
    // Self-intersection -> returns same rect
    WLX_Rect a = wlx_rect(10, 20, 300, 200);
    WLX_Rect result = wlx_rect_intersect(a, a);
    ASSERT_EQ_RECT(result, a, EDGE_EPS);
}

TEST(edge_intersect_adjacent) {
    // Touching but not overlapping (right edge of A == left edge of B)
    WLX_Rect a = wlx_rect(0, 0, 100, 100);
    WLX_Rect b = wlx_rect(100, 0, 100, 100);
    WLX_Rect result = wlx_rect_intersect(a, b);
    // x1=100, x2=100 -> w=0
    ASSERT_EQ_F(result.w, 0.0f, EDGE_EPS);
}

TEST(edge_intersect_negative_coords) {
    // Both rects have negative coordinates
    WLX_Rect a = wlx_rect(-200, -100, 150, 80);
    WLX_Rect b = wlx_rect(-180, -90, 100, 60);
    WLX_Rect result = wlx_rect_intersect(a, b);
    // a: x2 = -200+150 = -50, y2 = -100+80 = -20
    // b: x2 = -180+100 = -80, y2 = -90+60 = -30
    // x1=max(-200,-180)=-180, y1=max(-100,-90)=-90
    // x2=min(-50,-80)=-80, y2=min(-20,-30)=-30
    // w=-80-(-180)=100, h=-30-(-90)=60
    ASSERT_EQ_F(result.x, -180.0f, EDGE_EPS);
    ASSERT_EQ_F(result.y, -90.0f, EDGE_EPS);
    ASSERT_EQ_F(result.w, 100.0f, EDGE_EPS);
    ASSERT_EQ_F(result.h, 60.0f, EDGE_EPS);
}

// ============================================================================
// wlx_get_align_rect edge cases
// ============================================================================

TEST(edge_align_zero_size_parent) {
    WLX_Rect parent = wlx_rect(50, 50, 0, 0);
    WLX_Rect r = wlx_get_align_rect(parent, 100, 100, WLX_CENTER);
    // Parent is 0x0 -> child can't be bigger, w/h clamped to parent (0)
    ASSERT_EQ_F(r.w, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 0.0f, EDGE_EPS);
}

TEST(edge_align_zero_size_child) {
    WLX_Rect parent = wlx_rect(0, 0, 400, 300);
    WLX_Rect r = wlx_get_align_rect(parent, 0, 0, WLX_CENTER);
    // 0-size child -> w/h set to 0 (width >= 0 && parent.w > width is true)
    ASSERT_EQ_F(r.w, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 0.0f, EDGE_EPS);
    // Centering condition is (width > 0 && parent.w > width) -> false for width=0
    // So x/y stay at parent origin
    ASSERT_EQ_F(r.x, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(r.y, 0.0f, EDGE_EPS);
}

TEST(edge_align_child_equals_parent) {
    // Child exactly matches parent -> no movement needed
    WLX_Rect parent = wlx_rect(10, 20, 300, 200);
    WLX_Rect r = wlx_get_align_rect(parent, 300, 200, WLX_CENTER);
    // width > 0 but parent.w > width is false (300 > 300 is false)
    // So no centering applied, x stays at parent.x, w stays at parent.w
    ASSERT_EQ_F(r.x, 10.0f, EDGE_EPS);
    ASSERT_EQ_F(r.y, 20.0f, EDGE_EPS);
    ASSERT_EQ_F(r.w, 300.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 200.0f, EDGE_EPS);
}

TEST(edge_align_negative_dims) {
    // Both width and height < 0 -> returns parent rect unchanged
    WLX_Rect parent = wlx_rect(10, 20, 300, 200);
    WLX_Rect r = wlx_get_align_rect(parent, -1, -1, WLX_CENTER);
    ASSERT_EQ_RECT(r, parent, EDGE_EPS);
}

// ============================================================================
// wlx_point_in_rect edge cases
// ============================================================================

TEST(edge_point_zero_size_rect) {
    // Zero-dimension rect -> nothing is inside
    ASSERT_FALSE(wlx_point_in_rect(50, 50, 50, 50, 0, 0));
    ASSERT_FALSE(wlx_point_in_rect(50, 50, 50, 50, 0, 10));
    ASSERT_FALSE(wlx_point_in_rect(50, 50, 50, 50, 10, 0));
}

TEST(edge_point_negative_coords) {
    // Rect at negative coordinates
    ASSERT_TRUE(wlx_point_in_rect(-10, -10, -20, -20, 40, 40));
    ASSERT_FALSE(wlx_point_in_rect(-21, -21, -20, -20, 40, 40));
    ASSERT_TRUE(wlx_point_in_rect(19, 19, -20, -20, 40, 40));
    ASSERT_FALSE(wlx_point_in_rect(20, 20, -20, -20, 40, 40));  // exclusive
}

TEST(edge_point_unit_rect) {
    // 1x1 rect -> only the top-left pixel is inside
    ASSERT_TRUE(wlx_point_in_rect(5, 5, 5, 5, 1, 1));
    ASSERT_FALSE(wlx_point_in_rect(6, 5, 5, 5, 1, 1));
    ASSERT_FALSE(wlx_point_in_rect(5, 6, 5, 5, 1, 1));
    ASSERT_FALSE(wlx_point_in_rect(4, 5, 5, 5, 1, 1));
}

// ============================================================================
// wlx_resolve_widget_rect edge cases
// ============================================================================

TEST(edge_resolve_zero_cell) {
    // Zero-size cell rect
    WLX_Rect cell = wlx_rect(100, 100, 0, 0);
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 0, 0, 0, 0, WLX_ALIGN_NONE, false);
    ASSERT_EQ_F(r.w, 0.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 0.0f, EDGE_EPS);
}

TEST(edge_resolve_min_larger_than_cell) {
    // min_w/min_h larger than cell -> widget grows beyond cell
    WLX_Rect cell = wlx_rect(0, 0, 50, 50);
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 200, 200, 0, 0, WLX_ALIGN_NONE, false);
    ASSERT_TRUE(r.w >= 200.0f);
    ASSERT_TRUE(r.h >= 200.0f);
}

TEST(edge_resolve_max_zero) {
    // max=0 means unconstrained (not "max of 0")
    WLX_Rect cell = wlx_rect(0, 0, 300, 300);
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 0, 0, 0, 0, WLX_ALIGN_NONE, false);
    ASSERT_EQ_F(r.w, 300.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 300.0f, EDGE_EPS);
}

TEST(edge_resolve_overflow_preserves_origin) {
    // overflow=true -> widget positioned at cell origin regardless of alignment
    WLX_Rect cell = wlx_rect(50, 60, 200, 150);
    WLX_Rect r = wlx_resolve_widget_rect(cell, 400, 300, 0, 0, 0, 0, WLX_CENTER, true);
    ASSERT_EQ_F(r.x, 50.0f, EDGE_EPS);
    ASSERT_EQ_F(r.y, 60.0f, EDGE_EPS);
    ASSERT_EQ_F(r.w, 400.0f, EDGE_EPS);
    ASSERT_EQ_F(r.h, 300.0f, EDGE_EPS);
}

// ============================================================================
// Color edge cases
// ============================================================================

TEST(edge_brightness_max_white) {
    // Maximum brightness on already-white -> stays white (255 clamped)
    WLX_Color white = {255, 255, 255, 200};
    WLX_Color result = wlx_color_brightness(white, 1.0f);
    ASSERT_EQ_INT(result.r, 255);
    ASSERT_EQ_INT(result.g, 255);
    ASSERT_EQ_INT(result.b, 255);
    ASSERT_EQ_INT(result.a, 200);  // alpha preserved
}

TEST(edge_brightness_max_black) {
    // Maximum negative brightness on already-black -> stays black
    WLX_Color black = {0, 0, 0, 128};
    WLX_Color result = wlx_color_brightness(black, -1.0f);
    ASSERT_EQ_INT(result.r, 0);
    ASSERT_EQ_INT(result.g, 0);
    ASSERT_EQ_INT(result.b, 0);
    ASSERT_EQ_INT(result.a, 128);
}

TEST(edge_opacity_on_zero_alpha) {
    // Zero alpha with any opacity multiplier -> stays 0
    WLX_Color c = {100, 100, 100, 0};
    WLX_Color result = wlx_color_apply_opacity(c, 1.0f);
    ASSERT_EQ_INT(result.a, 0);
}

TEST(edge_opacity_negative) {
    // Negative opacity -> should clamp to 0 or handle gracefully
    WLX_Color c = {100, 100, 100, 200};
    WLX_Color result = wlx_color_apply_opacity(c, -0.5f);
    // alpha = 200 * (-0.5) would be negative -> clamp to 0
    ASSERT_TRUE(result.a == 0 || result.a <= 1);  // clamped
}

// ── Border clamp ────────────────────────────────────────────────────────────

TEST(border_clamp_subpixel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);
    ctx.immediate_mode = true;

    WLX_Rect r = {10, 10, 100, 50};
    WLX_Color white = {255, 255, 255, 255};

    // Sub-pixel: thick clamped to 1px, alpha scaled by thickness
    wlx_draw_rect_lines(&ctx, r, 0.5f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 1.0f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 128);  // 255 * 0.5 ≈ 128

    wlx_draw_rect_lines(&ctx, r, 0.3f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 1.0f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 77);   // 255 * 0.3 ≈ 77

    wlx_draw_rect_lines(&ctx, r, 0.001f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 1.0f, 0.001f);
    ASSERT_TRUE(_mock_last_border_color.a <= 1);     // nearly transparent

    // At or above 1px: no alpha change
    wlx_draw_rect_lines(&ctx, r, 1.0f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 1.0f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 255);   // unchanged

    wlx_draw_rect_lines(&ctx, r, 2.5f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 2.5f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 255);   // unchanged

    // Sub-pixel with pre-existing partial alpha
    WLX_Color half = {255, 255, 255, 128};
    wlx_draw_rect_lines(&ctx, r, 0.5f, half);
    ASSERT_EQ_F(_mock_last_border_thick, 1.0f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 64);    // 128 * 0.5 = 64

    test_frame_end(&ctx);
}

// ── wlx_begin_immediate opt-out ─────────────────────────────────────────────

TEST(begin_immediate_dispatches_directly) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Use wlx_begin_immediate instead of test_frame_begin
    memset(&_test_staged_input, 0, sizeof(_test_staged_input));
    wlx_begin_immediate(&ctx, ctx.rect, _test_input_handler);

    // immediate_mode should be set
    ASSERT_TRUE(ctx.immediate_mode);

    // Draws should dispatch directly, observable via mock state
    WLX_Rect r = {10, 20, 100, 50};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_lines(&ctx, r, 3.0f, white);
    ASSERT_EQ_F(_mock_last_border_thick, 3.0f, 0.001f);
    ASSERT_EQ_INT(_mock_last_border_color.a, 255);

    // No commands should be recorded
    ASSERT_EQ_INT((int)ctx.arena.commands.count, 0);

    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(edge_cases) {
    // wlx_compute_offsets
    RUN_TEST(edge_offsets_zero_total);
    RUN_TEST(edge_offsets_zero_total_with_sizes);
    RUN_TEST(edge_offsets_all_zero_px);
    RUN_TEST(edge_offsets_px_exceeds_total);
    RUN_TEST(edge_offsets_pct_exceeds_100);
    RUN_TEST(edge_offsets_flex_zero_weight);
    RUN_TEST(edge_offsets_all_flex_zero);
    RUN_TEST(edge_offsets_very_large_total);
    RUN_TEST(edge_offsets_many_slots);
    RUN_TEST(edge_offsets_min_larger_than_slot);
    RUN_TEST(edge_offsets_max_smaller_than_total);
    RUN_TEST(edge_offsets_min_equals_max);
    RUN_TEST(edge_offsets_single_auto);
    RUN_TEST(edge_offsets_mixed_all_types);

    // wlx_rect_inset
    RUN_TEST(edge_inset_very_large);
    RUN_TEST(edge_inset_exactly_half);
    RUN_TEST(edge_inset_negative_value);

    // wlx_rect_intersect
    RUN_TEST(edge_intersect_zero_size_rect);
    RUN_TEST(edge_intersect_identical);
    RUN_TEST(edge_intersect_adjacent);
    RUN_TEST(edge_intersect_negative_coords);

    // wlx_get_align_rect
    RUN_TEST(edge_align_zero_size_parent);
    RUN_TEST(edge_align_zero_size_child);
    RUN_TEST(edge_align_child_equals_parent);
    RUN_TEST(edge_align_negative_dims);

    // wlx_point_in_rect
    RUN_TEST(edge_point_zero_size_rect);
    RUN_TEST(edge_point_negative_coords);
    RUN_TEST(edge_point_unit_rect);

    // wlx_resolve_widget_rect
    RUN_TEST(edge_resolve_zero_cell);
    RUN_TEST(edge_resolve_min_larger_than_cell);
    RUN_TEST(edge_resolve_max_zero);
    RUN_TEST(edge_resolve_overflow_preserves_origin);

    // Color
    RUN_TEST(edge_brightness_max_white);
    RUN_TEST(edge_brightness_max_black);
    RUN_TEST(edge_opacity_on_zero_alpha);
    RUN_TEST(edge_opacity_negative);

    // Border clamp
    RUN_TEST(border_clamp_subpixel);

    // Opt-out (wlx_begin_immediate)
    RUN_TEST(begin_immediate_dispatches_directly);
}

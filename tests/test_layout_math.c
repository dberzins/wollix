// test_layout_math.c - tests for wlx_compute_offsets, wlx_get_align_rect,
// wlx_rect_intersect, wlx_rect_inset, wlx_point_in_rect, wlx_resolve_widget_rect.
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

#define EPS 0.01f

// ============================================================================
// wlx_compute_offsets - equal division (NULL sizes)
// ============================================================================

TEST(offsets_equal_split_2) {
    float off[3];
    wlx_compute_offsets(off, 2, 400.0f, 400.0f, NULL);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 200.0f, EPS);
    ASSERT_EQ_F(off[2], 400.0f, EPS);
}

TEST(offsets_equal_split_5) {
    float off[6];
    wlx_compute_offsets(off, 5, 500.0f, 500.0f, NULL);
    for (int i = 0; i <= 5; i++) {
        ASSERT_EQ_F(off[i], (float)i * 100.0f, EPS);
    }
}

TEST(offsets_single_slot) {
    float off[2];
    wlx_compute_offsets(off, 1, 300.0f, 300.0f, NULL);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 300.0f, EPS);
}

// ============================================================================
// wlx_compute_offsets - PIXELS
// ============================================================================

TEST(offsets_all_pixels) {
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_PX(150), WLX_SLOT_PX(50) };
    float off[4];
    wlx_compute_offsets(off, 3, 500.0f, 500.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 250.0f, EPS);
    ASSERT_EQ_F(off[3], 300.0f, EPS);  // 100+150+50=300, not 500
}

// ============================================================================
// wlx_compute_offsets - PERCENT
// ============================================================================

TEST(offsets_all_percent) {
    WLX_Slot_Size sizes[] = { WLX_SLOT_PCT(30), WLX_SLOT_PCT(50), WLX_SLOT_PCT(20) };
    float off[4];
    wlx_compute_offsets(off, 3, 400.0f, 400.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 120.0f, EPS);  // 30% of 400
    ASSERT_EQ_F(off[2], 320.0f, EPS);  // +50% of 400
    ASSERT_EQ_F(off[3], 400.0f, EPS);  // +20% of 400
}

// ============================================================================
// wlx_compute_offsets - FLEX
// ============================================================================

TEST(offsets_all_flex_equal) {
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1) };
    float off[4];
    wlx_compute_offsets(off, 3, 300.0f, 300.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 200.0f, EPS);
    ASSERT_EQ_F(off[3], 300.0f, EPS);
}

TEST(offsets_flex_weighted) {
    // 1:2:1 ratio over 400px -> 100, 200, 100
    WLX_Slot_Size sizes[] = { WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(2), WLX_SLOT_FLEX(1) };
    float off[4];
    wlx_compute_offsets(off, 3, 400.0f, 400.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 300.0f, EPS);
    ASSERT_EQ_F(off[3], 400.0f, EPS);
}

// ============================================================================
// wlx_compute_offsets - AUTO
// ============================================================================

TEST(offsets_auto_slots) {
    // AUTO acts like flex(1)
    WLX_Slot_Size sizes[] = { WLX_SLOT_AUTO, WLX_SLOT_AUTO };
    float off[3];
    wlx_compute_offsets(off, 2, 200.0f, 200.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 200.0f, EPS);
}

// ============================================================================
// wlx_compute_offsets - mixed sizes
// ============================================================================

TEST(offsets_mixed_px_flex) {
    // 100px fixed + flex(1) takes remainder of 300px total -> 100 + 200
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
    float off[3];
    wlx_compute_offsets(off, 2, 300.0f, 300.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 300.0f, EPS);
}

TEST(offsets_mixed_px_pct_flex) {
    // 50px + 25% of 400 (=100) + flex(1) takes remaining 250
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(50), WLX_SLOT_PCT(25), WLX_SLOT_FLEX(1) };
    float off[4];
    wlx_compute_offsets(off, 3, 400.0f, 400.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1],  50.0f, EPS);
    ASSERT_EQ_F(off[2], 150.0f, EPS);
    ASSERT_EQ_F(off[3], 400.0f, EPS);
}

TEST(offsets_zero_remaining_for_flex) {
    // Pixels consume all 200px, flex gets 0
    WLX_Slot_Size sizes[] = { WLX_SLOT_PX(100), WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
    float off[4];
    wlx_compute_offsets(off, 3, 200.0f, 200.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);
    ASSERT_EQ_F(off[2], 200.0f, EPS);
    ASSERT_EQ_F(off[3], 200.0f, EPS);  // flex slot is 0-width
}

// ============================================================================
// wlx_compute_offsets - min/max constraints (single-pass clamp)
// ============================================================================

TEST(offsets_flex_with_min) {
    // 3 flex(1) in 300px -> each 100px, but slot 0 has min=150 -> gets 150
    // Without redistribute, slots 1&2 still get 100 each (single-pass clamp)
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_FLEX_MIN(1, 150),
        WLX_SLOT_FLEX(1),
        WLX_SLOT_FLEX(1),
    };
    float off[4];
    wlx_compute_offsets(off, 3, 300.0f, 300.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 150.0f, EPS);  // clamped up to min
    ASSERT_EQ_F(off[2], 250.0f, EPS);  // 100
    ASSERT_EQ_F(off[3], 350.0f, EPS);  // 100 (overflows total without redistribute)
}

TEST(offsets_flex_with_max) {
    // 2 flex(1) in 400px -> each 200px, but slot 0 has max=100 -> gets 100
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_FLEX_MAX(1, 100),
        WLX_SLOT_FLEX(1),
    };
    float off[3];
    wlx_compute_offsets(off, 2, 400.0f, 400.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 100.0f, EPS);  // clamped down to max
    ASSERT_EQ_F(off[2], 300.0f, EPS);  // 200 (single-pass, no redistribute)
}

TEST(offsets_pct_minmax) {
    // 50% of 200 = 100, but min=120 -> gets 120
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PCT_MINMAX(50, 120, 0),
        WLX_SLOT_FLEX(1),
    };
    float off[3];
    wlx_compute_offsets(off, 2, 200.0f, 200.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 120.0f, EPS);  // clamped up to min
    // flex gets remaining after first pass: total - used_pct = 200 - 100 = 100
    // but offset is 120 + 100 = 220 (single-pass: px used was 0, pct used was 100)
}

// ============================================================================
// wlx_compute_offsets - monotonicity invariant
// ============================================================================

TEST(offsets_monotonic) {
    WLX_Slot_Size sizes[] = {
        WLX_SLOT_PX(50), WLX_SLOT_FLEX(2), WLX_SLOT_PCT(10),
        WLX_SLOT_FLEX(1), WLX_SLOT_AUTO
    };
    float off[6];
    wlx_compute_offsets(off, 5, 600.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0], 0.0f, EPS);
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(off[i] <= off[i + 1] + EPS);
    }
}

// ============================================================================
// wlx_compute_offsets - FILL (viewport-relative sizing)
// ============================================================================

TEST(offsets_fill_basic) {
    // Single FILL slot: should resolve to full viewport, not total
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL };
    float off[2];
    wlx_compute_offsets(off, 1, 1200.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 600.0f, EPS);  // viewport, not total
}

TEST(offsets_fill_plus_px) {
    // FILL(1.0) + PX(100): FILL = viewport, total = viewport + 100
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL, WLX_SLOT_PX(100) };
    float off[3];
    wlx_compute_offsets(off, 2, 1200.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 600.0f, EPS);
    ASSERT_EQ_F(off[2], 700.0f, EPS);
}

TEST(offsets_fill_plus_flex) {
    // FILL(1.0) + FLEX(1): FILL uses 600 of viewport, FLEX gets remaining from total
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL, WLX_SLOT_FLEX(1) };
    float off[3];
    wlx_compute_offsets(off, 2, 800.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 600.0f, EPS);    // FILL = viewport
    ASSERT_EQ_F(off[2], 800.0f, EPS);    // FLEX = total - viewport = 200
}

TEST(offsets_fill_flex_no_remaining) {
    // FILL(1.0) + FLEX(1) when total == viewport: FLEX gets 0
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL, WLX_SLOT_FLEX(1) };
    float off[3];
    wlx_compute_offsets(off, 2, 600.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 600.0f, EPS);
    ASSERT_EQ_F(off[2], 600.0f, EPS);    // FLEX = 0
}

TEST(offsets_fill_with_min) {
    // FILL_MIN(500) when viewport = 300 -> clamps to 500
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL_MIN(500) };
    float off[2];
    wlx_compute_offsets(off, 1, 1000.0f, 300.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 500.0f, EPS);  // min wins over viewport
}

TEST(offsets_fill_with_max) {
    // FILL_MAX(300) when viewport = 600 -> clamps to 300
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL_MAX(300) };
    float off[2];
    wlx_compute_offsets(off, 1, 1000.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 300.0f, EPS);  // max wins over viewport
}

TEST(offsets_fill_pct) {
    // FILL_PCT(50) -> half the viewport
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL_PCT(50) };
    float off[2];
    wlx_compute_offsets(off, 1, 1000.0f, 800.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 400.0f, EPS);  // 50% of 800
}

TEST(offsets_fill_multiple) {
    // Two FILL_PCT(50) slots: each = 50% of viewport
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL_PCT(50), WLX_SLOT_FILL_PCT(50) };
    float off[3];
    wlx_compute_offsets(off, 2, 1200.0f, 600.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 300.0f, EPS);
    ASSERT_EQ_F(off[2], 600.0f, EPS);
}

TEST(offsets_fill_no_scroll_panel) {
    // When viewport == total (no scroll panel), FILL behaves like PCT(100)
    WLX_Slot_Size sizes[] = { WLX_SLOT_FILL };
    float off[2];
    wlx_compute_offsets(off, 1, 500.0f, 500.0f, sizes);
    ASSERT_EQ_F(off[0],   0.0f, EPS);
    ASSERT_EQ_F(off[1], 500.0f, EPS);
}

// ============================================================================
// wlx_get_align_rect
// ============================================================================

TEST(align_none) {
    WLX_Rect parent = {10, 20, 200, 100};
    WLX_Rect r = wlx_get_align_rect(parent, 50, 30, WLX_ALIGN_NONE);
    // ALIGN_NONE -> top-left, sized down
    ASSERT_EQ_F(r.x, 10.0f, EPS);
    ASSERT_EQ_F(r.y, 20.0f, EPS);
    ASSERT_EQ_F(r.w, 50.0f, EPS);
    ASSERT_EQ_F(r.h, 30.0f, EPS);
}

TEST(align_top_left) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_TOP_LEFT);
    ASSERT_EQ_F(r.x,   0.0f, EPS);
    ASSERT_EQ_F(r.y,   0.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_top_right) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_TOP_RIGHT);
    ASSERT_EQ_F(r.x, 300.0f, EPS);
    ASSERT_EQ_F(r.y,   0.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_top_center) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_TOP_CENTER);
    ASSERT_EQ_F(r.x, 150.0f, EPS);
    ASSERT_EQ_F(r.y,   0.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_bottom_left) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_BOTTOM_LEFT);
    ASSERT_EQ_F(r.x,   0.0f, EPS);
    ASSERT_EQ_F(r.y, 250.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_bottom_right) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_BOTTOM_RIGHT);
    ASSERT_EQ_F(r.x, 300.0f, EPS);
    ASSERT_EQ_F(r.y, 250.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_bottom_center) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_BOTTOM_CENTER);
    ASSERT_EQ_F(r.x, 150.0f, EPS);
    ASSERT_EQ_F(r.y, 250.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_center) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_CENTER);
    ASSERT_EQ_F(r.x, 150.0f, EPS);
    ASSERT_EQ_F(r.y, 125.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_left) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_LEFT);
    ASSERT_EQ_F(r.x,   0.0f, EPS);
    ASSERT_EQ_F(r.y, 125.0f, EPS);  // vertically centered
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_right) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_RIGHT);
    ASSERT_EQ_F(r.x, 300.0f, EPS);
    ASSERT_EQ_F(r.y, 125.0f, EPS);  // vertically centered
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_top) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_TOP);
    ASSERT_EQ_F(r.x, 150.0f, EPS);  // horizontally centered
    ASSERT_EQ_F(r.y,   0.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_bottom) {
    WLX_Rect parent = {0, 0, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_BOTTOM);
    ASSERT_EQ_F(r.x, 150.0f, EPS);  // horizontally centered
    ASSERT_EQ_F(r.y, 250.0f, EPS);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
}

TEST(align_negative_dims) {
    // width=-1, height=-1 -> returns parent rect unchanged
    WLX_Rect parent = {10, 20, 200, 100};
    WLX_Rect r = wlx_get_align_rect(parent, -1, -1, WLX_CENTER);
    ASSERT_EQ_RECT(r, parent, EPS);
}

TEST(align_larger_than_parent) {
    // Child wider/taller than parent -> w/h stays at parent size
    WLX_Rect parent = {0, 0, 100, 80};
    WLX_Rect r = wlx_get_align_rect(parent, 200, 150, WLX_CENTER);
    ASSERT_EQ_F(r.w, 100.0f, EPS);  // clamped to parent
    ASSERT_EQ_F(r.h,  80.0f, EPS);
}

TEST(align_with_offset_parent) {
    // Parent not at origin
    WLX_Rect parent = {50, 100, 400, 300};
    WLX_Rect r = wlx_get_align_rect(parent, 100, 50, WLX_CENTER);
    ASSERT_EQ_F(r.x, 200.0f, EPS);  // 50 + (400-100)/2
    ASSERT_EQ_F(r.y, 225.0f, EPS);  // 100 + (300-50)/2
}

// ============================================================================
// wlx_rect_intersect
// ============================================================================

TEST(rect_intersect_overlap) {
    WLX_Rect a = {0, 0, 100, 100};
    WLX_Rect b = {50, 50, 100, 100};
    WLX_Rect r = wlx_rect_intersect(a, b);
    ASSERT_EQ_RECT(r, ((WLX_Rect){50, 50, 50, 50}), EPS);
}

TEST(rect_intersect_none) {
    WLX_Rect a = {0, 0, 50, 50};
    WLX_Rect b = {100, 100, 50, 50};
    WLX_Rect r = wlx_rect_intersect(a, b);
    ASSERT_EQ_F(r.w, 0.0f, EPS);
    ASSERT_EQ_F(r.h, 0.0f, EPS);
}

TEST(rect_intersect_contained) {
    WLX_Rect outer = {0, 0, 200, 200};
    WLX_Rect inner = {50, 50, 50, 50};
    WLX_Rect r = wlx_rect_intersect(outer, inner);
    ASSERT_EQ_RECT(r, inner, EPS);
}

TEST(rect_intersect_touching_edge) {
    // Adjacent but not overlapping
    WLX_Rect a = {0, 0, 100, 100};
    WLX_Rect b = {100, 0, 100, 100};
    WLX_Rect r = wlx_rect_intersect(a, b);
    ASSERT_EQ_F(r.w, 0.0f, EPS);
}

// ============================================================================
// wlx_rect_inset
// ============================================================================

TEST(rect_inset_normal) {
    WLX_Rect r = wlx_rect_inset((WLX_Rect){10, 20, 200, 100}, 5.0f);
    ASSERT_EQ_RECT(r, ((WLX_Rect){15, 25, 190, 90}), EPS);
}

TEST(rect_inset_zero) {
    WLX_Rect orig = {10, 20, 200, 100};
    WLX_Rect r = wlx_rect_inset(orig, 0.0f);
    ASSERT_EQ_RECT(r, orig, EPS);
}

TEST(rect_inset_negative) {
    WLX_Rect orig = {10, 20, 200, 100};
    WLX_Rect r = wlx_rect_inset(orig, -5.0f);
    ASSERT_EQ_RECT(r, orig, EPS);  // negative -> unchanged
}

TEST(rect_inset_too_large) {
    // Inset bigger than half the dimension -> w/h clamped to 0
    WLX_Rect r = wlx_rect_inset((WLX_Rect){0, 0, 20, 10}, 15.0f);
    ASSERT_EQ_F(r.w, 0.0f, EPS);
    ASSERT_EQ_F(r.h, 0.0f, EPS);
}

// ============================================================================
// wlx_point_in_rect
// ============================================================================

TEST(point_in_rect_inside) {
    ASSERT_TRUE(wlx_point_in_rect(50, 50, 0, 0, 100, 100));
}

TEST(point_in_rect_top_left_edge) {
    // Left and top edges are inclusive
    ASSERT_TRUE(wlx_point_in_rect(0, 0, 0, 0, 100, 100));
}

TEST(point_in_rect_right_edge_exclusive) {
    // Right edge is exclusive (px < x + w)
    ASSERT_FALSE(wlx_point_in_rect(100, 50, 0, 0, 100, 100));
}

TEST(point_in_rect_bottom_edge_exclusive) {
    ASSERT_FALSE(wlx_point_in_rect(50, 100, 0, 0, 100, 100));
}

TEST(point_in_rect_outside) {
    ASSERT_FALSE(wlx_point_in_rect(200, 200, 0, 0, 100, 100));
}

// ============================================================================
// wlx_resolve_widget_rect
// ============================================================================

TEST(resolve_widget_default_size) {
    // width=-1, height=-1 -> takes cell size
    WLX_Rect cell = {10, 20, 200, 100};
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 0, 0, 0, 0, WLX_ALIGN_NONE, false);
    ASSERT_EQ_RECT(r, cell, EPS);
}

TEST(resolve_widget_fixed_size) {
    WLX_Rect cell = {0, 0, 400, 300};
    WLX_Rect r = wlx_resolve_widget_rect(cell, 100, 50, 0, 0, 0, 0, WLX_CENTER, false);
    ASSERT_EQ_F(r.w, 100.0f, EPS);
    ASSERT_EQ_F(r.h,  50.0f, EPS);
    ASSERT_EQ_F(r.x, 150.0f, EPS);  // centered
    ASSERT_EQ_F(r.y, 125.0f, EPS);
}

TEST(resolve_widget_min_clamp) {
    WLX_Rect cell = {0, 0, 400, 300};
    // width=-1 -> cell.w=400 but min_w=450 -> w=450
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 450, 350, 0, 0, WLX_ALIGN_NONE, false);
    ASSERT_EQ_F(r.w, 450.0f, EPS);
    ASSERT_EQ_F(r.h, 350.0f, EPS);
}

TEST(resolve_widget_max_clamp) {
    WLX_Rect cell = {0, 0, 400, 300};
    WLX_Rect r = wlx_resolve_widget_rect(cell, -1, -1, 0, 0, 200, 150, WLX_ALIGN_NONE, false);
    ASSERT_EQ_F(r.w, 200.0f, EPS);
    ASSERT_EQ_F(r.h, 150.0f, EPS);
}

TEST(resolve_widget_overflow) {
    WLX_Rect cell = {50, 100, 200, 80};
    WLX_Rect r = wlx_resolve_widget_rect(cell, 300, 200, 0, 0, 0, 0, WLX_CENTER, true);
    // overflow -> position at cell origin, ignore alignment
    ASSERT_EQ_F(r.x,  50.0f, EPS);
    ASSERT_EQ_F(r.y, 100.0f, EPS);
    ASSERT_EQ_F(r.w, 300.0f, EPS);
    ASSERT_EQ_F(r.h, 200.0f, EPS);
}

// ============================================================================
// wlx_get_parent_rect / wlx_get_available_width / wlx_get_available_height
// ============================================================================

TEST(parent_rect_no_layout_returns_root) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // No layouts pushed - should return root rect
    WLX_Rect r = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r.x, 0.0f, EPS);
    ASSERT_EQ_F(r.y, 0.0f, EPS);
    ASSERT_EQ_F(r.w, 800.0f, EPS);
    ASSERT_EQ_F(r.h, 600.0f, EPS);

    test_frame_end(&ctx);
}

TEST(parent_rect_single_layout) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Push a 3-slot VERT layout - parent rect should be the root rect
    wlx_layout_begin(&ctx, 3, WLX_VERT);
    WLX_Rect r = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r.w, 800.0f, EPS);
    ASSERT_EQ_F(r.h, 600.0f, EPS);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(parent_rect_nested_layout) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Outer: 2-slot HORZ (200px + FLEX)
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(200), WLX_SLOT_FLEX(1) });

    // Consume slot 0 with a nested layout
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r0 = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r0.w, 200.0f, EPS);
    ASSERT_EQ_F(r0.h, 600.0f, EPS);
    wlx_layout_end(&ctx);

    // Consume slot 1 with a nested layout - gets remaining 600px
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r1 = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r1.w, 600.0f, EPS);
    ASSERT_EQ_F(r1.h, 600.0f, EPS);
    wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(available_width_height_helpers) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 1024, 768);
    test_frame_begin(&ctx, 0, 0, false, false);

    ASSERT_EQ_F(wlx_get_available_width(&ctx), 1024.0f, EPS);
    ASSERT_EQ_F(wlx_get_available_height(&ctx), 768.0f, EPS);

    wlx_layout_begin(&ctx, 2, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) });
    // Inside the outer layout, parent rect is still 1024x768
    ASSERT_EQ_F(wlx_get_available_height(&ctx), 768.0f, EPS);

    // Nest into slot 0 (100px tall)
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    ASSERT_EQ_F(wlx_get_available_height(&ctx), 100.0f, EPS);
    ASSERT_EQ_F(wlx_get_available_width(&ctx), 1024.0f, EPS);
    wlx_layout_end(&ctx);

    // Nest into slot 1 (668px tall)
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    ASSERT_EQ_F(wlx_get_available_height(&ctx), 668.0f, EPS);
    wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_panel_viewport_none) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // No scroll panel - returns zero rect
    WLX_Rect vp = wlx_get_scroll_panel_viewport(&ctx);
    ASSERT_EQ_F(vp.w, 0.0f, EPS);
    ASSERT_EQ_F(vp.h, 0.0f, EPS);

    test_frame_end(&ctx);
}

TEST(scroll_panel_viewport_active) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin(&ctx, 1000);  // 1000px content in 600px viewport
    WLX_Rect vp = wlx_get_scroll_panel_viewport(&ctx);
    ASSERT_EQ_F(vp.w, 800.0f, EPS);
    ASSERT_EQ_F(vp.h, 600.0f, EPS);
    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);

    test_frame_end(&ctx);
}

TEST(scroll_panel_viewport_nested) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Outer layout -> outer scroll panel (full 800x600)
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin(&ctx, 2000);  // outer: 2000px content

        // Check viewport is the outer panel
        WLX_Rect outer_vp = wlx_get_scroll_panel_viewport(&ctx);
        ASSERT_EQ_F(outer_vp.w, 800.0f, EPS);
        ASSERT_EQ_F(outer_vp.h, 600.0f, EPS);

        // Inner layout -> inner scroll panel with explicit height
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_scroll_panel_begin(&ctx, 500, .height = 200);  // inner: 200px viewport

            // Viewport should now be the inner panel's rect
            WLX_Rect inner_vp = wlx_get_scroll_panel_viewport(&ctx);
            ASSERT_EQ_F(inner_vp.h, 200.0f, EPS);
            // Inner width inherits from the outer content area (minus scrollbar)
            ASSERT_TRUE(inner_vp.w > 0.0f);
            ASSERT_TRUE(inner_vp.w <= 800.0f);

        wlx_scroll_panel_end(&ctx);
        wlx_layout_end(&ctx);

        // After closing the inner panel, viewport should revert to outer
        WLX_Rect restored_vp = wlx_get_scroll_panel_viewport(&ctx);
        ASSERT_EQ_F(restored_vp.w, 800.0f, EPS);
        ASSERT_EQ_F(restored_vp.h, 600.0f, EPS);

    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);

    test_frame_end(&ctx);
}

// ============================================================================
// WLX_SIZES / wlx_layout_begin_s
// ============================================================================

TEST(sizes_macro_count) {
    // Verify the sizeof/sizeof trick produces the correct count
    size_t count_2 = sizeof((WLX_Slot_Size[]){
        WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) }) / sizeof(WLX_Slot_Size);
    ASSERT_EQ_INT((int)count_2, 2);

    size_t count_5 = sizeof((WLX_Slot_Size[]){
        WLX_SLOT_PX(44), WLX_SLOT_FLEX(1), WLX_SLOT_PX(24),
        WLX_SLOT_AUTO, WLX_SLOT_PCT(20) }) / sizeof(WLX_Slot_Size);
    ASSERT_EQ_INT((int)count_5, 5);

    size_t count_1 = sizeof((WLX_Slot_Size[]){
        WLX_SLOT_FLEX(1) }) / sizeof(WLX_Slot_Size);
    ASSERT_EQ_INT((int)count_1, 1);
}

TEST(layout_begin_s_basic) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // 3-slot VERT via wlx_layout_begin_s
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_FLEX(1), WLX_SLOT_PX(50)));

    // Slot 0: 100px
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r0 = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r0.h, 100.0f, EPS);
    wlx_layout_end(&ctx);

    // Slot 1: 600 - 100 - 50 = 450px
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r1 = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r1.h, 450.0f, EPS);
    wlx_layout_end(&ctx);

    // Slot 2: 50px
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r2 = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r2.h, 50.0f, EPS);
    wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(layout_begin_s_with_options) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Use wlx_layout_begin_s with .padding option
    wlx_layout_begin_s(&ctx, WLX_HORZ,
        WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_FLEX(1)),
        .padding = 10);

    // Layout rect should be inset by padding
    WLX_Rect parent = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(parent.x, 10.0f, EPS);
    ASSERT_EQ_F(parent.w, 380.0f, EPS);  // 400 - 2*10

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(layout_begin_s_single_slot) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Single-slot - common for viewport-fill wrappers
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(500)));

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Rect r = wlx_get_parent_rect(&ctx);
    ASSERT_EQ_F(r.h, 500.0f, EPS);
    wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(layout_math) {
    // wlx_compute_offsets
    RUN_TEST(offsets_equal_split_2);
    RUN_TEST(offsets_equal_split_5);
    RUN_TEST(offsets_single_slot);
    RUN_TEST(offsets_all_pixels);
    RUN_TEST(offsets_all_percent);
    RUN_TEST(offsets_all_flex_equal);
    RUN_TEST(offsets_flex_weighted);
    RUN_TEST(offsets_auto_slots);
    RUN_TEST(offsets_mixed_px_flex);
    RUN_TEST(offsets_mixed_px_pct_flex);
    RUN_TEST(offsets_zero_remaining_for_flex);
    RUN_TEST(offsets_flex_with_min);
    RUN_TEST(offsets_flex_with_max);
    RUN_TEST(offsets_pct_minmax);
    RUN_TEST(offsets_monotonic);

    // wlx_compute_offsets - FILL (viewport-relative)
    RUN_TEST(offsets_fill_basic);
    RUN_TEST(offsets_fill_plus_px);
    RUN_TEST(offsets_fill_plus_flex);
    RUN_TEST(offsets_fill_flex_no_remaining);
    RUN_TEST(offsets_fill_with_min);
    RUN_TEST(offsets_fill_with_max);
    RUN_TEST(offsets_fill_pct);
    RUN_TEST(offsets_fill_multiple);
    RUN_TEST(offsets_fill_no_scroll_panel);

    // wlx_get_align_rect
    RUN_TEST(align_none);
    RUN_TEST(align_top_left);
    RUN_TEST(align_top_right);
    RUN_TEST(align_top_center);
    RUN_TEST(align_bottom_left);
    RUN_TEST(align_bottom_right);
    RUN_TEST(align_bottom_center);
    RUN_TEST(align_center);
    RUN_TEST(align_left);
    RUN_TEST(align_right);
    RUN_TEST(align_top);
    RUN_TEST(align_bottom);
    RUN_TEST(align_negative_dims);
    RUN_TEST(align_larger_than_parent);
    RUN_TEST(align_with_offset_parent);

    // wlx_rect_intersect
    RUN_TEST(rect_intersect_overlap);
    RUN_TEST(rect_intersect_none);
    RUN_TEST(rect_intersect_contained);
    RUN_TEST(rect_intersect_touching_edge);

    // wlx_rect_inset
    RUN_TEST(rect_inset_normal);
    RUN_TEST(rect_inset_zero);
    RUN_TEST(rect_inset_negative);
    RUN_TEST(rect_inset_too_large);

    // wlx_point_in_rect
    RUN_TEST(point_in_rect_inside);
    RUN_TEST(point_in_rect_top_left_edge);
    RUN_TEST(point_in_rect_right_edge_exclusive);
    RUN_TEST(point_in_rect_bottom_edge_exclusive);
    RUN_TEST(point_in_rect_outside);

    // wlx_resolve_widget_rect
    RUN_TEST(resolve_widget_default_size);
    RUN_TEST(resolve_widget_fixed_size);
    RUN_TEST(resolve_widget_min_clamp);
    RUN_TEST(resolve_widget_max_clamp);
    RUN_TEST(resolve_widget_overflow);

    // wlx_get_parent_rect / wlx_get_available_width / wlx_get_available_height
    RUN_TEST(parent_rect_no_layout_returns_root);
    RUN_TEST(parent_rect_single_layout);
    RUN_TEST(parent_rect_nested_layout);
    RUN_TEST(available_width_height_helpers);

    // wlx_get_scroll_panel_viewport
    RUN_TEST(scroll_panel_viewport_none);
    RUN_TEST(scroll_panel_viewport_active);
    RUN_TEST(scroll_panel_viewport_nested);

    // WLX_SIZES / wlx_layout_begin_s
    RUN_TEST(sizes_macro_count);
    RUN_TEST(layout_begin_s_basic);
    RUN_TEST(layout_begin_s_with_options);
    RUN_TEST(layout_begin_s_single_slot);
}

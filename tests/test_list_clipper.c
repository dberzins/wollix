// test_list_clipper.c - list clipper virtualization tests
// Covers fixed-pitch range math, positioning via spacers, virtualization
// (visible count independent of total count), and variable-height mode.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

// Stable source location for the panel state across frames within a test.
static WLX_Scroll_Panel_State *lc_open(WLX_Context *ctx, float total) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(ctx, total,
        wlx_default_scroll_panel_opt(.wheel_scroll_speed = 20.0f),
        __FILE__, __LINE__);
    return wlx_pool_scroll_panels(ctx)[ctx->arena.scroll_panels.count - 1];
}

static void lc_close(WLX_Context *ctx) {
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
}

// Build the visible rows as empty nested layouts (no draw). Exercises the
// spacer + row interaction under WLX_DEBUG.
static void lc_build_rows(WLX_Context *ctx, const WLX_List_Clipper *c) {
    for (int i = c->first; i < c->last; i++) {
        wlx_layout_begin(ctx, 1, WLX_VERT);
        wlx_layout_end(ctx);
    }
}

// ============================================================================
// Height helper
// ============================================================================

TEST(clipper_height_fixed_and_variable) {
    ASSERT_EQ_F(wlx_list_clipper_height(100, 20.0f, NULL), 2000.0f, 0.01f);
    ASSERT_EQ_F(wlx_list_clipper_height(0, 20.0f, NULL), 0.0f, 0.01f);
    float offs[] = { 0, 50, 60, 200, 260, 300 };
    ASSERT_EQ_F(wlx_list_clipper_height(5, 20.0f, offs), 300.0f, 0.01f);
}

// ============================================================================
// Fixed-pitch range
// ============================================================================

TEST(clipper_range_top) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(100, 20.0f, NULL));
    WLX_Rect vp = wlx_get_scroll_panel_viewport(&ctx);
    float off = wlx_get_scroll_panel_offset(&ctx);
    ASSERT_EQ_F(off, 0.0f, 0.5f);

    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 100, 20.0f, .id = "rows");
    int exp_last = (int)ceilf((off + vp.h) / 20.0f);
    if (exp_last > 100) exp_last = 100;
    ASSERT_EQ_INT(c.first, 0);
    ASSERT_EQ_INT(c.last, exp_last);
    lc_build_rows(&ctx, &c);
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

TEST(clipper_range_scrolled_and_position) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    float total = wlx_list_clipper_height(100, 20.0f, NULL);

    // Frame 1: force a scroll offset that persists to frame 2.
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Scroll_Panel_State *sp = lc_open(&ctx, total);
    sp->scroll_offset = 500.0f;
    WLX_List_Clipper c1 = wlx_list_clipper_begin(&ctx, 100, 20.0f, .id = "rows");
    lc_build_rows(&ctx, &c1);
    wlx_list_clipper_end(&ctx, &c1);
    lc_close(&ctx);
    test_frame_end(&ctx);

    // Frame 2: offset is now 500; check range and first-row placement.
    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, total);
    WLX_Rect vp = wlx_get_scroll_panel_viewport(&ctx);
    float off = wlx_get_scroll_panel_offset(&ctx);
    ASSERT_EQ_F(off, 500.0f, 1.0f);

    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 100, 20.0f, .id = "rows");
    ASSERT_EQ_INT(c.first, (int)floorf(off / 20.0f));        // 25
    ASSERT_EQ_INT(c.last, (int)ceilf((off + vp.h) / 20.0f)); // 40

    // The first built row should sit at the viewport top: y = vp.y - off + before.
    for (int i = c.first; i < c.last; i++) {
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        if (i == c.first) {
            WLX_Layout *row = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
            ASSERT_EQ_F(row->rect.y, vp.y, 1.0f);
        }
        wlx_layout_end(&ctx);
    }
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

TEST(clipper_range_clamps_when_content_fits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // 5 rows of 20 px = 100 px < 300 px panel: everything visible.
    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(5, 20.0f, NULL));
    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 5, 20.0f, .id = "rows");
    ASSERT_EQ_INT(c.first, 0);
    ASSERT_EQ_INT(c.last, 5);
    lc_build_rows(&ctx, &c);
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

TEST(clipper_empty_list) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(0, 20.0f, NULL));
    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 0, 20.0f, .id = "rows");
    ASSERT_EQ_INT(c.first, 0);
    ASSERT_EQ_INT(c.last, 0);
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

// Visible row count is bounded by the viewport, independent of total count.
TEST(clipper_virtualizes_visible_count) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(100, 20.0f, NULL));
    WLX_List_Clipper c100 = wlx_list_clipper_begin(&ctx, 100, 20.0f, .id = "rows");
    int n100 = c100.last - c100.first;
    lc_build_rows(&ctx, &c100);
    wlx_list_clipper_end(&ctx, &c100);
    lc_close(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(100000, 20.0f, NULL));
    WLX_List_Clipper cbig = wlx_list_clipper_begin(&ctx, 100000, 20.0f, .id = "rows");
    int nbig = cbig.last - cbig.first;
    lc_build_rows(&ctx, &cbig);
    wlx_list_clipper_end(&ctx, &cbig);
    lc_close(&ctx);
    test_frame_end(&ctx);

    // Same viewport -> same number of produced rows regardless of total.
    ASSERT_EQ_INT(n100, nbig);
    ASSERT_TRUE(nbig < 100); // far fewer than the full list
}

// ============================================================================
// Variable-height mode
// ============================================================================

TEST(clipper_variable_range_top) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100); // vp.h ~= 100
    // 5 items, heights 50,10,140,60,40 -> offsets prefix sums:
    static const float offs[] = { 0, 50, 60, 200, 260, 300 };

    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, wlx_list_clipper_height(5, 20.0f, offs));
    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 5, 20.0f,
        .id = "rows", .item_offsets = offs);
    // top=0, bottom~=100 -> items 0,1,2 visible.
    ASSERT_EQ_INT(c.first, 0);
    ASSERT_EQ_INT(c.last, 3);
    ASSERT_EQ_F(wlx_list_clipper_item_height(&c, 0), 50.0f, 0.01f);
    ASSERT_EQ_F(wlx_list_clipper_item_height(&c, 1), 10.0f, 0.01f);
    ASSERT_EQ_F(wlx_list_clipper_item_height(&c, 2), 140.0f, 0.01f);
    for (int i = c.first; i < c.last; i++) {
        wlx_layout_auto_slot_px(&ctx, wlx_list_clipper_item_height(&c, i));
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_layout_end(&ctx);
    }
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

TEST(clipper_variable_range_scrolled) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    static const float offs[] = { 0, 50, 60, 200, 260, 300 };
    float total = wlx_list_clipper_height(5, 20.0f, offs);

    // Frame 1: set offset 150.
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Scroll_Panel_State *sp = lc_open(&ctx, total);
    sp->scroll_offset = 150.0f;
    WLX_List_Clipper c1 = wlx_list_clipper_begin(&ctx, 5, 20.0f,
        .id = "rows", .item_offsets = offs);
    wlx_list_clipper_end(&ctx, &c1);
    lc_close(&ctx);
    test_frame_end(&ctx);

    // Frame 2: offset 150, bottom ~250 -> items 2,3 visible.
    test_frame_begin(&ctx, 0, 0, false, false);
    lc_open(&ctx, total);
    float off = wlx_get_scroll_panel_offset(&ctx);
    ASSERT_EQ_F(off, 150.0f, 1.0f);
    WLX_List_Clipper c = wlx_list_clipper_begin(&ctx, 5, 20.0f,
        .id = "rows", .item_offsets = offs);
    ASSERT_EQ_INT(c.first, 2);
    ASSERT_EQ_INT(c.last, 4);
    wlx_list_clipper_end(&ctx, &c);
    lc_close(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(list_clipper) {
    RUN_TEST(clipper_height_fixed_and_variable);
    RUN_TEST(clipper_range_top);
    RUN_TEST(clipper_range_scrolled_and_position);
    RUN_TEST(clipper_range_clamps_when_content_fits);
    RUN_TEST(clipper_empty_list);
    RUN_TEST(clipper_virtualizes_visible_count);
    RUN_TEST(clipper_variable_range_top);
    RUN_TEST(clipper_variable_range_scrolled);
}

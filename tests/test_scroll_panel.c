// test_scroll_panel.c - scroll panel behavior tests (offset, clamping, wheel, auto-height)

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

// Helper: run a scroll panel frame. Creates a wrapping layout, and
// opens a scroll panel. Returns the persistent state pointer.
// The caller must call `wlx_scroll_panel_end()` + `wlx_layout_end()` + `test_frame_end()`.
//
// Because `wlx_scroll_panel_begin_impl()` uses file/line for state persistence,
// we use wrapper functions so each "panel" has its own stable source location.

static void open_scroll_panel_A(WLX_Context *ctx, float content_height) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(ctx, content_height,
        wlx_default_scroll_panel_opt(.wheel_scroll_speed = 20.0f),
        __FILE__, __LINE__);
}

static void close_scroll_panel_A(WLX_Context *ctx) {
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
}

// Helper to get the scroll state for panel A by running a neutral frame
// and inspecting the state. Since wlx_get_state_impl uses file/line,
// we match the same file/line used in open_scroll_panel_A isn't reachable
// here - instead we just run full frame sequences and observe effects.

// ============================================================================
// Scroll offset tests
// ============================================================================

TEST(scroll_initial_zero) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Content taller than panel -> scrollbar appears, offset starts at 0
    // Frame 1: no input, just open/close
    test_frame_begin(&ctx, 0, 0, false, false);
    open_scroll_panel_A(&ctx, 600);  // 600px content in 300px panel
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: open the same panel again, check the created layout's y matches panel y
    // (if scroll_offset were non-zero, content_rect.y would differ from panel_rect.y)
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(&ctx, 600,
        wlx_default_scroll_panel_opt(.wheel_scroll_speed = 20.0f),
        __FILE__, __LINE__);

    // The content layout is the topmost layout. Its rect.y should be
    // panel.y - scroll_offset. For zero scroll, the y should be very close to 0.
    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content_layout = &ctx.layouts.items[ctx.layouts.count - 1];
    // scroll_offset = 0, so content rect y should equal panel rect y (approximately 0)
    ASSERT_EQ_F(content_layout->rect.y, 0.0f, 1.0f);

    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_wheel_down) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Frame 1: initialize the panel
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: scroll wheel down (negative delta = scroll down in most UIs)
    // The implementation does: scroll_offset -= wheel_delta * speed
    // So negative wheel_delta -> scroll_offset increases (scrolls down)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         -2.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 3: check that content has shifted up (content layout y < panel y)
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);

    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content = &ctx.layouts.items[ctx.layouts.count - 1];
    // scroll_offset > 0, so content rect y < panel rect y
    ASSERT_TRUE(content->rect.y < 0.0f);

    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_wheel_up) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Frame 1: init
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: scroll down first
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         -3.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 3: now scroll up (positive wheel_delta)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         1.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 4: check content y - should have scrolled back up partially
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);

    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content = &ctx.layouts.items[ctx.layouts.count - 1];
    // After -3*20=60 down then +1*20=20 up -> net 40 down, so y should be ~ -40
    ASSERT_EQ_F(content->rect.y, -40.0f, 1.0f);

    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_clamp_top) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Frame 1: init
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: try to scroll UP from zero (positive delta)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         5.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 3: check content hasn't moved above 0
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);

    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content = &ctx.layouts.items[ctx.layouts.count - 1];
    // scroll_offset clamped to 0, so y stays at panel y (~0)
    ASSERT_EQ_F(content->rect.y, 0.0f, 1.0f);

    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_clamp_bottom) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Panel is 300px tall, content is 600px -> max_scroll = 300
    // Frame 1: init
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: scroll waaaaay down (delta=-100, speed=20 -> would be 2000px)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         -100.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 600);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 3: check content clamped to max_scroll = 300
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 600);

    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content = &ctx.layouts.items[ctx.layouts.count - 1];
    // max_scroll = 600 - 300 = 300 -> content y = panel_y - 300
    ASSERT_EQ_F(content->rect.y, -300.0f, 1.0f);

    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_no_scroll_when_content_fits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Content shorter than panel -> no scrolling possible
    // Frame 1: init
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 200);  // 200 < 300 panel
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 2: try scrolling
    test_frame_begin_ex(&ctx, 200, 150, false, false, false,
                         -5.0f, NULL, NULL, NULL);
    open_scroll_panel_A(&ctx, 200);
    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);

    // Frame 3: scroll should be clamped at 0 (max_scroll = 0 because content fits)
    test_frame_begin(&ctx, 200, 150, false, false);
    open_scroll_panel_A(&ctx, 200);

    ASSERT_TRUE(ctx.layouts.count > 0);
    WLX_Layout *content = &ctx.layouts.items[ctx.layouts.count - 1];
    ASSERT_EQ_F(content->rect.y, 0.0f, 1.0f);

    close_scroll_panel_A(&ctx);
    test_frame_end(&ctx);
}

TEST(scroll_auto_height) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Auto-height: content_height = -1, the panel measures content size from inner layouts.
    // Frame 1: open auto-height panel, place some inner layouts to accumulate content_height
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(&ctx, -1,
        wlx_default_scroll_panel_opt(.wheel_scroll_speed = 20.0f),
        __FILE__, __LINE__);

    // Place inner content: 5 items at auto-sized (~60px each in 300px)
    wlx_layout_begin(&ctx, 5, WLX_VERT);
    wlx_layout_end(&ctx);

    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: the auto-height measurement from frame 1 is now stored.
    // Just verify the panel opens without crashing (auto-height is complex
    // and the key invariant is that it doesn't crash or produce garbage).
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(&ctx, -1,
        wlx_default_scroll_panel_opt(.wheel_scroll_speed = 20.0f),
        __FILE__, __LINE__);

    ASSERT_TRUE(ctx.layouts.count > 0);

    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Content height double-counting regression tests (5C fix)
// ============================================================================

// Helper: open/close an auto-height scroll panel at a fixed source location
// so that the persistent state ID is the same across frames within one test.
static void open_auto_scroll(WLX_Context *ctx) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin_impl(ctx, -1,
        wlx_default_scroll_panel_opt(), __FILE__, __LINE__);
}

static void close_auto_scroll(WLX_Context *ctx) {
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
}

// T1: Nested VERT inside a grid - content height should reflect the grid's
// row total, not the sum of every inner layout's accumulated heights.
TEST(scroll_nested_grid_no_double_count) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Frame 1: measure
    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        WLX_Slot_Size row_sizes[] = { WLX_SLOT_PX(40) };
        wlx_grid_begin(&ctx, 1, 2, .row_sizes = row_sizes);
            wlx_layout_begin(&ctx, 2, WLX_VERT);
                wlx_widget(&ctx, .height = 15);
                wlx_widget(&ctx, .height = 15);
            wlx_layout_end(&ctx);
            wlx_widget(&ctx, .height = 30);
        wlx_grid_end(&ctx);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);

    // Frame 2: check measured height
    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        WLX_Scroll_Panel_State *sp =
            ctx.scroll_panels.items[ctx.scroll_panels.count - 1];
        // Should be ~40px (one grid row), not inflated by inner layouts.
        ASSERT_TRUE(sp->content_height <= 60.0f);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);
}

// T2: Flat VERT - basic 3-widget measurement still correct.
TEST(scroll_flat_vert_height) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        wlx_layout_begin(&ctx, 3, WLX_VERT);
            wlx_widget(&ctx, .height = 50);
            wlx_widget(&ctx, .height = 50);
            wlx_widget(&ctx, .height = 50);
        wlx_layout_end(&ctx);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        WLX_Scroll_Panel_State *sp =
            ctx.scroll_panels.items[ctx.scroll_panels.count - 1];
        // 3 * 50 = 150px
        ASSERT_EQ_F(sp->content_height, 150.0f, 5.0f);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);
}

// T3: Nested VERT-in-VERT - inner content propagates correctly.
TEST(scroll_nested_vert_propagation) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        wlx_layout_begin(&ctx, 2, WLX_VERT);
            wlx_layout_begin(&ctx, 2, WLX_VERT);
                wlx_widget(&ctx, .height = 30);
                wlx_widget(&ctx, .height = 30);
            wlx_layout_end(&ctx);
            wlx_widget(&ctx, .height = 40);
        wlx_layout_end(&ctx);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        WLX_Scroll_Panel_State *sp =
            ctx.scroll_panels.items[ctx.scroll_panels.count - 1];
        // Inner: 30+30=60, Outer: 60+40=100 (plus padding from inner layout)
        ASSERT_TRUE(sp->content_height >= 95.0f && sp->content_height <= 115.0f);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);
}

// T4: HORZ layout - max semantics for height.
TEST(scroll_horz_layout_max_height) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        wlx_layout_begin(&ctx, 2, WLX_HORZ);
            wlx_widget(&ctx, .height = 60);
            wlx_widget(&ctx, .height = 80);
        wlx_layout_end(&ctx);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    open_auto_scroll(&ctx);
        WLX_Scroll_Panel_State *sp =
            ctx.scroll_panels.items[ctx.scroll_panels.count - 1];
        // HORZ: max(60, 80) = 80
        ASSERT_EQ_F(sp->content_height, 80.0f, 5.0f);
    close_auto_scroll(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(scroll_panel) {
    RUN_TEST(scroll_initial_zero);
    RUN_TEST(scroll_wheel_down);
    RUN_TEST(scroll_wheel_up);
    RUN_TEST(scroll_clamp_top);
    RUN_TEST(scroll_clamp_bottom);
    RUN_TEST(scroll_no_scroll_when_content_fits);
    RUN_TEST(scroll_auto_height);
    RUN_TEST(scroll_nested_grid_no_double_count);
    RUN_TEST(scroll_flat_vert_height);
    RUN_TEST(scroll_nested_vert_propagation);
    RUN_TEST(scroll_horz_layout_max_height);
}

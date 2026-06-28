// test_offscreen_cull.c - opt-in offscreen command culling (wlx_set_cull_offscreen)
// Verifies: default off preserves behavior; when on, rect-bounded commands
// fully outside the active clip are dropped while overlapping ones are kept;
// excluded types (text) survive; nested clips intersect (superset safety).

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

static int cull_count_type(WLX_Context *ctx, WLX_Cmd_Type t) {
    WLX_Cmd *cmds = wlx_pool_commands(ctx);
    int n = 0;
    for (size_t i = 0; i < ctx->arena.commands.count; i++) {
        if (cmds[i].type == t) n++;
    }
    return n;
}

// Draw one rect outside the clip and one inside, returning the recorded RECT
// count (inspected before frame end). `cull` toggles the feature.
static int run_inside_outside(bool cull) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    wlx_set_cull_offscreen(&ctx, cull);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true); // clip rect = (0,0,400,300)
        wlx_draw_rect(&ctx, wlx_rect(0, 1000, 50, 50), WLX_WHITE); // fully below clip
        wlx_draw_rect(&ctx, wlx_rect(0, 0, 50, 50), WLX_WHITE);    // inside clip
    int rects = cull_count_type(&ctx, WLX_CMD_RECT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    return rects;
}

TEST(cull_off_by_default_records_both) {
    ASSERT_EQ_INT(run_inside_outside(false), 2);
}

TEST(cull_on_drops_outside_keeps_inside) {
    ASSERT_EQ_INT(run_inside_outside(true), 1);
}

TEST(cull_edge_touching_is_dropped) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    wlx_set_cull_offscreen(&ctx, true);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true);
        // y starts exactly at the clip bottom: zero-area intersection -> dropped.
        wlx_draw_rect(&ctx, wlx_rect(0, 300, 50, 50), WLX_WHITE);
        ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_RECT), 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(cull_excludes_text) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    wlx_set_cull_offscreen(&ctx, true);
    WLX_Text_Style ts = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true);
        // Text far outside the clip is still recorded (no width at record time).
        wlx_draw_text(&ctx, "x", 0.0f, 1000.0f, ts);
        ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_TEXT), 1);
        // ...while a rect at the same position is dropped.
        wlx_draw_rect(&ctx, wlx_rect(0, 1000, 50, 50), WLX_WHITE);
        ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_RECT), 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// Nested clip: the effective clip is the intersection. A rect inside the outer
// clip but outside the inner clip must be dropped (superset-safety check).
TEST(cull_nested_clip_intersects) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    wlx_set_cull_offscreen(&ctx, true);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true); // outer (0,0,400,300)
        // A 100px-wide first slot hosts the inner clip; its rect is (0,0,100,300).
        WLX_Slot_Size cols[] = { WLX_SLOT_PX(100), WLX_SLOT_FLEX(1) };
        wlx_layout_begin(&ctx, 2, WLX_HORZ, .sizes = cols);
            wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true); // inner band (0,0,100,300)
                // x=200 is inside the outer clip but outside the inner band.
                wlx_draw_rect(&ctx, wlx_rect(200, 0, 50, 50), WLX_WHITE);
                ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_RECT), 0);
                // a rect inside the inner band is kept.
                wlx_draw_rect(&ctx, wlx_rect(0, 0, 50, 50), WLX_WHITE);
                ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_RECT), 1);
            wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// No active clip -> nothing is culled even with the flag on.
TEST(cull_no_clip_keeps_everything) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    wlx_set_cull_offscreen(&ctx, true);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_rect(&ctx, wlx_rect(0, 5000, 50, 50), WLX_WHITE);
    ASSERT_EQ_INT(cull_count_type(&ctx, WLX_CMD_RECT), 1);
    test_frame_end(&ctx);
}

SUITE(offscreen_cull) {
    RUN_TEST(cull_off_by_default_records_both);
    RUN_TEST(cull_on_drops_outside_keeps_inside);
    RUN_TEST(cull_edge_touching_is_dropped);
    RUN_TEST(cull_excludes_text);
    RUN_TEST(cull_nested_clip_intersects);
    RUN_TEST(cull_no_clip_keeps_everything);
}

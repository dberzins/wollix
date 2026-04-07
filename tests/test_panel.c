// test_panel.c - compound panel widget tests (stack balance, nesting, options)

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

// ============================================================================
// Stack balance tests
// ============================================================================

TEST(panel_stack_balance) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Test Panel");
            wlx_label(&ctx, "Item 1", .height = 30);
            wlx_label(&ctx, "Item 2", .height = 30);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

TEST(panel_no_title) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx);
            wlx_label(&ctx, "Item 1", .height = 30);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Composition tests
// ============================================================================

TEST(panel_in_split) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx);
            wlx_panel_begin(&ctx, .title = "Left");
                wlx_label(&ctx, "L1", .height = 30);
            wlx_panel_end(&ctx);
        wlx_split_next(&ctx);
            wlx_panel_begin(&ctx, .title = "Right",
                .title_font_size = 26, .title_height = 48);
                wlx_label(&ctx, "R1", .height = 30);
            wlx_panel_end(&ctx);
        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.scroll_panels.count);

    test_frame_end(&ctx);
}

TEST(panel_nested) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Outer");
            wlx_label(&ctx, "Before", .height = 30);
            wlx_panel_begin(&ctx, .title = "Inner", .capacity = 8);
                wlx_label(&ctx, "Nested", .height = 30);
            wlx_panel_end(&ctx);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Options tests
// ============================================================================

TEST(panel_custom_options) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Custom",
            .title_font_size = 26,
            .title_height = 48,
            .title_align = WLX_LEFT,
            .padding = 4,
            .capacity = 16);
            wlx_label(&ctx, "Item", .height = 30);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

TEST(panel_defaults_resolve) {
    // Verify that default sentinel values resolve correctly
    WLX_Panel_Opt opt = wlx_default_panel_opt();
    ASSERT_TRUE(opt.title == NULL);
    ASSERT_EQ_INT(0, opt.title_font_size);        // sentinel -> resolved to 18 in impl
    ASSERT_EQ_F(0.0f, opt.title_height, 0.001f);  // sentinel -> resolved to 32 in impl
    ASSERT_EQ_F(-1.0f, opt.padding, 0.001f);       // sentinel -> resolved to 2 in impl
    ASSERT_EQ_INT(0, opt.capacity);                 // sentinel -> resolved to 32 in impl
}

// ============================================================================
// Capacity tests
// ============================================================================

TEST(panel_full_capacity) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Full", .capacity = 4);
            // 4 widgets (title uses 1 slot, so 5 total = capacity+1 for title)
            for (int i = 0; i < 4; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Item", .height = 30);
                wlx_pop_id(&ctx);
            }
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

TEST(panel_no_title_full_capacity) {
    // Without title, all capacity slots are available for user widgets
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .capacity = 6);
            for (int i = 0; i < 6; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Item", .height = 30);
                wlx_pop_id(&ctx);
            }
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

TEST(panel_empty_no_children) {
    // Title only, no child widgets - valid edge case
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Empty Panel");
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

TEST(panel_empty_no_title_no_children) {
    // No title, no children - minimal valid panel
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Multi-frame CONTENT measurement stability
// ============================================================================

TEST(panel_content_measurement) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    // Run multiple frames to verify CONTENT measurement converges
    for (int frame = 0; frame < 5; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);

        wlx_layout_begin(&ctx, 1, WLX_VERT);

            wlx_panel_begin(&ctx, .title = "Stable");
                wlx_label(&ctx, "A", .height = 30);
                wlx_label(&ctx, "B", .height = 40);
                wlx_label(&ctx, "C", .height = 20);
            wlx_panel_end(&ctx);

        wlx_layout_end(&ctx);

        ASSERT_EQ_INT(0, (int)ctx.layouts.count);

        test_frame_end(&ctx);
    }
}

// ============================================================================
// Panel in loop with push_id
// ============================================================================

TEST(panel_in_loop_with_push_id) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 3, WLX_VERT);

        for (int i = 0; i < 3; i++) {
            wlx_push_id(&ctx, (size_t)i);
            wlx_panel_begin(&ctx, .title = "Repeated");
                wlx_label(&ctx, "Item", .height = 30);
            wlx_panel_end(&ctx);
            wlx_pop_id(&ctx);
        }

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(panel) {
    RUN_TEST(panel_stack_balance);
    RUN_TEST(panel_no_title);
    RUN_TEST(panel_in_split);
    RUN_TEST(panel_nested);
    RUN_TEST(panel_custom_options);
    RUN_TEST(panel_defaults_resolve);
    RUN_TEST(panel_full_capacity);
    RUN_TEST(panel_no_title_full_capacity);
    RUN_TEST(panel_empty_no_children);
    RUN_TEST(panel_empty_no_title_no_children);
    RUN_TEST(panel_content_measurement);
    RUN_TEST(panel_in_loop_with_push_id);
}

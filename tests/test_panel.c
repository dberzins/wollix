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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

        ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

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

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Per-side padding
// ============================================================================

TEST(panel_perside_defaults_sentinel) {
    WLX_Panel_Opt opt = wlx_default_panel_opt();
    ASSERT_EQ_F(-1.0f, opt.padding_top, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_right, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_bottom, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_left, 0.001f);
}

TEST(panel_perside_top_zero) {
    // .padding = 4, .padding_top = 0 -> content starts flush at top
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);

        wlx_layout_begin(&ctx, 1, WLX_VERT);

            wlx_panel_begin(&ctx,
                .title = "Flush Top",
                .padding = 4,
                .padding_top = 0);
                wlx_label(&ctx, "A", .height = 30);
                wlx_label(&ctx, "B", .height = 40);
            wlx_panel_end(&ctx);

        wlx_layout_end(&ctx);

        ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

        test_frame_end(&ctx);
    }
}

// ============================================================================
// Widget intrinsic height contribution tests
// ============================================================================

TEST(widget_intrinsic_height_contributes_min_h) {
    // A widget with .min_height inside a CONTENT slot contributes at least
    // min_height on frame 1, even when the cell starts at the 1px seed.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    float content_h_f1 = -1;
    float slot_h_f2 = -1;
    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)));
            if (frame == 0) {
                wlx_widget(&ctx, .min_height = 20);
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                content_h_f1 = l->accumulated_content_height;
            } else if (frame == 1) {
                wlx_widget(&ctx, .min_height = 20);
            } else {
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                slot_h_f2 = wlx_layout_offsets(&ctx, l)[1] - wlx_layout_offsets(&ctx, l)[0];
                wlx_widget(&ctx, .min_height = 20);
            }
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(content_h_f1 >= 20.0f);
    ASSERT_TRUE(slot_h_f2 >= 20.0f);
}

TEST(widget_intrinsic_height_label_default) {
    // A label inside a CONTENT slot with no .height and no .min_height
    // contributes at least font_size on frame 1 (min_height defaults to
    // font_size in wlx_resolve_opt_label).
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    float content_h_f1 = -1;
    float slot_h_f2 = -1;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)));
            if (frame == 0) {
                wlx_label(&ctx, "Test");
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                content_h_f1 = l->accumulated_content_height;
            } else if (frame == 2) {
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                slot_h_f2 = wlx_layout_offsets(&ctx, l)[1] - wlx_layout_offsets(&ctx, l)[0];
                wlx_label(&ctx, "Test");
            } else {
                wlx_label(&ctx, "Test");
            }
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    // Theme font_size = 16.
    ASSERT_TRUE(content_h_f1 >= 16.0f);
    ASSERT_TRUE(slot_h_f2 >= 16.0f);
}

TEST(widget_intrinsic_height_explicit_height_dominates) {
    // A label with .height = 50 and .min_height = 20 contributes 50 to
    // CONTENT measurement, not 20 (explicit height overrides min_h).
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    float content_h_f1 = -1;
    float slot_h_f2 = -1;

    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)));
            if (frame == 0) {
                wlx_label(&ctx, "Big", .height = 50, .min_height = 20);
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                content_h_f1 = l->accumulated_content_height;
            } else if (frame == 2) {
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                slot_h_f2 = wlx_layout_offsets(&ctx, l)[1] - wlx_layout_offsets(&ctx, l)[0];
                wlx_label(&ctx, "Big", .height = 50, .min_height = 20);
            } else {
                wlx_label(&ctx, "Big", .height = 50, .min_height = 20);
            }
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_EQ_F(content_h_f1, 50.0f, 1.0f);
    ASSERT_EQ_F(slot_h_f2, 50.0f, 1.0f);
}

TEST(widget_intrinsic_height_received_dominates) {
    // A label with .min_height = 20 in a PX(100) slot contributes 100
    // (the received cell height), not 20.  The cell allocation dominates
    // when it exceeds the widget's declared minimum.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_FLEX(1)));
        wlx_label(&ctx, "Flex", .min_height = 20);
        WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        float contrib = l->accumulated_content_height;
        ASSERT_EQ_F(contrib, 100.0f, 1.0f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Option M regression: padded child layout in linear CONTENT parent
// ============================================================================

TEST(content_padded_child_layout_contributes_padding) {
    // A child layout with padding = 10 (top + bottom = 20) inside a
    // linear CONTENT slot should contribute content + padding to the
    // CONTENT measurement.  Before Option M, only content was counted
    // for linear parents (grid parents already included padding).
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    float slot_h = -1;
    for (int frame = 0; frame < 3; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1)));

            wlx_layout_begin(&ctx, 2, WLX_VERT, .padding = 10);
                wlx_label(&ctx, "A", .height = 30);
                wlx_label(&ctx, "B", .height = 40);
            wlx_layout_end(&ctx);

            if (frame == 2) {
                WLX_Layout *parent = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
                slot_h = wlx_layout_offsets(&ctx, parent)[1] - wlx_layout_offsets(&ctx, parent)[0];
            }
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    // Child content: 30 + 40 = 70.  Child padding: 10 top + 10 bottom = 20.
    // CONTENT slot should resolve to 70 + 20 = 90.
    ASSERT_EQ_F(slot_h, 90.0f, 2.0f);
}

// ============================================================================
// Debug: false clip warning suppression for nested CONTENT bootstrapping
// ============================================================================

#ifdef WLX_DEBUG
static int _panel_debug_warn_count = 0;

static void _panel_debug_warn_cb(const char *file, int line,
                                 const char *msg, void *user_data) {
    (void)file; (void)line; (void)msg; (void)user_data;
    _panel_debug_warn_count++;
}

static void _panel_debug_warn_reset(WLX_Context *ctx) {
    _panel_debug_warn_count = 0;
    wlx_dbg_init(ctx);
    ctx->dbg->warn_cb = _panel_debug_warn_cb;
    ctx->dbg->warn_user_data = NULL;
}

TEST(panel_nested_horz_no_false_clip_warning) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _panel_debug_warn_reset(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    {
        wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Test");
            wlx_layout_begin(&ctx, 3, WLX_HORZ);
                wlx_label(&ctx, "A", .height = 40, .font_size = 16);
                wlx_label(&ctx, "B", .height = 40, .font_size = 16);
                wlx_label(&ctx, "C", .height = 40, .font_size = 16);
            wlx_layout_end(&ctx);
        wlx_panel_end(&ctx);

        wlx_layout_end(&ctx);
    }
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _panel_debug_warn_count);

    _panel_debug_warn_count = 0;
    ctx.dbg->warned_sites_count = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    {
        wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_panel_begin(&ctx, .title = "Test");
            wlx_layout_begin(&ctx, 3, WLX_HORZ);
                wlx_label(&ctx, "A", .height = 40, .font_size = 16);
                wlx_label(&ctx, "B", .height = 40, .font_size = 16);
                wlx_label(&ctx, "C", .height = 40, .font_size = 16);
            wlx_layout_end(&ctx);
        wlx_panel_end(&ctx);

        wlx_layout_end(&ctx);
    }
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _panel_debug_warn_count);
}
#endif

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

    // Widget intrinsic height
    RUN_TEST(widget_intrinsic_height_contributes_min_h);
    RUN_TEST(widget_intrinsic_height_label_default);
    RUN_TEST(widget_intrinsic_height_explicit_height_dominates);
    RUN_TEST(widget_intrinsic_height_received_dominates);

    // Option M regression
    RUN_TEST(content_padded_child_layout_contributes_padding);

    // Per-side padding
    RUN_TEST(panel_perside_defaults_sentinel);
    RUN_TEST(panel_perside_top_zero);

#ifdef WLX_DEBUG
    RUN_TEST(panel_nested_horz_no_false_clip_warning);
#endif
}

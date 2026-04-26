// test_split.c - compound split widget tests (stack balance, nesting, defaults)

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

TEST(split_stack_balance_basic) {
    // A simple split_begin / split_next / split_end cycle should leave
    // all stacks at zero after the frame.
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    // Need a root layout for the split to sit inside
    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(200));

            // Left pane: user creates inner layout
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "Left", .height = 30);
            wlx_layout_end(&ctx);

        wlx_split_next(&ctx);

            // Right pane: user creates inner layout
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "Right", .height = 30);
            wlx_layout_end(&ctx);

        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    // After closing the root layout, stacks should be at zero
    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

TEST(split_stack_balance_no_inner_layout) {
    // Split without user-created inner layouts - widgets go directly
    // into the scroll panel's internal 1-slot layout.
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx);
            wlx_label(&ctx, "Left pane", .height = 30);
        wlx_split_next(&ctx);
            wlx_label(&ctx, "Right pane", .height = 30);
        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

TEST(split_stack_balance_multiple_sequential) {
    // Two sequential splits in the same frame
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 2, WLX_VERT);

        // First split
        wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(250));
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "A-Left", .height = 30);
            wlx_layout_end(&ctx);
        wlx_split_next(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "A-Right", .height = 30);
            wlx_layout_end(&ctx);
        wlx_split_end(&ctx);

        // Second split (different line -> different IDs)
        wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(300));
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "B-Left", .height = 30);
            wlx_layout_end(&ctx);
        wlx_split_next(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "B-Right", .height = 30);
            wlx_layout_end(&ctx);
        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Nested split tests
// ============================================================================

TEST(split_nested) {
    // A split inside the right pane of another split
    WLX_Context ctx;
    test_ctx_init(&ctx, 1200, 800);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(200));

            // Outer left pane
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                wlx_label(&ctx, "Outer left", .height = 30);
            wlx_layout_end(&ctx);

        wlx_split_next(&ctx);

            // Outer right pane contains nested split
            wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(150));

                // Inner left
                wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                    wlx_label(&ctx, "Inner left", .height = 30);
                wlx_layout_end(&ctx);

            wlx_split_next(&ctx);

                // Inner right
                wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
                    wlx_label(&ctx, "Inner right", .height = 30);
                wlx_layout_end(&ctx);

            wlx_split_end(&ctx);  // close inner

        wlx_split_end(&ctx);  // close outer

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

TEST(split_nested_both_panes) {
    // Nested splits in BOTH panes of the outer split
    WLX_Context ctx;
    test_ctx_init(&ctx, 1200, 800);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx);

            // Left pane: nested split
            wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(100));
                wlx_label(&ctx, "LL", .height = 30);
            wlx_split_next(&ctx);
                wlx_label(&ctx, "LR", .height = 30);
            wlx_split_end(&ctx);

        wlx_split_next(&ctx);

            // Right pane: nested split
            wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(100));
                wlx_label(&ctx, "RL", .height = 30);
            wlx_split_next(&ctx);
                wlx_label(&ctx, "RR", .height = 30);
            wlx_split_end(&ctx);

        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Default option tests
// ============================================================================

TEST(split_defaults_resolve) {
    // Verify that the default options produce a valid layout:
    // - first_size defaults to PX(280)
    // - second_size defaults to FLEX(1)
    // - fill_size defaults to FILL
    // - padding defaults to 4
    WLX_Split_Opt opt = wlx_default_split_opt();

    // All sentinel zeros - will be resolved at runtime
    ASSERT_TRUE(wlx_slot_size_is_zero(opt.first_size));
    ASSERT_TRUE(wlx_slot_size_is_zero(opt.second_size));
    ASSERT_TRUE(wlx_slot_size_is_zero(opt.fill_size));
    ASSERT_EQ_F(-1.0f, opt.padding, 0.001f);  // -1 sentinel

    // back_colors default to zero (theme)
    ASSERT_TRUE(wlx_color_is_zero(opt.first_back_color));
    ASSERT_TRUE(wlx_color_is_zero(opt.second_back_color));
}

TEST(split_custom_options) {
    // Custom options should be preserved through the macro
    WLX_Split_Opt opt = wlx_default_split_opt(
        .first_size = WLX_SLOT_PX(300),
        .second_size = WLX_SLOT_FLEX(2),
        .padding = 8.0f,
        .first_back_color = (WLX_Color){255, 0, 0, 255}
    );

    ASSERT_EQ_F(300.0f, opt.first_size.value, 0.001f);
    ASSERT_EQ_INT(WLX_SIZE_PIXELS, (int)opt.first_size.kind);
    ASSERT_EQ_F(2.0f, opt.second_size.value, 0.001f);
    ASSERT_EQ_INT(WLX_SIZE_FLEX, (int)opt.second_size.kind);
    ASSERT_EQ_F(8.0f, opt.padding, 0.001f);
    ASSERT_EQ_INT(255, (int)opt.first_back_color.r);
}

TEST(split_fill_min_option) {
    // Test the fill_size override (e.g. FILL_MIN(400) as used by section_theming)
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx,
            .fill_size = WLX_SLOT_FILL_MIN(400),
            .first_size = WLX_SLOT_PX(300));

            wlx_label(&ctx, "Left", .height = 30);

        wlx_split_next(&ctx);

            wlx_label(&ctx, "Right", .height = 30);

        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Multi-frame persistence test
// ============================================================================

TEST(split_scroll_persists_across_frames) {
    // Verify that scroll state inside split panes persists across frames
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    // Frame 1: create the split, put widgets in it
    test_frame_begin(&ctx, 400, 300, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_split_begin(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Item", .height = 30);
                wlx_pop_id(&ctx);
            }
            wlx_layout_end(&ctx);
        wlx_split_next(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Content", .height = 30);
                wlx_pop_id(&ctx);
            }
            wlx_layout_end(&ctx);
        wlx_split_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: same layout - should not crash and stacks should balance
    test_frame_begin(&ctx, 400, 300, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_split_begin(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Item", .height = 30);
                wlx_pop_id(&ctx);
            }
            wlx_layout_end(&ctx);
        wlx_split_next(&ctx);
            wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
            for (int i = 0; i < 5; i++) {
                wlx_push_id(&ctx, (size_t)i);
                wlx_label(&ctx, "Content", .height = 30);
                wlx_pop_id(&ctx);
            }
            wlx_layout_end(&ctx);
        wlx_split_end(&ctx);
    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Split with push_id in a loop
// ============================================================================

TEST(split_in_loop_with_push_id) {
    // Multiple splits created in a loop inside a dynamic (auto) layout.
    // This was the original SPLIT_ISSUE.md failure: split_begin creates
    // nested static layouts whose slot_size_offsets allocations used to
    // corrupt the dynamic parent's contiguous region.  With dyn_offsets
    // on a separate sub-arena, the two no longer interfere.
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);

    for (int i = 0; i < 3; i++) {
        wlx_push_id(&ctx, (size_t)i);
        wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(180));

        wlx_split_begin(&ctx, .first_size = WLX_SLOT_PX(100));
            wlx_label(&ctx, "L", .height = 30);
        wlx_split_next(&ctx);
            wlx_label(&ctx, "R", .height = 30);
        wlx_split_end(&ctx);

        wlx_pop_id(&ctx);
    }

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Split with second pane back_color override
// ============================================================================

TEST(split_next_back_color_override) {
    // Verify wlx_split_next with .back_color doesn't crash
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx,
            .first_back_color = (WLX_Color){30, 30, 50, 255});

            wlx_label(&ctx, "Options", .height = 30);

        wlx_split_next(&ctx,
            .back_color = (WLX_Color){20, 20, 30, 255});

            wlx_label(&ctx, "Content", .height = 30);

        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Zero-padding test
// ============================================================================

TEST(split_zero_padding) {
    // Explicit padding = 0 should work (overrides the -1 sentinel default)
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx, .padding = 0.0f);
            wlx_label(&ctx, "Left", .height = 30);
        wlx_split_next(&ctx);
            wlx_label(&ctx, "Right", .height = 30);
        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Per-side padding
// ============================================================================

TEST(split_perside_defaults_sentinel) {
    // Default per-side values are -1 (sentinel)
    WLX_Split_Opt opt = wlx_default_split_opt();
    ASSERT_EQ_F(-1.0f, opt.padding_top, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_right, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_bottom, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.padding_left, 0.001f);
}

TEST(split_perside_top_zero) {
    // .padding_top = 0 while uniform defaults to 4 -> top flush
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx, .padding_top = 0);
            wlx_label(&ctx, "Left", .height = 30);
        wlx_split_next(&ctx);
            wlx_label(&ctx, "Right", .height = 30);
        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);

    test_frame_end(&ctx);
}

// ============================================================================
// Split gap
// ============================================================================

TEST(split_gap_basic) {
    // Split with gap=4, default first_size=PX(280), second=FLEX(1).
    // The internal HORZ layout uses gap for the 2-slot layout.
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);

        wlx_split_begin(&ctx, .gap = 4);

            // After split_begin: stack is [outer_vert, outer_vert_slot,
            // horz_split, scroll_panel_vert].
            // The HORZ split layout is at count-2 (under the scroll panel's vert).
            // Actually: outer VERT, HORZ(2 slots), scroll panel VERT.
            // Find the HORZ layout with 2 slots.
            bool found = false;
            for (size_t i = 0; i < ctx.arena.layouts.count; i++) {
                WLX_Layout *l = &wlx_pool_layouts(&ctx)[i];
                if (l->kind == WLX_LAYOUT_LINEAR && l->linear.orient == WLX_HORZ
                    && l->count == 2) {
                    ASSERT_EQ_F(l->gap, 4.0f, 0.01f);
                    found = true;
                    break;
                }
            }
            ASSERT_TRUE(found);

            wlx_label(&ctx, "Left", .height = 30);

        wlx_split_next(&ctx);

            wlx_label(&ctx, "Right", .height = 30);

        wlx_split_end(&ctx);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(split) {
    RUN_TEST(split_stack_balance_basic);
    RUN_TEST(split_stack_balance_no_inner_layout);
    RUN_TEST(split_stack_balance_multiple_sequential);
    RUN_TEST(split_nested);
    RUN_TEST(split_nested_both_panes);
    RUN_TEST(split_defaults_resolve);
    RUN_TEST(split_custom_options);
    RUN_TEST(split_fill_min_option);
    RUN_TEST(split_scroll_persists_across_frames);
    RUN_TEST(split_in_loop_with_push_id);
    RUN_TEST(split_next_back_color_override);
    RUN_TEST(split_zero_padding);

    // Per-side padding
    RUN_TEST(split_perside_defaults_sentinel);
    RUN_TEST(split_perside_top_zero);

    // Gap
    RUN_TEST(split_gap_basic);
}

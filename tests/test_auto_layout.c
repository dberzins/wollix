// test_auto_layout.c - tests for wlx_layout_auto_slot with mixed WLX_Slot_Size types
// Included from test_main.c (single TU build).

#define EPS_AL 0.5f

// Helper: read the size of slot `i` from the dynamic layout's offset array.
static float auto_slot_size(WLX_Context *ctx, WLX_Layout *l, size_t i) {
    float *off = wlx_layout_offsets(ctx, l); return off[i + 1] - off[i];
}

// ============================================================================
// wlx_layout_auto_slot - PX
// ============================================================================

TEST(auto_slot_px_basic) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(100));
    wlx_label(&ctx, "A", .height = 100);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 100.0f, EPS_AL);

    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(50));
    wlx_label(&ctx, "B", .height = 50);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 50.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - PCT
// ============================================================================

TEST(auto_slot_pct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // VERT layout: total = 600px
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // 25% of 600 = 150
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PCT(25));
    wlx_label(&ctx, "A", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 150.0f, EPS_AL);

    // 50% of 600 = 300
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PCT(50));
    wlx_label(&ctx, "B", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 300.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - FLEX (greedy)
// ============================================================================

TEST(auto_slot_flex_single) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // VERT layout: total = 600px
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // PX 100
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(100));
    wlx_label(&ctx, "Header", .height = 100);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 100.0f, EPS_AL);

    // PX 50
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(50));
    wlx_label(&ctx, "Spacer", .height = 50);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 50.0f, EPS_AL);

    // FLEX(1): should take remaining 600 - 100 - 50 = 450
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX(1));
    wlx_label(&ctx, "Body", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 2), 450.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(auto_slot_flex_greedy_takes_all) {
    // If FLEX appears in the middle, it greedily takes all remaining space.
    // A subsequent PX slot still gets its full size (it extends beyond total).
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // PX 100
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(100));
    wlx_label(&ctx, "A", .height = 100);

    // FLEX at index 1: takes remaining 600-100=500 (greedy)
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX(1));
    wlx_label(&ctx, "B", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 500.0f, EPS_AL);

    // PX 50 after FLEX: still resolves to 50px (extends past layout total)
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(50));
    wlx_label(&ctx, "C", .height = 50);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 2), 50.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - FILL
// ============================================================================

TEST(auto_slot_fill) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // No scroll panel -> viewport = root rect = 600 (VERT)
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // FILL = 1.0 * viewport = 600
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FILL);
    wlx_label(&ctx, "Full viewport", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 600.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(auto_slot_fill_pct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // FILL_PCT(50) = 0.5 * 600 = 300
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FILL_PCT(50));
    wlx_label(&ctx, "Half viewport", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 300.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - HORZ orientation
// ============================================================================

TEST(auto_slot_horz_pct) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // HORZ layout: total = 800px
    wlx_layout_begin_auto(&ctx, WLX_HORZ, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // 30% of 800 = 240
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PCT(30));
    wlx_label(&ctx, "Sidebar", .width = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 240.0f, EPS_AL);

    // 70% of 800 = 560
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PCT(70));
    wlx_label(&ctx, "Content", .width = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 560.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - min/max constraints
// ============================================================================

TEST(auto_slot_flex_with_min) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // VERT layout: total = 600
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Consume 590px with PX
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(590));
    wlx_label(&ctx, "Big", .height = 590);

    // FLEX with min 50: remaining is 10, but min clamps to 50
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX_MIN(1, 50));
    wlx_label(&ctx, "Footer", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 50.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(auto_slot_flex_with_max) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // VERT layout: total = 600
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // PX 100
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(100));
    wlx_label(&ctx, "Header", .height = 100);

    // FLEX with max 200: remaining is 500, but max clamps to 200
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX_MAX(1, 200));
    wlx_label(&ctx, "Body", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 200.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(auto_slot_pct_with_minmax) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // VERT: total = 600. PCT(5) = 30, but min 50 -> clamped to 50
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_layout_auto_slot(&ctx, WLX_SLOT_PCT_MINMAX(5, 50, 400));
    wlx_label(&ctx, "X", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 50.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot - mixed PX + FLEX pattern
// ============================================================================

TEST(auto_slot_mixed_header_body_footer) {
    // Common pattern: PX header, FLEX body, PX footer
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // Header 44px
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(44));
    wlx_label(&ctx, "Header", .height = 44);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 44.0f, EPS_AL);

    // Body: 600 - 44 - 24 = 532px remaining when FLEX is declared.
    // BUT FLEX is greedy: it sees used=44, takes 600-44=556.
    // The footer PX(24) will then extend past total.
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX(1));
    wlx_label(&ctx, "Body", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 556.0f, EPS_AL);

    // Footer 24px
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(24));
    wlx_label(&ctx, "Footer", .height = 24);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 2), 24.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

TEST(auto_slot_px_then_flex_end) {
    // Most practical pattern: N x PX items, then FLEX at end
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    // 5 x 40px = 200px consumed
    for (int i = 0; i < 5; i++) {
        wlx_push_id(&ctx, (size_t)i);
        wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(40));
        wlx_label(&ctx, "Item", .height = 40);
        wlx_pop_id(&ctx);
    }

    // FLEX takes remaining: 600 - 200 = 400
    wlx_layout_auto_slot(&ctx, WLX_SLOT_FLEX(1));
    wlx_label(&ctx, "Fill", .height = -1);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 5), 400.0f, EPS_AL);

    // Verify total count
    ASSERT_EQ_INT((int)l->count, 6);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// wlx_layout_auto_slot_px still works as wrapper
// ============================================================================

TEST(auto_slot_px_wrapper_compat) {
    // Ensure wlx_layout_auto_slot_px still works identically
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_layout_auto_slot_px(&ctx, 75);
    wlx_label(&ctx, "A", .height = 75);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 0), 75.0f, EPS_AL);

    wlx_layout_auto_slot_px(&ctx, 125);
    wlx_label(&ctx, "B", .height = 125);
    ASSERT_EQ_F(auto_slot_size(&ctx, l, 1), 125.0f, EPS_AL);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Auto layout with gap
// ============================================================================

TEST(gap_auto_layout) {
    // VERT auto layout with gap=4, 4 children of 30px each.
    // Each slot appended dynamically should include gap spacing.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_begin_auto(&ctx, WLX_VERT, 30, .gap = 4);
    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    ASSERT_EQ_F(l->gap, 4.0f, EPS_AL);

    wlx_label(&ctx, "A", .height = 30);
    wlx_label(&ctx, "B", .height = 30);
    wlx_label(&ctx, "C", .height = 30);
    wlx_label(&ctx, "D", .height = 30);

    // 4 slots dynamically appended with gap=4.
    // Dynamic append adds gap as leading space on non-first slots:
    // Slot 0: off[0]=0, off[1]=0+30=30
    // Slot 1: off[2]=30+30+4=64  (leading gap)
    // Slot 2: off[3]=64+30+4=98
    // Slot 3: off[4]=98+30+4=132
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[0],   0.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[1],  30.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[2],  64.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[3],  98.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[4], 132.0f, EPS_AL);

    wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(auto_layout) {
    // PX
    RUN_TEST(auto_slot_px_basic);

    // PCT
    RUN_TEST(auto_slot_pct);

    // FLEX
    RUN_TEST(auto_slot_flex_single);
    RUN_TEST(auto_slot_flex_greedy_takes_all);

    // FILL
    RUN_TEST(auto_slot_fill);
    RUN_TEST(auto_slot_fill_pct);

    // HORZ
    RUN_TEST(auto_slot_horz_pct);

    // min/max constraints
    RUN_TEST(auto_slot_flex_with_min);
    RUN_TEST(auto_slot_flex_with_max);
    RUN_TEST(auto_slot_pct_with_minmax);

    // Mixed patterns
    RUN_TEST(auto_slot_mixed_header_body_footer);
    RUN_TEST(auto_slot_px_then_flex_end);

    // Backward compat
    RUN_TEST(auto_slot_px_wrapper_compat);

    // Gap
    RUN_TEST(gap_auto_layout);
}

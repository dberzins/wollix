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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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

    // 4 slots dynamically appended with gap=4. As in a fixed layout,
    // off[i] is where slot i starts: appending a slot after the first
    // moves the boundary in front of it past the gap.
    // Slot 0: off[0]=0, off[1]=30
    // Slot 1: off[1]=30+4=34, off[2]=34+30=64
    // Slot 2: off[2]=64+4=68, off[3]=98
    // Slot 3: off[3]=98+4=102, off[4]=132 (the extent, no trailing gap)
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[0],   0.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[1],  34.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[2],  68.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[3], 102.0f, EPS_AL);
    ASSERT_EQ_F(wlx_layout_offsets(&ctx, l)[4], 132.0f, EPS_AL);

    wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Auto layout gap: between the slots, as in a fixed layout
// ============================================================================

static WLX_Rect al_rects[16];
static int      al_rect_count;

static void al_rec_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)c; (void)user;
    if (al_rect_count < 16) al_rects[al_rect_count++] = r;
}

// Four children, 30 units each with gap 4, in an auto VERT layout or a
// one-column auto grid. A filled child is a one-slot layout, so its rect
// is the slot's; otherwise it is a label of height 30 with a background,
// whose recorded rect is where the widget lands. Returns how many rects
// were read.
static int al_gap_children(bool grid, bool fill, WLX_Rect out[4]) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 200, 400);
    ctx.backend.draw_rect = al_rec_rect;
    al_rect_count = 0;
    test_frame_begin(&ctx, 0, 0, false, false);
    if (grid) wlx_grid_begin_auto(&ctx, 1, 30, .gap = 4);
    else      wlx_layout_begin_auto(&ctx, WLX_VERT, 30, .gap = 4);
    for (int i = 0; i < 4; i++) {
        wlx_push_id(&ctx, (size_t)i);
        if (fill) {
            wlx_layout_begin(&ctx, 1, WLX_VERT);
            out[i] = wlx_get_parent_rect(&ctx);
            wlx_layout_end(&ctx);
        } else {
            wlx_label(&ctx, "x", .height = 30, .show_background = true,
                .back_color = (WLX_Color){ 200, 0, 0, 255 });
        }
        wlx_pop_id(&ctx);
    }
    if (grid) wlx_grid_end(&ctx);
    else      wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
    if (fill) return 4;
    for (int i = 0; i < 4 && i < al_rect_count; i++) out[i] = al_rects[i];
    return al_rect_count;
}

// The gap sits between adjacent slots in both auto kinds: a filled child
// does not touch the one before it, and a fixed-height child is not
// centred in a slot that carries the gap.
TEST(auto_layout_gap_between_slots) {
    for (int k = 0; k < 4; k++) {
        bool grid = (k & 1) != 0, fill = (k & 2) != 0;
        WLX_Rect r[4];
        ASSERT_EQ_INT(al_gap_children(grid, fill, r), 4);
        for (int i = 0; i < 4; i++) {
            ASSERT_EQ_F(r[i].y, 34.0f * (float)i, 0.0f);
            ASSERT_EQ_F(r[i].h, 30.0f, 0.0f);
        }
    }
}

// A FLEX slot takes the space left after the gap in front of it, so it
// ends at the layout's edge.
TEST(auto_layout_flex_remainder_counts_gap) {
    WLX_Context ctx;
    WLX_Rect r[2];
    test_ctx_init(&ctx, 200, 100);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0, .gap = 4);
    for (int i = 0; i < 2; i++) {
        wlx_layout_auto_slot(&ctx, i == 0 ? WLX_SLOT_PX(30) : WLX_SLOT_FLEX(1));
        wlx_push_id(&ctx, (size_t)i);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        r[i] = wlx_get_parent_rect(&ctx);
        wlx_layout_end(&ctx);
        wlx_pop_id(&ctx);
    }
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_F(r[0].y, 0.0f, 0.0f);
    ASSERT_EQ_F(r[0].h, 30.0f, 0.0f);
    ASSERT_EQ_F(r[1].y, 34.0f, 0.0f);
    ASSERT_EQ_F(r[1].y + r[1].h, 100.0f, 0.0f);
    wlx_context_destroy(&ctx);
}

// An auto layout with fixed slot sizes yields the slot rects of the fixed
// layout with the same slots and gap, in both orientations.
TEST(auto_layout_matches_fixed_layout) {
    static const float sizes[4] = { 30.0f, 50.0f, 20.0f, 40.0f };
    for (int o = 0; o < 2; o++) {
        WLX_Orient orient = (o == 0) ? WLX_VERT : WLX_HORZ;
        WLX_Rect got[2][4];
        for (int fixed = 0; fixed < 2; fixed++) {
            WLX_Context ctx;
            test_ctx_init(&ctx, 400, 400);
            test_frame_begin(&ctx, 0, 0, false, false);
            if (fixed) {
                wlx_layout_begin(&ctx, 4, orient, .gap = 6,
                    .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(30), WLX_SLOT_PX(50),
                                                WLX_SLOT_PX(20), WLX_SLOT_PX(40) });
            } else {
                wlx_layout_begin_auto(&ctx, orient, 0, .gap = 6);
            }
            for (int i = 0; i < 4; i++) {
                if (!fixed) wlx_layout_auto_slot_px(&ctx, sizes[i]);
                wlx_push_id(&ctx, (size_t)i);
                wlx_layout_begin(&ctx, 1, WLX_VERT);
                got[fixed][i] = wlx_get_parent_rect(&ctx);
                wlx_layout_end(&ctx);
                wlx_pop_id(&ctx);
            }
            wlx_layout_end(&ctx);
            test_frame_end(&ctx);
            wlx_context_destroy(&ctx);
        }
        for (int i = 0; i < 4; i++) {
            ASSERT_EQ_F(got[0][i].x, got[1][i].x, 0.0f);
            ASSERT_EQ_F(got[0][i].y, got[1][i].y, 0.0f);
            ASSERT_EQ_F(got[0][i].w, got[1][i].w, 0.0f);
            ASSERT_EQ_F(got[0][i].h, got[1][i].h, 0.0f);
        }
    }
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
    RUN_TEST(auto_layout_gap_between_slots);
    RUN_TEST(auto_layout_flex_remainder_counts_gap);
    RUN_TEST(auto_layout_matches_fixed_layout);
}

// test_dyn_offsets.c - dyn_offsets arena isolation tests
// Validates that dynamic layout offsets (dyn_offsets) and static layout
// offsets (slot_size_offsets) live on separate sub-arenas and do not alias.

// ============================================================================
// Static layout nested inside a dynamic linear layout
// ============================================================================

TEST(dyn_offsets_static_inside_dynamic_linear) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);

    // Slot 0: contains a static nested layout
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(200));
    wlx_layout_begin(&ctx, 2, WLX_HORZ);
        wlx_label(&ctx, "A", .height = 30);
        wlx_label(&ctx, "B", .height = 30);
    wlx_layout_end(&ctx);

    // Slot 1: appended after the static nested layout closes.
    // Before dyn_offsets, this would trip the contiguity assert because
    // the static layout's scratch_alloc bumped slot_size_offsets.count.
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(150));
    wlx_label(&ctx, "C", .height = 30);

    // Slot 2: one more to confirm continued appending works
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(100));
    wlx_label(&ctx, "D", .height = 30);

    WLX_Layout *l = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    ASSERT_EQ_INT(3, (int)l->count);

    float *off = wlx_layout_offsets(&ctx, l);
    ASSERT_EQ_F(off[0],   0.0f, 0.5f);
    ASSERT_EQ_F(off[1], 200.0f, 0.5f);
    ASSERT_EQ_F(off[2], 350.0f, 0.5f);
    ASSERT_EQ_F(off[3], 450.0f, 0.5f);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
}

// ============================================================================
// Static layout nested inside a dynamic grid cell
// ============================================================================

TEST(dyn_offsets_static_inside_dynamic_grid) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin_auto(&ctx, 2, 50);

    // Row 0: cell (0,0) contains a static nested layout
    wlx_layout_begin(&ctx, 2, WLX_VERT);
        wlx_label(&ctx, "X", .height = 20);
        wlx_label(&ctx, "Y", .height = 20);
    wlx_layout_end(&ctx);
    // cell (0,1)
    wlx_label(&ctx, "Z", .height = 30);

    // Row 1: appended after the static nested layout in row 0.
    // The dynamic grid's row_offsets live on dyn_offsets, so the static
    // layout's slot_size_offsets allocations do not interfere.
    wlx_label(&ctx, "W1", .height = 30);
    wlx_label(&ctx, "W2", .height = 30);

    wlx_grid_end(&ctx);

    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

    test_frame_end(&ctx);
}

// ============================================================================
// slot_size_offsets and dyn_offsets do not alias
// ============================================================================

TEST(dyn_offsets_no_aliasing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    test_frame_begin(&ctx, 0, 0, false, false);

    // Create a static layout (allocates from slot_size_offsets)
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    WLX_Layout *stat = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    float *stat_off = wlx_layout_offsets(&ctx, stat);

    // Create a dynamic layout inside the first static slot
    // (allocates from dyn_offsets)
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0);
    WLX_Layout *dyn = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(77));
    wlx_label(&ctx, "sentinel", .height = 77);
    float *dyn_off = wlx_layout_offsets(&ctx, dyn);

    // The dynamic layout wrote 77 at offset[1].
    // The static layout's offsets must not have been affected.
    ASSERT_EQ_F(dyn_off[1], 77.0f, 0.5f);

    // Static offsets are computed from the full 600px height split into 2
    float expected_static = 300.0f;
    ASSERT_EQ_F(stat_off[1], expected_static, 0.5f);

    // Verify they point into different buffers
    ASSERT_TRUE(stat_off != dyn_off);

    // The base pointers of the two sub-arenas themselves must differ
    ASSERT_TRUE(ctx.arena.slot_size_offsets.items != ctx.arena.dyn_offsets.items);

    wlx_layout_end(&ctx);  // close dynamic
    wlx_layout_end(&ctx);  // close static slot 0's content; consume slot 1
    // (test_frame_end drains remaining layouts)
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(dyn_offsets) {
    RUN_TEST(dyn_offsets_static_inside_dynamic_linear);
    RUN_TEST(dyn_offsets_static_inside_dynamic_grid);
    RUN_TEST(dyn_offsets_no_aliasing);
}

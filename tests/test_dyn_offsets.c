// test_dyn_offsets.c - dyn_offsets arena isolation tests
// Validates that dynamic layout offsets (dyn_offsets) and static layout
// offsets (slot_size_offsets) live on separate sub-arenas and do not alias.

static int dyn_err_count;

static void dyn_err_capture(const WLX_Error *e, void *user) {
    (void)e; (void)user;
    dyn_err_count++;
}

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

    // Slot 1: appended after the static nested layout closes. The static
    // layout's offsets live on slot_size_offsets, so this region is intact.
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
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
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Dynamic layouts nested inside a dynamic linear layout
// ============================================================================

TEST(dyn_offsets_dynamic_inside_dynamic_linear) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    wlx_set_error_handler(&ctx, dyn_err_capture, NULL);
    dyn_err_count = 0;
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 30);
    WLX_Layout *outer = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_label(&ctx, "a", .height = 30);

    // Slot 1: a dynamic child grows the shared offset buffer past the
    // outer layout's tail, then closes.
    wlx_layout_begin_auto(&ctx, WLX_VERT, 10);
    ASSERT_EQ_F(wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1].rect.y, 30.0f, 0.5f);
        wlx_label(&ctx, "b1", .height = 10);
        wlx_label(&ctx, "b2", .height = 10);
    wlx_layout_end(&ctx);

    // Slot 2: the outer layout takes its next slot past the child's region.
    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(60));
    wlx_grid_begin_auto(&ctx, 2, 20);
    ASSERT_EQ_F(wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1].rect.y, 60.0f, 0.5f);
        wlx_label(&ctx, "c1", .height = 20);
        wlx_label(&ctx, "c2", .height = 20);
        wlx_label(&ctx, "c3", .height = 20);
    wlx_grid_end(&ctx);

    wlx_layout_auto_slot(&ctx, WLX_SLOT_PX(60));
    wlx_label(&ctx, "d", .height = 60);

    ASSERT_EQ_INT(4, (int)outer->count);
    float *off = wlx_layout_offsets(&ctx, outer);
    ASSERT_EQ_F(off[0],   0.0f, 0.5f);
    ASSERT_EQ_F(off[1],  30.0f, 0.5f);
    ASSERT_EQ_F(off[2],  60.0f, 0.5f);
    ASSERT_EQ_F(off[3], 120.0f, 0.5f);
    ASSERT_EQ_F(off[4], 180.0f, 0.5f);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(0, dyn_err_count);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// A low-level auto layout keeps taking slots after a dynamic child closes
// ============================================================================

TEST(dyn_offsets_low_level_after_closed_dynamic_child) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    wlx_set_error_handler(&ctx, dyn_err_capture, NULL);
    dyn_err_count = 0;
    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Layout outer = wlx_create_layout_auto(&ctx, (WLX_Rect){0, 0, 200, 600}, WLX_VERT, 20);
    WLX_Rect r0 = wlx_get_slot_rect(&ctx, &outer, -1, 1);

    // Begun and ended after the low-level layout was created: its offsets
    // sit past the low-level layout's tail and are dead once it closes.
    wlx_layout_begin_auto(&ctx, WLX_VERT, 10);
        wlx_label(&ctx, "x", .height = 10);
    wlx_layout_end(&ctx);

    WLX_Rect r1 = wlx_get_slot_rect(&ctx, &outer, -1, 1);
    ASSERT_EQ_F(r0.y,  0.0f, 0.5f);
    ASSERT_EQ_F(r1.y, 20.0f, 0.5f);
    ASSERT_EQ_F(r1.h, 20.0f, 0.5f);
    ASSERT_EQ_INT(2, (int)outer.count);

    test_frame_end(&ctx);
    ASSERT_EQ_INT(0, dyn_err_count);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(dyn_offsets) {
    RUN_TEST(dyn_offsets_static_inside_dynamic_linear);
    RUN_TEST(dyn_offsets_static_inside_dynamic_grid);
    RUN_TEST(dyn_offsets_no_aliasing);
    RUN_TEST(dyn_offsets_dynamic_inside_dynamic_linear);
    RUN_TEST(dyn_offsets_low_level_after_closed_dynamic_child);
}

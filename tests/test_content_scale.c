// test_content_scale.c - the content scale: the optional backend callback,
// the once-per-frame sample and its contract, the device-grid slot snap,
// the geometry-generation bump on a change, and the accessor.
//
// The runner defines WLX_DEBUG, so a contract error with no handler installed
// aborts; the cases that provoke one install the capturing handler first.

static WLX_Error cs_last;
static int       cs_count;

static void cs_capture(const WLX_Error *e, void *user) {
    (void)user;
    cs_last = *e;
    cs_count++;
}

static float cs_scale_value = 1.0f;
static int   cs_scale_calls = 0;

static float cs_get_scale(void *user) {
    (void)user;
    cs_scale_calls++;
    return cs_scale_value;
}

static void cs_init(WLX_Context *ctx, bool with_callback) {
    test_ctx_init(ctx, 100, 50);
    if (with_callback) ctx->backend.get_content_scale = cs_get_scale;
    wlx_set_error_handler(ctx, cs_capture, NULL);
    memset(&cs_last, 0, sizeof cs_last);
    cs_count = 0;
    cs_scale_calls = 0;
    cs_scale_value = 1.0f;
}

// Three equal slots across the 100-unit root row, read back as the x of
// each slot through a nested layout.
static void cs_three_slots(WLX_Context *ctx, float out_x[3]) {
    wlx_layout_begin(ctx, 3, WLX_HORZ);
    for (int i = 0; i < 3; i++) {
        wlx_layout_begin(ctx, 1, WLX_VERT);
        out_x[i] = wlx_get_parent_rect(ctx).x;
        wlx_layout_end(ctx);
    }
    wlx_layout_end(ctx);
}

TEST(content_scale_null_callback_is_one) {
    WLX_Context ctx;
    cs_init(&ctx, false);
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_content_scale(&ctx), 1.0f, 0.0f);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(0, wlx_error_count(&ctx));
    ASSERT_EQ_INT(0, (int)ctx.text_env_generation);   // a first frame at 1.0 is not a change
    wlx_context_destroy(&ctx);
}

TEST(content_scale_before_first_frame_is_one) {
    WLX_Context ctx;
    memset(&ctx, 0, sizeof ctx);
    ASSERT_EQ_F(wlx_content_scale(&ctx), 1.0f, 0.0f);
}

TEST(content_scale_is_sampled_once_per_frame) {
    WLX_Context ctx;
    cs_init(&ctx, true);
    cs_scale_value = 2.0f;
    test_frame_begin(&ctx, 0, 0, false, false);
    cs_three_slots(&ctx, (float[3]){0});
    test_frame_end(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(2, cs_scale_calls);
    ASSERT_EQ_F(wlx_content_scale(&ctx), 2.0f, 0.0f);
    wlx_context_destroy(&ctx);
}

// 100 / 3 = 33.33.. per slot. On the unit grid the boundaries land on 33
// and 67; on the 2x device grid on 33.5 and 66.5 (67 and 133 device
// pixels). Exact comparisons: the snap is arithmetic, not approximation.
TEST(content_scale_snaps_to_device_grid_2x) {
    WLX_Context ctx;
    float at_1x[3], at_2x[3];
    cs_init(&ctx, true);

    cs_scale_value = 1.0f;
    test_frame_begin(&ctx, 0, 0, false, false);
    cs_three_slots(&ctx, at_1x);
    test_frame_end(&ctx);
    ASSERT_EQ_F(at_1x[0], 0.0f, 0.0f);
    ASSERT_EQ_F(at_1x[1], 33.0f, 0.0f);
    ASSERT_EQ_F(at_1x[2], 67.0f, 0.0f);

    cs_scale_value = 2.0f;
    test_frame_begin(&ctx, 0, 0, false, false);
    cs_three_slots(&ctx, at_2x);
    test_frame_end(&ctx);
    ASSERT_EQ_F(at_2x[0], 0.0f, 0.0f);
    ASSERT_EQ_F(at_2x[1], 33.5f, 0.0f);
    ASSERT_EQ_F(at_2x[2], 66.5f, 0.0f);
    ASSERT_EQ_INT(0, wlx_error_count(&ctx));
    wlx_context_destroy(&ctx);
}

// At 1.5x every boundary times 1.5 is a whole device pixel (50, 100, 150),
// which no unit-grid snap (33 or 34) could give.
TEST(content_scale_snaps_to_device_grid_1_5x) {
    WLX_Context ctx;
    float off[4];
    cs_init(&ctx, true);
    cs_scale_value = 1.5f;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_compute_offsets_ctx(&ctx, off, 3, 100.0f, 100.0f, NULL, 0.0f);
    test_frame_end(&ctx);
    for (int i = 0; i <= 3; i++) {
        float px = off[i] * 1.5f;
        ASSERT_EQ_F(px, floorf(px + 0.5f), 1e-4f);
    }
    ASSERT_EQ_F(off[0], 0.0f, 0.0f);
    ASSERT_EQ_F(off[1] * 1.5f, 50.0f, 1e-4f);
    ASSERT_EQ_F(off[2] * 1.5f, 100.0f, 1e-4f);
    ASSERT_EQ_F(off[3], 100.0f, 0.0f);
    wlx_context_destroy(&ctx);
}

// Where each of three children ends, in units: an auto VERT layout
// (grid = false) or a one-column auto grid (grid = true), slot or row size
// `size`, no gap, at content scale `scale`.
static void cs_auto_ends(bool grid, float scale, float size, float out_end[3]) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 200, 400);
    ctx.backend.get_content_scale = cs_get_scale;
    cs_scale_value = scale;
    test_frame_begin(&ctx, 0, 0, false, false);
    if (grid) wlx_grid_begin_auto(&ctx, 1, size);
    else      wlx_layout_begin_auto(&ctx, WLX_VERT, size);
    for (int i = 0; i < 3; i++) {
        wlx_push_id(&ctx, (size_t)i);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        WLX_Rect r = wlx_get_parent_rect(&ctx);
        out_end[i] = r.y + r.h;
        wlx_layout_end(&ctx);
        wlx_pop_id(&ctx);
    }
    if (grid) wlx_grid_end(&ctx);
    else      wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
    cs_scale_value = 1.0f;
}

// Auto-grid rows snap to the device grid as every other slot boundary
// does: at 1.5x a 31-unit row (46.5 device px) ends on whole pixels, where
// the auto VERT layout's slots end.
TEST(content_scale_auto_grid_rows_snap) {
    float grid[3], vert[3];
    cs_auto_ends(true, 1.5f, 31.0f, grid);
    cs_auto_ends(false, 1.5f, 31.0f, vert);
    ASSERT_EQ_F(grid[0] * 1.5f, 47.0f, 1e-3f);
    ASSERT_EQ_F(grid[1] * 1.5f, 94.0f, 1e-3f);
    ASSERT_EQ_F(grid[2] * 1.5f, 141.0f, 1e-3f);
    for (int i = 0; i < 3; i++) ASSERT_EQ_F(grid[i], vert[i], 0.0f);
}

// At 1.0 the device grid is the unit grid: a fractional row size lands on
// whole units, as in an auto VERT layout.
TEST(content_scale_auto_grid_fractional_rows_at_one) {
    float grid[3], vert[3];
    cs_auto_ends(true, 1.0f, 10.4f, grid);
    cs_auto_ends(false, 1.0f, 10.4f, vert);
    ASSERT_EQ_F(grid[0], 10.0f, 0.0f);
    ASSERT_EQ_F(grid[1], 20.0f, 0.0f);
    ASSERT_EQ_F(grid[2], 30.0f, 0.0f);
    for (int i = 0; i < 3; i++) ASSERT_EQ_F(grid[i], vert[i], 0.0f);
}

// The standalone offsets entry has no context and snaps on the unit grid.
TEST(content_scale_standalone_offsets_snap_on_unit_grid) {
    float off[4];
    wlx_compute_offsets(off, 3, 100.0f, 100.0f, NULL, 0.0f);
    ASSERT_EQ_F(off[1], 33.0f, 0.0f);
    ASSERT_EQ_F(off[2], 67.0f, 0.0f);
    ASSERT_EQ_F(off[3], 100.0f, 0.0f);
}

TEST(content_scale_bad_value_reports_and_runs_at_one) {
    WLX_Context ctx;
    const float bad[3] = { nanf(""), 0.0f, 100.0f };
    cs_init(&ctx, true);
    for (int i = 0; i < 3; i++) {
        cs_scale_value = bad[i];
        cs_count = 0;
        test_frame_begin(&ctx, 0, 0, false, false);
        ASSERT_EQ_F(wlx_content_scale(&ctx), 1.0f, 0.0f);
        ASSERT_EQ_INT(1, cs_count);
        ASSERT_EQ_INT(WLX_ERR_BACKEND_ANSWER, (int)cs_last.code);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT(3, wlx_error_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(content_scale_change_bumps_generation) {
    WLX_Context ctx;
    cs_init(&ctx, true);
    cs_scale_value = 1.0f;
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    uint32_t g0 = ctx.text_env_generation;

    test_frame_begin(&ctx, 0, 0, false, false);   // same scale: no bump
    test_frame_end(&ctx);
    ASSERT_EQ_INT((int)g0, (int)ctx.text_env_generation);

    cs_scale_value = 2.0f;                         // change: one bump
    test_frame_begin(&ctx, 0, 0, false, false);
    test_frame_end(&ctx);
    ASSERT_EQ_INT((int)g0 + 1, (int)ctx.text_env_generation);

    test_frame_begin(&ctx, 0, 0, false, false);   // held: no bump
    test_frame_end(&ctx);
    ASSERT_EQ_INT((int)g0 + 1, (int)ctx.text_env_generation);
    wlx_context_destroy(&ctx);
}

SUITE(content_scale) {
    RUN_TEST(content_scale_null_callback_is_one);
    RUN_TEST(content_scale_before_first_frame_is_one);
    RUN_TEST(content_scale_is_sampled_once_per_frame);
    RUN_TEST(content_scale_snaps_to_device_grid_2x);
    RUN_TEST(content_scale_snaps_to_device_grid_1_5x);
    RUN_TEST(content_scale_auto_grid_rows_snap);
    RUN_TEST(content_scale_auto_grid_fractional_rows_at_one);
    RUN_TEST(content_scale_standalone_offsets_snap_on_unit_grid);
    RUN_TEST(content_scale_bad_value_reports_and_runs_at_one);
    RUN_TEST(content_scale_change_bumps_generation);
}

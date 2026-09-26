// test_error_surface.c - contract errors: the handler and the record, the
// layout begin site every begin form records, and (in later cases) the
// degradation each entry documents for release builds.
//
// The runner defines WLX_DEBUG, so a contract error with no handler installed
// aborts (the debug default); every case here installs the capturing handler
// first. The NDEBUG default (print once per site, return) is covered in
// test_hard_assert.c.

static WLX_Error es_last;
static int       es_count;

static void es_capture(const WLX_Error *e, void *user) {
    (void)user;
    es_last = *e;
    es_count++;
}

static void es_init(WLX_Context *ctx) {
    test_ctx_init(ctx, 400, 300);
    wlx_set_error_handler(ctx, es_capture, NULL);
    memset(&es_last, 0, sizeof es_last);
    es_count = 0;
}

static const WLX_Layout *es_top(const WLX_Context *ctx) {
    return &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
}

static bool es_same_file(const char *file) {
    return file != NULL && strcmp(file, __FILE__) == 0;
}

// --- The surface ---

TEST(error_handler_receives_record) {
    WLX_Context ctx;
    es_init(&ctx);
    wlx_error_report(&ctx, WLX_ERR_BAD_ARGUMENT, "probe", "f.c", 7);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    ASSERT_EQ_STR("probe", es_last.message);
    ASSERT_EQ_STR("f.c", es_last.file);
    ASSERT_EQ_INT(7, es_last.line);
    ASSERT_TRUE(es_last.layout_file == NULL);
    ASSERT_EQ_INT(0, es_last.layout_line);
    ASSERT_EQ_INT(1, wlx_error_count(&ctx));
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)ctx.last_error.code);
    wlx_context_destroy(&ctx);
}

TEST(error_handler_null_restores_default) {
    WLX_Context ctx;
    int token = 0;
    es_init(&ctx);
    wlx_set_error_handler(&ctx, es_capture, &token);
    ASSERT_TRUE(ctx.error_handler == es_capture);
    ASSERT_TRUE(ctx.error_user == &token);
    wlx_set_error_handler(&ctx, NULL, NULL);
    ASSERT_TRUE(ctx.error_handler == NULL);
    ASSERT_TRUE(ctx.error_user == NULL);
    wlx_context_destroy(&ctx);
}

TEST(error_count_is_never_throttled) {
    WLX_Context ctx;
    es_init(&ctx);
    for (int i = 0; i < 3; i++) {
        wlx_error_report(&ctx, WLX_ERR_SLOT_OVERRUN, "same site", "s.c", 1);
    }
    ASSERT_EQ_INT(3, es_count);
    ASSERT_EQ_INT(3, wlx_error_count(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(error_layout_site_recorded_by_every_begin) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 8, WLX_VERT); int l_linear = __LINE__;   // one slot per nested begin below
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_linear, es_top(&ctx)->line);
    // A report inside names the innermost open layout.
    wlx_error_report(&ctx, WLX_ERR_SLOT_OVERRUN, "probe", NULL, 0);
    ASSERT_TRUE(es_same_file(es_last.layout_file));
    ASSERT_EQ_INT(l_linear, es_last.layout_line);
    ASSERT_TRUE(es_last.file == NULL);

    wlx_layout_begin_auto(&ctx, WLX_VERT, 20.0f); int l_auto = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_auto, es_top(&ctx)->line);
    wlx_layout_end(&ctx);

    wlx_grid_begin(&ctx, 2, 2); int l_grid = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_grid, es_top(&ctx)->line);
    wlx_grid_end(&ctx);

    wlx_grid_begin_auto(&ctx, 2, 20.0f); int l_grid_auto = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_grid_auto, es_top(&ctx)->line);
    wlx_grid_end(&ctx);

    wlx_grid_begin_auto_tile(&ctx, 50.0f, 20.0f); int l_tile = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_tile, es_top(&ctx)->line);
    wlx_grid_end(&ctx);

    wlx_scroll_panel_begin(&ctx, 500.0f); int l_panel = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_panel, es_top(&ctx)->line);
    wlx_scroll_panel_end(&ctx);

    wlx_overlay_begin(&ctx, 1, wlx_rect(10, 10, 100, 100)); int l_overlay = __LINE__;
    ASSERT_TRUE(es_same_file(es_top(&ctx)->file));
    ASSERT_EQ_INT(l_overlay, es_top(&ctx)->line);
    wlx_overlay_end(&ctx);

    wlx_layout_end(&ctx);
    // Outside every layout a report carries no layout site.
    wlx_error_report(&ctx, WLX_ERR_UNBALANCED_END, "probe", NULL, 0);
    ASSERT_TRUE(es_last.layout_file == NULL);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(2, es_count);
    wlx_context_destroy(&ctx);
}

TEST(error_direct_create_has_no_site) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Layout l = wlx_create_layout(&ctx, ctx.rect, 2, WLX_VERT, 0.0f);
    ASSERT_TRUE(l.file == NULL);
    ASSERT_EQ_INT(0, l.line);
    ASSERT_FALSE(l.slot_clamped);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

SUITE(error_surface) {
    RUN_TEST(error_handler_receives_record);
    RUN_TEST(error_handler_null_restores_default);
    RUN_TEST(error_count_is_never_throttled);
    RUN_TEST(error_layout_site_recorded_by_every_begin);
    RUN_TEST(error_direct_create_has_no_site);
}

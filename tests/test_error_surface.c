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

// --- The layout family: each violation reports, then degrades as documented ---

static WLX_Layout *es_top_mut(WLX_Context *ctx) {
    return &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
}

TEST(slot_overrun_reports_and_collapses) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT); int lb = __LINE__;
    WLX_Layout *top = es_top_mut(&ctx);
    WLX_Rect a = wlx_get_slot_rect(&ctx, top, -1, 1);
    WLX_Rect b = wlx_get_slot_rect(&ctx, top, -1, 1);
    ASSERT_EQ_INT(0, es_count);
    ASSERT_EQ_F(150.0f, a.h, 0.001f);
    ASSERT_EQ_F(150.0f, b.y, 0.001f);
    // The third fetch overruns: reported once, collapsed at the far edge, index unchanged.
    WLX_Rect c = wlx_get_slot_rect(&ctx, top, -1, 1);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_SLOT_OVERRUN, (int)es_last.code);
    ASSERT_TRUE(es_last.file == NULL);                  // a direct fetch has no caller site
    ASSERT_TRUE(es_same_file(es_last.layout_file));
    ASSERT_EQ_INT(lb, es_last.layout_line);
    ASSERT_EQ_RECT(((WLX_Rect){ 0, 300, 400, 0 }), c, 0.001f);
    ASSERT_EQ_INT(2, (int)top->index);
    ASSERT_TRUE(top->slot_clamped);
    // A widget overrunning through its prologue names its own line.
    (void)wlx_button(&ctx, "third"); int lw = __LINE__;
    ASSERT_EQ_INT(2, es_count);
    ASSERT_TRUE(es_same_file(es_last.file));
    ASSERT_EQ_INT(lw, es_last.line);
    ASSERT_EQ_INT(lb, es_last.layout_line);
    ASSERT_EQ_F(300.0f, wlx_last_rect(&ctx).y, 0.001f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// Builds one frame: a VERT parent whose first slot is CONTENT, holding a
// two-slot child with `labels` labels (a third one overruns). Returns the
// parent's CONTENT slot height as measured for the next frame.
static float es_content_height_with_labels(int labels) {
    WLX_Context ctx;
    es_init(&ctx);
    WLX_Slot_Size sizes[2] = { WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) };
    float measured = -1.0f;
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_VERT, .sizes = sizes);
        if (frame == 1) {
            const WLX_Layout *parent = es_top(&ctx);
            const float *off = wlx_layout_offsets(&ctx, parent);
            measured = off[1] - off[0];
        }
        wlx_layout_begin(&ctx, 2, WLX_VERT);
        for (int i = 0; i < labels; i++) {
            wlx_push_id(&ctx, (WLX_Id)i + 1);
            (void)wlx_label(&ctx, "row");
            wlx_pop_id(&ctx);
        }
        wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    wlx_context_destroy(&ctx);
    return measured;
}

TEST(slot_overrun_skips_content_contribution) {
    float two   = es_content_height_with_labels(2);
    float three = es_content_height_with_labels(3);   // the third label overruns
    ASSERT_TRUE(two > 0.0f);
    ASSERT_EQ_F(two, three, 0.001f);
}

TEST(positional_slot_past_count_reports) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    WLX_Rect r = wlx_get_slot_rect(&ctx, es_top_mut(&ctx), 5, 1);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_SLOT_OVERRUN, (int)es_last.code);
    ASSERT_TRUE(strstr(es_last.message, "position") != NULL);
    ASSERT_EQ_RECT(((WLX_Rect){ 0, 300, 400, 0 }), r, 0.001f);
    ASSERT_EQ_INT(0, (int)es_top(&ctx)->index);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(grid_cell_out_of_range_is_ignored) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_grid_begin(&ctx, 2, 2);
    wlx_grid_cell(&ctx, 5, 7);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_GRID_BOUNDS, (int)es_last.code);
    ASSERT_FALSE(es_top(&ctx)->grid.cell_set);
    WLX_Rect r = wlx_get_slot_rect(&ctx, es_top_mut(&ctx), -1, 1);   // the cursor stood at (0, 0)
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_RECT(((WLX_Rect){ 0, 0, 200, 150 }), r, 0.001f);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(grid_auto_advance_past_last_row_collapses) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_grid_begin(&ctx, 2, 2);
    WLX_Layout *top = es_top_mut(&ctx);
    for (int i = 0; i < 4; i++) (void)wlx_get_slot_rect(&ctx, top, -1, 1);
    ASSERT_EQ_INT(0, es_count);
    ASSERT_EQ_INT(2, (int)top->grid.cursor_row);
    WLX_Rect fifth = wlx_get_slot_rect(&ctx, top, -1, 1);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_GRID_BOUNDS, (int)es_last.code);
    ASSERT_EQ_RECT(((WLX_Rect){ 0, 300, 400, 0 }), fifth, 0.001f);
    ASSERT_EQ_INT(2, (int)top->grid.cursor_row);
    ASSERT_EQ_INT(0, (int)top->grid.cursor_col);
    ASSERT_TRUE(top->slot_clamped);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(layout_end_without_begin_reports_and_returns) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);   // one too many
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_UNBALANCED_END, (int)es_last.code);
    ASSERT_TRUE(es_last.layout_file == NULL);
    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    wlx_layout_begin(&ctx, 1, WLX_VERT);   // the frame goes on
    (void)wlx_button(&ctx, "ok");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(1, es_count);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(1, es_count);
    wlx_context_destroy(&ctx);
}

TEST(pops_without_push_report) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_pop_id(&ctx);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_UNBALANCED_END, (int)es_last.code);
    ASSERT_EQ_INT(0, (int)ctx.arena.id_stack.count);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_INT(2, es_count);
    ASSERT_EQ_INT(0, (int)ctx.arena.opacity_stack.count);
    wlx_scroll_panel_end(&ctx);   // no panel open: the innermost layout must stay
    ASSERT_EQ_INT(3, es_count);
    ASSERT_EQ_INT(1, (int)ctx.arena.layouts.count);
    ASSERT_EQ_INT(0, (int)ctx.arena.scroll_panels.count);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(widget_without_layout_uses_root_rect) {
    WLX_Context ctx, ref;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    (void)wlx_button(&ctx, "orphan"); int lw = __LINE__;
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_NO_LAYOUT, (int)es_last.code);
    ASSERT_TRUE(es_same_file(es_last.file));
    ASSERT_EQ_INT(lw, es_last.line);
    ASSERT_TRUE(es_last.layout_file == NULL);
    WLX_Rect orphan = wlx_last_rect(&ctx);
    test_frame_end(&ctx);
    // The same button in a one-slot root layout gets the same rect.
    test_ctx_init(&ref, 400, 300);
    test_frame_begin(&ref, 0, 0, false, false);
    wlx_layout_begin(&ref, 1, WLX_VERT);
    (void)wlx_button(&ref, "orphan");
    wlx_layout_end(&ref);
    test_frame_end(&ref);
    ASSERT_EQ_RECT(wlx_last_rect(&ref), orphan, 0.001f);
    wlx_context_destroy(&ctx);
    wlx_context_destroy(&ref);
}

TEST(open_layouts_at_end_report_once) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_begin(&ctx, 1, WLX_VERT); int inner = __LINE__;
    wlx_end(&ctx);   // two begins, no ends
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_UNBALANCED_END, (int)es_last.code);
    ASSERT_TRUE(es_same_file(es_last.layout_file));
    ASSERT_EQ_INT(inner, es_last.layout_line);
    // The next frame starts clean.
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);
    ASSERT_EQ_INT(1, es_count);
    wlx_context_destroy(&ctx);
}

TEST(dynamic_positional_access_reports) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_auto(&ctx, WLX_VERT, 20.0f);
    WLX_Rect r = wlx_get_slot_rect(&ctx, es_top_mut(&ctx), 0, 1);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    ASSERT_EQ_RECT(((WLX_Rect){ 0, 0, 400, 20 }), r, 0.001f);   // taken in sequence
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->count);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(layout_entries_without_layout_report_and_return) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_auto_slot_px(&ctx, 10.0f);
    wlx_grid_cell(&ctx, 0, 0);
    wlx_slot_style(&ctx, .border_width = 1.0f);
    wlx_grid_cell_style(&ctx, .border_width = 1.0f);
    ASSERT_EQ_INT(4, es_count);
    ASSERT_EQ_INT(WLX_ERR_NO_LAYOUT, (int)es_last.code);
    // Inside a linear layout the grid-only entries report a bad argument.
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_grid_cell(&ctx, 0, 0);
    ASSERT_EQ_INT(5, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// --- Arguments, pointers and limits ---

TEST(zero_and_negative_counts_clamp_to_one) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 0, WLX_VERT);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    ASSERT_TRUE(es_same_file(es_last.file));
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->count);
    (void)wlx_button(&ctx, "fills the one slot");
    ASSERT_EQ_INT(1, es_count);
    wlx_layout_end(&ctx);
    wlx_layout_begin(&ctx, (size_t)-1, WLX_VERT);   // a negative int, converted
    ASSERT_EQ_INT(2, es_count);
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->count);
    wlx_layout_end(&ctx);
    wlx_grid_begin(&ctx, 0, 3);
    ASSERT_EQ_INT(3, es_count);
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->grid.rows);
    ASSERT_EQ_INT(3, (int)es_top(&ctx)->grid.cols);
    wlx_grid_end(&ctx);
    wlx_grid_begin(&ctx, 2, 0);
    ASSERT_EQ_INT(4, es_count);
    ASSERT_EQ_INT(2, (int)es_top(&ctx)->grid.rows);
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->grid.cols);
    wlx_grid_end(&ctx);
    wlx_grid_begin_auto(&ctx, 0, 20.0f);
    ASSERT_EQ_INT(5, es_count);
    ASSERT_EQ_INT(1, (int)es_top(&ctx)->grid.cols);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(non_positive_pixel_sizes_clamp) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_grid_begin_auto(&ctx, 2, 0.0f);
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    ASSERT_EQ_F(1.0f, es_top(&ctx)->grid.row_size, 0.001f);
    wlx_grid_auto_row_px(&ctx, -1.0f);
    ASSERT_EQ_INT(2, es_count);
    ASSERT_EQ_F(1.0f, es_top(&ctx)->grid.next_row_size, 0.001f);
    wlx_grid_end(&ctx);
    wlx_grid_begin_auto_tile(&ctx, 0.0f, -2.0f);
    ASSERT_EQ_INT(4, es_count);   // width, then height
    ASSERT_EQ_F(1.0f, es_top(&ctx)->grid.row_size, 0.001f);
    wlx_grid_end(&ctx);
    wlx_layout_begin_auto(&ctx, WLX_VERT, -5.0f);
    ASSERT_EQ_INT(5, es_count);
    ASSERT_EQ_F(1.0f, es_top(&ctx)->linear.slot_size, 0.001f);
    wlx_layout_end(&ctx);
    // Variable-size mode with no size set before the child: 1 px, reported.
    wlx_layout_begin_auto(&ctx, WLX_VERT, 0.0f);
    ASSERT_EQ_INT(5, es_count);
    WLX_Rect r = wlx_get_slot_rect(&ctx, es_top_mut(&ctx), -1, 1);
    ASSERT_EQ_INT(6, es_count);
    ASSERT_EQ_F(1.0f, r.h, 0.001f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(required_null_pointers_are_inert) {
    WLX_Context ctx;
    es_init(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 12, WLX_VERT);
    const char *opts[2] = { "a", "b" };
    size_t cmds = ctx.arena.commands.count;
    size_t index_before = es_top(&ctx)->index;
    ASSERT_FALSE(wlx_inputbox(&ctx, "in", NULL, 16));
    ASSERT_FALSE(wlx_slider(&ctx, "sl", NULL));
    ASSERT_FALSE(wlx_toggle(&ctx, "tg", NULL));
    ASSERT_FALSE(wlx_radio(&ctx, "rd", NULL, 0));
    ASSERT_FALSE(wlx_dropdown(&ctx, "dd", NULL, opts, 2));
    ASSERT_FALSE(wlx_menu_begin(&ctx, NULL, 10, 10));
    ASSERT_FALSE(wlx_menu_button_begin(&ctx, "mb", NULL));
    ASSERT_FALSE(wlx_submenu_begin(&ctx, NULL));
    ASSERT_FALSE(wlx_editor(&ctx, "ed", NULL, 0, NULL));
    ASSERT_EQ_INT(9, es_count);
    ASSERT_EQ_INT(WLX_ERR_BAD_ARGUMENT, (int)es_last.code);
    ASSERT_TRUE(es_same_file(es_last.file));
    ASSERT_EQ_INT((int)cmds, (int)ctx.arena.commands.count);        // nothing drawn
    ASSERT_EQ_INT((int)index_before, (int)es_top(&ctx)->index);     // no slot taken
    // A NULL options array with a nonzero count is reported and shown empty;
    // the widget itself still runs.
    int sel = 0;
    (void)wlx_dropdown(&ctx, "dd2", &sel, NULL, 3);
    ASSERT_EQ_INT(10, es_count);
    ASSERT_EQ_INT((int)index_before + 1, (int)es_top(&ctx)->index);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(content_limit_reports_through_channel) {
    enum { OVER_MAX = WLX_CONTENT_SLOTS_MAX + 1 };
    WLX_Context ctx;
    es_init(&ctx);
    WLX_Slot_Size sizes[OVER_MAX];
    for (size_t i = 0; i < OVER_MAX; i++) sizes[i] = WLX_SLOT_FLEX(1);
    sizes[0] = WLX_SLOT_CONTENT;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, OVER_MAX, WLX_VERT, .sizes = sizes); int lb = __LINE__;
    ASSERT_EQ_INT(1, es_count);
    ASSERT_EQ_INT(WLX_ERR_LIMIT, (int)es_last.code);
    ASSERT_TRUE(es_same_file(es_last.file));
    ASSERT_EQ_INT(lb, es_last.line);
    ASSERT_FALSE(es_top(&ctx)->has_content_slot_measures);   // not tracked, still laid out
    (void)wlx_button(&ctx, "still draws");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(1, es_count);
    wlx_context_destroy(&ctx);
}

SUITE(error_surface) {
    RUN_TEST(error_handler_receives_record);
    RUN_TEST(error_handler_null_restores_default);
    RUN_TEST(error_count_is_never_throttled);
    RUN_TEST(error_layout_site_recorded_by_every_begin);
    RUN_TEST(error_direct_create_has_no_site);
    RUN_TEST(slot_overrun_reports_and_collapses);
    RUN_TEST(slot_overrun_skips_content_contribution);
    RUN_TEST(positional_slot_past_count_reports);
    RUN_TEST(grid_cell_out_of_range_is_ignored);
    RUN_TEST(grid_auto_advance_past_last_row_collapses);
    RUN_TEST(layout_end_without_begin_reports_and_returns);
    RUN_TEST(pops_without_push_report);
    RUN_TEST(widget_without_layout_uses_root_rect);
    RUN_TEST(open_layouts_at_end_report_once);
    RUN_TEST(dynamic_positional_access_reports);
    RUN_TEST(layout_entries_without_layout_report_and_return);
    RUN_TEST(zero_and_negative_counts_clamp_to_one);
    RUN_TEST(non_positive_pixel_sizes_clamp);
    RUN_TEST(required_null_pointers_are_inert);
    RUN_TEST(content_limit_reports_through_channel);
}

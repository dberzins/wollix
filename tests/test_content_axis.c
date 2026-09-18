// test_content_axis.c - main-axis selection in parent content contribution.
//
// The per-CONTENT-slot measure bucket stores the parent layout's main-axis
// extent: child heights for VERT parents, child widths for HORZ parents.
// A cross-axis value must never leak into the bucket.

static int _ca_warn_count = 0;

static void _ca_warn_sink(const char *file, int line, const char *msg, void *user) {
    (void)file; (void)line; (void)msg; (void)user;
    _ca_warn_count++;
}

static int ca_approx(float a, float b) { return fabsf(a - b) < 0.01f; }

// Debug channel with a counting sink, keeping suite output clean.
static void _ca_quiet_debug(WLX_Context *ctx) {
    wlx_dbg_init(ctx);
    ctx->dbg->warn_cb = _ca_warn_sink;
}

TEST(content_contribution_vert_parent_takes_heights) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
    WLX_Layout *parent = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    ASSERT_TRUE(parent->has_content_slot_measures);

    wlx_contribute_to_parent_layout(&ctx, parent, (WLX_Parent_Contribution){
        .h_contrib = 37.0f, .w_contrib = 91.0f,
        .slot_index = 0, .grid_row = WLX_SLOT_SKIP,
    });
    ASSERT_TRUE(wlx_layout_content_measures(&ctx, parent)[0] == 37.0f);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(content_contribution_horz_parent_takes_widths) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
    WLX_Layout *parent = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    ASSERT_TRUE(parent->has_content_slot_measures);

    wlx_contribute_to_parent_layout(&ctx, parent, (WLX_Parent_Contribution){
        .h_contrib = 37.0f, .w_contrib = 91.0f,
        .slot_index = 0, .grid_row = WLX_SLOT_SKIP,
    });
    ASSERT_TRUE(wlx_layout_content_measures(&ctx, parent)[0] == 91.0f);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// A nested layout's height aggregate must not leak into a HORZ parent's
// width bucket (nested layouts offer no intrinsic width).
TEST(content_nested_layout_contributes_nothing_to_horz_bucket) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
    WLX_Layout *parent = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];

    wlx_layout_begin(&ctx, 1, WLX_VERT);   // nested layout in the CONTENT slot
    wlx_layout_end(&ctx);

    ASSERT_TRUE(wlx_layout_content_measures(&ctx, parent)[0] == 0.0f);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Intrinsic widths per widget, through the mock measurer (w = len * fs / 2).
// Fixture: HORZ [CONTENT, FLEX]; the bucket for slot 0 is read back after the
// widget call, before layout end.
// ---------------------------------------------------------------------------

static WLX_Layout *ca_begin_horz_content(WLX_Context *ctx) {
    test_frame_begin(ctx, 0, 0, false, false);
    wlx_layout_begin(ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
    return &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
}

static void ca_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

TEST(intrinsic_label_measured_text) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_label(&ctx, "Hi", .font_size = 20);           // mock: 2 * 20 * 0.5
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 20.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_explicit_width_overrides_measure) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_label(&ctx, "Hi", .font_size = 20, .width = 77);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 77.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_button_text_plus_image_band) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_button(&ctx, "Go", .font_size = 20,
        .texture = (WLX_Texture){ .handle = 1, .width = 30, .height = 30 },
        .image_size = 30, .image_text_gap = 4);
    // text 20 + gap 4 + image 30
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 54.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

// No explicit .image_size: the band is the face's auto rule (font-derived
// with text present), never the raw texture width.
TEST(intrinsic_button_image_auto_band) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_button(&ctx, "Go", .font_size = 20,
        .texture = (WLX_Texture){ .handle = 1, .width = 64, .height = 64 },
        .image_text_gap = 4);
    // text 20 + gap 4 + auto band (font 20 * 1.5 = 30) - not the 64px texture
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 54.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_checkbox_glyph_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    bool checked = false;
    wlx_checkbox(&ctx, "Hi", &checked, .font_size = 20);
    // glyph 20 + label gap 20*0.5 + label 20
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 50.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_toggle_glyph_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    bool on = false;
    wlx_toggle(&ctx, "Hi", &on, .font_size = 20);
    float thr = ctx.theme->toggle.track_to_height_ratio > 0.0f
              ? ctx.theme->toggle.track_to_height_ratio : 2.0f;
    // track 20*thr + label gap 10 + label 20
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0],
                          20.0f * thr + 10.0f + 20.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_radio_glyph_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    int active = 0;
    wlx_radio(&ctx, "Hi", &active, 0, .font_size = 20);
    // circle 20 + label gap 10 + label 20
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 50.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_image_texture_width) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    WLX_Texture tex = { .handle = 1, .width = 48, .height = 24 };
    wlx_image(&ctx, tex);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 48.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_separator_thickness) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_separator(&ctx, .thickness = 3);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 3.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(intrinsic_slider_contributes_explicit_width_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    float v = 0.5f;
    wlx_slider(&ctx, "s", &v, .font_size = 20);       // none-row: no intrinsic
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 0.0f));

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    WLX_Layout *parent2 = ca_begin_horz_content(&ctx);
    wlx_slider(&ctx, "s", &v, .font_size = 20, .width = 150);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent2)[0], 150.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}


// The scroll panel is a none-row for intrinsic width: only an explicit
// .width contributes (the viewport rect would be circular).
TEST(intrinsic_scroll_panel_contributes_explicit_width_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_scroll_panel_begin(&ctx, 100.0f, .padding = 0);
    wlx_scroll_panel_end(&ctx);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 0.0f));

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    WLX_Layout *parent2 = ca_begin_horz_content(&ctx);
    wlx_scroll_panel_begin(&ctx, 100.0f, .padding = 0, .width = 180);
    wlx_scroll_panel_end(&ctx);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent2)[0], 180.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}
// The intrinsic is the single-line unwrapped measure regardless of .wrap
// (labels and buttons default wrap = true): the contribution is a pure
// function of content, so the fit slot renders the text unwrapped.
TEST(intrinsic_wrap_label_measures_single_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    WLX_Layout *parent = ca_begin_horz_content(&ctx);

    wlx_label(&ctx, "Hi", .font_size = 20, .wrap = true);
    ASSERT_TRUE(ca_approx(wlx_layout_content_measures(&ctx, parent)[0], 20.0f));

    ca_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// End-to-end resolution: a HORZ CONTENT slot settles to the contributed
// width on the next frame (the persistent measure state is keyed to the
// wlx_layout_begin call site, so each scenario loops two frames over the
// same site). Root rect is 400 px wide.
// ---------------------------------------------------------------------------

TEST(horz_content_resolves_widget_width_next_frame) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    float slot_w[2] = { 0 };
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        slot_w[frame] = off[1] - off[0];
        wlx_widget(&ctx, .width = 200, .height = 30);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(ca_approx(slot_w[0], 1.0f));    // unmeasured floor
    ASSERT_TRUE(ca_approx(slot_w[1], 200.0f));  // contributed width
    wlx_context_destroy(&ctx);
}

TEST(horz_content_fit_to_label_button) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    float slot_w[2] = { 0 };
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        slot_w[frame] = off[1] - off[0];
        wlx_button(&ctx, "Go", .font_size = 20);   // mock: 20 px
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(ca_approx(slot_w[1], 20.0f));
    wlx_context_destroy(&ctx);
}

TEST(horz_content_sidebar_and_flex_remainder) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    float slot_w = 0, flex_w = 0;
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        slot_w = off[1] - off[0];
        flex_w = off[2] - off[1];
        wlx_label(&ctx, "Settings", .font_size = 20);  // mock: 8 * 10 = 80
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(ca_approx(slot_w, 80.0f));
    ASSERT_TRUE(ca_approx(flex_w, 320.0f));   // FLEX takes the remainder
    wlx_context_destroy(&ctx);
}

TEST(horz_content_min_max_clamp_width) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    float min_w = 0;
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT_MIN(50), WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        min_w = off[1] - off[0];
        wlx_label(&ctx, "Hi", .font_size = 20);        // 20 < min 50
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(ca_approx(min_w, 50.0f));

    float max_w = 0;
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT_MAX(30), WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        max_w = off[1] - off[0];
        wlx_label(&ctx, "Settings", .font_size = 20);  // 80 > max 30
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_TRUE(ca_approx(max_w, 30.0f));

    wlx_context_destroy(&ctx);
}

TEST(horz_content_two_slots_settle_independently) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    float w0 = 0, w1 = 0;
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 3, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_CONTENT,
                                        WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        w0 = off[1] - off[0];
        w1 = off[2] - off[1];
        wlx_label(&ctx, "Hi", .font_size = 20);        // 20
        wlx_label(&ctx, "Hell", .font_size = 20);      // 40
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(ca_approx(w0, 20.0f));
    ASSERT_TRUE(ca_approx(w1, 40.0f));
    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Same-frame dx correction: on the very frame content is first measured, the
// deferred replay shifts later slots' commands to the corrected offsets
// (mirroring the VERT dy correction), scissors included.
// ---------------------------------------------------------------------------

static WLX_Rect _ca_rects[8];
static int _ca_rect_count = 0;
static void _ca_rec_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user;
    (void)c;
    if (_ca_rect_count < 8) _ca_rects[_ca_rect_count++] = r;
}

static WLX_Rect _ca_scissors[8];
static int _ca_scissor_count = 0;
static void _ca_rec_scissor(WLX_Rect r, void *user) {
    (void)user;
    if (_ca_scissor_count < 8) _ca_scissors[_ca_scissor_count++] = r;
}

TEST(horz_content_dx_corrects_same_frame) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    ctx.backend.draw_rect = _ca_rec_rect;

    WLX_Color fill = { 10, 20, 30, 255 };
    for (int frame = 0; frame < 2; frame++) {
        _ca_rect_count = 0;
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        wlx_widget(&ctx, .width = 200, .height = 30, .back_color = fill);
        wlx_widget(&ctx, .height = 30, .back_color = fill);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);

        // The FLEX sibling's fill must sit at the corrected slot boundary
        // already on the measuring frame (provisional boundary was ~1 px).
        ASSERT_EQ_INT(2, _ca_rect_count);
        ASSERT_TRUE(ca_approx(_ca_rects[1].x, 200.0f));
    }

    wlx_context_destroy(&ctx);
}

TEST(horz_content_dx_shifts_scissor_with_content) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);
    ctx.backend.begin_scissor = _ca_rec_scissor;

    for (int frame = 0; frame < 2; frame++) {
        _ca_scissor_count = 0;
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        wlx_widget(&ctx, .width = 200, .height = 30);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true);   // in the FLEX slot
        wlx_widget(&ctx, .height = 20);
        wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);

        // The clip layout's scissor replays at the corrected slot start, so
        // the clip stays aligned with the content it crops.
        ASSERT_TRUE(_ca_scissor_count >= 1);
        ASSERT_TRUE(ca_approx(_ca_scissors[0].x, 200.0f));
    }

    wlx_context_destroy(&ctx);
}

// The retained slot-size array lives in the byte scratch sub-arena, which is
// realloc-grown. A sibling declared before the CONTENT widget that records a
// large text span forces that growth on a fresh context's first frame; the
// intrinsic-width gate must still read the slot kinds correctly (through
// the offset-derived accessor, never a pointer captured at layout_begin).
// Under ASan the pre-accessor code reports a heap-use-after-free here.
TEST(content_sizes_survive_scratch_growth) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ca_quiet_debug(&ctx);

    enum { BIG = 8192 };
    static char big[BIG + 1];
    memset(big, 'x', BIG);
    big[BIG] = '\0';

    float slot_w[2] = { 0 };
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        wlx_layout_begin(&ctx, 2, WLX_HORZ,
            .sizes = (WLX_Slot_Size[]){ WLX_SLOT_CONTENT, WLX_SLOT_FLEX(1) });
        WLX_Layout *p = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
        const float *off = wlx_layout_offsets(&ctx, p);
        slot_w[frame] = off[1] - off[0];
        // Slot 1 first: its text span grows the scratch past the array's
        // original block before the CONTENT slot's widget is declared.
        wlx_label(&ctx, big, .font_size = 20, .pos = 1);
        wlx_label(&ctx, "Go", .font_size = 20, .pos = 0);   // mock: 20 px
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    // The recorded span is the fitted line, not all of `big`, but it still
    // carries the scratch past its first block (the realloc that moves the
    // retained sizes array).
    ASSERT_TRUE(ctx.arena.scratch.capacity > WLX_DA_INIT_CAP);
    ASSERT_TRUE(ca_approx(slot_w[0], 1.0f));                // unmeasured floor
    ASSERT_TRUE(ca_approx(slot_w[1], 20.0f));               // gate saw CONTENT
    wlx_context_destroy(&ctx);
}

SUITE(content_axis) {
    RUN_TEST(content_contribution_vert_parent_takes_heights);
    RUN_TEST(content_contribution_horz_parent_takes_widths);
    RUN_TEST(content_nested_layout_contributes_nothing_to_horz_bucket);
    RUN_TEST(intrinsic_label_measured_text);
    RUN_TEST(intrinsic_explicit_width_overrides_measure);
    RUN_TEST(intrinsic_button_text_plus_image_band);
    RUN_TEST(intrinsic_button_image_auto_band);
    RUN_TEST(intrinsic_checkbox_glyph_row);
    RUN_TEST(intrinsic_toggle_glyph_row);
    RUN_TEST(intrinsic_radio_glyph_row);
    RUN_TEST(intrinsic_image_texture_width);
    RUN_TEST(intrinsic_separator_thickness);
    RUN_TEST(intrinsic_slider_contributes_explicit_width_only);
    RUN_TEST(intrinsic_scroll_panel_contributes_explicit_width_only);
    RUN_TEST(intrinsic_wrap_label_measures_single_line);
    RUN_TEST(horz_content_resolves_widget_width_next_frame);
    RUN_TEST(horz_content_fit_to_label_button);
    RUN_TEST(horz_content_sidebar_and_flex_remainder);
    RUN_TEST(horz_content_min_max_clamp_width);
    RUN_TEST(horz_content_two_slots_settle_independently);
    RUN_TEST(horz_content_dx_corrects_same_frame);
    RUN_TEST(horz_content_dx_shifts_scissor_with_content);
    RUN_TEST(content_sizes_survive_scratch_growth);
}

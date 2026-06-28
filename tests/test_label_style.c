// test_label_style.c - tests for wlx_label .style and .vertical_metric
// added by LABEL_STYLE feature: .style aggregate intake overrides the
// individual typography fields when set, .vertical_metric pins block
// centering to the typographic em size for cross-backend cap parity.
// Included from test_main.c (single TU build).

// ============================================================================
// Capture helpers
// ============================================================================

#define LS_MAX_DRAWS 8

static int            _ls_text_count;
static float          _ls_text_xs[LS_MAX_DRAWS];
static float          _ls_text_ys[LS_MAX_DRAWS];
static WLX_Text_Style _ls_text_styles[LS_MAX_DRAWS];

static void ls_rec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text;
    if (_ls_text_count < LS_MAX_DRAWS) {
        _ls_text_xs[_ls_text_count]     = x;
        _ls_text_ys[_ls_text_count]     = y;
        _ls_text_styles[_ls_text_count] = style;
    }
    _ls_text_count++;
}

// Backend whose measured line height is line_h_mult * font_size. With
// line_h_mult > 1, LINE_HEIGHT centering differs from FONT_SIZE centering.
static float _ls_line_h_mult = 1.0f;

static void ls_measure_text_with_mult(const char *text, WLX_Text_Style style,
                                       float *out_w, float *out_h) {
    int fs = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs * _ls_line_h_mult;
}

static void ls_reset(void) {
    _ls_text_count = 0;
    memset(_ls_text_xs, 0, sizeof(_ls_text_xs));
    memset(_ls_text_ys, 0, sizeof(_ls_text_ys));
    memset(_ls_text_styles, 0, sizeof(_ls_text_styles));
}

static WLX_Backend ls_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_text    = ls_rec_draw_text;
    b.measure_text = ls_measure_text_with_mult;
    return b;
}

static void ls_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = ls_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, w, h);
}

static void ls_begin(WLX_Context *ctx, float w, float h) {
    ls_reset();
    ls_ctx_init(ctx, w, h);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void ls_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// ============================================================================
// Aggregate .style intake
// ============================================================================

TEST(label_style_individual_path_unchanged) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_begin(&ctx, 200, 200);
    wlx_label(&ctx, "Hi", .font_size = 20, .spacing = 3);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_INT(20, _ls_text_styles[0].font_size);
    ASSERT_EQ_INT(3,  _ls_text_styles[0].spacing);
}

TEST(label_style_aggregate_overrides_individual_when_set) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_begin(&ctx, 200, 200);
    WLX_Text_Style st = { .font_size = 24, .spacing = 7, .color = (WLX_Color){10, 20, 30, 255} };
    wlx_label(&ctx, "Hi", .style = st, .font_size = 12, .spacing = 2);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_INT(24, _ls_text_styles[0].font_size);
    ASSERT_EQ_INT(7,  _ls_text_styles[0].spacing);
    ASSERT_EQ_INT(10, _ls_text_styles[0].color.r);
    ASSERT_EQ_INT(20, _ls_text_styles[0].color.g);
    ASSERT_EQ_INT(30, _ls_text_styles[0].color.b);
}

TEST(label_style_unset_keeps_individual_typography) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_begin(&ctx, 200, 200);
    // .style left default (font_size == 0); individual font_size wins.
    wlx_label(&ctx, "Hi", .style = WLX_TEXT_STYLE_DEFAULT, .font_size = 18);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_INT(18, _ls_text_styles[0].font_size);
}

TEST(label_style_with_zero_color_falls_back_to_front_color) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_begin(&ctx, 200, 200);
    // .style.color is {0}; explicit .front_color should be promoted into the
    // resolved text style so themed foreground keeps working.
    WLX_Text_Style st = { .font_size = 16, .color = {0} };
    WLX_Color fg = (WLX_Color){200, 150, 100, 255};
    wlx_label(&ctx, "Hi", .style = st, .front_color = fg);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_INT(200, _ls_text_styles[0].color.r);
    ASSERT_EQ_INT(150, _ls_text_styles[0].color.g);
    ASSERT_EQ_INT(100, _ls_text_styles[0].color.b);
    ASSERT_EQ_INT(255, _ls_text_styles[0].color.a);
}

// ============================================================================
// Vertical metric: LINE_HEIGHT (default) vs FONT_SIZE
// ============================================================================
//
// With line_h = font_size * 2 in a 100x100 rect, single line, align CENTER:
//   LINE_HEIGHT: total_h = 2*fs = 20, vert.y = (100 - 20)/2 = 40
//   FONT_SIZE:   total_h = 1*fs = 10, vert.y = (100 - 10)/2 = 45

TEST(label_vmetric_default_uses_line_height) {
    WLX_Context ctx;
    _ls_line_h_mult = 2.0f;
    ls_begin(&ctx, 100, 100);
    wlx_label(&ctx, "Hi", .font_size = 10, .align = WLX_CENTER, .wrap = false);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_F(40.0f, _ls_text_ys[0], 0.5f);
}

TEST(label_vmetric_font_size_centers_on_em) {
    WLX_Context ctx;
    _ls_line_h_mult = 2.0f;
    ls_begin(&ctx, 100, 100);
    wlx_label(&ctx, "Hi", .font_size = 10, .align = WLX_CENTER, .wrap = false,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_F(45.0f, _ls_text_ys[0], 0.5f);
}

TEST(label_vmetric_font_size_via_style) {
    WLX_Context ctx;
    _ls_line_h_mult = 2.0f;
    ls_begin(&ctx, 100, 100);
    // Same vmetric effect when typography comes from .style.
    WLX_Text_Style st = { .font_size = 10 };
    wlx_label(&ctx, "Hi", .style = st, .align = WLX_CENTER, .wrap = false,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE);
    ls_end(&ctx);

    ASSERT_TRUE(_ls_text_count >= 1);
    ASSERT_EQ_F(45.0f, _ls_text_ys[0], 0.5f);
}

TEST(label_vmetric_line_height_equals_font_size_no_shift) {
    // When the backend already reports line_h == font_size the two metrics
    // must produce the same y, proving FONT_SIZE is a no-op on parity backends.
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_begin(&ctx, 100, 100);
    wlx_label(&ctx, "Hi", .font_size = 10, .align = WLX_CENTER, .wrap = false);
    ls_end(&ctx);
    float y_default = (_ls_text_count >= 1) ? _ls_text_ys[0] : -1.0f;

    ls_begin(&ctx, 100, 100);
    wlx_label(&ctx, "Hi", .font_size = 10, .align = WLX_CENTER, .wrap = false,
        .vertical_metric = WLX_VMETRIC_FONT_SIZE);
    ls_end(&ctx);
    float y_em = (_ls_text_count >= 1) ? _ls_text_ys[0] : -2.0f;

    ASSERT_EQ_F(y_default, y_em, 0.001f);
}

// ============================================================================
// Defaults
// ============================================================================

TEST(label_defaults_style_zero_and_vmetric_line_height) {
    WLX_Label_Opt opt = wlx_default_label_opt();
    ASSERT_EQ_INT(0, opt.style.font_size);
    ASSERT_EQ_INT(0, opt.style.spacing);
    ASSERT_EQ_INT((int)WLX_VMETRIC_LINE_HEIGHT, (int)opt.vertical_metric);
}

// ============================================================================
// Debug-only: conflicting .style and .font_size emits a once-per-site warning
// ============================================================================

#ifdef WLX_DEBUG

static int _ls_warn_count = 0;

static void _ls_warn_cb(const char *file, int line, const char *msg, void *user_data) {
    (void)file; (void)line; (void)msg; (void)user_data;
    _ls_warn_count++;
}

TEST(label_style_debug_warns_when_style_and_font_size_both_set) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_reset();
    ls_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_dbg_init(&ctx);
    _ls_warn_count = 0;
    ctx.dbg->warn_cb = _ls_warn_cb;
    ctx.dbg->warn_user_data = NULL;

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Text_Style st = { .font_size = 18 };
    wlx_label(&ctx, "Hi", .style = st, .font_size = 12);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);

    ASSERT_EQ_INT(1, _ls_warn_count);
}

TEST(label_style_debug_no_warn_when_only_style_set) {
    WLX_Context ctx;
    _ls_line_h_mult = 1.0f;
    ls_reset();
    ls_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_dbg_init(&ctx);
    _ls_warn_count = 0;
    ctx.dbg->warn_cb = _ls_warn_cb;
    ctx.dbg->warn_user_data = NULL;

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    WLX_Text_Style st = { .font_size = 18 };
    wlx_label(&ctx, "Hi", .style = st);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);

    ASSERT_EQ_INT(0, _ls_warn_count);
}

#endif // WLX_DEBUG

static void test_label_style_run(void) {
    RUN_TEST(label_style_individual_path_unchanged);
    RUN_TEST(label_style_aggregate_overrides_individual_when_set);
    RUN_TEST(label_style_unset_keeps_individual_typography);
    RUN_TEST(label_style_with_zero_color_falls_back_to_front_color);
    RUN_TEST(label_vmetric_default_uses_line_height);
    RUN_TEST(label_vmetric_font_size_centers_on_em);
    RUN_TEST(label_vmetric_font_size_via_style);
    RUN_TEST(label_vmetric_line_height_equals_font_size_no_shift);
    RUN_TEST(label_defaults_style_zero_and_vmetric_line_height);
#ifdef WLX_DEBUG
    RUN_TEST(label_style_debug_warns_when_style_and_font_size_both_set);
    RUN_TEST(label_style_debug_no_warn_when_only_style_set);
#endif
}

SUITE(label_style) {
    test_label_style_run();
}
// test_text_spacing.c - text spacing opt-in API tests.
// Covers: default natural spacing, explicit spacing propagation through command
// recording and replay, and typography resolution with nonzero spacing.
// Included from test_main.c (single TU build).

// ============================================================================
// Spacing-capturing backend stub
// ============================================================================

static int   _tsp_draw_text_count  = 0;
static int   _tsp_draw_span_count  = 0;
static int   _tsp_captured_spacing = -1;
static int   _tsp_span_spacing     = -1;

static void tsp_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)x; (void)y;
    _tsp_draw_text_count++;
    _tsp_captured_spacing = style.spacing;
}

static void tsp_draw_text_slice(const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    (void)text; (void)len; (void)x; (void)y;
    _tsp_draw_span_count++;
    _tsp_span_spacing = style.spacing;
}

static void tsp_measure_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    (void)style;
    size_t n = text ? strlen(text) : 0;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)n * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

static void tsp_measure_text_slice(const char *text, size_t len, WLX_Text_Style style, float *out_w, float *out_h) {
    (void)text; (void)len;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

static void tsp_reset(void) {
    _tsp_draw_text_count  = 0;
    _tsp_draw_span_count  = 0;
    _tsp_captured_spacing = -1;
    _tsp_span_spacing     = -1;
}

static inline void tsp_ctx_init(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.draw_text         = tsp_draw_text;
    ctx->backend.draw_text_slice   = tsp_draw_text_slice;
    ctx->backend.measure_text      = tsp_measure_text;
    ctx->backend.measure_text_slice = tsp_measure_text_slice;
}

// ============================================================================
// Tests
// ============================================================================

TEST(tsp_default_style_spacing_is_zero) {
    WLX_Text_Style s = WLX_TEXT_STYLE_DEFAULT;
    ASSERT_EQ_INT(0, s.spacing);
}

TEST(tsp_zero_initialized_style_spacing_is_zero) {
    WLX_Text_Style s = {0};
    ASSERT_EQ_INT(0, s.spacing);
}

TEST(tsp_typography_defaults_spacing_is_zero) {
    // WLX_Label_Opt uses WLX_TEXT_TYPOGRAPHY_FIELDS and WLX_TEXT_TYPOGRAPHY_DEFAULTS.
    // Verify the spacing field defaults to 0.
    WLX_Label_Opt opt = wlx_default_label_opt();
    ASSERT_EQ_INT(0, opt.spacing);
}

TEST(tsp_explicit_spacing_round_trips_through_cmd_record_replay) {
    // Record a text draw command with nonzero spacing, then replay and verify
    // the spacing value reaches the backend draw_text_slice callback.
    // Deferred text spans are replayed via draw_text_slice when available.
    WLX_Context ctx;
    tsp_ctx_init(&ctx, 400, 300);
    tsp_reset();

    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Text_Style style = { .font_size = 14, .spacing = 3 };
    wlx_draw_text(&ctx, "Hello", 10.0f, 20.0f, style);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _tsp_draw_span_count);
    ASSERT_EQ_INT(3, _tsp_span_spacing);
}

TEST(tsp_natural_spacing_round_trips_zero) {
    WLX_Context ctx;
    tsp_ctx_init(&ctx, 400, 300);
    tsp_reset();

    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Text_Style style = { .font_size = 14, .spacing = 0 };
    wlx_draw_text(&ctx, "Hi", 0.0f, 0.0f, style);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _tsp_draw_span_count);
    ASSERT_EQ_INT(0, _tsp_span_spacing);
}

TEST(tsp_label_widget_propagates_explicit_spacing) {
    // Verify that the label widget passes opt.spacing into the recorded
    // WLX_Text_Style that replays as draw_text_slice.
    WLX_Context ctx;
    tsp_ctx_init(&ctx, 400, 300);
    tsp_reset();

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_label(&ctx, "Test", .font_size = 14, .spacing = 2);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_tsp_draw_span_count > 0);
    ASSERT_EQ_INT(2, _tsp_span_spacing);
}

TEST(tsp_slider_opt_spacing_defaults_zero) {
    // WLX_Slider_Opt has its own spacing field outside the shared macro.
    WLX_Slider_Opt opt = wlx_default_slider_opt();
    ASSERT_EQ_INT(0, opt.spacing);
}

TEST(tsp_button_opt_spacing_defaults_zero) {
    WLX_Button_Opt opt = wlx_default_button_opt();
    ASSERT_EQ_INT(0, opt.spacing);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(text_spacing) {
    RUN_TEST(tsp_default_style_spacing_is_zero);
    RUN_TEST(tsp_zero_initialized_style_spacing_is_zero);
    RUN_TEST(tsp_typography_defaults_spacing_is_zero);
    RUN_TEST(tsp_explicit_spacing_round_trips_through_cmd_record_replay);
    RUN_TEST(tsp_natural_spacing_round_trips_zero);
    RUN_TEST(tsp_label_widget_propagates_explicit_spacing);
    RUN_TEST(tsp_slider_opt_spacing_defaults_zero);
    RUN_TEST(tsp_button_opt_spacing_defaults_zero);
}

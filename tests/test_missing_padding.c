// test_missing_padding.c - tests for content_padding* on the
// remaining widgets: wlx_checkbox, wlx_radio, wlx_toggle, wlx_slider,
// wlx_progress. The inset shrinks only the inner content (chrome
// glyph + label, or just the track) while the slot contribution
// stays put. Hit-rect contracts match the per-widget convention
// documented for the rollout.
//
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs: capture rect / rounded-rect / rounded-rect-lines / text
// draws into small ring buffers. Each widget asserts on the relevant index.
// ============================================================================

#define MP_REC_MAX 16

static WLX_Rect _mp_rects[MP_REC_MAX];
static int      _mp_rect_count;
static WLX_Rect _mp_rounded[MP_REC_MAX];
static int      _mp_rounded_count;
static WLX_Rect _mp_rounded_lines[MP_REC_MAX];
static int      _mp_rounded_lines_count;
// Square rounded-rect-line draws (radio ring) dispatch through draw_ring.
// Record the inscribed rect so the assertion shape matches the rounded
// recorder.
static WLX_Rect _mp_rings[MP_REC_MAX];
static int      _mp_ring_count;
static float    _mp_text_x;
static float    _mp_text_y;
static int      _mp_text_count;

static void mp_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)c;
    if (_mp_rect_count < MP_REC_MAX) _mp_rects[_mp_rect_count] = r;
    _mp_rect_count++;
}

static void mp_rec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)roundness; (void)segments; (void)c;
    if (_mp_rounded_count < MP_REC_MAX) _mp_rounded[_mp_rounded_count] = r;
    _mp_rounded_count++;
}

static void mp_rec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    (void)roundness; (void)segments; (void)thick; (void)c;
    if (_mp_rounded_lines_count < MP_REC_MAX) _mp_rounded_lines[_mp_rounded_lines_count] = r;
    _mp_rounded_lines_count++;
}

static void mp_rec_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color c) {
    (void)inner_r; (void)segments; (void)c;
    if (_mp_ring_count < MP_REC_MAX) {
        _mp_rings[_mp_ring_count] = (WLX_Rect){
            cx - outer_r, cy - outer_r, outer_r * 2.0f, outer_r * 2.0f
        };
    }
    _mp_ring_count++;
}

static void mp_rec_draw_text(const char *t, float x, float y, WLX_Text_Style s) {
    (void)t; (void)s;
    if (_mp_text_count == 0) {
        _mp_text_x = x;
        _mp_text_y = y;
    }
    _mp_text_count++;
}

static void mp_rec_reset(void) {
    memset(_mp_rects, 0, sizeof(_mp_rects));
    _mp_rect_count = 0;
    memset(_mp_rounded, 0, sizeof(_mp_rounded));
    _mp_rounded_count = 0;
    memset(_mp_rounded_lines, 0, sizeof(_mp_rounded_lines));
    _mp_rounded_lines_count = 0;
    memset(_mp_rings, 0, sizeof(_mp_rings));
    _mp_ring_count = 0;
    _mp_text_x = 0;
    _mp_text_y = 0;
    _mp_text_count = 0;
}

static WLX_Backend mp_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect = mp_rec_draw_rect;
    b.draw_rect_rounded = mp_rec_draw_rect_rounded;
    b.draw_rect_rounded_lines = mp_rec_draw_rect_rounded_lines;
    b.draw_ring = mp_rec_draw_ring;
    b.draw_text = mp_rec_draw_text;
    return b;
}

static void mp_ctx_init(WLX_Context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = mp_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, 200, 200);
}

static void mp_begin(WLX_Context *ctx) {
    mp_rec_reset();
    mp_ctx_init(ctx);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void mp_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// Theme with a non-zero project-wide padding so the
// WLX_PADDING_USE_THEME path is observable.
static WLX_Theme mp_theme_padded(void) {
    WLX_Theme t = wlx_theme_dark;
    t.padding = 6.0f;
    return t;
}

// ============================================================================
// Checkbox: chrome glyph drawn via wlx_draw_box (roundness 0 -> draw_rect).
// First recorded draw_rect is the checkbox fill.
// ============================================================================

TEST(missing_padding_checkbox_default_zero_inset) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 0.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].w, 16.0f, 0.001f);
}

TEST(missing_padding_checkbox_uniform_shifts_glyph) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16,
                 .content_padding = 20);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 20.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].w, 16.0f, 0.001f);
}

TEST(missing_padding_checkbox_asymmetric_per_side) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16,
                 .content_padding_left = 24, .content_padding_top = 40);
    mp_end(&ctx);

    // content_rect = (24, 40, 176, 160); acr top centered: 40 + (160-16)/2 = 112.
    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 24.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].y, 112.0f, 0.001f);
}

TEST(missing_padding_checkbox_theme_opt_in) {
    WLX_Context ctx;
    mp_rec_reset();
    mp_ctx_init(&ctx);
    WLX_Theme custom = mp_theme_padded();
    ctx.theme = &custom;
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16,
                 .content_padding = WLX_PADDING_USE_THEME);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 6.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(missing_padding_checkbox_clamp_on_tight_slot) {
    WLX_Context ctx;
    mp_rec_reset();
    mp_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin_s(&ctx, WLX_HORZ, WLX_SIZES(WLX_SLOT_PX(20), WLX_SLOT_FLEX(1)));
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16,
                 .content_padding_left = 12, .content_padding_right = 12);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 12 + 12 = 24 > 20 -> scale by 20/24 = 5/6 -> left = 10. Glyph x = 10.
    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 10.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

static bool mp_padded_checkbox(WLX_Context *ctx, bool *checked) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    bool clicked = wlx_checkbox(ctx, "X", checked, .align = WLX_LEFT,
                                .font_size = 16, .content_padding = 40);
    wlx_layout_end(ctx);
    return clicked;
}

TEST(missing_padding_checkbox_hit_rect_uses_wr_when_full_slot_hit) {
    // full_slot_hit defaults to true. With content_padding = 40 the chrome
    // glyph sits inside the inset, but a click anywhere in the full slot
    // must still register because the hit rect uses wr, not content_rect.
    WLX_Context ctx;
    mp_ctx_init(&ctx);

    bool checked = false;

    // Press at (5, 5): outside the (40, 40, 120, 120) inset content rect
    // but inside the 200x200 slot.
    mp_rec_reset();
    test_frame_begin(&ctx, 5, 5, true, true);
    (void)mp_padded_checkbox(&ctx, &checked);
    test_frame_end(&ctx);

    // Release at (5, 5): click fires.
    mp_rec_reset();
    test_frame_begin(&ctx, 5, 5, false, false);
    bool clicked = mp_padded_checkbox(&ctx, &checked);
    test_frame_end(&ctx);

    ASSERT_TRUE(clicked);
    ASSERT_TRUE(checked);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Radio: ring drawn via wlx_draw_rect_rounded_lines (border_width > 0 from
// theme default). First recorded rounded-lines call is the ring.
// ============================================================================

TEST(missing_padding_radio_default_zero_inset) {
    WLX_Context ctx;
    mp_begin(&ctx);
    int active = -1;
    wlx_radio(&ctx, NULL, &active, 0, .align = WLX_LEFT, .font_size = 16);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_ring_count >= 1);
    ASSERT_EQ_F(_mp_rings[0].x, 0.0f, 0.001f);
    ASSERT_EQ_F(_mp_rings[0].w, 16.0f, 0.001f);
}

TEST(missing_padding_radio_uniform_shifts_ring) {
    WLX_Context ctx;
    mp_begin(&ctx);
    int active = -1;
    wlx_radio(&ctx, NULL, &active, 0, .align = WLX_LEFT, .font_size = 16,
              .content_padding = 20);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_ring_count >= 1);
    ASSERT_EQ_F(_mp_rings[0].x, 20.0f, 0.001f);
    ASSERT_EQ_F(_mp_rings[0].w, 16.0f, 0.001f);
}

TEST(missing_padding_radio_theme_opt_in) {
    WLX_Context ctx;
    mp_rec_reset();
    mp_ctx_init(&ctx);
    WLX_Theme custom = mp_theme_padded();
    ctx.theme = &custom;
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    int active = -1;
    wlx_radio(&ctx, NULL, &active, 0, .align = WLX_LEFT, .font_size = 16,
              .content_padding = WLX_PADDING_USE_THEME);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_mp_ring_count >= 1);
    ASSERT_EQ_F(_mp_rings[0].x, 6.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Toggle: track drawn via wlx_draw_rect_rounded (hardcoded roundness = 1).
// First recorded rounded call is the track.
// ============================================================================

TEST(missing_padding_toggle_default_zero_inset) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool on = false;
    wlx_toggle(&ctx, NULL, &on, .align = WLX_LEFT, .font_size = 16);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 0.0f, 0.001f);
    // track_w = font_size * track_to_height_ratio (default 2.0) = 32.
    ASSERT_EQ_F(_mp_rounded[0].w, 32.0f, 0.001f);
}

TEST(missing_padding_toggle_uniform_shifts_track) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool on = false;
    wlx_toggle(&ctx, NULL, &on, .align = WLX_LEFT, .font_size = 16,
               .content_padding = 20);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 20.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].w, 32.0f, 0.001f);
}

TEST(missing_padding_toggle_asymmetric_per_side) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool on = false;
    wlx_toggle(&ctx, NULL, &on, .align = WLX_LEFT, .font_size = 16,
               .content_padding_top = 32, .content_padding_left = 12);
    mp_end(&ctx);

    // content_rect = (12, 32, 188, 168); acr y centered: 32 + (168-16)/2 = 108.
    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 12.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].y, 108.0f, 0.001f);
}

// ============================================================================
// Slider: track drawn via wlx_draw_rect_rounded. With label=NULL and
// show_value=false the track spans content_rect.w.
// ============================================================================

TEST(missing_padding_slider_default_zero_inset) {
    WLX_Context ctx;
    mp_begin(&ctx);
    float value = 0.0f;
    wlx_slider(&ctx, NULL, &value, .show_value = false, .font_size = 16);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 0.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].w, 200.0f, 0.001f);
}

TEST(missing_padding_slider_uniform_shifts_track) {
    WLX_Context ctx;
    mp_begin(&ctx);
    float value = 0.0f;
    wlx_slider(&ctx, NULL, &value, .show_value = false, .font_size = 16,
               .content_padding = 20);
    mp_end(&ctx);

    // content_rect = (20, 20, 160, 160); track_x = 20, track_w = 160,
    // track_y = 20 + (160-6)/2 = 97.
    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 20.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].w, 160.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].y, 97.0f, 0.001f);
}

TEST(missing_padding_slider_asymmetric_per_side) {
    WLX_Context ctx;
    mp_begin(&ctx);
    float value = 0.0f;
    wlx_slider(&ctx, NULL, &value, .show_value = false, .font_size = 16,
               .content_padding_top = 40, .content_padding_bottom = 0);
    mp_end(&ctx);

    // content_rect = (0, 40, 200, 160); track_y = 40 + (160-6)/2 = 117.
    ASSERT_TRUE(_mp_rounded_count >= 1);
    ASSERT_EQ_F(_mp_rounded[0].x, 0.0f, 0.001f);
    ASSERT_EQ_F(_mp_rounded[0].y, 117.0f, 0.001f);
}

// ============================================================================
// Progress: track drawn via wlx_draw_box (roundness 0 -> draw_rect).
// First recorded draw_rect is the track.
// ============================================================================

TEST(missing_padding_progress_default_zero_inset) {
    WLX_Context ctx;
    mp_begin(&ctx);
    wlx_progress(&ctx, 0.0f);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 0.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].w, 200.0f, 0.001f);
    // track_height = 6 (theme) centred in 200 -> y = 97.
    ASSERT_EQ_F(_mp_rects[0].y, 97.0f, 0.001f);
}

TEST(missing_padding_progress_uniform_shifts_track) {
    WLX_Context ctx;
    mp_begin(&ctx);
    wlx_progress(&ctx, 0.0f, .content_padding = 20);
    mp_end(&ctx);

    // content_rect = (20, 20, 160, 160); track centered y = 20 + (160-6)/2 = 97.
    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 20.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].w, 160.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].y, 97.0f, 0.001f);
}

TEST(missing_padding_progress_track_height_clamped_to_content_rect) {
    // 10px tall slot with track_height=20 and no padding: track clamps to
    // content_rect.h = 10 instead of overflowing. Override min_height to 1
    // so the slot is not forced taller than 10 by the resolver default.
    WLX_Context ctx;
    mp_rec_reset();
    mp_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(10), WLX_SLOT_FLEX(1)));
    wlx_progress(&ctx, 0.0f, .track_height = 20, .min_height = 1);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].h, 10.0f, 0.001f);
    ASSERT_EQ_F(_mp_rects[0].y, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Cross-cutting: slot padding (outer) and content padding (inner) compose
// on checkbox. Slot padding = 4 shrinks the slot rect to (4, 4, 192, 192);
// content padding = 8 further insets the chrome glyph by another 8 px on
// every side, putting it at x = 12.
// ============================================================================

TEST(missing_padding_slot_plus_content_compose_on_checkbox) {
    WLX_Context ctx;
    mp_begin(&ctx);
    bool checked = false;
    wlx_checkbox(&ctx, "", &checked, .align = WLX_LEFT, .font_size = 16,
                 .padding = 4, .content_padding = 8);
    mp_end(&ctx);

    ASSERT_TRUE(_mp_rect_count >= 1);
    ASSERT_EQ_F(_mp_rects[0].x, 12.0f, 0.001f);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(missing_padding) {
    RUN_TEST(missing_padding_checkbox_default_zero_inset);
    RUN_TEST(missing_padding_checkbox_uniform_shifts_glyph);
    RUN_TEST(missing_padding_checkbox_asymmetric_per_side);
    RUN_TEST(missing_padding_checkbox_theme_opt_in);
    RUN_TEST(missing_padding_checkbox_clamp_on_tight_slot);
    RUN_TEST(missing_padding_checkbox_hit_rect_uses_wr_when_full_slot_hit);
    RUN_TEST(missing_padding_radio_default_zero_inset);
    RUN_TEST(missing_padding_radio_uniform_shifts_ring);
    RUN_TEST(missing_padding_radio_theme_opt_in);
    RUN_TEST(missing_padding_toggle_default_zero_inset);
    RUN_TEST(missing_padding_toggle_uniform_shifts_track);
    RUN_TEST(missing_padding_toggle_asymmetric_per_side);
    RUN_TEST(missing_padding_slider_default_zero_inset);
    RUN_TEST(missing_padding_slider_uniform_shifts_track);
    RUN_TEST(missing_padding_slider_asymmetric_per_side);
    RUN_TEST(missing_padding_progress_default_zero_inset);
    RUN_TEST(missing_padding_progress_uniform_shifts_track);
    RUN_TEST(missing_padding_progress_track_height_clamped_to_content_rect);
    RUN_TEST(missing_padding_slot_plus_content_compose_on_checkbox);
}

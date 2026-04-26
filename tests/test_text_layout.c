// test_text_layout.c - regression tests for two-pass text layout with
// an invariant-violating (kerning) mock backend.

// ============================================================================
// Kerning mock: returns sum_of_glyph_widths - 2.0 for strings > 3 chars,
// simulating WASM backend shaping/kerning behavior.
// ============================================================================

static void mock_measure_text_kerning(const char *text, WLX_Text_Style style,
                                       float *out_w, float *out_h) {
    int fs = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    float w = (float)len * (float)fs * 0.5f;
    if (len > 3) w -= 2.0f;
    if (out_w) *out_w = w;
    if (out_h) *out_h = (float)fs;
}

// ============================================================================
// Capturing draw_text: records glyph positions during frame replay.
// ============================================================================

#define TL_MAX_CAPTURES_ 64

static struct { float x; float y; } _tl_captures[TL_MAX_CAPTURES_];
static int _tl_capture_count = 0;

static void _tl_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)style;
    if (_tl_capture_count < TL_MAX_CAPTURES_) {
        _tl_captures[_tl_capture_count].x = x;
        _tl_captures[_tl_capture_count].y = y;
        _tl_capture_count++;
    }
}

static inline void _tl_reset_captures(void) {
    _tl_capture_count = 0;
}

// ============================================================================
// Helpers
// ============================================================================

static inline void test_ctx_init_kerning(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.measure_text = mock_measure_text_kerning;
}

static inline void test_ctx_init_kerning_capture(WLX_Context *ctx, float w, float h) {
    test_ctx_init_kerning(ctx, w, h);
    ctx->backend.draw_text = _tl_capture_draw_text;
}

// ============================================================================
// Tests
// ============================================================================

TEST(right_aligned_no_spurious_wrap) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    // Per-glyph: 1 char -> 1*10*0.5 = 5.0 (len<=3, no kerning).
    // 5 glyphs -> 25.0 total.  rect.w = 25 -> fits in one line.
    WLX_Rect rect = {0, 0, 25, 100};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = 0, cy = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_calc_cursor_position(&ctx, rect, "ABCDE", style, WLX_RIGHT, true, &cx, &cy);
    test_frame_end(&ctx);

    // WLX_RIGHT centers vertically: y = (100 - 10) / 2 = 45.
    // Single line -> cursor stays on that line.
    ASSERT_EQ_F(cy, 45.0f, 0.1f);
}

TEST(center_aligned_no_spurious_wrap) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {0, 0, 25, 100};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = 0, cy = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_calc_cursor_position(&ctx, rect, "ABCDE", style, WLX_CENTER, true, &cx, &cy);
    test_frame_end(&ctx);

    // WLX_CENTER centers vertically: y = (100 - 10) / 2 = 45.
    ASSERT_EQ_F(cy, 45.0f, 0.1f);
}

TEST(wrap_right_aligned_per_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    // rect.w = 20, glyph = 5.0 each, 5 glyphs -> wraps after 4.
    // Line 0: ABCD (width 20), Line 1: E (width 5).
    WLX_Rect rect = {10, 20, 20, 100};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "ABCDE", style, WLX_RIGHT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(5, _tl_capture_count);

    // Vertical: total_h = 2*10 = 20, y_start = 20 + (100-20)/2 = 60.
    // Line 0 (ABCD, width=20 == rect.w): right-align is no-op, x starts at 10.
    ASSERT_EQ_F(_tl_captures[0].x, 10.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[0].y, 60.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[3].x, 25.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[3].y, 60.0f, 0.1f);

    // Line 1 (E, width=5): right-aligned x = 10 + (20-5) = 25.
    ASSERT_EQ_F(_tl_captures[4].x, 25.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[4].y, 70.0f, 0.1f);

    // Both lines end at rect.x + rect.w = 30.
    float line0_end = _tl_captures[3].x + 5.0f;
    float line1_end = _tl_captures[4].x + 5.0f;
    ASSERT_EQ_F(line0_end, 30.0f, 0.1f);
    ASSERT_EQ_F(line1_end, 30.0f, 0.1f);
}

TEST(cursor_consistent_with_render) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {10, 20, 20, 100};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = 0, cy = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_calc_cursor_position(&ctx, rect, "ABCDE", style, WLX_RIGHT, true, &cx, &cy);
    test_frame_end(&ctx);

    // Last glyph (E) on line 1: x = 25, advance = 5 -> end_x = 30.
    // Line 1 y = 70.
    ASSERT_EQ_F(cx, 30.0f, 0.1f);
    ASSERT_EQ_F(cy, 70.0f, 0.1f);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(text_layout) {
    RUN_TEST(right_aligned_no_spurious_wrap);
    RUN_TEST(center_aligned_no_spurious_wrap);
    RUN_TEST(wrap_right_aligned_per_line);
    RUN_TEST(cursor_consistent_with_render);
}

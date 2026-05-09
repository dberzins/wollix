// test_text_layout.c - regression tests for fitted text line/run layout with
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
// Capturing draw_text: records emitted runs during frame replay.
// ============================================================================

#define TL_MAX_CAPTURES_ 64
#define TL_CAPTURE_TEXT_MAX_ 64

static struct { char text[TL_CAPTURE_TEXT_MAX_]; float x; float y; } _tl_captures[TL_MAX_CAPTURES_];
static int _tl_capture_count = 0;

static void _tl_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)style;
    if (_tl_capture_count < TL_MAX_CAPTURES_) {
        const char *src = text ? text : "";
        size_t len = strlen(src);
        if (len >= TL_CAPTURE_TEXT_MAX_) len = TL_CAPTURE_TEXT_MAX_ - 1;
        memcpy(_tl_captures[_tl_capture_count].text, src, len);
        _tl_captures[_tl_capture_count].text[len] = '\0';
        _tl_captures[_tl_capture_count].x = x;
        _tl_captures[_tl_capture_count].y = y;
        _tl_capture_count++;
    }
}

static inline void _tl_reset_captures(void) {
    memset(_tl_captures, 0, sizeof(_tl_captures));
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

static inline bool tl_calc_cursor_at(WLX_Context *ctx, WLX_Rect rect, const char *text, WLX_Text_Style style,
    WLX_Align align, bool wrap, size_t cursor_pos, float *cursor_x, float *cursor_y) {
    return wlx_calc_cursor_position_for_text(ctx, rect, text, style, align, wrap, cursor_pos, cursor_x, cursor_y);
}

// ============================================================================
// Scissor-capture helpers for draw_text_fitted clip tests
// ============================================================================

#define TL_SCISSOR_LOG_CAP_ 32

typedef struct { int kind; WLX_Rect rect; } TL_Scissor_Entry_;

static TL_Scissor_Entry_ _tl_scissor_log[TL_SCISSOR_LOG_CAP_];
static size_t _tl_scissor_count = 0;

static void _tl_reset_scissor(void) {
    _tl_scissor_count = 0;
    memset(_tl_scissor_log, 0, sizeof(_tl_scissor_log));
}

static void _tl_begin_scissor(WLX_Rect r) {
    if (_tl_scissor_count < TL_SCISSOR_LOG_CAP_)
        _tl_scissor_log[_tl_scissor_count++] = (TL_Scissor_Entry_){1, r};
}

static void _tl_end_scissor(void) {
    WLX_Rect z = {0};
    if (_tl_scissor_count < TL_SCISSOR_LOG_CAP_)
        _tl_scissor_log[_tl_scissor_count++] = (TL_Scissor_Entry_){2, z};
}

static inline void test_ctx_init_kerning_clip(WLX_Context *ctx, float w, float h) {
    test_ctx_init_kerning_capture(ctx, w, h);
    ctx->backend.begin_scissor = _tl_begin_scissor;
    ctx->backend.end_scissor   = _tl_end_scissor;
}

// ============================================================================
// Tests
// ============================================================================

TEST(right_aligned_no_spurious_wrap) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    // Whole-run measurement: ABCDE is 5*10*0.5 - 2 = 23, so it fits.
    WLX_Rect rect = {0, 0, 25, 100};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = 0, cy = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_calc_cursor_position(&ctx, rect, "ABCDE", style, WLX_RIGHT, true, &cx, &cy);
    test_frame_end(&ctx);

    // WLX_RIGHT centers vertically and places the cursor at the line end.
    ASSERT_EQ_F(cx, 25.0f, 0.1f);
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

    // WLX_CENTER centers vertically and places the cursor at the line end.
    ASSERT_EQ_F(cx, 24.0f, 0.1f);
    ASSERT_EQ_F(cy, 45.0f, 0.1f);
}

TEST(single_line_fitted_emits_one_run) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 20};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "ABCDE", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "ABCDE");
    ASSERT_EQ_F(_tl_captures[0].x, 0.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
}

TEST(range_measure_uses_whole_run_metrics) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "ABCDE";
    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 0, strlen(text), style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(w, 23.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

TEST(range_measure_rejects_utf8_split) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "A" "\xC3\xB6" "B";
    WLX_Text_Style style = { .font_size = 10 };
    float w = 1.0f, h = 1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 0, 2, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_FALSE(ok);
    ASSERT_EQ_F(w, 0.0f, 0.1f);
    ASSERT_EQ_F(h, 0.0f, 0.1f);
}

TEST(range_measure_accepts_utf8_boundary) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "A" "\xC3\xB6" "B";
    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 1, 3, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(w, 10.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

TEST(wrap_right_aligned_per_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    // rect.w = 20. ABCD measures 18 and ABCDE measures 23, so it wraps
    // after the largest fitting whole-run prefix.
    WLX_Rect rect = {10, 20, 20, 100};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "ABCDE", style, WLX_RIGHT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _tl_capture_count);

    // Vertical: total_h = 2*10 = 20, y_start = 20 + (100-20)/2 = 60.
    // Line 0 (ABCD, width=18): right-aligned x = 10 + (20-18) = 12.
    ASSERT_EQ_STR(_tl_captures[0].text, "ABCD");
    ASSERT_EQ_F(_tl_captures[0].x, 12.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[0].y, 60.0f, 0.1f);

    // Line 1 (E, width=5): right-aligned x = 10 + (20-5) = 25.
    ASSERT_EQ_STR(_tl_captures[1].text, "E");
    ASSERT_EQ_F(_tl_captures[1].x, 25.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[1].y, 70.0f, 0.1f);

    // Both lines end at rect.x + rect.w = 30.
    float line0_end = _tl_captures[0].x + 18.0f;
    float line1_end = _tl_captures[1].x + 5.0f;
    ASSERT_EQ_F(line0_end, 30.0f, 0.1f);
    ASSERT_EQ_F(line1_end, 30.0f, 0.1f);
}

TEST(no_wrap_overflow_stops_after_first_fitting_prefix) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 10, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "ABCDE", style, WLX_TOP_LEFT, false);
    ASSERT_TRUE(wlx_calc_cursor_position(&ctx, rect, "ABCDE", style, WLX_TOP_LEFT, false, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "AB");
    ASSERT_EQ_F(_tl_captures[0].x, 0.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_F(cx, 10.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);
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

    // Last run (E) on line 1: x = 25, advance = 5 -> end_x = 30.
    // Line 1 y = 70.
    ASSERT_EQ_F(cx, 30.0f, 0.1f);
    ASSERT_EQ_F(cy, 70.0f, 0.1f);
}

TEST(cursor_positions_within_single_line) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "ABCDE";
    WLX_Rect rect = {0, 0, 100, 20};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 0, &cx, &cy));
    ASSERT_EQ_F(cx, 0.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 2, &cx, &cy));
    ASSERT_EQ_F(cx, 10.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, strlen(text), &cx, &cy));
    ASSERT_EQ_F(cx, 23.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);

    test_frame_end(&ctx);
}

TEST(cursor_wrap_boundary_stays_on_previous_line) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {10, 20, 20, 100};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, "ABCDE", style, WLX_TOP_LEFT, true, 4, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, 28.0f, 0.1f);
    ASSERT_EQ_F(cy, 20.0f, 0.1f);
}

TEST(cursor_newline_separator_and_next_line_positions) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "A\r\nB";
    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 1, &cx, &cy));
    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 2, &cx, &cy));
    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);

    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 3, &cx, &cy));
    ASSERT_EQ_F(cx, 0.0f, 0.1f);
    ASSERT_EQ_F(cy, 10.0f, 0.1f);

    test_frame_end(&ctx);
}

TEST(cursor_forced_codepoint_progress_position) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {0, 0, 0, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, "AB", style, WLX_TOP_LEFT, true, 1, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);
}

TEST(cursor_normalizes_utf8_split_boundary) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "A" "\xC3\xB6" "B";
    WLX_Rect rect = {0, 0, 100, 20};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(tl_calc_cursor_at(&ctx, rect, text, style, WLX_TOP_LEFT, true, 2, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);
}

TEST(explicit_newline_starts_new_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "AB\nC", style, WLX_TOP_LEFT, false);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "AB");
    ASSERT_EQ_F(_tl_captures[0].x, 0.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[1].text, "C");
    ASSERT_EQ_F(_tl_captures[1].x, 0.0f, 0.1f);
    ASSERT_EQ_F(_tl_captures[1].y, 10.0f, 0.1f);
}

TEST(partially_visible_last_line_still_emits) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 15};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "A\nB\nC", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "A");
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[1].text, "B");
    ASSERT_EQ_F(_tl_captures[1].y, 10.0f, 0.1f);
}

TEST(trailing_newline_preserves_empty_visual_line) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_calc_cursor_position(&ctx, rect, "A\n", style, WLX_TOP_LEFT, true, &cx, &cy);
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, 0.0f, 0.1f);
    ASSERT_EQ_F(cy, 10.0f, 0.1f);
}

TEST(leading_newline_preserves_empty_visual_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "\nA", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "A");
    ASSERT_EQ_F(_tl_captures[0].y, 10.0f, 0.1f);
}

TEST(consecutive_newlines_preserve_empty_visual_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "A\n\nB", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "A");
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[1].text, "B");
    ASSERT_EQ_F(_tl_captures[1].y, 20.0f, 0.1f);
}

TEST(empty_string_cursor_stays_at_aligned_origin) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    WLX_Rect rect = {2, 4, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_calc_cursor_position(&ctx, rect, "", style, WLX_TOP_LEFT, true, &cx, &cy);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(cx, 2.0f, 0.1f);
    ASSERT_EQ_F(cy, 4.0f, 0.1f);
}

TEST(carriage_return_newlines_start_new_lines) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 100, 40};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "A\r\nB\rC", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(3, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "A");
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[1].text, "B");
    ASSERT_EQ_F(_tl_captures[1].y, 10.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[2].text, "C");
    ASSERT_EQ_F(_tl_captures[2].y, 20.0f, 0.1f);
}

TEST(narrow_rect_forces_codepoint_progress) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();

    WLX_Rect rect = {0, 0, 0, 30};
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, rect, "AB", style, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _tl_capture_count);
    ASSERT_EQ_STR(_tl_captures[0].text, "A");
    ASSERT_EQ_F(_tl_captures[0].y, 0.0f, 0.1f);
    ASSERT_EQ_STR(_tl_captures[1].text, "B");
    ASSERT_EQ_F(_tl_captures[1].y, 10.0f, 0.1f);
}

TEST(cursor_falls_back_to_last_visible_line_when_line_cap_hits) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 2000);

    char text[WLX_TEXT_RUN_MAX_LINES + 33];
    size_t text_len = sizeof(text) - 1;
    memset(text, 'A', text_len);
    text[text_len] = '\0';

    WLX_Rect rect = {0, 0, 0, 2000};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(wlx_calc_cursor_position(&ctx, rect, text, style, WLX_TOP_LEFT, true, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, (float)(WLX_TEXT_RUN_MAX_LINES - 1) * 10.0f, 0.1f);
}

TEST(cursor_stops_at_text_unit_cap_on_single_line) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 4000, 40);

    char text[WLX_TEXT_RUN_MAX_UNITS + 17];
    size_t text_len = sizeof(text) - 1;
    memset(text, 'A', text_len);
    text[text_len] = '\0';

    WLX_Rect rect = {0, 0, 3000, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(wlx_calc_cursor_position(&ctx, rect, text, style, WLX_TOP_LEFT, true, &cx, &cy));
    test_frame_end(&ctx);

    ASSERT_EQ_F(cx, (float)WLX_TEXT_RUN_MAX_UNITS * 5.0f - 2.0f, 0.1f);
    ASSERT_EQ_F(cy, 0.0f, 0.1f);
}

TEST(range_measure_accepts_invalid_utf8_fallback_byte) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char text[] = { 'A', (char)0x80, 'B', '\0' };
    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 1, 2, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(w, 5.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

TEST(range_measure_accepts_long_invalid_utf8_fallback_bytes) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    char text[WLX_TEXT_RANGE_STACK_CAP + 33];
    size_t text_len = sizeof(text) - 1;
    memset(text, (char)0x80, text_len);
    text[text_len] = '\0';

    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, text_len, 0, text_len, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(w, (float)text_len * 5.0f - 2.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

TEST(invalid_utf8_fallback_bytes_make_progress) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char text[] = { 'A', (char)0x80, 'B', '\0' };
    WLX_Rect rect = {0, 0, 5, 40};
    WLX_Text_Style style = { .font_size = 10 };
    float cx = -1.0f, cy = -1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_calc_cursor_position(&ctx, rect, text, style, WLX_TOP_LEFT, true, &cx, &cy);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(cx, 5.0f, 0.1f);
    ASSERT_EQ_F(cy, 20.0f, 0.1f);
}

// ============================================================================
// Conditional scissor coverage for wlx_draw_text_fitted
// ============================================================================

TEST(fitted_text_skips_scissor_when_lines_fit) {
    WLX_Context ctx;
    test_ctx_init_kerning_clip(&ctx, 200, 50);
    _tl_reset_scissor();
    _tl_reset_captures();

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_label(&ctx, "Hello", .font_size = 10, .height = 50);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, (int)_tl_scissor_count);
    ASSERT_TRUE(_tl_capture_count >= 1);
}

TEST(fitted_text_scissors_when_first_unit_exceeds_width) {
    WLX_Context ctx;
    test_ctx_init_kerning_clip(&ctx, 200, 50);
    _tl_reset_scissor();
    _tl_reset_captures();

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, (WLX_Rect){10, 20, 3, 20}, "A", (WLX_Text_Style){ .font_size = 10 }, WLX_TOP_LEFT, false);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, (int)_tl_scissor_count);
    ASSERT_EQ_INT(_tl_scissor_log[0].kind, 1);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.x, 10.0f, 0.1f);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.y, 20.0f, 0.1f);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.w, 3.0f, 0.1f);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.h, 20.0f, 0.1f);
    ASSERT_EQ_INT(_tl_scissor_log[1].kind, 2);
}

TEST(fitted_text_scissors_partially_visible_line) {
    WLX_Context ctx;
    test_ctx_init_kerning_clip(&ctx, 200, 50);
    _tl_reset_scissor();
    _tl_reset_captures();

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_text_fitted(&ctx, (WLX_Rect){0, 0, 10, 15}, "ABCDE", (WLX_Text_Style){ .font_size = 10 }, WLX_TOP_LEFT, true);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, (int)_tl_scissor_count);
    ASSERT_EQ_INT(_tl_scissor_log[0].kind, 1);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.w, 10.0f, 0.1f);
    ASSERT_EQ_F(_tl_scissor_log[0].rect.h, 15.0f, 0.1f);
    ASSERT_EQ_INT(_tl_scissor_log[1].kind, 2);
}

// ============================================================================
// Slice-aware span callback tests
// ============================================================================

// Capture for slice measure calls
static size_t _tl_slice_measure_called_len = 0;
static char _tl_slice_measure_text[64];
static void _tl_capture_measure_slice(const char *text, size_t len, WLX_Text_Style style,
                                       float *out_w, float *out_h) {
    _tl_slice_measure_called_len = len;
    size_t copy_len = len < 63 ? len : 63;
    memcpy(_tl_slice_measure_text, text, copy_len);
    _tl_slice_measure_text[copy_len] = '\0';
    // Produce a consistent measurement via a null-terminated copy
    char buf[64];
    if (len < sizeof(buf)) {
        memcpy(buf, text, len);
        buf[len] = '\0';
        mock_measure_text(buf, style, out_w, out_h);
    }
}

// Capture for slice draw calls
static size_t _tl_slice_draw_called_len = 0;
static char _tl_slice_draw_text[64];
static float _tl_slice_draw_x = -999.0f;
static float _tl_slice_draw_y = -999.0f;
static void _tl_capture_draw_slice(const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    (void)style;
    _tl_slice_draw_called_len = len;
    size_t copy_len = len < 63 ? len : 63;
    memcpy(_tl_slice_draw_text, text, copy_len);
    _tl_slice_draw_text[copy_len] = '\0';
    _tl_slice_draw_x = x;
    _tl_slice_draw_y = y;
}

static void _tl_reset_slice_captures(void) {
    _tl_slice_measure_called_len = 0;
    _tl_slice_measure_text[0] = '\0';
    _tl_slice_draw_called_len = 0;
    _tl_slice_draw_text[0] = '\0';
    _tl_slice_draw_x = -999.0f;
    _tl_slice_draw_y = -999.0f;
}

// span_measure_prefers_slice_callback: when measure_text_slice is set, it is
// called instead of the fallback path.
TEST(span_measure_prefers_slice_callback) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);
    _tl_reset_slice_captures();
    ctx.backend.measure_text_slice = _tl_capture_measure_slice;

    const char *text = "ABCDE";
    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 0, 3, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_INT((int)_tl_slice_measure_called_len, 3);
    ASSERT_EQ_F(w, 3.0f * 10.0f * 0.5f, 0.1f); // "ABC" = 3 * 5 = 15
}

// span_draw_immediate_prefers_slice_callback: in immediate mode with
// draw_text_slice set, the slice callback is called with the correct length.
TEST(span_draw_immediate_prefers_slice_callback) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_slice_captures();
    _tl_reset_captures();
    ctx.backend.draw_text_slice = _tl_capture_draw_slice;

    const char *text = "HELLO";
    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_draw_text_range(&ctx, text, strlen(text), 1, 4, 5.0f, 10.0f, style);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_INT((int)_tl_slice_draw_called_len, 3);
    ASSERT_EQ_INT(_tl_capture_count, 0); // draw_text not called
    ASSERT_EQ_F(_tl_slice_draw_x, 5.0f, 0.1f);
    ASSERT_EQ_F(_tl_slice_draw_y, 10.0f, 0.1f);
}

// span_measure_fallback_null_text: wlx_measure_text_range with NULL text is
// normalized to an empty span; returns true with w=0, h=font_size.
TEST(span_measure_fallback_null_text) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    float w = 1.0f, h = 1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, NULL, 0, 0, 0, (WLX_Text_Style){ .font_size = 10 }, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_F(w, 0.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f); // empty span produces height = font_size
}

// span_measure_fallback_empty_span: zero-length range returns immediately.
TEST(span_measure_fallback_empty_span) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);

    const char *text = "ABC";
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 1, 1, (WLX_Text_Style){ .font_size = 10 }, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    // empty range produces the empty-string measure: w=0, h=font_size
    ASSERT_EQ_F(w, 0.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

// span_range_routes_through_span_helpers: with slice callback set, range
// draw and measure agree with each other (parity).
TEST(span_range_routes_through_span_helpers) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);
    _tl_reset_slice_captures();
    ctx.backend.measure_text_slice = _tl_capture_measure_slice;

    const char *text = "XYZ";
    WLX_Text_Style style = { .font_size = 10 };
    float w_slice = 0.0f, h_slice = 0.0f;
    float w_plain = 0.0f, h_plain = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_measure_text_range(&ctx, text, strlen(text), 0, strlen(text), style, &w_slice, &h_slice);
    test_frame_end(&ctx);

    // Without slice callback for comparison
    ctx.backend.measure_text_slice = NULL;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_measure_text_range(&ctx, text, strlen(text), 0, strlen(text), style, &w_plain, &h_plain);
    test_frame_end(&ctx);

    ASSERT_EQ_F(w_slice, w_plain, 0.1f);
    ASSERT_EQ_F(h_slice, h_plain, 0.1f);
}

// span_measure_long_range_fallback: range > WLX_TEXT_RANGE_STACK_CAP still
// measures correctly via heap allocation when no slice callback is set.
TEST(span_measure_long_range_fallback) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);
    // No slice callback; fallback path must heap-allocate for long ranges.
    ctx.backend.measure_text_slice = NULL;

    // Build a string longer than WLX_TEXT_RANGE_STACK_CAP (1024 bytes)
    static char long_text[1100];
    memset(long_text, 'A', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    size_t text_len = sizeof(long_text) - 1;

    WLX_Text_Style style = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, long_text, text_len, 0, text_len, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    // kerning mock: text_len * 5.0 - 2.0 for len > 3
    ASSERT_EQ_F(w, (float)text_len * 5.0f - 2.0f, 0.1f);
    ASSERT_EQ_F(h, 10.0f, 0.1f);
}

// span_draw_range_long_range_fallback: drawing a range > stack cap in
// immediate mode (no slice callback) still completes without error.
TEST(span_draw_range_long_range_fallback) {
    WLX_Context ctx;
    test_ctx_init_kerning_capture(&ctx, 200, 200);
    _tl_reset_captures();
    ctx.backend.draw_text_slice = NULL;

    static char long_text[1100];
    memset(long_text, 'B', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    size_t text_len = sizeof(long_text) - 1;

    WLX_Text_Style style = { .font_size = 10 };

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_draw_text_range(&ctx, long_text, text_len, 0, text_len, 0.0f, 0.0f, style);
    test_frame_end(&ctx);

    ASSERT_TRUE(ok);
    ASSERT_EQ_INT(1, _tl_capture_count);
}

// span_utf8_boundary_still_rejected_at_range_level: even with slice callbacks
// set, a split UTF-8 boundary is rejected before the callback is invoked.
TEST(span_utf8_boundary_still_rejected_at_range_level) {
    WLX_Context ctx;
    test_ctx_init_kerning(&ctx, 200, 200);
    _tl_reset_slice_captures();
    ctx.backend.measure_text_slice = _tl_capture_measure_slice;
    _tl_slice_measure_called_len = 99; // sentinel

    const char *text = "A" "\xC3\xB6" "B"; // U+00F6 is 2 bytes
    WLX_Text_Style style = { .font_size = 10 };
    float w = 1.0f, h = 1.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    bool ok = wlx_measure_text_range(&ctx, text, strlen(text), 0, 2, style, &w, &h);
    test_frame_end(&ctx);

    ASSERT_FALSE(ok);
    ASSERT_EQ_F(w, 0.0f, 0.1f);
    ASSERT_EQ_F(h, 0.0f, 0.1f);
    ASSERT_EQ_INT((int)_tl_slice_measure_called_len, 99); // callback not invoked
}

SUITE(text_layout) {
    RUN_TEST(right_aligned_no_spurious_wrap);
    RUN_TEST(center_aligned_no_spurious_wrap);
    RUN_TEST(single_line_fitted_emits_one_run);
    RUN_TEST(range_measure_uses_whole_run_metrics);
    RUN_TEST(range_measure_rejects_utf8_split);
    RUN_TEST(range_measure_accepts_utf8_boundary);
    RUN_TEST(wrap_right_aligned_per_line);
    RUN_TEST(no_wrap_overflow_stops_after_first_fitting_prefix);
    RUN_TEST(cursor_consistent_with_render);
    RUN_TEST(cursor_positions_within_single_line);
    RUN_TEST(cursor_wrap_boundary_stays_on_previous_line);
    RUN_TEST(cursor_newline_separator_and_next_line_positions);
    RUN_TEST(cursor_forced_codepoint_progress_position);
    RUN_TEST(cursor_normalizes_utf8_split_boundary);
    RUN_TEST(explicit_newline_starts_new_line);
    RUN_TEST(partially_visible_last_line_still_emits);
    RUN_TEST(trailing_newline_preserves_empty_visual_line);
    RUN_TEST(leading_newline_preserves_empty_visual_line);
    RUN_TEST(consecutive_newlines_preserve_empty_visual_line);
    RUN_TEST(carriage_return_newlines_start_new_lines);
    RUN_TEST(empty_string_cursor_stays_at_aligned_origin);
    RUN_TEST(narrow_rect_forces_codepoint_progress);
    RUN_TEST(cursor_falls_back_to_last_visible_line_when_line_cap_hits);
    RUN_TEST(cursor_stops_at_text_unit_cap_on_single_line);
    RUN_TEST(range_measure_accepts_invalid_utf8_fallback_byte);
    RUN_TEST(range_measure_accepts_long_invalid_utf8_fallback_bytes);
    RUN_TEST(invalid_utf8_fallback_bytes_make_progress);
    RUN_TEST(fitted_text_skips_scissor_when_lines_fit);
    RUN_TEST(fitted_text_scissors_when_first_unit_exceeds_width);
    RUN_TEST(fitted_text_scissors_partially_visible_line);
    RUN_TEST(span_measure_prefers_slice_callback);
    RUN_TEST(span_draw_immediate_prefers_slice_callback);
    RUN_TEST(span_measure_fallback_null_text);
    RUN_TEST(span_measure_fallback_empty_span);
    RUN_TEST(span_range_routes_through_span_helpers);
    RUN_TEST(span_measure_long_range_fallback);
    RUN_TEST(span_draw_range_long_range_fallback);
    RUN_TEST(span_utf8_boundary_still_rejected_at_range_level);
}

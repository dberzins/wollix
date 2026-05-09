// test_input.c - inputbox text editing tests (focus, typing, backspace, cursor, buffer limits)

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

// Helper: call `wlx_inputbox_impl()` from a stable source location.
// Returns true if focused (same as `wlx_inputbox_impl()`'s return value).
// The caller must have an active layout.
static bool do_inputbox_A(WLX_Context *ctx, char *buf, size_t buf_size) {
    return wlx_inputbox_impl(ctx, "Label:", buf, buf_size,
        wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10),
        __FILE__, __LINE__);
}

static bool do_inputbox_B(WLX_Context *ctx, char *buf, size_t buf_size) {
    return wlx_inputbox_impl(ctx, "Other:", buf, buf_size,
        wlx_default_inputbox_opt(.height = 40, .content_padding = 4, .font_size = 10),
        __FILE__, __LINE__);
}

static int _input_cursor_draw_count = 0;
static float _input_cursor_x1 = 0.0f;
static float _input_cursor_y1 = 0.0f;
static float _input_cursor_x2 = 0.0f;
static float _input_cursor_y2 = 0.0f;
static int _input_text_draw_count = 0;
static char _input_text_draw_text[64];

static void input_capture_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    (void)thick;
    (void)c;
    _input_cursor_draw_count++;
    _input_cursor_x1 = x1;
    _input_cursor_y1 = y1;
    _input_cursor_x2 = x2;
    _input_cursor_y2 = y2;
}

static inline void input_reset_cursor_capture(void) {
    _input_cursor_draw_count = 0;
    _input_cursor_x1 = 0.0f;
    _input_cursor_y1 = 0.0f;
    _input_cursor_x2 = 0.0f;
    _input_cursor_y2 = 0.0f;
}

static void input_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)x;
    (void)y;
    (void)style;
    _input_text_draw_count++;
    const char *source = text ? text : "";
    size_t len = strlen(source);
    if (len >= sizeof(_input_text_draw_text)) len = sizeof(_input_text_draw_text) - 1;
    memcpy(_input_text_draw_text, source, len);
    _input_text_draw_text[len] = '\0';
}

static inline void input_reset_text_capture(void) {
    _input_text_draw_count = 0;
    _input_text_draw_text[0] = '\0';
}

static void input_measure_tall_text(const char *text, WLX_Text_Style style, float *out_w, float *out_h) {
    int font_size = style.font_size > 0 ? style.font_size : 10;
    size_t len = text ? strlen(text) : 0;
    if (out_w) *out_w = (float)len * (float)font_size * 0.5f;
    if (out_h) *out_h = (float)font_size + 4.0f;
}

static inline void test_ctx_init_cursor_capture(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.draw_line = input_capture_draw_line;
}

static inline void test_ctx_init_tall_text_input_capture(WLX_Context *ctx, float w, float h) {
    test_ctx_init_cursor_capture(ctx, w, h);
    ctx->backend.draw_text = input_capture_draw_text;
    ctx->backend.measure_text = input_measure_tall_text;
}

#define INPUT_CLIP_LOG_CAP 16

typedef struct {
    int kind;
    WLX_Rect rect;
} Input_Clip_Log_Entry;

static Input_Clip_Log_Entry _input_clip_log[INPUT_CLIP_LOG_CAP];
static size_t _input_clip_count = 0;

static void input_reset_clip_capture(void) {
    _input_clip_count = 0;
    memset(_input_clip_log, 0, sizeof(_input_clip_log));
}

static void input_capture_begin_scissor(WLX_Rect rect) {
    if (_input_clip_count < INPUT_CLIP_LOG_CAP) {
        _input_clip_log[_input_clip_count++] = (Input_Clip_Log_Entry){1, rect};
    }
}

static void input_capture_end_scissor(void) {
    if (_input_clip_count < INPUT_CLIP_LOG_CAP) {
        _input_clip_log[_input_clip_count++] = (Input_Clip_Log_Entry){2, {0}};
    }
}

static inline void test_ctx_init_clip_capture(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.begin_scissor = input_capture_begin_scissor;
    ctx->backend.end_scissor = input_capture_end_scissor;
}

static inline void test_ctx_init_clip_line_capture(WLX_Context *ctx, float w, float h) {
    test_ctx_init_clip_capture(ctx, w, h);
    ctx->backend.draw_line = input_capture_draw_line;
}

static bool do_inputbox_long_cursor(WLX_Context *ctx, char *buf, size_t buf_size) {
    return wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.height = 500, .content_padding = 4, .font_size = 10, .wrap = true, .border_width = 0),
        __FILE__, __LINE__);
}

// ============================================================================
// Focus tests
// ============================================================================

TEST(input_focus_on_click) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click inside the widget area.
    // The widget occupies the full ctx rect when it's the only one in a 1-slot layout.
    // Click roughly in the center.
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool focused = do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    ASSERT_TRUE(focused);
    test_frame_end(&ctx);
}

TEST(input_unfocus_enter) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press Enter -> should lose focus
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_ENTER] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool focused = do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    ASSERT_FALSE(focused);
    test_frame_end(&ctx);
}

TEST(input_unfocus_click_away) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click inside to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: click at (0,0) - for a 400x300 widget, (0,0) is on the edge
    // but still inside. We need to click truly outside the widget rect.
    // However the widget fills the full area. Let's use two inputboxes instead.
    // Actually, let's just verify focus is cleared by Enter, which we tested above.
    // For click-away, we need the click to miss the widget. Since the inputbox
    // uses wlx_get_interaction on the resolved widget rect (which may be smaller
    // than full cell due to label+padding), clicking far outside should work.
    // The widget_begin resolves rect within the cell, so a click at (0,0) should
    // be outside the actual input rect even though it's inside the layout cell.
    //
    // Actually inputbox interaction is on the whole widget rect (wr), not just
    // the input box part. So with a single full-screen input, click-away is
    // impossible. Use two inputboxes in separate layout slots instead.

    // Reset and use two widgets
    test_ctx_init(&ctx, 400, 300);
    buf[0] = '\0';
    char buf2[64] = "";

    // Frame 1: click inside widget A (top half, y=75)
    test_frame_begin(&ctx, 200, 75, true, true);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    bool fA1 = do_inputbox_A(&ctx, buf, sizeof(buf));
    do_inputbox_B(&ctx, buf2, sizeof(buf2));
    wlx_layout_end(&ctx);
    ASSERT_TRUE(fA1);
    test_frame_end(&ctx);

    // Frame 2: click inside widget B (bottom half, y=225)
    test_frame_begin(&ctx, 200, 225, true, true);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    bool fA2 = do_inputbox_A(&ctx, buf, sizeof(buf));
    bool fB2 = do_inputbox_B(&ctx, buf2, sizeof(buf2));
    wlx_layout_end(&ctx);
    ASSERT_FALSE(fA2);   // A lost focus
    ASSERT_TRUE(fB2);    // B gained focus
    test_frame_end(&ctx);
}

// ============================================================================
// Typing tests
// ============================================================================

TEST(input_type_char) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type 'A'
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "A");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "A");
}

TEST(input_type_multiple_chars) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type "Hi"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "Hi");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "Hi");
}

TEST(input_type_across_frames) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type 'A'
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "A");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 3: type 'B'
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "B");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "AB");
}

// ============================================================================
// Backspace tests
// ============================================================================

TEST(input_backspace) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABC";

    // Frame 1: click to focus (cursor starts at end, pos=3)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press backspace
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_BACKSPACE] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "AB");
}

TEST(input_backspace_empty) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: backspace on empty - should be no-op
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_BACKSPACE] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "");
}

// ============================================================================
// Cursor movement tests
// ============================================================================

TEST(input_cursor_move_left_right) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "AB";

    // Frame 1: focus (cursor at end, pos=2)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press LEFT (cursor moves to pos=1)
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 3: type 'X' at cursor pos=1 -> "AXB"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "X");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "AXB");
}

TEST(input_cursor_move_left_insert) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "CD";

    // Frame 1: focus (cursor at end, pos=2)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press LEFT twice -> cursor at pos=0
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // second LEFT press
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 4: type 'Z' at pos=0 -> "ZCD"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "Z");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "ZCD");
}

// ============================================================================
// Buffer overflow protection
// ============================================================================

TEST(input_buffer_overflow) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // Tiny buffer: only 4 chars + null
    char buf[5] = "";

    // Frame 1: focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type "ABCDE" - should only fit 4 chars
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "ABCDE");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT((int)strlen(buf), 4);
    ASSERT_EQ_STR(buf, "ABCD");
}

TEST(input_no_type_when_unfocused) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: no click -> not focused, send text input
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "X");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool focused = do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    ASSERT_FALSE(focused);
    test_frame_end(&ctx);

    // Buffer should still be empty
    ASSERT_EQ_STR(buf, "");
}

TEST(input_cursor_position_uses_full_buffer_layout) {
    WLX_Context ctx;
    test_ctx_init_cursor_capture(&ctx, 74, 500);
    char buf[700];
    char legacy_prefix[WLX_INPUTBOX_CURSOR_TEMP_SIZE];
    WLX_Text_Style ts = { .font_size = 10 };
    WLX_Rect wr = {0, 0, 74, 500};
    WLX_Rect input_rect = { wr.x + 4, wr.y + 4, wr.w - 8, wr.h - 8 };
    WLX_Rect text_rect = {
        .x = input_rect.x + 2,
        .y = input_rect.y,
        .w = input_rect.w - 6,
        .h = input_rect.h,
    };
    float expected_x = text_rect.x;
    float expected_y = text_rect.y;
    float legacy_x = text_rect.x;
    float legacy_y = text_rect.y;

    memset(buf, 'A', 520);
    buf[520] = '\0';
    input_reset_cursor_capture();

    test_frame_begin(&ctx, 20, 250, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = do_inputbox_long_cursor(&ctx, buf, sizeof(buf));
    ASSERT_TRUE(focused);

    ASSERT_TRUE(wlx_calc_cursor_position_for_text(&ctx, text_rect, buf, ts, WLX_TOP_LEFT, true, 520,
        &expected_x, &expected_y));
    if (expected_x > text_rect.x) expected_x += WLX_INPUTBOX_CURSOR_PADDING;

    memcpy(legacy_prefix, buf, WLX_INPUTBOX_CURSOR_TEMP_SIZE - 1);
    legacy_prefix[WLX_INPUTBOX_CURSOR_TEMP_SIZE - 1] = '\0';
    ASSERT_TRUE(wlx_calc_cursor_position(&ctx, text_rect, legacy_prefix, ts, WLX_TOP_LEFT, true,
        &legacy_x, &legacy_y));
    if (legacy_x > text_rect.x) legacy_x += WLX_INPUTBOX_CURSOR_PADDING;

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(1, _input_cursor_draw_count);
    ASSERT_EQ_F(_input_cursor_x1, expected_x, 0.1f);
    ASSERT_EQ_F(_input_cursor_y1, expected_y, 0.1f);
    ASSERT_TRUE(fabsf(expected_x - legacy_x) > 0.1f || fabsf(expected_y - legacy_y) > 0.1f);
    ASSERT_EQ_F(_input_cursor_x2, expected_x, 0.1f);
    ASSERT_EQ_F(_input_cursor_y2, expected_y + 10.0f, 0.1f);
}

TEST(input_text_clip_restores_scroll_panel_clip) {
    WLX_Context ctx;
    test_ctx_init_clip_capture(&ctx, 200, 120);
    char buf[128] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    input_reset_clip_capture();

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(&ctx, 80, .height = 80, .show_scrollbar = false);
    wlx_inputbox(&ctx, NULL, buf, sizeof(buf),
        .height = 30, .content_padding = 4, .font_size = 10,
        .wrap = true, .border_width = 0);
    wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_input_clip_count >= 5);
    ASSERT_EQ_INT(_input_clip_log[0].kind, 1);
    ASSERT_EQ_INT(_input_clip_log[1].kind, 1);
    ASSERT_EQ_INT(_input_clip_log[2].kind, 2);
    ASSERT_EQ_INT(_input_clip_log[3].kind, 1);
    ASSERT_EQ_INT(_input_clip_log[4].kind, 2);
    ASSERT_EQ_RECT(_input_clip_log[3].rect, _input_clip_log[0].rect, 0.1f);
    ASSERT_TRUE(_input_clip_log[1].rect.w < _input_clip_log[0].rect.w);
    ASSERT_TRUE(_input_clip_log[1].rect.h < _input_clip_log[0].rect.h);
}

TEST(inputbox_draws_when_line_height_exceeds_text_rect) {
    WLX_Context ctx;
    test_ctx_init_tall_text_input_capture(&ctx, 80, 40);
    char buf[64] = "abc";

    input_reset_cursor_capture();
    input_reset_text_capture();

    test_frame_begin(&ctx, 40, 20, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
        wlx_default_inputbox_opt(.height = 40, .content_padding = 10,
            .font_size = 16, .wrap = false, .border_width = 1),
        __FILE__, __LINE__);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(focused);
    ASSERT_EQ_INT(1, _input_text_draw_count);
    ASSERT_EQ_STR(_input_text_draw_text, "abc");
    ASSERT_EQ_INT(1, _input_cursor_draw_count);
    ASSERT_EQ_F(_input_cursor_x1, 41.0f, 0.1f);
    ASSERT_EQ_F(_input_cursor_y1, 12.0f, 0.1f);
}

// ============================================================================
// UTF-8 typing tests
// ============================================================================

TEST(input_type_utf8_2byte) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type "ö" (0xC3 0xB6 = 2 bytes)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "\xC3\xB6");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "\xC3\xB6");
    ASSERT_EQ_INT(2, strlen(buf));
}

TEST(input_type_utf8_mixed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type "A"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "A");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 3: type "ö"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "\xC3\xB6");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "A\xC3\xB6");
}

// ============================================================================
// UTF-8 cursor movement tests
// ============================================================================

TEST(input_cursor_utf8_left) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // "Aö" = 1 + 2 = 3 bytes
    char buf[64] = "A\xC3\xB6";

    // Frame 1: click to focus (cursor at end = byte 3)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press LEFT -> cursor should jump to byte 1 (before ö, skipping 2 bytes)
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 3: press LEFT again -> cursor at byte 0
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 4: type "X" at position 0 -> "XAö"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "X");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "XA\xC3\xB6");
}

TEST(input_cursor_utf8_right) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // "öB" = 2 + 1 = 3 bytes
    char buf[64] = "\xC3\xB6\x42";

    // Frame 1: click to focus (cursor at end = byte 3)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press LEFT twice -> cursor at byte 0
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 4: press RIGHT -> cursor should jump to byte 2 (after ö)
    bool keys_right[WLX_KEY_COUNT] = {0};
    keys_right[WLX_KEY_RIGHT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_right, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 5: type "X" at byte 2 -> "öXB"
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "X");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "\xC3\xB6XB");
}

// ============================================================================
// UTF-8 backspace tests
// ============================================================================

TEST(input_backspace_utf8) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // "AöB" = 1 + 2 + 1 = 4 bytes
    char buf[64] = "A\xC3\xB6\x42";

    // Frame 1: click to focus (cursor at end = byte 4)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: press LEFT -> cursor at byte 3 (after ö, before B)
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 3: press Backspace -> should delete ö (2 bytes), leaving "AB"
    bool keys_bs[WLX_KEY_COUNT] = {0};
    keys_bs[WLX_KEY_BACKSPACE] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_bs, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "AB");
}

TEST(input_backspace_utf8_at_start) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // "öB" = 2 + 1 = 3 bytes
    char buf[64] = "\xC3\xB6\x42";

    // Frame 1: click to focus (cursor at end = byte 3)
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2-3: press LEFT twice -> cursor at byte 0
    bool keys_left[WLX_KEY_COUNT] = {0};
    keys_left[WLX_KEY_LEFT] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_left, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 4: press Backspace at position 0 -> no-op
    bool keys_bs[WLX_KEY_COUNT] = {0};
    keys_bs[WLX_KEY_BACKSPACE] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, keys_bs, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "\xC3\xB6\x42");
}

// ============================================================================
// UTF-8 buffer overflow test
// ============================================================================

TEST(input_utf8_buffer_full) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // Buffer of 4: can hold "ö" (2 bytes) + NUL, but not "ö" + "€" (2+3=5 > 3 usable)
    char buf[4] = "";

    // Frame 1: click to focus
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Frame 2: type "ö" (2 bytes) - should fit (2 + NUL = 3 <= 4)
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "\xC3\xB6");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "\xC3\xB6");

    // Frame 3: type "€" (3 bytes) - doesn't fit (2 + 3 = 5 > 3 usable), buffer unchanged
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "\xE2\x82\xAC");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "\xC3\xB6");
}

// ============================================================================
// Inputbox cursor scissor coverage
// ============================================================================

// Fitted text clips only when a drawn run would overflow. The inputbox cursor
// is a line primitive, so it keeps its own narrow scope whenever it is visible.
TEST(inputbox_cursor_uses_scissor_scope) {
    WLX_Context ctx;
    test_ctx_init_clip_line_capture(&ctx, 400, 60);
    char buf[64] = "abc";

    input_reset_clip_capture();
    input_reset_cursor_capture();

    // Frame 1: click to focus so the cursor is drawn on this frame.
    test_frame_begin(&ctx, 200, 30, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
        wlx_default_inputbox_opt(.height = 60, .content_padding = 4,
            .font_size = 10, .wrap = false, .border_width = 0),
        __FILE__, __LINE__);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // The cursor blink_time starts at 0 and frame_time is 0 in tests,
    // so 0 < 0.5 * 1.0: cursor is visible.
    ASSERT_EQ_INT(_input_cursor_draw_count, 1);

    ASSERT_TRUE(_input_clip_count >= 2);

    int cursor_begin = -1;
    for (size_t i = 0; i < _input_clip_count; i++) {
        if (_input_clip_log[i].kind == 1) { cursor_begin = (int)i; break; }
    }
    ASSERT_TRUE(cursor_begin >= 0);

    int cursor_end = -1;
    for (size_t i = (size_t)(cursor_begin + 1); i < _input_clip_count; i++) {
        if (_input_clip_log[i].kind == 2) { cursor_end = (int)i; break; }
    }
    ASSERT_TRUE(cursor_end > cursor_begin);
}

TEST(inputbox_typing_resets_cursor_blink) {
    WLX_Context ctx;
    test_ctx_init_cursor_capture(&ctx, 400, 300);
    char buf[64] = "";

    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    for (int frame_index = 0; frame_index < 40; frame_index++) {
        test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                             NULL, NULL, NULL);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        do_inputbox_A(&ctx, buf, sizeof(buf));
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    input_reset_cursor_capture();
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                         NULL, NULL, "X");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    do_inputbox_A(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "X");
    ASSERT_EQ_INT(1, _input_cursor_draw_count);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input) {
    // Focus
    RUN_TEST(input_focus_on_click);
    RUN_TEST(input_unfocus_enter);
    RUN_TEST(input_unfocus_click_away);

    // Typing
    RUN_TEST(input_type_char);
    RUN_TEST(input_type_multiple_chars);
    RUN_TEST(input_type_across_frames);

    // Backspace
    RUN_TEST(input_backspace);
    RUN_TEST(input_backspace_empty);

    // Cursor movement
    RUN_TEST(input_cursor_move_left_right);
    RUN_TEST(input_cursor_move_left_insert);

    // Buffer limits
    RUN_TEST(input_buffer_overflow);
    RUN_TEST(input_no_type_when_unfocused);
    RUN_TEST(input_cursor_position_uses_full_buffer_layout);
    RUN_TEST(input_text_clip_restores_scroll_panel_clip);
    RUN_TEST(inputbox_draws_when_line_height_exceeds_text_rect);

    // Scissor scope structure
    RUN_TEST(inputbox_cursor_uses_scissor_scope);
    RUN_TEST(inputbox_typing_resets_cursor_blink);

    // UTF-8 typing
    RUN_TEST(input_type_utf8_2byte);
    RUN_TEST(input_type_utf8_mixed);

    // UTF-8 cursor movement
    RUN_TEST(input_cursor_utf8_left);
    RUN_TEST(input_cursor_utf8_right);

    // UTF-8 backspace
    RUN_TEST(input_backspace_utf8);
    RUN_TEST(input_backspace_utf8_at_start);

    // UTF-8 buffer overflow
    RUN_TEST(input_utf8_buffer_full);
}

// test_input_multiline.c - inputbox multiline mode (ADR_032): Enter inserts a
// newline and keeps focus (incl. OS auto-repeat), selection replacement,
// single-line Enter-blur retention, read_only rejection with focus held,
// password exclusion in resolve, Escape blur, and full-buffer truncation.

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

// ============================================================================
// Fixture
// ============================================================================

static bool ml_inputbox(WLX_Context *ctx, char *buf, size_t buf_size,
                        bool multiline, bool read_only) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0,
            .multiline = multiline, .read_only = read_only,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool ml_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size,
                           bool multiline, bool read_only,
                           int mx, bool down, bool clicked) {
    test_frame_begin(ctx, mx, 150, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ml_inputbox(ctx, buf, buf_size, multiline, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool ml_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                         bool multiline, bool read_only,
                         WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ml_inputbox(ctx, buf, buf_size, multiline, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

// An OS auto-repeat tick: the key is held down and repeated, not freshly
// pressed.
static bool ml_frame_key_repeat(WLX_Context *ctx, char *buf, size_t buf_size,
                                bool multiline, bool read_only,
                                WLX_Key_Code key) {
    bool keys_down[WLX_KEY_COUNT] = {0};
    bool keys_repeated[WLX_KEY_COUNT] = {0};
    keys_down[key] = true;
    keys_repeated[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          keys_down, NULL, keys_repeated, 0, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ml_inputbox(ctx, buf, buf_size, multiline, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static void ml_frame_type(WLX_Context *ctx, char *buf, size_t buf_size,
                          bool multiline, bool read_only, const char *text) {
    test_frame_begin_ex(ctx, 200, 150, false, false, false, 0.0f,
                        NULL, NULL, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ml_inputbox(ctx, buf, buf_size, multiline, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// ============================================================================
// Enter behavior
// ============================================================================

TEST(multiline_enter_inserts_newline_keeps_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ab";

    // Click near the right edge -> caret at end of "ab".
    bool focused = ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ASSERT_TRUE(focused);

    // Enter inserts "\n" at the caret and the field stays focused.
    focused = ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "ab\n");
    ASSERT_TRUE(focused);

    // Subsequent typing lands after the newline.
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "c");
    ASSERT_EQ_STR(buf, "ab\nc");
    wlx_context_destroy(&ctx);
}

TEST(multiline_enter_autorepeat_inserts_again) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "x";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "x\n");

    // Held Enter: an auto-repeat tick inserts another newline and keeps focus.
    bool focused = ml_frame_key_repeat(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ENTER);
    ASSERT_EQ_STR(buf, "x\n\n");
    ASSERT_TRUE(focused);
    wlx_context_destroy(&ctx);
}

TEST(multiline_enter_replaces_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "hello";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_A, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "\n");
    wlx_context_destroy(&ctx);
}

TEST(singleline_enter_blurs_no_insert) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ab";

    bool focused = ml_frame_mouse(&ctx, buf, sizeof(buf), false, false, 380, true, true);
    ASSERT_TRUE(focused);

    // Default mode keeps the pre-ADR_032 contract: Enter blurs, no newline.
    focused = ml_frame_key(&ctx, buf, sizeof(buf), false, false, WLX_KEY_ENTER, 0);
    ASSERT_FALSE(focused);
    ASSERT_EQ_STR(buf, "ab");
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Mode composition
// ============================================================================

TEST(multiline_readonly_rejects_insert_keeps_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ab";

    bool focused = ml_frame_mouse(&ctx, buf, sizeof(buf), true, true, 380, true, true);
    ASSERT_TRUE(focused);

    focused = ml_frame_key(&ctx, buf, sizeof(buf), true, true, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "ab");
    ASSERT_TRUE(focused);
    wlx_context_destroy(&ctx);
}

// Stable call site so the password+multiline widget keeps one ID across
// frames.
static bool ml_pw_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0,
            .password = true, .multiline = true,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

TEST(password_forces_multiline_off) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // Resolve-level exclusion (ADR_032 Decision 4).
    ctx.theme = &wlx_theme_dark;
    WLX_Inputbox_Opt opt = wlx_default_inputbox_opt(.password = true, .multiline = true);
    wlx_resolve_opt_inputbox(&ctx, &opt);
    ASSERT_FALSE(opt.multiline);

    // Behavioral confirmation: a password+multiline field keeps the
    // single-line contract -> Enter blurs and inserts nothing.
    char buf[64] = "pw";
    test_frame_begin(&ctx, 380, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ml_pw_inputbox(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(focused);

    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_ENTER] = true;
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                        NULL, keys_pressed, NULL);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    focused = ml_pw_inputbox(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(focused);
    ASSERT_EQ_STR(buf, "pw");
    wlx_context_destroy(&ctx);
}

TEST(multiline_escape_blurs) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ab";

    bool focused = ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ASSERT_TRUE(focused);

    focused = ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ESCAPE, 0);
    ASSERT_FALSE(focused);
    ASSERT_EQ_STR(buf, "ab");
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Capacity boundary
// ============================================================================

TEST(multiline_full_buffer_inserts_nothing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // 3 chars + NUL: the buffer is at capacity, so Enter has no room.
    char buf[4] = "abc";

    bool focused = ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ASSERT_TRUE(focused);

    focused = ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf, "abc");
    ASSERT_TRUE(focused);

    // One char of room left: the newline still fits exactly. Fresh context so
    // the click is not a double-click on the previous widget state.
    WLX_Context ctx2;
    test_ctx_init(&ctx2, 400, 300);
    char buf2[5] = "abc";
    ml_frame_mouse(&ctx2, buf2, sizeof(buf2), true, false, 380, true, true);
    ml_frame_key(&ctx2, buf2, sizeof(buf2), true, false, WLX_KEY_ENTER, 0);
    ASSERT_EQ_STR(buf2, "abc\n");
    wlx_context_destroy(&ctx);
    wlx_context_destroy(&ctx2);
}

// ============================================================================
// Vertical caret motion with sticky column
//
// Geometry relies on the mock backend's deterministic measurement (5px per
// ASCII char at font_size 10, line_h 10; see test_text_layout.c). Column
// positions are proven by moving the caret with keys, pressing UP/DOWN, then
// typing a marker character and asserting where it lands in the buffer.
// ============================================================================

TEST(multiline_down_moves_same_column) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD\nEFGH";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Caret at column 2 of "ABCD"; DOWN lands at column 2 of "EFGH" (byte 7).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "ABCD\nEFXGH");
    wlx_context_destroy(&ctx);
}

TEST(multiline_up_moves_same_column) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD\nEFGH";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_END, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_LEFT, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_LEFT, 0);

    // Caret at column 2 of "EFGH"; UP lands at column 2 of "ABCD" (byte 2).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_UP, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "ABXCD\nEFGH");
    wlx_context_destroy(&ctx);
}

TEST(multiline_sticky_column_across_short_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // Long -> short -> long: the middle line has one character.
    char buf[64] = "ABCDE\nZ\nFGHIJ";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    for (int i = 0; i < 4; i++)
        ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Column 4 on "ABCDE"; DOWN clamps to the end of "Z" by walk, the second
    // DOWN restores column 4 on "FGHIJ" (byte 12) from the latched column.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "ABCDE\nZ\nFGHIXJ");
    wlx_context_destroy(&ctx);
}

TEST(multiline_sticky_column_roundtrip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCDE\nZ\nFGHIJ";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    for (int i = 0; i < 4; i++)
        ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Down through the short line and back up: the column survives the
    // round-trip because no horizontal motion happened in between.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_UP, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_UP, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "ABCDXE\nZ\nFGHIJ");
    wlx_context_destroy(&ctx);
}

TEST(multiline_up_first_down_last_clamp) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "AB\nCD";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // UP on the first line clamps to the line start (ADR_032 Decision 6).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_UP, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "XAB\nCD");

    // DOWN on the last line clamps to the line end.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_END, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_LEFT, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "Y");
    ASSERT_EQ_STR(buf, "XAB\nCDY");
    wlx_context_destroy(&ctx);
}

TEST(multiline_shift_down_extends_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "AB\nCD";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());

    // SHIFT+DOWN keeps the anchor at 0 and moves the caret one line down
    // (byte 3), selecting "AB\n"; typing replaces the selection.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, WLX_MOD_SHIFT);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "XCD");
    wlx_context_destroy(&ctx);
}

TEST(multiline_horizontal_motion_resets_column) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD\nE\nFGHI";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    for (int i = 0; i < 4; i++)
        ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Column 4 latched; DOWN clamps to the end of "E" (byte 6). LEFT moves to
    // the line start and drops the latch, so the next DOWN aims at column 0
    // of "FGHI" (byte 7) instead of the stale column 4 (byte 11).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_LEFT, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "ABCD\nE\nXFGHI");
    wlx_context_destroy(&ctx);
}

TEST(multiline_wrapped_soft_line_traversal) {
    WLX_Context ctx;
    // Narrow field: widget 43px wide -> text band 26px -> 5 chars per soft
    // line (mock measure), so "AAAA BBBB" wraps to "AAAA " / "BBBB".
    test_ctx_init(&ctx, 43, 300);
    char buf[64] = "AAAA BBBB";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 40, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Column 2 of the first soft line; DOWN lands at column 2 of the second
    // soft line (byte 7) even though no hard newline exists.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "AAAA BBXBB");
    wlx_context_destroy(&ctx);
}

TEST(multiline_wrapped_overwide_word_breaks_inside) {
    WLX_Context ctx;
    // Same 5-char band: "AA BBBBBBBB" wraps to "AA " / "BBBBB" / "BBB" -
    // the cut after the space, then the over-wide word broken inside.
    test_ctx_init(&ctx, 43, 300);
    char buf[64] = "AA BBBBBBBB";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 40, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_HOME, test_command_mod());
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_RIGHT, 0);

    // Column 1 of "AA "; DOWN lands at column 1 of "BBBBB" (byte 4).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "AA BXBBBBBBB");
    // Rows are now "AA " / "BXBBB" / "BBBB"; column 2 of the second soft
    // line lands at column 2 of the third (byte 10).
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_DOWN, 0);
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, "Y");
    ASSERT_EQ_STR(buf, "AA BXBBBBBYBB");
    wlx_context_destroy(&ctx);
}

// Caret line capture: the multiline inputbox draws no other line while its
// border is off, so every captured line is the caret.
#define ML_MAX_LINES_CAP_ 16
static struct { float x; float y0; float y1; } _ml_lines[ML_MAX_LINES_CAP_];
static int _ml_line_count = 0;

static void _ml_capture_draw_line(float x1, float y1, float x2, float y2, float thick,
                                  WLX_Color c, void *user) {
    (void)x2; (void)thick; (void)c; (void)user;
    if (_ml_line_count < ML_MAX_LINES_CAP_) {
        _ml_lines[_ml_line_count].x = x1;
        _ml_lines[_ml_line_count].y0 = y1;
        _ml_lines[_ml_line_count].y1 = y2;
        _ml_line_count++;
    }
}

static void ml_assert_caret_inside_band(void) {
    ASSERT_TRUE(_ml_line_count >= 1);
    for (int i = 0; i < _ml_line_count; i++) {
        ASSERT_TRUE(_ml_lines[i].x >= 4.0f);
        ASSERT_TRUE(_ml_lines[i].x < 36.0f);
    }
}

TEST(multiline_wrapped_caret_stays_inside_after_trailing_spaces) {
    WLX_Context ctx;
    // 5-char band again: "AAAAA" fills the first soft line, so a typed
    // space does not fit and opens the next soft line instead of hanging
    // past the field; the caret follows it and stays drawn.
    test_ctx_init(&ctx, 43, 300);
    ctx.backend.draw_line = _ml_capture_draw_line;
    char buf[64] = "AAAAA";

    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 40, true, true);
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_END, test_command_mod());
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, " ");
    ml_frame_type(&ctx, buf, sizeof(buf), true, false, " ");
    ASSERT_EQ_STR(buf, "AAAAA  ");

    // After the second space (caret 7): on the second soft line, inside
    // the band (right edge x 35).
    _ml_line_count = 0;
    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 200, false, false);
    ml_assert_caret_inside_band();

    // Between the two spaces (caret 6) likewise.
    ml_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_LEFT, 0);
    _ml_line_count = 0;
    ml_frame_mouse(&ctx, buf, sizeof(buf), true, false, 200, false, false);
    ml_assert_caret_inside_band();
    wlx_context_destroy(&ctx);
}

// Stable call site + drivers for the wrap = false variant: hard newlines must
// still split visual lines, so UP/DOWN traverse them (assumption check from
// the multiline design: line records split on hard breaks regardless of wrap).
static bool ml_nowrap_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .wrap = false,
            .multiline = true,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static void ml_nowrap_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size,
                                  int mx, bool down, bool clicked) {
    test_frame_begin(ctx, mx, 150, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ml_nowrap_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void ml_nowrap_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                                WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ml_nowrap_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void ml_nowrap_frame_type(WLX_Context *ctx, char *buf, size_t buf_size,
                                 const char *text) {
    test_frame_begin_ex(ctx, 200, 150, false, false, false, 0.0f,
                        NULL, NULL, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    ml_nowrap_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(multiline_nowrap_hard_lines_traversal) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD\nEFGH";

    ml_nowrap_frame_mouse(&ctx, buf, sizeof(buf), 380, true, true);
    ml_nowrap_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_HOME, test_command_mod());
    ml_nowrap_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_RIGHT, 0);
    ml_nowrap_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_RIGHT, 0);

    ml_nowrap_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_DOWN, 0);
    ml_nowrap_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "ABCD\nEFXGH");
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_multiline) {
    RUN_TEST(multiline_enter_inserts_newline_keeps_focus);
    RUN_TEST(multiline_enter_autorepeat_inserts_again);
    RUN_TEST(multiline_enter_replaces_selection);
    RUN_TEST(singleline_enter_blurs_no_insert);
    RUN_TEST(multiline_readonly_rejects_insert_keeps_focus);
    RUN_TEST(password_forces_multiline_off);
    RUN_TEST(multiline_escape_blurs);
    RUN_TEST(multiline_full_buffer_inserts_nothing);

    // Vertical caret motion (sticky column)
    RUN_TEST(multiline_down_moves_same_column);
    RUN_TEST(multiline_up_moves_same_column);
    RUN_TEST(multiline_sticky_column_across_short_line);
    RUN_TEST(multiline_sticky_column_roundtrip);
    RUN_TEST(multiline_up_first_down_last_clamp);
    RUN_TEST(multiline_shift_down_extends_selection);
    RUN_TEST(multiline_horizontal_motion_resets_column);
    RUN_TEST(multiline_wrapped_soft_line_traversal);
    RUN_TEST(multiline_wrapped_overwide_word_breaks_inside);
    RUN_TEST(multiline_wrapped_caret_stays_inside_after_trailing_spaces);
    RUN_TEST(multiline_nowrap_hard_lines_traversal);
}

// test_input_selection.c - inputbox selection tests: shift+arrow extension,
// replace-on-edit, mouse caret placement, drag/double/triple-click selection,
// the point->offset hit test, word bounds, and the highlight draw.

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
//
// Geometry (mock measure: 5px per char at font_size 10):
//   ctx 400x300, layout padding 0 -> widget rect {0,0,400,300}
//   content_padding 4, border 0, no label, no icon -> input_rect {4,4,392,292}
//   text_rect.x = 9; single line centered vertically -> line origin_y = 145
//   codepoint boundary k sits at x = 9 + 5k; clicking at 10 + 5k hits k.

#define SEL_BOUNDARY_X(k) (10 + 5 * (k))

static bool sel_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0,
            .selection_color = {9, 9, 9, 99},
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static void sel_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size,
                            int mx, bool down, bool clicked) {
    test_frame_begin(ctx, mx, 150, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sel_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void sel_frame_mouse_mods(WLX_Context *ctx, char *buf, size_t buf_size,
                                 int mx, bool down, bool clicked, uint32_t mods) {
    test_frame_begin_full(ctx, mx, 150, down, clicked, down, 0.0f,
                          NULL, NULL, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sel_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void sel_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                          WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sel_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void sel_frame_type(WLX_Context *ctx, char *buf, size_t buf_size, const char *text) {
    test_frame_begin_ex(ctx, 200, 150, false, false, false, 0.0f,
                        NULL, NULL, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sel_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// Focus with the click landing far past the text so the caret goes to the end.
static void sel_frame_focus_at_end(WLX_Context *ctx, char *buf, size_t buf_size) {
    sel_frame_mouse(ctx, buf, buf_size, 380, true, true);
}

// ============================================================================
// Keyboard selection
// ============================================================================

TEST(sel_shift_arrow_extends_and_typing_replaces) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);

    // Selection [2,4) replaced by the typed character.
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "ABX");
}

TEST(sel_backspace_deletes_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "AB");

    // Caret collapsed to the selection start: typing appends at the end.
    sel_frame_type(&ctx, buf, sizeof(buf), "Y");
    ASSERT_EQ_STR(buf, "ABY");
}

TEST(sel_delete_key_deletes_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_DELETE, 0);
    ASSERT_EQ_STR(buf, "AB");
}

TEST(sel_plain_left_collapses_to_start) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);

    // Plain LEFT collapses [2,4) to its start without moving further.
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, 0);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "ABXCD");
}

TEST(sel_plain_right_collapses_to_end) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);

    // Plain RIGHT collapses [2,4) to its end without moving further.
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_RIGHT, 0);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "ABCDX");
}

TEST(sel_shift_home_end_extends) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCD";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));

    // Shift+HOME selects everything back to the line start.
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_HOME, WLX_MOD_SHIFT);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "X");
}

// ============================================================================
// Mouse caret placement + selection
// ============================================================================

TEST(sel_mouse_click_places_caret) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCDEF";

    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(2), true, true);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "ABXCDEF");
}

TEST(sel_mouse_drag_selects_range) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCDEF";

    // Press at boundary 1, drag (still held) to boundary 4, then backspace
    // deletes the dragged selection [1,4).
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(4), true, false);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "AEF");
}

TEST(sel_shift_click_extends) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "ABCDEF";

    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    // Release, then shift+click at boundary 4 keeps the anchor at 1.
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), false, false);
    sel_frame_mouse_mods(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(4), true, true, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "AEF");
}

TEST(sel_double_click_selects_word) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "foo bar";

    // Two clicks on the same boundary inside "foo" within the multi-click
    // window select the whole word.
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "X bar");
}

TEST(sel_triple_click_selects_all) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "foo bar";

    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "X");
}

TEST(sel_double_click_times_out) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "foo bar";

    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);

    // 30 idle frames at 1/60s exceed the 0.4s multi-click window, so the
    // second click is a fresh single click (caret only, no word selection).
    for (int i = 0; i < 30; i++) {
        sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), false, false);
    }
    sel_frame_mouse(&ctx, buf, sizeof(buf), SEL_BOUNDARY_X(1), true, true);
    sel_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "fXoo bar");
}

// ============================================================================
// Hit test + word bounds (direct)
// ============================================================================

TEST(sel_offset_at_point_single_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Rect rect = {0, 0, 100, 100};
    WLX_Text_Style ts = { .font_size = 10 };
    const char *text = "ABCDEF";

    test_frame_begin(&ctx, 0, 0, false, false);

    // Midpoint rule at 5px per char: boundary 2 sits at x=10, char 2 spans
    // 10..15 with midpoint 12.5.
    ASSERT_EQ_INT(2, (int)wlx_text_offset_at_point(&ctx, rect, text, 6, ts, WLX_TOP_LEFT, false, 12.0f, 5.0f));
    ASSERT_EQ_INT(3, (int)wlx_text_offset_at_point(&ctx, rect, text, 6, ts, WLX_TOP_LEFT, false, 13.0f, 5.0f));
    // Past the line end clamps to the end; before the start clamps to 0.
    ASSERT_EQ_INT(6, (int)wlx_text_offset_at_point(&ctx, rect, text, 6, ts, WLX_TOP_LEFT, false, 200.0f, 5.0f));
    ASSERT_EQ_INT(0, (int)wlx_text_offset_at_point(&ctx, rect, text, 6, ts, WLX_TOP_LEFT, false, -5.0f, 5.0f));
    // py below the single line clamps to that line.
    ASSERT_EQ_INT(6, (int)wlx_text_offset_at_point(&ctx, rect, text, 6, ts, WLX_TOP_LEFT, false, 200.0f, 90.0f));

    test_frame_end(&ctx);
}

TEST(sel_offset_at_point_multi_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Rect rect = {0, 0, 100, 100};
    WLX_Text_Style ts = { .font_size = 10 };
    const char *text = "AB\nCD";

    test_frame_begin(&ctx, 0, 0, false, false);

    // Line 0 occupies y [0,10), line 1 y [10,20).
    ASSERT_EQ_INT(3, (int)wlx_text_offset_at_point(&ctx, rect, text, 5, ts, WLX_TOP_LEFT, true, 0.0f, 15.0f));
    ASSERT_EQ_INT(5, (int)wlx_text_offset_at_point(&ctx, rect, text, 5, ts, WLX_TOP_LEFT, true, 200.0f, 15.0f));
    ASSERT_EQ_INT(2, (int)wlx_text_offset_at_point(&ctx, rect, text, 5, ts, WLX_TOP_LEFT, true, 200.0f, 5.0f));
    // py below every line clamps to the last one.
    ASSERT_EQ_INT(3, (int)wlx_text_offset_at_point(&ctx, rect, text, 5, ts, WLX_TOP_LEFT, true, 0.0f, 90.0f));

    test_frame_end(&ctx);
}

TEST(sel_word_bounds) {
    size_t start = 99, end = 99;

    wlx_text_word_bounds("foo bar", 7, 1, &start, &end);
    ASSERT_EQ_INT(0, (int)start);
    ASSERT_EQ_INT(3, (int)end);

    // On the separator run between words.
    wlx_text_word_bounds("foo bar", 7, 3, &start, &end);
    ASSERT_EQ_INT(3, (int)start);
    ASSERT_EQ_INT(4, (int)end);

    // Past the end clamps to the last word.
    wlx_text_word_bounds("foo bar", 7, 7, &start, &end);
    ASSERT_EQ_INT(4, (int)start);
    ASSERT_EQ_INT(7, (int)end);

    wlx_text_word_bounds("", 0, 0, &start, &end);
    ASSERT_EQ_INT(0, (int)start);
    ASSERT_EQ_INT(0, (int)end);
}

// ============================================================================
// Highlight draw
// ============================================================================

static int _sel_rect_count = 0;
static WLX_Rect _sel_rect_last = {0};

static void sel_capture_draw_rect(WLX_Rect r, WLX_Color c) {
    if (c.r == 9 && c.g == 9 && c.b == 9 && c.a == 99) {
        _sel_rect_count++;
        _sel_rect_last = r;
    }
}

TEST(sel_highlight_rect_geometry) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = sel_capture_draw_rect;
    char buf[64] = "ABCDEF";

    sel_frame_focus_at_end(&ctx, buf, sizeof(buf));
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    sel_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);

    // One idle frame with the selection [4,6) live: highlight spans the last
    // two characters. Line origin: x = 9 + 4*5 = 29, y = 145 (centered), and
    // the span is 2 chars * 5px wide, one line (10px) tall.
    _sel_rect_count = 0;
    sel_frame_mouse(&ctx, buf, sizeof(buf), 380, false, false);

    ASSERT_EQ_INT(1, _sel_rect_count);
    ASSERT_EQ_F(_sel_rect_last.x, 29.0f, 0.1f);
    ASSERT_EQ_F(_sel_rect_last.y, 145.0f, 0.1f);
    ASSERT_EQ_F(_sel_rect_last.w, 10.0f, 0.1f);
    ASSERT_EQ_F(_sel_rect_last.h, 10.0f, 0.1f);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_selection) {
    // Keyboard selection
    RUN_TEST(sel_shift_arrow_extends_and_typing_replaces);
    RUN_TEST(sel_backspace_deletes_selection);
    RUN_TEST(sel_delete_key_deletes_selection);
    RUN_TEST(sel_plain_left_collapses_to_start);
    RUN_TEST(sel_plain_right_collapses_to_end);
    RUN_TEST(sel_shift_home_end_extends);

    // Mouse
    RUN_TEST(sel_mouse_click_places_caret);
    RUN_TEST(sel_mouse_drag_selects_range);
    RUN_TEST(sel_shift_click_extends);
    RUN_TEST(sel_double_click_selects_word);
    RUN_TEST(sel_triple_click_selects_all);
    RUN_TEST(sel_double_click_times_out);

    // Hit test + word bounds
    RUN_TEST(sel_offset_at_point_single_line);
    RUN_TEST(sel_offset_at_point_multi_line);
    RUN_TEST(sel_word_bounds);

    // Highlight
    RUN_TEST(sel_highlight_rect_geometry);
}

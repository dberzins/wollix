// test_input_scroll.c - inputbox multiline internal scroll (ADR_033):
// caret-follow keeps the caret line inside the band on caret moves and
// edits (Ctrl+HOME/END, UP/DOWN, Enter at the bottom, paste), the wheel
// scrolls a hovered overflowing field and consumes the delta (and only
// then), content that fits never scrolls, and the scrollbar-width
// prediction (TEXAREA_STEADY_SCROLL_STATE) keeps its sticky-hysteresis
// and non-wrap stability contracts.
//
// Geometry model (mock backend): char width = font_size/2, line height =
// font_size. Fixture: 400x60 context, content_padding 4, border 0,
// font_size 10 -> text band {9, 4, 383, 52}, line_h 10. A 10-hard-line
// buffer is 100px of content -> max_scroll 48.

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

static uint32_t sc_command_mod(void) {
#if defined(__APPLE__)
    return WLX_MOD_SUPER;
#else
    return WLX_MOD_CTRL;
#endif
}

#define SC_TEN_LINES "l0\nl1\nl2\nl3\nl4\nl5\nl6\nl7\nl8\nl9"

static bool sc_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = true,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool sc_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size,
                           int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = sc_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool sc_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                         WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 30, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = sc_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static void sc_frame_wheel(WLX_Context *ctx, char *buf, size_t buf_size,
                           int mx, int my, float wheel_delta) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel_delta,
                        NULL, NULL, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sc_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The tests run a single inputbox, so the one state entry of matching size
// is its persistent state.
static WLX_Inputbox_State *sc_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Inputbox_State))
            return (WLX_Inputbox_State *)slot->data;
    }
    return NULL;
}

// ============================================================================
// Caret-follow
// ============================================================================

TEST(scroll_follows_ctrl_end_and_home) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    bool focused = sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    ASSERT_TRUE(focused);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (int)st->scroll_y);

    // Ctrl+END jumps to the buffer end on the last line; the view follows
    // to the bottom: content 100 - band 52 = 48.
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    ASSERT_EQ_INT(48, (int)st->scroll_y);

    // Ctrl+HOME jumps back to offset 0; the view follows to the top.
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_HOME, sc_command_mod());
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

TEST(scroll_enter_at_bottom_keeps_following) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(48, (int)st->scroll_y);

    // Enter appends a newline; the caret lands on the new trailing empty
    // line (content 110), and the view follows to the new bottom (58).
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_ENTER, 0);
    ASSERT_EQ_INT(58, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

TEST(scroll_up_down_across_band_edges) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(48, (int)st->scroll_y);

    // Five UPs from the last line put the caret on line 4 (content y 40),
    // which is above the band top at scroll 48 -> the view follows up.
    for (int i = 0; i < 5; i++)
        sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_UP, 0);
    ASSERT_EQ_INT(40, (int)st->scroll_y);

    // Five DOWNs land back on line 9 (bottom 100), past the band bottom at
    // scroll 40 (40 + 52 = 92) -> the view follows down to 48.
    for (int i = 0; i < 5; i++)
        sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(48, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

TEST(scroll_paste_follows_caret) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // Caret at the end of "l2" (offset 8), no scroll yet.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (int)st->scroll_y);

    // Pasting 8 hard lines moves the caret to the empty line after the
    // pasted "H\n" (line index 10, band 100..110 in content space); the
    // minimal follow puts that band at the bottom: 110 - 52 = 58.
    test_set_clipboard("A\nB\nC\nD\nE\nF\nG\nH\n");
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, sc_command_mod());
    ASSERT_EQ_INT(58, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Wheel
// ============================================================================

TEST(scroll_wheel_hovered_overflow_consumes) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // No focus needed: hover plus overflow owns the wheel. Delta -1 scrolls
    // down by the shared wheel speed (20px) and the event is consumed.
    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(20, (int)st->scroll_y);
    ASSERT_EQ_INT(0, (int)ctx.input.wheel_delta);
    wlx_context_destroy(&ctx);
}

TEST(scroll_wheel_content_fits_not_consumed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[64] = "ab\ncd";

    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    ASSERT_EQ_INT(-1, (int)ctx.input.wheel_delta);
    wlx_context_destroy(&ctx);
}

TEST(scroll_wheel_not_hovered_not_consumed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // Focus the field, then wheel with the pointer outside it: the field
    // must leave the delta alone even while focused.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_wheel(&ctx, buf, sizeof(buf), -50, -50, -1.0f);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    ASSERT_EQ_INT(-1, (int)ctx.input.wheel_delta);
    wlx_context_destroy(&ctx);
}

TEST(scroll_wheel_away_then_keypress_follows_back) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(48, (int)st->scroll_y);

    // Wheel up scrolls the caret line out of view; the caret did not move,
    // so the view stays where the wheel put it.
    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, 1.0f);
    ASSERT_EQ_INT(28, (int)st->scroll_y);

    // The next caret motion snaps the view back to the caret line.
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(48, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Scrollbar
// ============================================================================

// Track geometry with the fixture: track = {4, 4, 392, 52}, bar width 10
// (theme default), content 100 -> thumb is 27.04 tall and the usable track
// is 24.96, so dragging past either end clamps to max_scroll / 0.

TEST(scroll_thumb_drag_maps_to_offset) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, false, false);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Press the thumb (at scroll 0 it spans y 4..31) and drag far past the
    // bottom: the position clamps to the track end -> max_scroll.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 10, true, true);
    ASSERT_TRUE(st->caret.dragging_scrollbar);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 100, true, false);
    ASSERT_EQ_INT(48, (int)st->scroll_y);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 100, false, false);
    ASSERT_FALSE(st->caret.dragging_scrollbar);

    // Press the thumb at its new position (y 28.96..56) and drag past the
    // top: clamps to 0.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 30, true, true);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 0, true, false);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 0, false, false);
    wlx_context_destroy(&ctx);
}

TEST(scroll_thumb_press_keeps_caret_and_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // Caret at the end of "l2" (offset 8) from a click in the text.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, false, false);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(8, (int)st->caret.cursor_pos);

    // A press on the thumb starts the drag gesture but must not move the
    // caret, start a selection, or blur the field.
    bool focused = sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 10, true, true);
    ASSERT_TRUE(focused);
    ASSERT_TRUE(st->caret.dragging_scrollbar);
    ASSERT_EQ_INT(8, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(8, (int)st->caret.selection_anchor);
    ASSERT_FALSE(st->caret.mouse_selecting);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 390, 10, false, false);
    wlx_context_destroy(&ctx);
}

// show_scrollbar = false: no bar strip (clicks there place the caret) while
// wheel scrolling still works.

static bool sc_inputbox_nobar(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = true, .show_scrollbar = false,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool sc_frame_mouse_nobar(WLX_Context *ctx, char *buf, size_t buf_size,
                                 int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = sc_inputbox_nobar(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static void sc_frame_wheel_nobar(WLX_Context *ctx, char *buf, size_t buf_size,
                                 int mx, int my, float wheel_delta) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel_delta,
                        NULL, NULL, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    sc_inputbox_nobar(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(scroll_hidden_scrollbar_keeps_wheel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // With the bar hidden there is no strip suppression: a click where the
    // bar would sit lands in the text on line 0 -> caret at the end of "l0".
    sc_frame_mouse_nobar(&ctx, buf, sizeof(buf), 390, 6, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(2, (int)st->caret.cursor_pos);
    ASSERT_FALSE(st->caret.dragging_scrollbar);
    sc_frame_mouse_nobar(&ctx, buf, sizeof(buf), 390, 6, false, false);

    // Wheel scrolling is unaffected by the hidden bar.
    sc_frame_wheel_nobar(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    ASSERT_EQ_INT(20, (int)st->scroll_y);
    ASSERT_EQ_INT(0, (int)ctx.input.wheel_delta);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Drag-select auto-scroll
// ============================================================================

TEST(scroll_drag_select_autoscrolls) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    // Click on line 2 anchors the selection at offset 8, then hold the
    // button with the pointer 44px below the band: the view auto-scrolls
    // toward the pointer (11px/frame at the mock 60fps plus caret-follow
    // catch-up) and the selection grows to the buffer end within a few
    // frames, capped at max_scroll.
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(8, (int)st->caret.cursor_pos);

    for (int i = 0; i < 6; i++)
        sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 100, true, false);
    ASSERT_EQ_INT(48, (int)st->scroll_y);
    ASSERT_EQ_INT(29, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(8, (int)st->caret.selection_anchor);
    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 100, false, false);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Content-fits identity
// ============================================================================

TEST(scroll_content_fits_never_scrolls) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[64] = "ab\ncd";

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);

    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Multiline text-run budget
// ============================================================================

TEST(scroll_geometry_past_old_unit_cap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);

    // 60 hard lines of 10 codepoints = 600 units, past the 512-unit global
    // cap that bounded all geometry before the multiline budget. Content is
    // 600px; Ctrl+END must follow the caret to the true bottom (548), which
    // only works if the build covered the whole buffer.
    static char buf[1024];
    size_t off = 0;
    for (int i = 0; i < 60; i++) {
        memcpy(buf + off, "abcdefghij\n", 11);
        off += 11;
    }
    buf[off - 1] = '\0';  // drop the trailing newline -> exactly 60 lines

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);

    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    ASSERT_EQ_INT(548, (int)st->scroll_y);
    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_HOME, sc_command_mod());
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

TEST(scroll_freeze_at_multiline_cap_is_bounded) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);

    // 500 hard lines of 10 codepoints = 5000 units, past the 4096-unit
    // multiline budget. The build stops mid-buffer (409 full lines plus a
    // 6-unit partial = 410 records, 4100px of content), the caret pins to
    // the end of the last built line, and the view clamps to the built
    // bottom (4100 - 52 = 4048) instead of crashing or scrolling into
    // unbuilt space.
    static char buf[6000];
    size_t off = 0;
    for (int i = 0; i < 500; i++) {
        memcpy(buf + off, "abcdefghij\n", 11);
        off += 11;
    }
    buf[off - 1] = '\0';

    sc_frame_mouse(&ctx, buf, sizeof(buf), 200, 30, true, true);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);

    sc_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_END, sc_command_mod());
    ASSERT_EQ_INT(4048, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Scrollbar width prediction (wrap mode)
// ============================================================================

// Wrapped-width geometry with the fixture: the full band is 383px wide (76
// 5px chars per wrapped line), the narrowed band 373px (74 chars). A 375-char
// unbroken run is the ambiguity band: 5 lines (50px, fits) at full width but
// 6 lines (60px, overflows) at the narrowed width. 456 chars overflow at
// both widths (6 / 7 lines).

TEST(scroll_wrap_hysteresis_keeps_scrollbar) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    static char buf[512];
    memset(buf, 'a', 456);
    buf[375] = '\0';

    // Entering the ambiguity band from below (fresh field): the full-width
    // probe fits, so no scrollbar and no scrollable overflow.
    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    WLX_Inputbox_State *st = sc_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_FALSE(st->sb_was_visible);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    ASSERT_EQ_INT(-1, (int)ctx.input.wheel_delta);

    // Grow past the band at full width: the scrollbar comes up.
    buf[375] = 'a';
    sc_frame_mouse(&ctx, buf, sizeof(buf), -50, -50, false, false);
    ASSERT_TRUE(st->sb_was_visible);

    // Shrink back into the ambiguity band: the narrowed probe still
    // overflows, so the scrollbar sticks (hysteresis) and holds steady
    // across frames instead of flipping to the full-width answer.
    buf[375] = '\0';
    for (int i = 0; i < 3; i++) {
        sc_frame_mouse(&ctx, buf, sizeof(buf), -50, -50, false, false);
        ASSERT_TRUE(st->sb_was_visible);
    }

    // The retained scrollbar is functional: 6 lines in the narrowed band
    // leave max_scroll 60 - 52 = 8, so the wheel scrolls and is consumed.
    sc_frame_wheel(&ctx, buf, sizeof(buf), 200, 30, -1.0f);
    ASSERT_EQ_INT(8, (int)st->scroll_y);
    ASSERT_EQ_INT(0, (int)ctx.input.wheel_delta);

    // Content that fits even the narrowed band resolves the hysteresis.
    buf[2] = '\0';
    sc_frame_mouse(&ctx, buf, sizeof(buf), -50, -50, false, false);
    ASSERT_FALSE(st->sb_was_visible);
    ASSERT_EQ_INT(0, (int)st->scroll_y);
    wlx_context_destroy(&ctx);
}

// wrap = false: content height is anti-monotonic in width (a width-truncated
// line stops the whole build via stop_after_line), so the prediction must not
// fire. Fixture: a 75-char line fits the full band (375 <= 383) but truncates
// at the narrowed one (74 chars), dropping the 6 lines below it from the
// narrow build.

static void sc_frame_nowrap(WLX_Context *ctx, char *buf, size_t buf_size,
                            float wheel_delta) {
    test_frame_begin_ex(ctx, 200, 30, false, false, false, wheel_delta,
                        NULL, NULL, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = true, .wrap = false),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(scroll_nowrap_long_line_stays_stable) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    static char buf[128];
    memset(buf, 'a', 75);
    memcpy(buf + 75, "\nx\nx\nx\nx\nx\nx", 13);

    // Every frame decides from the full-width build (7 lines, 70px,
    // overflow -> scrollbar) and then rebuilds at the narrowed width, where
    // the build stops after the truncated long line (content 10px). The
    // wrap gate keeps that narrow result out of the next frame's decision,
    // so the scrollbar must hold steady instead of flickering.
    for (int i = 0; i < 4; i++) {
        sc_frame_nowrap(&ctx, buf, sizeof(buf), 0.0f);
        WLX_Inputbox_State *st = sc_state(&ctx);
        ASSERT_TRUE(st != NULL);
        ASSERT_TRUE(st->sb_was_visible);
        ASSERT_EQ_INT(0, (int)st->scroll_y);
    }

    // Pre-existing non-wrap quirk, locked in as parity with the old flow:
    // the bar is up but the narrowed content fits, so there is nothing to
    // scroll and the wheel is left for an enclosing panel.
    sc_frame_nowrap(&ctx, buf, sizeof(buf), -1.0f);
    ASSERT_EQ_INT(0, (int)sc_state(&ctx)->scroll_y);
    ASSERT_EQ_INT(-1, (int)ctx.input.wheel_delta);
    wlx_context_destroy(&ctx);
}

#ifdef WLX_PERF

// The point of the prediction: a steadily overflowing field builds its line
// records once per frame. The transition frame (fits -> overflow) pays the
// corrective second build; every settled frame after it pays one.
TEST(scroll_steady_scrollbar_single_build) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 60);
    char buf[128] = SC_TEN_LINES;

    sc_frame_mouse(&ctx, buf, sizeof(buf), -50, -50, false, false);
    const WLX_Perf_Frame *pf = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(pf != NULL);
    ASSERT_EQ_INT(2, (int)pf->text.fitted_text_runs);

    for (int i = 0; i < 3; i++) {
        sc_frame_mouse(&ctx, buf, sizeof(buf), -50, -50, false, false);
        pf = wlx_perf_get_last_frame(&ctx);
        ASSERT_EQ_INT(1, (int)pf->text.fitted_text_runs);
    }
    wlx_context_destroy(&ctx);
}

#endif  // WLX_PERF

// ============================================================================
// Suite
// ============================================================================

SUITE(input_scroll) {
    RUN_TEST(scroll_follows_ctrl_end_and_home);
    RUN_TEST(scroll_enter_at_bottom_keeps_following);
    RUN_TEST(scroll_up_down_across_band_edges);
    RUN_TEST(scroll_paste_follows_caret);
    RUN_TEST(scroll_wheel_hovered_overflow_consumes);
    RUN_TEST(scroll_wheel_content_fits_not_consumed);
    RUN_TEST(scroll_wheel_not_hovered_not_consumed);
    RUN_TEST(scroll_wheel_away_then_keypress_follows_back);
    RUN_TEST(scroll_thumb_drag_maps_to_offset);
    RUN_TEST(scroll_thumb_press_keeps_caret_and_focus);
    RUN_TEST(scroll_hidden_scrollbar_keeps_wheel);
    RUN_TEST(scroll_drag_select_autoscrolls);
    RUN_TEST(scroll_content_fits_never_scrolls);
    RUN_TEST(scroll_geometry_past_old_unit_cap);
    RUN_TEST(scroll_freeze_at_multiline_cap_is_bounded);
    RUN_TEST(scroll_wrap_hysteresis_keeps_scrollbar);
    RUN_TEST(scroll_nowrap_long_line_stays_stable);
#ifdef WLX_PERF
    RUN_TEST(scroll_steady_scrollbar_single_build);
#endif
}

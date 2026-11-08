// test_input_hscroll.c - inputbox single-line overflow: wrap is a multiline
// option (a non-multiline field always builds one visual row, whatever its
// .wrap), and an overflowing row scrolls horizontally: the caret stays in
// view on every caret move or edit, the view clamps back when text is
// deleted, resets on blur and follows again on the focus frame, follows a
// drag past either edge with the hit clamped to the band, consumes the
// horizontal wheel while focused and overflowing (a caret scrolled out on
// either side is not drawn), keeps the band alignment while the row fits,
// and drops line separators from the text it takes.
//
// Geometry model (mock backend): char width = font_size/2, line height =
// font_size. Fixture: 120x40 context, content_padding 4, border 0,
// font_size 10 -> glyphs are 5 px wide. The first test pins the band from
// the records; later tests read it from hs_band().

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

// Capture of the frame's text draws (count and the first row's origin)
// and of the caret (the one draw_line of caret width).
static int   hs_text_draws;
static float hs_text_x0, hs_text_y0;
static int   hs_caret_draws;
static float hs_caret_x, hs_caret_y1, hs_caret_y2;

static void hs_draw_text(const char *t, float x, float y, WLX_Text_Style s, void *u) {
    (void)t; (void)s; (void)u;
    if (hs_text_draws == 0) { hs_text_x0 = x; hs_text_y0 = y; }
    hs_text_draws++;
}

static void hs_draw_line(float x1, float y1, float x2, float y2, float t, WLX_Color c, void *u) {
    (void)x2; (void)c; (void)u;
    if (t == WLX_TEXT_CARET_WIDTH) {
        hs_caret_x = x1; hs_caret_y1 = y1; hs_caret_y2 = y2;
        hs_caret_draws++;
    }
}

static void hs_reset_capture(void) {
    hs_text_draws = 0; hs_text_x0 = hs_text_y0 = -1.0f;
    hs_caret_draws = 0; hs_caret_x = hs_caret_y1 = hs_caret_y2 = -1.0f;
}

static void hs_ctx_init(WLX_Context *ctx) {
    test_ctx_init(ctx, 120, 40);
    ctx->backend.draw_text = hs_draw_text;
    ctx->backend.draw_line = hs_draw_line;
}

typedef struct {
    bool multiline;
    bool wrap;
    bool password;
    WLX_Align align;
} HS_Opt;

#define HS_DEFAULT (HS_Opt){ .multiline = false, .wrap = true, .password = false, .align = WLX_LEFT }

static bool hs_inputbox(WLX_Context *ctx, char *buf, size_t buf_size, HS_Opt o) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = o.multiline, .wrap = o.wrap,
            .password = o.password, .content_align = o.align,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool hs_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size, HS_Opt o,
                           int mx, int my, bool down, bool clicked) {
    hs_reset_capture();
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = hs_inputbox(ctx, buf, buf_size, o);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool hs_frame_type(WLX_Context *ctx, char *buf, size_t buf_size, HS_Opt o,
                          const char *text) {
    hs_reset_capture();
    test_frame_begin_full(ctx, 200, 30, false, false, false, 0.0f,
                          NULL, NULL, NULL, 0, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = hs_inputbox(ctx, buf, buf_size, o);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool hs_frame_key(WLX_Context *ctx, char *buf, size_t buf_size, HS_Opt o,
                         WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    hs_reset_capture();
    test_frame_begin_full(ctx, 200, 30, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = hs_inputbox(ctx, buf, buf_size, o);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

// The tests run a single inputbox, so the one state entry of matching size
// is its persistent state.
static WLX_Inputbox_State *hs_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Inputbox_State))
            return (WLX_Inputbox_State *)slot->data;
    }
    return NULL;
}

// Text band of the fixture's single-line field, derived the way the widget
// derives it: the 120x40 slot, content padding 4 on every side, no border,
// the field inset on the left, caret room on the right.
static WLX_Rect hs_band(void) {
    WLX_Rect input_rect = { 4.0f, 4.0f, 112.0f, 32.0f };
    float w = input_rect.w - WLX_TEXT_FIELD_INSET - WLX_TEXT_CARET_WIDTH - WLX_TEXT_CARET_PADDING;
    return (WLX_Rect){ input_rect.x + WLX_TEXT_FIELD_INSET, input_rect.y, w, input_rect.h };
}

// ============================================================================
// Wrap is a multiline option
// ============================================================================

// A non-multiline field with .wrap = true and text wider than its band
// builds one visual row: one text draw, and the row stays at its centred
// y as more text is typed (the row never moves down to make room for a
// second one).
TEST(hscroll_single_line_ignores_wrap_option) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    o.wrap = true;

    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "short row");
    ASSERT_EQ_INT(1, hs_text_draws);
    float row_y = hs_text_y0;

    // 30 glyphs of 5 px overflow the 103 px band by a wide margin; with a
    // space in the middle a wrapping build would open a second row.
    hs_frame_type(&ctx, buf, sizeof(buf), o, " and then a much longer tail");
    ASSERT_TRUE(strlen(buf) > 25);
    ASSERT_EQ_INT(1, hs_text_draws);
    ASSERT_EQ_F(row_y, hs_text_y0, 0.01f);
    wlx_context_destroy(&ctx);
}

// The documented default stays: wlx_inputbox_opt_defaults() reports
// wrap = true, and a multiline field with the default wraps its text.
TEST(hscroll_default_wrap_stays_true_for_multiline) {
    ASSERT_TRUE(wlx_inputbox_opt_defaults().wrap);

    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "one two three four five six seven";
    HS_Opt o = HS_DEFAULT;
    o.multiline = true;
    o.align = WLX_TOP_LEFT;
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true);
    ASSERT_TRUE(hs_text_draws >= 2);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Overflow: one row, horizontal scroll, caret-follow
// ============================================================================

static float hs_caret_margin(void) { return WLX_TEXT_CARET_WIDTH + WLX_TEXT_CARET_PADDING; }

// The frame drew exactly one caret, inside the band.
static bool hs_caret_inside(void) {
    WLX_Rect band = hs_band();
    return hs_caret_draws == 1
        && hs_caret_x >= band.x - 0.01f
        && hs_caret_x <= band.x + band.w + 0.01f;
}

// Reference row origin for a fitting text: the records prepared on the
// band itself, as the pre-scroll build placed them.
static void hs_reference_origin(WLX_Context *ctx, const char *text, WLX_Align align,
                                float *out_x, float *out_y) {
    static WLX_Text_Line_Record ref_lines[WLX_TEXT_RUN_MAX_LINES];
    WLX_Text_Line_Array_Result ref = {0};
    WLX_Text_Style ts = { .font_size = 10 };
    ASSERT_TRUE(wlx_text_prepare_lines_slice(ctx, hs_band(), text, strlen(text), ts,
        (WLX_Text_Prepare_Opt){ .align = align, .wrap = false },
        ref_lines, WLX_TEXT_RUN_MAX_LINES, &ref));
    ASSERT_EQ_INT(1, (int)ref.line_count);
    *out_x = ref_lines[0].origin_x;
    *out_y = ref_lines[0].origin_y;
}

// A row that fits renders where the band alignment puts it, for the
// left, centred and right alignments, and never scrolls.
TEST(hscroll_fitting_row_keeps_band_alignment) {
    static const WLX_Align aligns[3] = { WLX_LEFT, WLX_CENTER, WLX_RIGHT };
    for (int i = 0; i < 3; i++) {
        WLX_Context ctx;
        hs_ctx_init(&ctx);
        char buf[64] = "abc";
        HS_Opt o = HS_DEFAULT;
        o.align = aligns[i];
        ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
        hs_frame_type(&ctx, buf, sizeof(buf), o, NULL);
        ASSERT_EQ_INT(1, hs_text_draws);
        float rx = 0.0f, ry = 0.0f;
        hs_reference_origin(&ctx, buf, aligns[i], &rx, &ry);
        ASSERT_EQ_F(rx, hs_text_x0, 0.01f);
        ASSERT_EQ_F(ry, hs_text_y0, 0.01f);
        WLX_Inputbox_State *st = hs_state(&ctx);
        ASSERT_TRUE(st != NULL);
        ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
        wlx_context_destroy(&ctx);
    }
}

// Typing from empty past the band edge keeps one row at one y, with the
// caret drawn on every frame (the issue's jump and vanished caret).
TEST(hscroll_row_y_constant_typing_past_edge) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "a");
    float row_y = hs_text_y0;
    for (int i = 0; i < 30; i++) {
        hs_frame_type(&ctx, buf, sizeof(buf), o, (i % 5 == 4) ? " " : "a");
        ASSERT_EQ_INT(1, hs_text_draws);
        ASSERT_EQ_F(row_y, hs_text_y0, 0.01f);
        ASSERT_TRUE(hs_caret_inside());
    }
    ASSERT_EQ_INT(31, (int)strlen(buf));
    wlx_context_destroy(&ctx);
}

// The caret stays inside the band after every caret move or edit that
// can leave it: typing past the edge, END, HOME, RIGHT past the right
// edge, LEFT past the left edge while scrolled, paste, undo, redo, and
// a click on the scrolled row.
TEST(hscroll_caret_stays_inside_band) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[256] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);

    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    ASSERT_EQ_INT(40, (int)strlen(buf));
    ASSERT_TRUE(hs_caret_inside());
    ASSERT_TRUE(st->scroll_x > 0.0f);

    // HOME: back to offset 0, the view follows to the start.
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, 0);
    ASSERT_EQ_INT(0, (int)st->caret.cursor_pos);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_TRUE(hs_caret_inside());

    // RIGHT, 25 times: the caret crosses the right edge and the view
    // follows one glyph at a time.
    for (int i = 0; i < 25; i++) {
        hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_RIGHT, 0);
        ASSERT_TRUE(hs_caret_inside());
    }
    ASSERT_EQ_INT(25, (int)st->caret.cursor_pos);
    ASSERT_TRUE(st->scroll_x > 0.0f);

    // END: the row end, scrolled to the maximum.
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, 0);
    ASSERT_EQ_INT(40, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);

    // LEFT past the left edge while scrolled: the view follows back the
    // minimal distance, so the caret rests on the band's left edge with
    // the glyphs before it still scrolled away (10 glyphs, 50 px).
    for (int i = 0; i < 30; i++) {
        hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_LEFT, 0);
        ASSERT_TRUE(hs_caret_inside());
    }
    ASSERT_EQ_INT(10, (int)st->caret.cursor_pos);
    ASSERT_EQ_F(50.0f, st->scroll_x, 0.01f);
    // The caret line is centred on the boundary and clamped half a caret
    // width inside the band's clip.
    ASSERT_EQ_F(hs_band().x + WLX_TEXT_CARET_WIDTH * 0.5f, hs_caret_x, 0.01f);

    // Paste at the end: the caret lands after the pasted text, in view.
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, 0);
    test_set_clipboard("abcdefghijklmnopqrstuvwxyz");
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_V, test_command_mod());
    ASSERT_EQ_INT(66, (int)strlen(buf));
    ASSERT_EQ_INT(66, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());

    // Undo removes the paste (the caret returns to 40), redo restores it.
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_INT(40, (int)strlen(buf));
    ASSERT_TRUE(hs_caret_inside());
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_Z, test_command_mod() | WLX_MOD_SHIFT);
    ASSERT_EQ_INT(66, (int)strlen(buf));
    ASSERT_TRUE(hs_caret_inside());

    // A click on the scrolled row places the caret under the pointer.
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, (int)hs_band().x + 10, 20, true, true);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// An end-of-row caret rests one caret margin inside the band's right
// edge and is drawn there.
TEST(hscroll_caret_drawn_at_row_end) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    ASSERT_EQ_INT(1, hs_caret_draws);
    WLX_Rect band = hs_band();
    ASSERT_EQ_F(band.x + band.w - hs_caret_margin(), hs_caret_x, 0.01f);
    // An idle frame keeps it there (no follow runs; the view does not move).
    hs_frame_type(&ctx, buf, sizeof(buf), o, NULL);
    ASSERT_EQ_INT(1, hs_caret_draws);
    ASSERT_EQ_F(band.x + band.w - hs_caret_margin(), hs_caret_x, 0.01f);
    wlx_context_destroy(&ctx);
}

// A click on a scrolled row hits the boundary under the pointer: the
// scrolled-away glyphs plus the glyphs between the band's left edge and
// the pointer.
TEST(hscroll_click_on_scrolled_row_hits_under_pointer) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    float scroll_before = st->scroll_x;
    ASSERT_TRUE(scroll_before > 0.0f);

    int mx = (int)hs_band().x + 10;
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, mx, 20, true, true);
    float rel = scroll_before + ((float)mx - hs_band().x);
    size_t expected = (size_t)(rel / 5.0f + 0.5f);
    ASSERT_EQ_INT((int)expected, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// Deleting text while scrolled slides the row back: once it fits, the
// scroll is zero and the row starts at the band.
TEST(hscroll_delete_clamps_scroll_back) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->scroll_x > 0.0f);
    float prev = st->scroll_x;
    for (int i = 0; i < 25; i++) {
        hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_BACKSPACE, 0);
        ASSERT_TRUE(st->scroll_x <= prev);
        ASSERT_TRUE(hs_caret_inside());
        prev = st->scroll_x;
    }
    ASSERT_EQ_INT(15, (int)strlen(buf));
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_F(hs_band().x, hs_text_x0, 0.01f);
    wlx_context_destroy(&ctx);
}

// Focus loss shows the start of the text; a click re-focuses at the
// visible start and the follow runs from there.
TEST(hscroll_blur_resets_view) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->scroll_x > 0.0f);

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_ESCAPE, 0);
    bool focused = hs_frame_type(&ctx, buf, sizeof(buf), o, NULL);
    ASSERT_FALSE(focused);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_F(hs_band().x, hs_text_x0, 0.01f);
    ASSERT_EQ_INT(0, hs_caret_draws);

    // Re-focus two glyphs in (a click on the first click's boundary would
    // count as a double-click: the mock clock never advances).
    focused = hs_frame_mouse(&ctx, buf, sizeof(buf), o, (int)hs_band().x + 10, 20, true, true);
    ASSERT_TRUE(focused);
    ASSERT_EQ_INT(2, (int)st->caret.cursor_pos);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// Horizontal alignment applies while the row fits and yields to the
// scroll while it overflows: a right-aligned row snaps to the scrolled
// left anchor on overflow and is right-aligned again once it fits.
TEST(hscroll_alignment_yields_to_scroll_on_overflow) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "abc";
    HS_Opt o = HS_DEFAULT;
    o.align = WLX_RIGHT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, 0);
    WLX_Rect band = hs_band();
    ASSERT_EQ_F(band.x + band.w - 15.0f, hs_text_x0, 0.01f);

    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->scroll_x > 0.0f);
    ASSERT_EQ_F(band.x - st->scroll_x, hs_text_x0, 0.01f);
    ASSERT_TRUE(hs_caret_inside());

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_A, test_command_mod());
    hs_frame_type(&ctx, buf, sizeof(buf), o, "xyz");
    ASSERT_EQ_STR(buf, "xyz");
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_F(band.x + band.w - 15.0f, hs_text_x0, 0.01f);
    wlx_context_destroy(&ctx);
}

// The password mask is the display row: it scrolls, and the caret stays
// inside the band.
TEST(hscroll_password_mask_scrolls) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    o.password = true;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->scroll_x > 0.0f);
    ASSERT_TRUE(hs_text_x0 < hs_band().x);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// The composition anchor follows the scrolled caret: its line is the
// band and its cursor offset is inside the band width.
TEST(hscroll_composition_anchor_inside_band) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Text_Input_Area a = wlx_text_input_area(&ctx);
    ASSERT_TRUE(a.active);
    WLX_Rect band = hs_band();
    ASSERT_EQ_F(band.x, a.line.x, 0.01f);
    ASSERT_TRUE(a.cursor >= 0.0f);
    ASSERT_TRUE(a.cursor <= band.w);
    ASSERT_EQ_F(band.w - hs_caret_margin(), a.cursor, 0.01f);
    wlx_context_destroy(&ctx);
}

// At the per-run unit budget the row end is reachable; past it the
// record freezes and the caret pins at the record end, still in view.
TEST(hscroll_unit_cap_row_end_reachable_then_frozen) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    static char buf[1024];
    buf[0] = '\0';
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    char chunk[65];
    memset(chunk, 'A', 64);
    chunk[64] = '\0';
    for (int i = 0; i < 8; i++) hs_frame_type(&ctx, buf, sizeof(buf), o, chunk);
    ASSERT_EQ_INT(512, (int)strlen(buf));
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, test_command_mod());
    ASSERT_EQ_INT(512, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());
    float at_cap = 512.0f * 5.0f + hs_caret_margin() - hs_band().w;
    ASSERT_EQ_F(at_cap, st->scroll_x, 0.01f);

    hs_frame_type(&ctx, buf, sizeof(buf), o, "AAAAAAAA");
    ASSERT_EQ_INT(520, (int)strlen(buf));
    ASSERT_EQ_INT(520, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());
    ASSERT_EQ_F(at_cap, st->scroll_x, 0.01f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Pointer and wheel
// ============================================================================

// Caret geometry independent of the blink phase: the composition anchor
// (recorded on every focused frame) has its caret offset inside the band.
static bool hs_anchor_inside(WLX_Context *ctx) {
    WLX_Text_Input_Area a = wlx_text_input_area(ctx);
    WLX_Rect band = hs_band();
    return a.active && a.cursor >= 0.0f && a.cursor <= band.w
        && fabsf(a.line.x - band.x) < 0.01f;
}

// A custom-input frame: the pointer at (mx, my) with a horizontal wheel
// delta and nothing else.
static bool hs_frame_wheel_x(WLX_Context *ctx, char *buf, size_t buf_size, HS_Opt o,
                             int mx, int my, float wheel_delta_x) {
    WLX_Input_State in = {0};
    in.mouse_x = mx;
    in.mouse_y = my;
    in.wheel_delta_x = wheel_delta_x;
    hs_reset_capture();
    test_frame_begin_input(ctx, &in);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = hs_inputbox(ctx, buf, buf_size, o);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

// A drag held past the right edge auto-scrolls the row and grows the
// selection to the end; held past the left edge it scrolls back to the
// start. The anchor never moves.
TEST(hscroll_drag_past_edges_autoscrolls) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, 0);
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;

    // Press two glyphs in (the anchor), then hold past the right edge.
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, (int)hs_band().x + 10, 20, true, true);
    ASSERT_EQ_INT(2, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(2, (int)st->caret.selection_anchor);
    for (int i = 0; i < 60; i++) {
        hs_frame_mouse(&ctx, buf, sizeof(buf), o, 118, 20, true, false);
        ASSERT_EQ_INT(2, (int)st->caret.selection_anchor);
    }
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_EQ_INT(40, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_anchor_inside(&ctx));

    // Hold past the left edge: back to the start.
    for (int i = 0; i < 120; i++) {
        hs_frame_mouse(&ctx, buf, sizeof(buf), o, 0, 20, true, false);
        ASSERT_EQ_INT(2, (int)st->caret.selection_anchor);
    }
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_INT(0, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_anchor_inside(&ctx));
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, 0, 20, false, false);
    ASSERT_EQ_INT(2, (int)st->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

// A hovered, focused, overflowing row consumes the horizontal wheel and
// moves the view without moving the caret; the next caret change snaps
// the view back to the caret.
TEST(hscroll_wheel_focused_overflow_consumes) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);

    // A positive delta scrolls the offset back toward the start.
    hs_frame_wheel_x(&ctx, buf, sizeof(buf), o, 60, 20, 1.0f);
    ASSERT_EQ_F(0.0f, ctx.input.wheel_delta_x, 0.0001f);
    ASSERT_EQ_F(max_scroll - WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED, st->scroll_x, 0.01f);
    ASSERT_EQ_INT(40, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(0, hs_caret_draws);

    // A caret move snaps the view back so the caret is in view.
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(39, (int)st->caret.cursor_pos);
    ASSERT_TRUE(hs_caret_inside());
    ASSERT_EQ_F(39.0f * 5.0f + hs_caret_margin() - hs_band().w, st->scroll_x, 0.01f);
    wlx_context_destroy(&ctx);
}

// A fitting row, and an unfocused overflowing row (pinned at its start),
// leave the horizontal wheel delta to the enclosing panel.
TEST(hscroll_wheel_left_alone_when_nothing_can_move) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "abc";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    hs_frame_wheel_x(&ctx, buf, sizeof(buf), o, 60, 20, -1.0f);
    ASSERT_EQ_F(-1.0f, ctx.input.wheel_delta_x, 0.0001f);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, 0);
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    ASSERT_TRUE(st->scroll_x > 0.0f);
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_ESCAPE, 0);
    bool focused = hs_frame_wheel_x(&ctx, buf, sizeof(buf), o, 60, 20, -1.0f);
    ASSERT_FALSE(focused);
    ASSERT_EQ_F(-1.0f, ctx.input.wheel_delta_x, 0.0001f);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_F(hs_band().x, hs_text_x0, 0.01f);
    wlx_context_destroy(&ctx);
}

// HOME and END on a scrolled row move the view with the caret: the row
// start at the band, the row end one caret margin inside its right edge;
// the command modifier's buffer jumps do the same.
TEST(hscroll_home_end_on_scrolled_row) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;
    WLX_Rect band = hs_band();

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, 0);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_EQ_F(band.x, hs_text_x0, 0.01f);
    ASSERT_TRUE(hs_caret_inside());
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, 0);
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_EQ_F(band.x + band.w - hs_caret_margin(), hs_caret_x, 0.01f);
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, test_command_mod());
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_END, test_command_mod());
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// A keyboard focus follows the caret too: the blur reset the view to the
// row start and the focus frame parks the caret at the end, where the
// last focused frame latched it, so no caret move marks the frame. Tab
// with nothing focused walks to the field.
TEST(hscroll_keyboard_refocus_follows_caret) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_ESCAPE, 0);
    ASSERT_FALSE(hs_frame_type(&ctx, buf, sizeof(buf), o, NULL));
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);

    ASSERT_TRUE(hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_TAB, 0));
    ASSERT_EQ_INT(40, (int)st->caret.cursor_pos);
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_TRUE(hs_caret_inside());
    ASSERT_TRUE(hs_anchor_inside(&ctx));

    // The settled frame keeps the view there.
    ASSERT_TRUE(hs_frame_type(&ctx, buf, sizeof(buf), o, NULL));
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// Text entering a single-line field loses its line separators, whether
// pasted or typed: the field stays one row and the paste undoes as one
// step. A multiline field keeps them.
TEST(hscroll_single_line_drops_line_separators) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);

    test_set_clipboard("abc\r\ndef\nghi\r");
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_V, test_command_mod());
    ASSERT_EQ_STR("abcdefghi", buf);
    ASSERT_EQ_INT(9, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(1, hs_text_draws);
    ASSERT_TRUE(hs_caret_inside());

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_Z, test_command_mod());
    ASSERT_EQ_STR("", buf);
    ASSERT_EQ_INT(0, (int)st->caret.cursor_pos);

    hs_frame_type(&ctx, buf, sizeof(buf), o, "x\ny");
    ASSERT_EQ_STR("xy", buf);
    ASSERT_EQ_INT(1, hs_text_draws);
    wlx_context_destroy(&ctx);

    WLX_Context mctx;
    hs_ctx_init(&mctx);
    char mbuf[128] = "";
    HS_Opt m = HS_DEFAULT;
    m.multiline = true;
    ASSERT_TRUE(hs_frame_mouse(&mctx, mbuf, sizeof(mbuf), m, 60, 20, true, true));
    test_set_clipboard("abc\ndef");
    hs_frame_key(&mctx, mbuf, sizeof(mbuf), m, WLX_KEY_V, test_command_mod());
    ASSERT_EQ_STR("abc\ndef", mbuf);
    ASSERT_EQ_INT(2, hs_text_draws);
    wlx_context_destroy(&mctx);
}

// The wheel can scroll the caret out past the left edge as well as the
// right; a caret outside the band is not drawn on either side, and the
// next caret move brings it back.
TEST(hscroll_wheel_out_left_hides_caret) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, 0);
    ASSERT_EQ_F(0.0f, st->scroll_x, 0.0f);
    ASSERT_TRUE(hs_caret_inside());

    // A negative delta scrolls the view on toward the row end.
    hs_frame_wheel_x(&ctx, buf, sizeof(buf), o, 60, 20, -1.0f);
    ASSERT_EQ_F(WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED, st->scroll_x, 0.01f);
    ASSERT_EQ_F(hs_band().x - WLX_SCROLL_PANEL_DEFAULT_WHEEL_SCROLL_SPEED, hs_text_x0, 0.01f);
    ASSERT_EQ_INT(0, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(0, hs_caret_draws);

    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(1, (int)st->caret.cursor_pos);
    ASSERT_EQ_F(5.0f, st->scroll_x, 0.01f);
    ASSERT_TRUE(hs_caret_inside());
    wlx_context_destroy(&ctx);
}

// During a drag past the right edge the hit is clamped to the band, so
// the view advances by the auto-scroll gain, not by the pointer's
// overshoot: a pointer held 60 px out needs several frames to reach the
// row end, where an unclamped hit would have jumped there at once.
TEST(hscroll_drag_hit_clamps_to_band_edge) {
    WLX_Context ctx;
    hs_ctx_init(&ctx);
    char buf[128] = "";
    HS_Opt o = HS_DEFAULT;
    ASSERT_TRUE(hs_frame_mouse(&ctx, buf, sizeof(buf), o, 60, 20, true, true));
    hs_frame_type(&ctx, buf, sizeof(buf), o, "0123456789012345678901234567890123456789");
    hs_frame_key(&ctx, buf, sizeof(buf), o, WLX_KEY_HOME, 0);
    WLX_Inputbox_State *st = hs_state(&ctx);
    ASSERT_TRUE(st != NULL);
    float max_scroll = 40.0f * 5.0f + hs_caret_margin() - hs_band().w;
    WLX_Rect band = hs_band();
    int far_right = (int)(band.x + band.w + WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT);

    hs_frame_mouse(&ctx, buf, sizeof(buf), o, (int)band.x + 10, 20, true, true);
    ASSERT_EQ_INT(2, (int)st->caret.cursor_pos);
    float prev = st->scroll_x;
    int frames_to_end = 0;
    for (int i = 0; i < 60 && st->scroll_x < max_scroll - 0.01f; i++) {
        hs_frame_mouse(&ctx, buf, sizeof(buf), o, far_right, 20, true, false);
        // Per frame: the gain's step plus at most the caret room a
        // boundary at the edge adds; never the 60 px overshoot.
        float step = st->scroll_x - prev;
        ASSERT_TRUE(step <= WLX_TEXT_DRAG_SCROLL_MAX_OVERSHOOT * WLX_TEXT_DRAG_SCROLL_GAIN / 60.0f
                            + 5.0f + hs_caret_margin() + 0.01f);
        ASSERT_TRUE(hs_anchor_inside(&ctx));
        prev = st->scroll_x;
        frames_to_end++;
    }
    ASSERT_EQ_F(max_scroll, st->scroll_x, 0.01f);
    ASSERT_TRUE(frames_to_end >= 4);
    ASSERT_EQ_INT(40, (int)st->caret.cursor_pos);
    ASSERT_EQ_INT(2, (int)st->caret.selection_anchor);
    hs_frame_mouse(&ctx, buf, sizeof(buf), o, far_right, 20, false, false);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_hscroll) {
    RUN_TEST(hscroll_single_line_ignores_wrap_option);
    RUN_TEST(hscroll_default_wrap_stays_true_for_multiline);
    RUN_TEST(hscroll_fitting_row_keeps_band_alignment);
    RUN_TEST(hscroll_row_y_constant_typing_past_edge);
    RUN_TEST(hscroll_caret_stays_inside_band);
    RUN_TEST(hscroll_caret_drawn_at_row_end);
    RUN_TEST(hscroll_click_on_scrolled_row_hits_under_pointer);
    RUN_TEST(hscroll_delete_clamps_scroll_back);
    RUN_TEST(hscroll_blur_resets_view);
    RUN_TEST(hscroll_alignment_yields_to_scroll_on_overflow);
    RUN_TEST(hscroll_password_mask_scrolls);
    RUN_TEST(hscroll_composition_anchor_inside_band);
    RUN_TEST(hscroll_unit_cap_row_end_reachable_then_frozen);
    RUN_TEST(hscroll_drag_past_edges_autoscrolls);
    RUN_TEST(hscroll_wheel_focused_overflow_consumes);
    RUN_TEST(hscroll_wheel_left_alone_when_nothing_can_move);
    RUN_TEST(hscroll_home_end_on_scrolled_row);
    RUN_TEST(hscroll_keyboard_refocus_follows_caret);
    RUN_TEST(hscroll_single_line_drops_line_separators);
    RUN_TEST(hscroll_wheel_out_left_hides_caret);
    RUN_TEST(hscroll_drag_hit_clamps_to_band_edge);
}

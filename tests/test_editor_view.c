// test_editor_view.c - wlx_editor read-only windowed view: line index
// construction and guards (length / revision / boundary-byte probe, idle
// frames rebuild nothing), anchor clamp on document shrink, wheel
// consume/leave rules (incl. editor inside a scroll panel), exact vertical
// thumb proportion, Shift+wheel and horizontal-axis wheel scroll,
// PageUp/PageDown, and the window build drawing exactly the anchored lines.
//
// Geometry model (mock backend): char width = font_size/2, line height =
// font_size. Fixture: 400x100 context, content_padding 4, border 0,
// font_size 10 -> input_rect {4,4,392,92}, band {9,4,383,92} before strips,
// line_h 10.

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

// One editor per test: the single state entry of matching size is its state,
// and the single index cache entry is its line index.
static WLX_Editor_State *ev_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Editor_State))
            return (WLX_Editor_State *)slot->data;
    }
    return NULL;
}

static WLX_Editor_Line_Index *ev_index(WLX_Context *ctx) {
    return ctx->editor_indices.count > 0 ? &ctx->editor_indices.items[0] : NULL;
}

static bool ev_editor(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t revision) {
    bool focused = false;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .revision = revision, .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool ev_frame_full(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                          int mx, int my, bool down, bool clicked, float wheel, uint32_t mods,
                          const bool *keys_pressed) {
    test_frame_begin_full(ctx, mx, my, down, clicked, down, wheel, NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ev_editor(ctx, buf, cap, len, rev);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool ev_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev) {
    return ev_frame_full(ctx, buf, cap, len, rev, 0, 0, false, false, 0.0f, 0, NULL);
}

// Whole-struct staging variant for contract fields the parameter helpers do
// not carry (horizontal wheel, right/middle buttons).
static bool ev_frame_input(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                           const WLX_Input_State *in) {
    test_frame_begin_input(ctx, in);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ev_editor(ctx, buf, cap, len, rev);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool ev_frame_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                         WLX_Key_Code key) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    return ev_frame_full(ctx, buf, cap, len, rev, 200, 50, false, false, 0.0f, 0, keys_pressed);
}

// Fill buf with n short lines "l00\n".."lNN\n" (4 bytes each), returning the
// length. Content height with line_h 10 is (n + trailing empty) * 10.
static size_t ev_fill_lines(char *buf, size_t cap, size_t n) {
    size_t off = 0;
    for (size_t i = 0; i < n && off + 4 < cap; i++) {
        buf[off++] = 'l';
        buf[off++] = (char)('0' + (i / 10) % 10);
        buf[off++] = (char)('0' + i % 10);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// ============================================================================
// Draw-text capture (records replay through the mock backend at wlx_end)
// ============================================================================

#define EV_MAX_CAPTURES_ 32

static struct { char text[64]; float x; float y; } _ev_captures[EV_MAX_CAPTURES_];
static int _ev_capture_count = 0;

static void _ev_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)style;
    if (_ev_capture_count < EV_MAX_CAPTURES_) {
        const char *src = text ? text : "";
        size_t len = strlen(src);
        if (len >= sizeof(_ev_captures[0].text)) len = sizeof(_ev_captures[0].text) - 1;
        memcpy(_ev_captures[_ev_capture_count].text, src, len);
        _ev_captures[_ev_capture_count].text[len] = '\0';
        _ev_captures[_ev_capture_count].x = x;
        _ev_captures[_ev_capture_count].y = y;
        _ev_capture_count++;
    }
}

static void _ev_reset_captures(void) {
    memset(_ev_captures, 0, sizeof(_ev_captures));
    _ev_capture_count = 0;
}

// Rect capture for the scrollbar thumb assertions.
#define EV_MAX_RECTS_ 128

static struct { WLX_Rect r; } _ev_rects[EV_MAX_RECTS_];
static int _ev_rect_count = 0;

static void _ev_capture_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)c;
    if (_ev_rect_count < EV_MAX_RECTS_) _ev_rects[_ev_rect_count++].r = r;
}

static void _ev_reset_rects(void) {
    memset(_ev_rects, 0, sizeof(_ev_rects));
    _ev_rect_count = 0;
}

static bool _ev_rect_captured(WLX_Rect want, float eps) {
    for (int i = 0; i < _ev_rect_count; i++) {
        WLX_Rect g = _ev_rects[i].r;
        if (fabsf(g.x - want.x) <= eps && fabsf(g.y - want.y) <= eps
            && fabsf(g.w - want.w) <= eps && fabsf(g.h - want.h) <= eps) return true;
    }
    return false;
}

// ============================================================================
// Line index construction and guards
// ============================================================================

TEST(editor_index_matches_build_ground_truth) {
    static const char *corpora[] = {
        "alpha\nbeta\ngamma",
        "a\r\nb\r\n",
        "one line",
        "h\xC3\xB6la\n\xC3\xB6",
        "\n\n",
        "trailing\n",
    };

    for (size_t c = 0; c < sizeof(corpora) / sizeof(corpora[0]); c++) {
        WLX_Context ctx;
        test_ctx_init(&ctx, 400, 100);

        char buf[128];
        size_t len = strlen(corpora[c]);
        memcpy(buf, corpora[c], len + 1);

        ev_frame(&ctx, buf, sizeof(buf), &len, 0);

        WLX_Editor_Line_Index *idx = ev_index(&ctx);
        ASSERT_TRUE(idx != NULL);

        // Ground truth: a full wide-rect build of the same text; each record
        // is one hard line, so record source_starts are the line starts.
        WLX_Text_Line_Record lines[16];
        test_frame_begin(&ctx, 0, 0, false, false);
        WLX_Text_Build_Inputs inputs = {
            .ctx = &ctx, .text = buf, .length = len,
            .style = (WLX_Text_Style){ .font_size = 10 },
            .rect = { 0, 0, 10000.0f, 10000.0f },
            .wrap = false, .line_h = 10.0f,
            .text_unit_cap = 1u << 20,
        };
        WLX_Text_Build_Cursor cursor = {0};
        size_t truth_count = wlx_text_build_lines(&inputs, &cursor, lines, 16);
        test_frame_end(&ctx);

        ASSERT_EQ_INT((long)truth_count, (long)idx->count);
        for (size_t i = 0; i < truth_count; i++) {
            ASSERT_EQ_INT((long)lines[i].source_start, (long)idx->offsets[i]);
        }

        wlx_context_destroy(&ctx);
    }

    // An empty document is one empty line.
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    char buf[8] = "";
    size_t len = 0;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(1, (long)idx->count);
    ASSERT_EQ_INT(0, (long)idx->offsets[0]);
    wlx_context_destroy(&ctx);
}

TEST(editor_index_idle_frames_never_rebuild) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 20);

    for (int i = 0; i < 6; i++) ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(1, (long)idx->rebuilds);
    ASSERT_EQ_INT(21, (long)idx->count); // 20 lines + trailing empty line
    wlx_context_destroy(&ctx);
}

TEST(editor_index_guard_length_change) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 5);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    buf[len++] = 'x'; // same revision, longer document
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(2, (long)idx->rebuilds);
    // The trailing empty line became a one-char line: still 6 entries.
    ASSERT_EQ_INT(6, (long)idx->count);
    wlx_context_destroy(&ctx);
}

TEST(editor_index_guard_revision_bump) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 5);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(1, (long)idx->rebuilds);

    ev_frame(&ctx, buf, sizeof(buf), &len, 1); // revision bump, same bytes
    ASSERT_EQ_INT(2, (long)idx->rebuilds);
    wlx_context_destroy(&ctx);
}

TEST(editor_index_guard_boundary_probe) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "aaa\nbbb\nccc\nddd", 16);
    size_t len = 15;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(4, (long)idx->count); // offsets 0, 4, 8, 12

    // Overwrite the separator before the probed middle entry (offset 8):
    // same length, same revision - only the probe can catch it.
    buf[7] = 'x';
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_INT(2, (long)idx->rebuilds);
    ASSERT_EQ_INT(3, (long)idx->count);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Anchor
// ============================================================================

TEST(editor_anchor_clamps_on_document_shrink) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 20);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Park the anchor at the last line; the frame clamps the pixel scroll to
    // content_h - band_h = 210 - 92 = 118 -> 11.8 lines.
    st->first_line = 20;
    st->y_frac = 0.0f;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_INT(11, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.01f);

    // Shrink to 5 lines (content 60 < band 92): scroll clamps to zero.
    len = ev_fill_lines(buf, sizeof(buf), 5);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_INT(0, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Wheel rules
// ============================================================================

TEST(editor_wheel_scrolls_and_consumes_when_overflowing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 20);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Hovered wheel-down: scroll_y 0 -> 40px = 4 lines; delta consumed.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, -2.0f, 0, NULL);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(4, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.0f, 0.001f);
    ASSERT_EQ_F(ctx.input.wheel_delta, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(editor_wheel_left_alone_when_content_fits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 3);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, -2.0f, 0, NULL);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (long)st->first_line);
    ASSERT_EQ_F(ctx.input.wheel_delta, -2.0f, 0.001f); // left to enclosing panels
    wlx_context_destroy(&ctx);
}

TEST(editor_wheel_unhovered_left_alone) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 20);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Pointer outside the widget: the overflowing editor must not steal the
    // wheel from whatever the pointer is actually over.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, -10, -10, false, false, -2.0f, 0, NULL);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (long)st->first_line);
    wlx_context_destroy(&ctx);
}

// Editor inside a scroll panel: with editor overflow the editor consumes and
// the panel holds still; with editor content fitting the panel scrolls.
TEST(editor_inside_scroll_panel_wheel_routing) {
    for (int overflowing = 0; overflowing <= 1; overflowing++) {
        WLX_Context ctx;
        test_ctx_init(&ctx, 400, 300);

        char buf[512];
        size_t len = ev_fill_lines(buf, sizeof(buf), overflowing ? 40 : 3);

        for (int f = 0; f < 2; f++) {
            // The 200-tall editor centers in the panel's 600px content, so it
            // spans y 200..400; y 250 is inside both editor and viewport.
            float wheel = (f == 1) ? -2.0f : 0.0f;
            test_frame_begin_ex(&ctx, 200, 250, false, false, false, wheel, NULL, NULL, NULL);
            wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
            wlx_scroll_panel_begin_impl(&ctx, 600.0f,
                wlx_default_scroll_panel_opt(), "ev_panel", 1);
            (void)wlx_editor_impl(&ctx, NULL, buf, sizeof(buf), &len,
                wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
                    .border_width = 0, .height = 200),
                "ev_in_panel", 1);
            wlx_scroll_panel_end(&ctx);
            wlx_layout_end(&ctx);
            test_frame_end(&ctx);
        }

        WLX_Scroll_Panel_State *panel = NULL;
        for (size_t i = 0; i < ctx.states.capacity; i++) {
            WLX_State_Map_Slot *slot = &ctx.states.slots[i];
            if (slot->id != 0 && slot->data_size == sizeof(WLX_Scroll_Panel_State))
                panel = (WLX_Scroll_Panel_State *)slot->data;
        }
        WLX_Editor_State *st = ev_state(&ctx);
        ASSERT_TRUE(panel != NULL);
        ASSERT_TRUE(st != NULL);

        if (overflowing) {
            ASSERT_TRUE(st->first_line > 0);            // editor scrolled
            ASSERT_EQ_F(panel->scroll_offset, 0.0f, 0.001f); // panel held still
        } else {
            ASSERT_EQ_INT(0, (long)st->first_line);
            ASSERT_TRUE(panel->scroll_offset > 0.0f);   // panel scrolled
        }

        wlx_context_destroy(&ctx);
    }
}

TEST(editor_shift_wheel_scrolls_horizontally) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // One 200-char line is 1000px wide. The window build truncates it at
    // the band, so its measured width can never exceed the band; the open
    // reach flag is what extends the horizontal range past the truncation.
    char buf[256];
    memset(buf, 'a', 200);
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->h_reach_open);

    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, -2.0f, WLX_MOD_SHIFT, NULL);
    ASSERT_EQ_F(st->scroll_x, 40.0f, 0.001f);
    ASSERT_EQ_F(ctx.input.wheel_delta, 0.0f, 0.001f);

    // Without overflow on the horizontal axis the Shift+wheel is left alone.
    WLX_Context ctx2;
    test_ctx_init(&ctx2, 400, 100);
    char buf2[64];
    size_t len2 = ev_fill_lines(buf2, sizeof(buf2), 3);
    ev_frame(&ctx2, buf2, sizeof(buf2), &len2, 0);
    ev_frame_full(&ctx2, buf2, sizeof(buf2), &len2, 0, 200, 50, false, false, -2.0f, WLX_MOD_SHIFT, NULL);
    ASSERT_EQ_F(ctx2.input.wheel_delta, -2.0f, 0.001f);
    wlx_context_destroy(&ctx2);
    wlx_context_destroy(&ctx);
}

// The horizontal wheel axis scrolls scroll_x directly, no Shift needed;
// without horizontal overflow the delta is left for enclosing consumers.
TEST(editor_horizontal_wheel_scrolls_x) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    memset(buf, 'a', 200);
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->h_reach_open);

    WLX_Input_State in = {0};
    in.mouse_x = 200;
    in.mouse_y = 50;
    in.wheel_delta_x = -2.0f;
    ev_frame_input(&ctx, buf, sizeof(buf), &len, 0, &in);
    ASSERT_EQ_F(st->scroll_x, 40.0f, 0.001f);
    ASSERT_EQ_F(ctx.input.wheel_delta_x, 0.0f, 0.001f);

    // Content fits horizontally: the delta must survive the frame.
    WLX_Context ctx2;
    test_ctx_init(&ctx2, 400, 100);
    char buf2[64];
    size_t len2 = ev_fill_lines(buf2, sizeof(buf2), 3);
    ev_frame(&ctx2, buf2, sizeof(buf2), &len2, 0);
    ev_frame_input(&ctx2, buf2, sizeof(buf2), &len2, 0, &in);
    ASSERT_EQ_F(ctx2.input.wheel_delta_x, -2.0f, 0.001f);
    wlx_context_destroy(&ctx2);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// PageUp / PageDown (caret-coupled: viewport-sized caret motion + follow)
// ============================================================================

TEST(editor_page_keys_move_caret_by_viewport) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);

    // Focus with a click on line 4 ("l04", 4 bytes per line): the x lands
    // past the 3-char text, so the caret clamps to the line end.
    bool focused = ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, true, true, 0.0f, 0, NULL);
    ASSERT_TRUE(focused);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(4 * 4 + 3, (long)st->caret.cursor_pos);

    // PageDown moves the caret one viewport (92px band / 10px lines = 9
    // lines) at the sticky column; caret-follow drags the view along:
    // caret line 13 -> scroll = 13*10 + 10 - 92 = 48.
    ev_frame_key(&ctx, buf, sizeof(buf), &len, 0, WLX_KEY_PAGE_DOWN);
    ASSERT_EQ_INT(13 * 4 + 3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(4, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.01f);

    // PageUp moves the caret back; follow snaps the caret line into view.
    ev_frame_key(&ctx, buf, sizeof(buf), &len, 0, WLX_KEY_PAGE_UP);
    ASSERT_EQ_INT(4 * 4 + 3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(4, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Exact vertical thumb + window draw
// ============================================================================

TEST(editor_vertical_thumb_exact_from_line_count) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40); // content_h = 410

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Thumb from the exact content height 41 * 10 = 410: track {4,4,392,92},
    // thumb h = 92/410*92, y = 4 + 0, x at the track's right edge.
    float sb_w = ctx.theme->scrollbar.width > 0.0f ? ctx.theme->scrollbar.width : 10.0f;
    WLX_Rect want = wlx_scrollbar_rect((WLX_Rect){ 4, 4, 392, 92 }, 410.0f, 0.0f, sb_w);
    ASSERT_TRUE(_ev_rect_captured(want, 0.01f));
    wlx_context_destroy(&ctx);
}

// A long document's proportional thumb is a fraction of a pixel: it draws
// at the floor, and a drag to the track end still reaches the document end.
TEST(editor_vertical_thumb_floors_on_long_document) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    static char buf[8192];
    size_t len = ev_fill_lines(buf, sizeof(buf), 2000); // content_h = 20010

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Track {4,4,392,92}: the proportional thumb would be 92/20010*92
    // (0.42px); it draws WLX_SCROLLBAR_MIN_THUMB tall at the track top.
    float sb_w = ctx.theme->scrollbar.width > 0.0f ? ctx.theme->scrollbar.width : 10.0f;
    WLX_Rect want = { 4 + 392 - sb_w, 4, sb_w, WLX_SCROLLBAR_MIN_THUMB };
    ASSERT_TRUE(_ev_rect_captured(want, 0.01f));

    // Press the thumb and drag far below the track: the position clamps
    // to the leftover track, which maps to the scroll limit.
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    int tx = (int)(want.x + want.w * 0.5f);
    int ty = (int)(want.y + want.h * 0.5f);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, tx, ty, true, true, 0.0f, 0, NULL);
    ASSERT_TRUE(st->caret.dragging_scrollbar);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, tx, 500, true, false, 0.0f, 0, NULL);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, tx, 500, false, false, 0.0f, 0, NULL);

    // max_scroll 20010 - 92 = 19918 -> anchor line 1991, frac 0.8.
    ASSERT_EQ_INT(1991, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.01f);

    // The idle thumb now ends at the track bottom.
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    want.y = 4 + 92 - WLX_SCROLLBAR_MIN_THUMB;
    ASSERT_TRUE(_ev_rect_captured(want, 0.01f));
    wlx_context_destroy(&ctx);
}

// Horizontal thumb among the captured rects: one strip (10px) tall and
// narrower than the widget background.
static bool _ev_find_hbar_thumb(WLX_Rect *out) {
    for (int i = 0; i < _ev_rect_count; i++) {
        WLX_Rect g = _ev_rects[i].r;
        if (g.h == 10.0f && g.w < 390.0f) {
            if (out) *out = g;
            return true;
        }
    }
    return false;
}

// The horizontal bar's visibility follows the window's reach flag, but the
// bar also shortens the band. The window must size itself from the pre-strip
// height: sized from the post-strip band, a long line sitting at the window's
// bottom edge would leave the window when the bar appears and re-enter when
// it hides, blinking the bar on alternating idle frames.
TEST(editor_hbar_visibility_stable_at_window_edge) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300); // band {9,4,383,292}, 31-line viewport
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    // 40 short lines, one 200-char line (1000px, document line index 40),
    // 5 short lines. Vertical overflow keeps the vertical bar on.
    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    memset(buf + len, 'x', 200);
    len += 200;
    buf[len++] = '\n';
    for (int i = 0; i < 5; i++) {
        buf[len++] = 't'; buf[len++] = '0'; buf[len++] = (char)('0' + i);
        buf[len++] = '\n';
    }
    buf[len] = '\0';

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Every anchor must render a constant bar across idle frames; the bar is
    // due whenever the long line falls inside the window (viewport 31 lines
    // plus overscan). first_line 8 puts it exactly on the window's last
    // record - the anchor that oscillated when the window followed band.h.
    for (size_t fl = 0; fl <= 40; fl++) {
        st->first_line = fl;
        st->y_frac = 0.0f;
        st->scroll_x = 0.0f;
        // The revision bump makes the settle frame rebuild the index and
        // release the width reach - the widget's own document-change reset,
        // which this loop used to hand-roll by zeroing the reach fields.
        ev_frame(&ctx, buf, sizeof(buf), &len, (uint32_t)fl + 1);
        bool want = fl + 31 + WLX_EDITOR_OVERSCAN_LINES > 40;
        for (int f = 0; f < 4; f++) {
            _ev_reset_rects();
            ev_frame(&ctx, buf, sizeof(buf), &len, (uint32_t)fl + 1);
            ASSERT_TRUE(_ev_find_hbar_thumb(NULL) == want);
        }
    }
    wlx_context_destroy(&ctx);
}

// The thumb drawn on the frame a wheel notch moves scroll_x must land where
// the next frame maps it: drawn from the frame-start horizontal range (stale
// once scroll_x changed), it sat one frame ahead and pulsed on every notch.
TEST(editor_hbar_thumb_stable_after_wheel_notch) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[256];
    memset(buf, 'a', 200); // one 200-char line, 1000px wide
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    for (int notch = 0; notch < 4; notch++) {
        _ev_reset_rects();
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      -2.0f, WLX_MOD_SHIFT, NULL);
        WLX_Rect wheel_thumb = {0};
        ASSERT_TRUE(_ev_find_hbar_thumb(&wheel_thumb));

        _ev_reset_rects();
        ev_frame(&ctx, buf, sizeof(buf), &len, 0);
        WLX_Rect idle_thumb = {0};
        ASSERT_TRUE(_ev_find_hbar_thumb(&idle_thumb));

        ASSERT_EQ_F(wheel_thumb.x, idle_thumb.x, 0.001f);
        ASSERT_EQ_F(wheel_thumb.w, idle_thumb.w, 0.001f);
    }
    wlx_context_destroy(&ctx);
}

// A thumb drag maps the pointer through the horizontal range, but the live
// range follows scroll_x and the reach flag - both changed by the drag's own
// output. A gesture must map through the range frozen at the press: mapped
// live, a thumb held still near a long line's end re-maps to a new scroll_x
// every frame, and the view oscillates between the drag position and past
// the line end.
TEST(editor_hbar_drag_hold_is_stable_on_long_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[256];
    memset(buf, 'a', 200); // one 200-char line, 1000px wide
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Shift+wheel to the line's end (scroll_x clamps at the line width
    // plus caret room: 1000 + 4 - 383 = 621),
    // then one idle frame to capture the settled thumb.
    for (int i = 0; i < 40; i++) {
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      -2.0f, WLX_MOD_SHIFT, NULL);
    }
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Rect thumb = {0};
    ASSERT_TRUE(_ev_find_hbar_thumb(&thumb));

    // Press the thumb's center, then hold the pointer still at ~55% of the
    // track (hb_track {9,86,383,10}) - inside the window where the reach
    // flag flips between open and closed on alternating frames.
    int press_x = (int)(thumb.x + thumb.w * 0.5f);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, press_x, 90, true, true,
                  0.0f, 0, NULL);
    int hold_x = (int)(9.0f + ((float)press_x - thumb.x) + 0.55f * 383.0f);

    float held_scroll_x = 0.0f;
    WLX_Rect held_thumb = {0};
    for (int f = 0; f < 6; f++) {
        _ev_reset_rects();
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, hold_x, 90, true, false,
                      0.0f, 0, NULL);
        WLX_Rect t = {0};
        ASSERT_TRUE(_ev_find_hbar_thumb(&t));
        if (f == 0) {
            held_scroll_x = st->scroll_x;
            held_thumb = t;
            continue;
        }
        ASSERT_EQ_F(st->scroll_x, held_scroll_x, 0.001f);
        ASSERT_EQ_F(t.x, held_thumb.x, 0.001f);
        ASSERT_EQ_F(t.w, held_thumb.w, 0.001f);
    }

    // Release: the view and the thumb stay where the drag left them - the
    // live range must land where the frozen gesture range left off.
    _ev_reset_rects();
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, hold_x, 90, false, false,
                  0.0f, 0, NULL);
    ASSERT_EQ_F(st->scroll_x, held_scroll_x, 0.001f);
    WLX_Rect released = {0};
    ASSERT_TRUE(_ev_find_hbar_thumb(&released));
    ASSERT_EQ_F(released.x, held_thumb.x, 0.001f);
    ASSERT_EQ_F(released.w, held_thumb.w, 0.001f);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_F(st->scroll_x, held_scroll_x, 0.001f);
    wlx_context_destroy(&ctx);
}

// The thumb maps only through the content measured so far, never the
// reach-open scroll-limit extension: a reach-dependent thumb range snaps
// when the flag flips at a long line's end (one notch back jumped the
// thumb far left and shrank it). The window build measures one band ahead
// of the view, so at scroll_x 0 the thumb already leaves drag room, the
// range grows smoothly as the line is discovered, and it plateaus at the
// true width with no jump on any frame.
TEST(editor_hbar_thumb_continuous_across_reach_flip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[256];
    memset(buf, 'a', 200); // one 200-char line, 1000px wide
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // At scroll_x 0 the lookahead has measured ~two bands of the line: the
    // thumb sits at the track's left with about half the track free to
    // drag into - never the full track.
    WLX_Rect prev = {0};
    ASSERT_TRUE(_ev_find_hbar_thumb(&prev));
    ASSERT_EQ_F(prev.x, 9.0f, 0.01f);
    ASSERT_TRUE(prev.w > 150.0f && prev.w < 250.0f);

    // Wheel right to the end: the thumb advances every notch, its width
    // never grows, and no frame steps discontinuously - including the one
    // where the lookahead finds the line's true end (reach closes).
    for (int i = 0; i < 16; i++) {
        _ev_reset_rects();
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      -2.0f, WLX_MOD_SHIFT, NULL);
        WLX_Rect t = {0};
        ASSERT_TRUE(_ev_find_hbar_thumb(&t));
        ASSERT_TRUE(t.x > prev.x);
        ASSERT_TRUE(t.x - prev.x < 25.0f);
        ASSERT_TRUE(t.w <= prev.w + 0.001f);
        ASSERT_TRUE(prev.w - t.w < 12.0f);
        prev = t;
    }

    // Settled at the end: scroll_x clamps at the content extent (line
    // width 1000 + caret room 4) minus the band: 1004 - 383 = 621, and the
    // thumb's right edge sits exactly at the track end (band.x + band.w).
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_TRUE(_ev_find_hbar_thumb(&prev));
    ASSERT_EQ_F(prev.x + prev.w, 392.0f, 0.01f);

    // Wheel back across the reach reopen: each 40px notch moves the thumb
    // exactly 40 * track_w / 1004 = 15.26px left at constant width. The
    // flag flipping open must not re-map the range.
    for (int i = 0; i < 3; i++) {
        _ev_reset_rects();
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      2.0f, WLX_MOD_SHIFT, NULL);
        WLX_Rect t = {0};
        ASSERT_TRUE(_ev_find_hbar_thumb(&t));
        ASSERT_EQ_F(t.w, prev.w, 0.001f);
        ASSERT_EQ_F(prev.x - t.x, 40.0f * 383.0f / 1004.0f, 0.01f);
        prev = t;
    }
    wlx_context_destroy(&ctx);
}

// Dragging the thumb away from scroll_x 0 must scroll the view: without
// the measure lookahead the thumb range collapsed onto the reach, so the
// thumb filled the whole track and the drag mapping had no room to move.
TEST(editor_hbar_drag_scrolls_from_origin) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[256];
    memset(buf, 'a', 200); // one 200-char line, 1000px wide
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    WLX_Rect thumb = {0};
    ASSERT_TRUE(_ev_find_hbar_thumb(&thumb));

    // Press the thumb's center and pull 100px right: the view follows.
    int press_x = (int)(thumb.x + thumb.w * 0.5f);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, press_x, 90, true, true,
                  0.0f, 0, NULL);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, press_x + 100, 90, true, false,
                  0.0f, 0, NULL);
    ASSERT_TRUE(st->scroll_x > 50.0f);
    wlx_context_destroy(&ctx);
}

// The max-seen width reach is sticky only within one document: deleting the
// widest line releases it at the rebuild, so the horizontal bar drops within
// a frame instead of staying inflated forever.
TEST(editor_hbar_drops_after_widest_line_deleted) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    // "s0", one 200-char line (1000px), "s1", "s2": no vertical overflow,
    // horizontal overflow only from the long line.
    char buf[256];
    memcpy(buf, "s0\n", 3);
    memset(buf + 3, 'a', 200);
    buf[203] = '\n';
    memcpy(buf + 204, "s1\ns2\n", 6);
    size_t len = 210;
    buf[len] = '\0';

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_TRUE(_ev_find_hbar_thumb(NULL));

    // Delete the widest line (bytes [3, 204) including its newline). The
    // length guard rebuilds the index on the next frame and releases the
    // reach; the frame after that draws no horizontal bar.
    memmove(buf + 3, buf + 204, len - 204);
    len -= 201;
    buf[len] = '\0';
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    _ev_reset_rects();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_TRUE(!_ev_find_hbar_thumb(NULL));

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_TRUE(st->max_line_w < 383.0f);
    wlx_context_destroy(&ctx);
}

// Releasing the reach on document change must not disturb an h-scrolled
// view being edited: the release may not feed the same frame's scroll_x
// clamp (a zeroed reach clamps to zero and snaps the view left), and the
// bar drawn on the edit frame may not blink off.
TEST(editor_edit_while_h_scrolled_keeps_view) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_rect = _ev_capture_draw_rect;

    char buf[256];
    memset(buf, 'a', 200); // one 200-char line, 1000px wide
    buf[200] = '\0';
    size_t len = 200;

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Shift+wheel 8 notches right: scroll_x = 8 * 2 * 20 = 320.
    for (int i = 0; i < 8; i++) {
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      -2.0f, WLX_MOD_SHIFT, NULL);
    }
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    float scroll_before = st->scroll_x;
    ASSERT_TRUE(scroll_before > 100.0f);

    // Click inside the scrolled view: focus plus a caret mid-band (byte
    // (150 - 9 + 320) / 5 = 92), then release in place.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 150, 9, true, true,
                  0.0f, 0, NULL);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 150, 9, false, false,
                  0.0f, 0, NULL);
    ASSERT_EQ_INT(92, (long)st->caret.cursor_pos);
    ASSERT_EQ_F(st->scroll_x, scroll_before, 0.001f);

    // Backspace: the document changes and the index rebuilds, but the view
    // stays put (the caret never left the band, so caret-follow is a no-op)
    // and the edit frame still draws the horizontal bar.
    _ev_reset_rects();
    ev_frame_key(&ctx, buf, sizeof(buf), &len, 0, WLX_KEY_BACKSPACE);
    ASSERT_EQ_INT(199, (long)len);
    ASSERT_EQ_INT(91, (long)st->caret.cursor_pos);
    ASSERT_EQ_F(st->scroll_x, scroll_before, 0.001f);
    ASSERT_TRUE(_ev_find_hbar_thumb(NULL));

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_F(st->scroll_x, scroll_before, 0.001f);
    wlx_context_destroy(&ctx);
}

// A press beside or below the horizontal strip is not a text click, and a
// press at the band's bottom edge belongs to the last visible row. Falling
// through, such a press hit-tested at band bottom - the first line past
// the window - teleporting the caret to an offscreen empty line, scrolling
// it into view, and snapping scroll_x back to the caret's column 0.
TEST(editor_click_under_hbar_strip_is_not_a_text_click) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // 8 short lines, a 200-char line (1000px, index 8), then a trailing
    // newline so index 9 is an empty line. content_h 100 > 92 keeps the
    // vertical bar on; the long line keeps the horizontal strip on. Bands:
    // {9,4,373,82}, strip row y [86,96), vertical bar column x [386,396).
    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 8);
    memset(buf + len, 'a', 200);
    len += 200;
    buf[len++] = '\n';
    buf[len] = '\0';

    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Anchor so the empty line 9 sits exactly flush at the band's bottom
    // edge (scroll_y 8 puts its top at y 86), scrolled into the long line.
    st->first_line = 0;
    st->y_frac = 0.8f;
    st->scroll_x = 300.0f;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Press the corner square beside the strip (right of the strip's
    // band-wide rect, inside the strip row): must be inert.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 388, 90, true, true,
                  0.0f, 0, NULL);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_F(st->scroll_x, 300.0f, 0.001f);
    ASSERT_TRUE(st->first_line == 0);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.001f);

    // Press inside the band's bottom row, just above the strip: the caret
    // lands on the long line - the last visible row, not the flush empty
    // line - and the view must not move on either axis.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 85, true, true,
                  0.0f, 0, NULL);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL && idx->count >= 10);
    ASSERT_TRUE(st->caret.cursor_pos >= idx->offsets[8]);
    ASSERT_TRUE(st->caret.cursor_pos < idx->offsets[9]);
    ASSERT_EQ_F(st->scroll_x, 300.0f, 0.001f);
    ASSERT_TRUE(st->first_line == 0);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(editor_window_draws_lines_at_anchor) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text = _ev_capture_draw_text;

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->first_line = 7;
    st->y_frac = 0.5f;

    _ev_reset_captures();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // First drawn line is document line 7 ("l07"), half a line above the
    // band top: y = 4 - 5 = -1.
    ASSERT_TRUE(_ev_capture_count > 0);
    ASSERT_EQ_STR(_ev_captures[0].text, "l07");
    ASSERT_EQ_F(_ev_captures[0].y, -1.0f, 0.01f);
    ASSERT_EQ_F(_ev_captures[0].x, 9.0f, 0.01f);

    // The window covers the viewport plus overscan, not the document: at
    // most band_h/line_h + 2 partials + overscan records.
    ASSERT_TRUE(_ev_capture_count <= 9 + 2 + WLX_EDITOR_OVERSCAN_LINES);

    // Horizontal scroll re-anchors records left of the band.
    st->scroll_x = 6.0f;
    st->max_line_w = 500.0f; // pretend a wide line was seen so the range exists
    _ev_reset_captures();
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_TRUE(_ev_capture_count > 0);
    ASSERT_EQ_F(_ev_captures[0].x, 3.0f, 0.01f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Line-number gutter
// ============================================================================

static bool evg_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                      int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = false;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .line_numbers = true, .out_focused = &focused),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

TEST(editor_gutter_numbers_and_band_shift) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text = _ev_capture_draw_text;

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40); // 41 lines -> 2 digits

    evg_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);
    _ev_reset_captures();
    evg_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);

    // Widest number "41" measures 10px; padding is 0.75 digits (3.75px)
    // per side, so the gutter is 17.5px wide: the text band starts at
    // 9 + 17.5 = 26.5, and numbers right-align at pad from the band.
    bool found_text = false, found_num1 = false;
    for (int i = 0; i < _ev_capture_count; i++) {
        if (strcmp(_ev_captures[i].text, "l00") == 0) {
            found_text = true;
            ASSERT_EQ_F(_ev_captures[i].x, 26.5f, 0.01f);
        }
        if (strcmp(_ev_captures[i].text, "1") == 0) {
            found_num1 = true;
            ASSERT_EQ_F(_ev_captures[i].x, 9.0f + 17.5f - 3.75f - 5.0f, 0.01f);
            ASSERT_EQ_F(_ev_captures[i].y, 4.0f, 0.01f);
        }
    }
    ASSERT_TRUE(found_text);
    ASSERT_TRUE(found_num1);
    wlx_context_destroy(&ctx);
}

TEST(editor_gutter_press_is_inert) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);

    // Two plain frames teach the state the gutter width, then a press in
    // the gutter strip (x 9..24) must not focus.
    evg_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);
    evg_frame(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);
    bool focused = evg_frame(&ctx, buf, sizeof(buf), &len, 15, 30, true, true);
    ASSERT_FALSE(focused);

    // Focus with a click in the text band, note the caret, then press the
    // gutter again: still focused widget elsewhere keeps caret untouched.
    evg_frame(&ctx, buf, sizeof(buf), &len, 15, 30, false, false);
    focused = evg_frame(&ctx, buf, sizeof(buf), &len, 100, 50, true, true);
    ASSERT_TRUE(focused);
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    size_t caret_before = st->caret.cursor_pos;

    evg_frame(&ctx, buf, sizeof(buf), &len, 100, 50, false, false);
    evg_frame(&ctx, buf, sizeof(buf), &len, 15, 70, true, true);
    ASSERT_EQ_INT((long)caret_before, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT((long)caret_before, (long)st->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

SUITE(editor_view) {
    RUN_TEST(editor_index_matches_build_ground_truth);
    RUN_TEST(editor_index_idle_frames_never_rebuild);
    RUN_TEST(editor_index_guard_length_change);
    RUN_TEST(editor_index_guard_revision_bump);
    RUN_TEST(editor_index_guard_boundary_probe);
    RUN_TEST(editor_anchor_clamps_on_document_shrink);
    RUN_TEST(editor_wheel_scrolls_and_consumes_when_overflowing);
    RUN_TEST(editor_wheel_left_alone_when_content_fits);
    RUN_TEST(editor_wheel_unhovered_left_alone);
    RUN_TEST(editor_inside_scroll_panel_wheel_routing);
    RUN_TEST(editor_shift_wheel_scrolls_horizontally);
    RUN_TEST(editor_horizontal_wheel_scrolls_x);
    RUN_TEST(editor_page_keys_move_caret_by_viewport);
    RUN_TEST(editor_vertical_thumb_exact_from_line_count);
    RUN_TEST(editor_vertical_thumb_floors_on_long_document);
    RUN_TEST(editor_hbar_visibility_stable_at_window_edge);
    RUN_TEST(editor_hbar_thumb_stable_after_wheel_notch);
    RUN_TEST(editor_hbar_drag_hold_is_stable_on_long_line);
    RUN_TEST(editor_hbar_thumb_continuous_across_reach_flip);
    RUN_TEST(editor_hbar_drag_scrolls_from_origin);
    RUN_TEST(editor_hbar_drops_after_widest_line_deleted);
    RUN_TEST(editor_edit_while_h_scrolled_keeps_view);
    RUN_TEST(editor_click_under_hbar_strip_is_not_a_text_click);
    RUN_TEST(editor_window_draws_lines_at_anchor);
    RUN_TEST(editor_gutter_numbers_and_band_shift);
    RUN_TEST(editor_gutter_press_is_inert);
}

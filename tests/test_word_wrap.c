// test_word_wrap.c - word-boundary wrapping in the build kernel: a row cuts
// at its latest whitespace, a word wider than the row breaks inside it,
// overflowing whitespace hangs on the row it follows in display builds and
// cuts or opens the next row in editable builds (wrap_strict_ws), advance_w
// is the ink extent on rows another row follows, separators and budgets are
// untouched, and alignment and the scissor test read the ink extent.
//
// Geometry model (mock backend): char width = font_size/2 = 5 px at
// font_size 10, line height 10; a 50 px row holds ten bytes. Records are
// built straight through the kernel (wlx_text_build_lines) so the row
// width, the tab advance and the unit budget are under direct control.

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

#define WW_MAX_RECS 32

static WLX_Text_Style ww_style(void) {
    WLX_Text_Style ts = { .font = 0, .font_size = 10, .color = {255, 255, 255, 255}, .spacing = 0 };
    return ts;
}

// Build records for text at a row width inside one frame. unit_cap 0 keeps
// the standard run budget; strict_ws selects the editable-build whitespace
// rule.
static size_t ww_build_opt(WLX_Context *ctx, const char *text, float row_w, bool wrap,
    bool strict_ws, float tab_advance, size_t unit_cap, WLX_Text_Line_Record *recs, size_t cap)
{
    WLX_Text_Style ts = ww_style();
    test_frame_begin(ctx, 0, 0, false, false);
    WLX_Text_Build_Inputs in = {
        .ctx = ctx,
        .text = text,
        .length = strlen(text),
        .style = ts,
        .rect = wlx_rect(0, 0, row_w, 100),
        .wrap = wrap,
        .wrap_strict_ws = strict_ws,
        .line_h = wlx_text_line_height(ctx, ts, NULL),
        .text_unit_cap = unit_cap > 0 ? unit_cap : (size_t)WLX_TEXT_RUN_MAX_UNITS,
        .tab_advance = tab_advance,
    };
    WLX_Text_Build_Cursor cur = { .text_unit_count = 0 };
    size_t n = wlx_text_build_lines(&in, &cur, recs, cap);
    test_frame_end(ctx);
    return n;
}

// Display-build flavour (whitespace hangs), the default for the pins.
static size_t ww_build(WLX_Context *ctx, const char *text, float row_w, bool wrap,
    float tab_advance, size_t unit_cap, WLX_Text_Line_Record *recs, size_t cap)
{
    return ww_build_opt(ctx, text, row_w, wrap, false, tab_advance, unit_cap, recs, cap);
}

// Prepare aligned records the way the widgets do, inside one frame.
static size_t ww_prepare_opt(WLX_Context *ctx, const char *text, float row_w, bool wrap,
    bool strict_ws, WLX_Align align, WLX_Text_Line_Record *recs, size_t cap)
{
    WLX_Text_Line_Array_Result res;
    test_frame_begin(ctx, 0, 0, false, false);
    bool ok = wlx_text_prepare_lines_slice(ctx, wlx_rect(0, 0, row_w, 100), text, strlen(text),
        ww_style(), (WLX_Text_Prepare_Opt){ .align = align, .wrap = wrap,
            .wrap_strict_ws = strict_ws }, recs, cap, &res);
    test_frame_end(ctx);
    return ok ? res.line_count : 0; // 0 fails the caller's count pin
}

static size_t ww_prepare(WLX_Context *ctx, const char *text, float row_w, bool wrap,
    WLX_Align align, WLX_Text_Line_Record *recs, size_t cap)
{
    return ww_prepare_opt(ctx, text, row_w, wrap, false, align, recs, cap);
}

static void ww_assert_row(const WLX_Text_Line_Record *r, size_t visible_start,
    size_t visible_end, float measured_w, float advance_w)
{
    ASSERT_EQ_INT((long)r->visible_start, (long)visible_start);
    ASSERT_EQ_INT((long)r->visible_end, (long)visible_end);
    ASSERT_EQ_F(r->measured_w, measured_w, 0.01f);
    ASSERT_EQ_F(r->advance_w, advance_w, 0.01f);
    ASSERT_FALSE(r->empty_visual);
}

// ============================================================================
// The cut and its fallback
// ============================================================================

TEST(ww_cut_at_last_space) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // "aaaa bbbb " fills the row exactly; the first c overflows and the
    // row cuts after the second space, not inside "cccc".
    size_t n = ww_build(&ctx, "aaaa bbbb cccc", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 10, 50.0f, 45.0f);
    ww_assert_row(&recs[1], 10, 14, 20.0f, 20.0f);
    ASSERT_FALSE(recs[0].ended_by_newline);
    ASSERT_EQ_INT((long)recs[0].source_end, (long)recs[1].source_start);
    wlx_context_destroy(&ctx);
}

TEST(ww_long_word_breaks_mid_word) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // No opportunity in the row: the last unit that fits ends it, as before.
    size_t n = ww_build(&ctx, "aaaaaaaaaaaaa", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 10, 50.0f, 50.0f);
    ww_assert_row(&recs[1], 10, 13, 15.0f, 15.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_memo_tracks_latest_space) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // "aa bb cc d" fits (ten bytes); the second d overflows and the cut
    // lands after the third space, the latest opportunity, not the first.
    size_t n = ww_build(&ctx, "aa bb cc ddd", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 9, 45.0f, 40.0f);
    ww_assert_row(&recs[1], 9, 12, 15.0f, 15.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_leading_whitespace_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // The leading space is the row's only opportunity, so the over-long
    // word after it moves whole to the next row and breaks there.
    size_t n = ww_build(&ctx, " aaaaaaaaaaaa", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 3);
    ww_assert_row(&recs[0], 0, 1, 5.0f, 0.0f);
    ww_assert_row(&recs[1], 1, 11, 50.0f, 50.0f);
    ww_assert_row(&recs[2], 11, 13, 10.0f, 10.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_multibyte_word_moves_whole) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Two-byte units (the mock measures per byte, 10 px each): the third
    // e-acute overflows and the whole word moves; the cut is a codepoint
    // boundary because it sits after an ASCII space.
    size_t n = ww_build(&ctx, "aaaa \xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
    ww_assert_row(&recs[1], 5, 13, 40.0f, 40.0f);
    // Three-byte units (15 px each) likewise.
    n = ww_build(&ctx, "aaaa \xE4\xB8\xAD\xE4\xB8\xAD", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
    ww_assert_row(&recs[1], 5, 11, 30.0f, 30.0f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hanging whitespace
// ============================================================================

TEST(ww_hanging_space) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // "aaaa bbbbb" fills the row; the space after it hangs past the width
    // and the row ends with it, so the next row opens on a glyph and the
    // word that fit is not moved.
    size_t n = ww_build(&ctx, "aaaa bbbbb cc", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 11, 55.0f, 50.0f);
    ww_assert_row(&recs[1], 11, 13, 10.0f, 10.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_hanging_run) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // The whole whitespace run hangs; the ink extent stops at the last b.
    size_t n = ww_build(&ctx, "aaaa bbbbb   cc", 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 13, 65.0f, 50.0f);
    ww_assert_row(&recs[1], 13, 15, 10.0f, 10.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_hanging_tab_under_expansion) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Tab stops every 20 px: the tab after "aaaa bbbbb" (50 px) advances
    // to 60, past the width, and hangs like a space.
    size_t n = ww_build(&ctx, "aaaa bbbbb\tcc", 50, true, 20.0f, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 11, 60.0f, 50.0f);
    ww_assert_row(&recs[1], 11, 13, 10.0f, 10.0f);
    // A tab that fits is an opportunity like a space.
    n = ww_build(&ctx, "aa\tbb\tcccccc", 50, true, 20.0f, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    // "aa" 10, tab -> 20, "bb" 30, tab -> 40, "cc" 50, third c overflows:
    // cut after the second tab.
    ww_assert_row(&recs[0], 0, 6, 40.0f, 30.0f);
    ww_assert_row(&recs[1], 6, 12, 30.0f, 30.0f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// What the rule leaves alone
// ============================================================================

TEST(ww_separators_unaffected) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    size_t n = ww_build(&ctx, "aa bb\r\ncc dd\ncc", 200, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 3);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 25.0f);
    ASSERT_TRUE(recs[0].ended_by_newline);
    ASSERT_EQ_INT((long)recs[0].separator_start, 5);
    ASSERT_EQ_INT((long)recs[0].separator_end, 7);
    ww_assert_row(&recs[1], 7, 12, 25.0f, 25.0f);
    ASSERT_TRUE(recs[1].ended_by_newline);
    ww_assert_row(&recs[2], 13, 15, 10.0f, 10.0f);
    ASSERT_FALSE(recs[2].ended_by_newline);
    // A trailing space before a newline is content: the row keeps it in
    // advance_w, and the trailing empty record still follows.
    n = ww_build(&ctx, "aa \n", 200, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 3, 15.0f, 15.0f);
    ASSERT_TRUE(recs[0].ended_by_newline);
    ASSERT_TRUE(recs[1].empty_visual);
    ASSERT_EQ_INT((long)recs[1].source_start, 4);
    wlx_context_destroy(&ctx);
}

TEST(ww_budget_ended_row_keeps_measured) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // A five-unit budget ends the row right after the space with no
    // overflow: no cut, the space counts in advance_w, and the build ends.
    size_t n = ww_build(&ctx, "aaaa bbbb cccc", 50, true, 0, 5, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 1);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 25.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_no_wrap_untouched) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Classic no-wrap truncates at the last unit that fits, inside the
    // word, and stops the build.
    size_t n = ww_build(&ctx, "aaaa bbbb cccc", 50, false, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 1);
    ww_assert_row(&recs[0], 0, 10, 50.0f, 50.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_records_tile_and_progress) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Forty "wNN " tokens: two per 50 px row, every cut after a space.
    char buf[256];
    size_t off = 0;
    for (int i = 0; i < 40; i++) off += (size_t)snprintf(buf + off, sizeof(buf) - off, "w%02d ", i);
    size_t n = ww_build(&ctx, buf, 50, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 20);
    for (size_t i = 0; i < n; i++) {
        ASSERT_FALSE(recs[i].empty_visual);
        ASSERT_TRUE(recs[i].visible_end > recs[i].source_start);
        ASSERT_EQ_INT((long)(recs[i].visible_end - recs[i].visible_start), 8);
        ASSERT_TRUE(buf[recs[i].visible_end - 1] == ' ');
        if (i + 1 < n) {
            ASSERT_EQ_INT((long)recs[i].source_end, (long)recs[i + 1].source_start);
            ASSERT_EQ_F(recs[i].measured_w, 40.0f, 0.01f);
            ASSERT_EQ_F(recs[i].advance_w, 35.0f, 0.01f);
        } else {
            // The last row ends at the text end: not wrap-ended, so its
            // trailing space stays in advance_w.
            ASSERT_EQ_F(recs[i].advance_w, 40.0f, 0.01f);
        }
    }
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Consumers of the ink extent
// ============================================================================

TEST(ww_alignment_uses_ink_extent) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    size_t n = ww_prepare(&ctx, "aaaa bbbb cccc", 50, true, WLX_RIGHT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    // Right: the glyphs of "aaaa bbbb" (45 px) end at the edge; the cut's
    // trailing space does not push them left.
    ASSERT_EQ_F(recs[0].origin_x, 5.0f, 0.01f);
    ASSERT_EQ_F(recs[1].origin_x, 30.0f, 0.01f);
    n = ww_prepare(&ctx, "aaaa bbbb cccc", 50, true, WLX_CENTER, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ASSERT_EQ_F(recs[0].origin_x, 2.5f, 0.01f);
    ASSERT_EQ_F(recs[1].origin_x, 15.0f, 0.01f);
    wlx_context_destroy(&ctx);
}

TEST(ww_scissor_reads_ink_extent) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Only whitespace hangs past the rect: no scissor.
    size_t n = ww_prepare(&ctx, "aaaa bbbbb cc", 50, true, WLX_LEFT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ASSERT_TRUE(recs[0].measured_w > 50.0f);
    ASSERT_FALSE(wlx_text_lines_need_scissor(wlx_rect(0, 0, 50, 100), recs, n));
    // A glyph wider than the row overflows in ink: scissor, in wrap mode
    // (the first unit ends the row alone) and in no-wrap mode alike. The
    // lone-space row a 3 px width produces carries an ink extent of 0.
    n = ww_prepare(&ctx, "ab cd", 3, true, WLX_LEFT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 5);
    ASSERT_EQ_F(recs[0].advance_w, 5.0f, 0.01f);
    ASSERT_EQ_F(recs[2].measured_w, 5.0f, 0.01f);
    ASSERT_EQ_F(recs[2].advance_w, 0.0f, 0.01f);
    ASSERT_TRUE(wlx_text_lines_need_scissor(wlx_rect(0, 0, 3, 100), recs, n));
    n = ww_prepare(&ctx, "ab", 3, false, WLX_LEFT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 1);
    ASSERT_TRUE(wlx_text_lines_need_scissor(wlx_rect(0, 0, 3, 100), recs, n));
    wlx_context_destroy(&ctx);
}

TEST(ww_textarea_draws_word_rows) {
    WLX_Context ctx;
    // Widget 67 px wide with padding 4 and no border: a 50 px text band
    // (the multiline inputbox reserves 17 px of chrome at this size).
    test_ctx_init(&ctx, 67, 100);
    test_stream_install(&ctx);
    test_stream_reset();
    char buf[64] = "aaaa bbbb cccc";
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .multiline = true, .wrap = true),
        __FILE__, __LINE__);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    int texts = 0;
    float lens[4] = {0};
    for (int i = 0; i < _test_stream.count; i++) {
        if (_test_stream.cmds[i].kind != 5) continue;
        if (texts < 4) lens[texts] = _test_stream.cmds[i].rect.w;
        texts++;
    }
    ASSERT_EQ_INT(texts, 2);
    ASSERT_EQ_F(lens[0], 10.0f, 0.01f);
    ASSERT_EQ_F(lens[1], 4.0f, 0.01f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Editable builds: overflowing whitespace never hangs
// ============================================================================

TEST(ww_strict_space_cuts_at_memo) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // "aaaa bbbbb" fills the row; the space after it does not fit, so the
    // row cuts at the memo and the word moves down with its space. The
    // first row keeps its own trailing space out of advance_w.
    size_t n = ww_build_opt(&ctx, "aaaa bbbbb cc", 50, true, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
    ww_assert_row(&recs[1], 5, 13, 40.0f, 40.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_strict_run_cuts_at_memo) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // The first overflowing space of the run cuts; the run follows the
    // word onto the next row, which then fits exactly.
    size_t n = ww_build_opt(&ctx, "aaaa bbbbb   cc", 50, true, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
    ww_assert_row(&recs[1], 5, 15, 50.0f, 50.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_strict_tab_under_expansion) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Tab stops every 20 px: the tab after "aaaa bbbbb" (50 px) would
    // advance to 60 and cuts the row instead; on the next row it advances
    // from 25 to 40 and "cc" ends at 50.
    size_t n = ww_build_opt(&ctx, "aaaa bbbbb\tcc", 50, true, true, 20.0f, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
    ww_assert_row(&recs[1], 5, 13, 50.0f, 50.0f);
    // A tab that fits is an opportunity in both modes.
    n = ww_build_opt(&ctx, "aa\tbb\tcccccc", 50, true, true, 20.0f, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 6, 40.0f, 30.0f);
    ww_assert_row(&recs[1], 6, 12, 30.0f, 30.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_strict_space_without_memo_opens_next_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // A word fills the row and no earlier opportunity exists: the space is
    // rejected and opens the next row at column zero.
    size_t n = ww_build_opt(&ctx, "aaaaaaaaaa bb", 50, true, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 10, 50.0f, 50.0f);
    ww_assert_row(&recs[1], 10, 13, 15.0f, 15.0f);
    ASSERT_TRUE("aaaaaaaaaa bb"[recs[1].visible_start] == ' ');
    wlx_context_destroy(&ctx);
}

TEST(ww_strict_spaces_only_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Twelve spaces: the eleventh overflows and cuts at the memo, which is
    // the tenth; the first row has no ink at all.
    size_t n = ww_build_opt(&ctx, "            ", 50, true, true, 0, 0, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 10, 50.0f, 0.0f);
    ww_assert_row(&recs[1], 10, 12, 10.0f, 10.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_strict_rows_fit_width) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Every strict row measures within the width: no scissor, and right
    // alignment sits each row's glyphs at the edge.
    size_t n = ww_prepare_opt(&ctx, "aaaa bbbbb cc", 50, true, true, WLX_RIGHT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    for (size_t i = 0; i < n; i++) ASSERT_TRUE(recs[i].measured_w <= 50.0f);
    ASSERT_FALSE(wlx_text_lines_need_scissor(wlx_rect(0, 0, 50, 100), recs, n));
    ASSERT_EQ_F(recs[0].origin_x, 30.0f, 0.01f);
    ASSERT_EQ_F(recs[1].origin_x, 10.0f, 0.01f);
    // The display build of the same text still hangs its space.
    n = ww_prepare(&ctx, "aaaa bbbbb cc", 50, true, WLX_LEFT, recs, WW_MAX_RECS);
    ASSERT_EQ_INT((int)n, 2);
    ww_assert_row(&recs[0], 0, 11, 55.0f, 50.0f);
    wlx_context_destroy(&ctx);
}

TEST(ww_cluster_rows_cut_at_units) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    WLX_Text_Line_Record recs[WW_MAX_RECS];
    // Two ZWJ families (18 bytes, 90 px each; corpus macro from
    // test_grapheme.c, same TU) after "aaaa ": the first overflows and cuts
    // the row at the space, then each family stands on a row of its own (a
    // first unit wider than the row) and never breaks inside. The display
    // and strict rules agree because no whitespace overflows.
    const char *text = "aaaa " GR_FAMILY GR_FAMILY " b";
    for (int strict = 0; strict < 2; strict++) {
        size_t n = ww_build_opt(&ctx, text, 50, true, strict == 1, 0, 0, recs, WW_MAX_RECS);
        ASSERT_EQ_INT((int)n, 4);
        ww_assert_row(&recs[0], 0, 5, 25.0f, 20.0f);
        ww_assert_row(&recs[1], 5, 23, 90.0f, 90.0f);
        ww_assert_row(&recs[2], 23, 41, 90.0f, 90.0f);
        ww_assert_row(&recs[3], 41, 43, 10.0f, 10.0f);
    }
    wlx_context_destroy(&ctx);
}

SUITE(word_wrap) {
    RUN_TEST(ww_cluster_rows_cut_at_units);
    RUN_TEST(ww_cut_at_last_space);
    RUN_TEST(ww_long_word_breaks_mid_word);
    RUN_TEST(ww_memo_tracks_latest_space);
    RUN_TEST(ww_leading_whitespace_row);
    RUN_TEST(ww_multibyte_word_moves_whole);
    RUN_TEST(ww_hanging_space);
    RUN_TEST(ww_hanging_run);
    RUN_TEST(ww_hanging_tab_under_expansion);
    RUN_TEST(ww_separators_unaffected);
    RUN_TEST(ww_budget_ended_row_keeps_measured);
    RUN_TEST(ww_no_wrap_untouched);
    RUN_TEST(ww_records_tile_and_progress);
    RUN_TEST(ww_alignment_uses_ink_extent);
    RUN_TEST(ww_scissor_reads_ink_extent);
    RUN_TEST(ww_textarea_draws_word_rows);
    RUN_TEST(ww_strict_space_cuts_at_memo);
    RUN_TEST(ww_strict_run_cuts_at_memo);
    RUN_TEST(ww_strict_tab_under_expansion);
    RUN_TEST(ww_strict_space_without_memo_opens_next_row);
    RUN_TEST(ww_strict_spaces_only_rows);
    RUN_TEST(ww_strict_rows_fit_width);
}

// test_grapheme.c - the text unit as an approximated grapheme cluster: the
// boundary predicate, the two unit steppers as exact inverses, the four
// join rules with their exclusions, and the consumers that read the unit
// (caret normalization, hit-test, wrap cut, insert truncation).
//
// Geometry model (mock backend): char width = font_size/2 = 5 px per byte
// at font_size 10, so a 3-byte cluster is one 15 px unit and an 18-byte
// ZWJ family one 90 px unit.

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

// Corpus (byte lengths in brackets).
#define GR_MARK     "\xCC\x81"                           /* [2] U+0301 */
#define GR_EACUTE   "e" GR_MARK                          /* [3] e + U+0301 */
#define GR_CYR      "\xD0\xB0\xD2\x83"                   /* [4] a + U+0483 */
#define GR_MAN      "\xF0\x9F\x91\xA8"                   /* [4] U+1F468 */
#define GR_WOMAN    "\xF0\x9F\x91\xA9"                   /* [4] U+1F469 */
#define GR_GIRL     "\xF0\x9F\x91\xA7"                   /* [4] U+1F467 */
#define GR_ZWJ      "\xE2\x80\x8D"                       /* [3] U+200D */
#define GR_FAMILY   GR_MAN GR_ZWJ GR_WOMAN GR_ZWJ GR_GIRL /* [18] */
#define GR_HEART    "\xE2\x9D\xA4\xEF\xB8\x8F"           /* [6] U+2764 U+FE0F */
#define GR_THUMBS   "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"   /* [8] U+1F44D U+1F3FD */
#define GR_RI_L     "\xF0\x9F\x87\xB1"                   /* [4] U+1F1F1 */
#define GR_RI_V     "\xF0\x9F\x87\xBB"                   /* [4] U+1F1FB */
#define GR_FLAG     GR_RI_L GR_RI_V                      /* [8] */
#define GR_SCOTLAND "\xF0\x9F\x8F\xB4" \
    "\xF3\xA0\x81\xA7\xF3\xA0\x81\xA2\xF3\xA0\x81\xB3\xF3\xA0\x81\xA3\xF3\xA0\x81\xB4" \
    "\xF3\xA0\x81\xBF"                                   /* [28] flag, 5 tags, terminator */

#define GR_MAX_BOUNDS 64

// Pin the boundary set of s: the predicate at every offset, the forward
// walk from 0 and the backward walk from the end all agree with expect
// (which lists 0 and len).
static void gr_assert_boundaries(const char *s, size_t len, const size_t *expect, size_t n) {
    size_t k = 0;
    for (size_t pos = 0; pos <= len; pos++) {
        bool b = wlx_text_unit_boundary(s, len, pos);
        bool e = k < n && expect[k] == pos;
        ASSERT_EQ_INT((int)b, (int)e);
        if (e) k++;
    }
    ASSERT_EQ_INT((long)k, (long)n);

    size_t pos = 0;
    k = 1;
    while (pos < len) {
        pos = wlx_text_unit_next(s, len, pos);
        ASSERT_TRUE(k < n);
        ASSERT_EQ_INT((long)pos, (long)expect[k]);
        k++;
    }
    ASSERT_EQ_INT((long)k, (long)n);

    pos = len;
    k = n - 1;
    while (pos > 0) {
        pos = wlx_text_unit_prev(s, len, pos);
        ASSERT_TRUE(k > 0);
        k--;
        ASSERT_EQ_INT((long)pos, (long)expect[k]);
    }
    ASSERT_EQ_INT((long)k, 0);
}

#define GR_BOUNDS(s, ...) do {                                                  \
    const size_t _b[] = { __VA_ARGS__ };                                       \
    gr_assert_boundaries((s), sizeof(s) - 1, _b, wlx_array_len(_b));            \
} while (0)

// ============================================================================
// The four rules
// ============================================================================

TEST(gr_extend_marks_join_their_base) {
    GR_BOUNDS("ab", 0, 1, 2);
    GR_BOUNDS(GR_EACUTE, 0, 3);
    GR_BOUNDS(GR_CYR, 0, 4);
    GR_BOUNDS("e" GR_MARK GR_MARK GR_MARK, 0, 7);
    GR_BOUNDS("x" GR_EACUTE "y", 0, 1, 4, 5);
    // A precomposed e-acute is one codepoint and one unit; the mark on a
    // 3-byte base joins it too.
    GR_BOUNDS("\xC3\xA9" GR_MARK, 0, 4);
    GR_BOUNDS("\xE2\x82\xAC" GR_MARK, 0, 5);
}

TEST(gr_zwj_sequences) {
    GR_BOUNDS(GR_FAMILY, 0, 18);
    // ZWJ between two letters: no break before the ZWJ, a break after it.
    GR_BOUNDS("a" GR_ZWJ "b", 0, 4, 5);
    // The recorded over-join: a ZWJ after a letter still joins a following
    // pictographic (UAX #29 would break after the ZWJ here).
    GR_BOUNDS("a" GR_ZWJ GR_FAMILY, 0, 22);
    // A lone ZWJ, and a ZWJ at text start followed by a letter.
    GR_BOUNDS(GR_ZWJ, 0, 3);
    GR_BOUNDS(GR_ZWJ "b", 0, 3, 4);
}

TEST(gr_variation_selectors_and_modifiers) {
    GR_BOUNDS(GR_HEART, 0, 6);
    GR_BOUNDS(GR_THUMBS, 0, 8);
    GR_BOUNDS(GR_SCOTLAND, 0, 28);
    GR_BOUNDS("a" GR_HEART "b", 0, 1, 7, 8);
}

TEST(gr_regional_indicators_pair_by_parity) {
    GR_BOUNDS(GR_RI_L, 0, 4);
    GR_BOUNDS(GR_FLAG, 0, 8);
    GR_BOUNDS(GR_RI_L GR_RI_V GR_RI_L, 0, 8, 12);
    GR_BOUNDS(GR_RI_L GR_RI_V GR_RI_L GR_RI_V, 0, 8, 16);
    GR_BOUNDS("a" GR_FLAG "b", 0, 1, 9, 10);
    // A modifier-class codepoint after a flag joins it (Extend), and the
    // pairing restarts after a non-indicator.
    GR_BOUNDS(GR_FLAG "\xEF\xB8\x8F", 0, 11);
    GR_BOUNDS(GR_RI_L "x" GR_RI_L GR_RI_V, 0, 4, 5, 13);
}

TEST(gr_controls_and_malformed_never_join) {
    GR_BOUNDS("\n" GR_MARK, 0, 1, 3);
    GR_BOUNDS("\r\n" GR_MARK, 0, 1, 2, 4);   // CR LF is the separator grammar's
    GR_BOUNDS("\t" GR_MARK, 0, 1, 3);
    GR_BOUNDS("\x7F" GR_MARK, 0, 1, 3);
    GR_BOUNDS("\xC2\x85" GR_MARK, 0, 2, 4);  // C1 control U+0085
    GR_BOUNDS(" " GR_MARK, 0, 3);            // a space carries its mark
    GR_BOUNDS("\xFF" GR_MARK, 0, 1, 3);
    GR_BOUNDS(GR_MARK "\xFF" GR_MARK, 0, 2, 3, 5);
    GR_BOUNDS(GR_MARK, 0, 2);
    GR_BOUNDS("\x80" GR_MARK, 0, 1, 3);      // stray continuation byte
    // A truncated lead before a mark is one malformed byte, then the mark.
    GR_BOUNDS("\xE2\x82" GR_MARK, 0, 1, 2, 4);
}

TEST(gr_stacked_marks_are_one_unit) {
    enum { N = 1000 };
    static char s[1 + 2 * N];
    s[0] = 'e';
    for (int i = 0; i < N; i++) { s[1 + 2 * i] = '\xCC'; s[2 + 2 * i] = '\x81'; }
    size_t len = sizeof(s);
    ASSERT_EQ_INT((long)wlx_text_unit_next(s, len, 0), (long)len);
    ASSERT_EQ_INT(0, (long)wlx_text_unit_prev(s, len, len));
    ASSERT_FALSE(wlx_text_unit_boundary(s, len, 1));
    ASSERT_FALSE(wlx_text_unit_boundary(s, len, 1001));
    ASSERT_EQ_INT(0, (long)wlx_text_normalize_cursor_offset(s, len, 1001));
}

// ============================================================================
// Inverse property over a mixed corpus
// ============================================================================

TEST(gr_steppers_are_exact_inverses_over_corpus) {
    const char s[] =
        "word " GR_EACUTE " \xC3\xA9 " GR_CYR "\r\n"
        GR_FAMILY " a" GR_ZWJ "b " GR_HEART GR_THUMBS "\n"
        GR_FLAG GR_RI_L "x" GR_RI_L GR_RI_V GR_SCOTLAND "\t"
        "\xE2\x82\xAC" GR_MARK " \xFF" GR_MARK "\x80" "end";
    size_t len = sizeof(s) - 1;

    // The boundary set by the predicate.
    static bool bset[512];
    ASSERT_TRUE(len < wlx_array_len(bset));
    size_t count = 0;
    for (size_t pos = 0; pos <= len; pos++) {
        bset[pos] = wlx_text_unit_boundary(s, len, pos);
        if (bset[pos]) count++;
    }
    ASSERT_TRUE(bset[0] && bset[len]);

    // The forward walk visits exactly the set, in order.
    size_t visited = 1;
    for (size_t pos = 0; pos < len;) {
        size_t next = wlx_text_unit_next(s, len, pos);
        ASSERT_TRUE(next > pos && next <= len);
        ASSERT_TRUE(bset[next]);
        for (size_t q = pos + 1; q < next; q++) ASSERT_FALSE(bset[q]);
        pos = next;
        visited++;
    }
    ASSERT_EQ_INT((long)visited, (long)count);

    // The backward walk visits the same set, in reverse.
    visited = 1;
    for (size_t pos = len; pos > 0;) {
        size_t prev = wlx_text_unit_prev(s, len, pos);
        ASSERT_TRUE(prev < pos);
        ASSERT_TRUE(bset[prev]);
        for (size_t q = prev + 1; q < pos; q++) ASSERT_FALSE(bset[q]);
        pos = prev;
        visited++;
    }
    ASSERT_EQ_INT((long)visited, (long)count);

    // Every offset resolves to its neighbouring boundaries: a boundary is
    // its own round trip, a non-boundary (inside a cluster or inside a
    // UTF-8 sequence) steps to the boundaries on either side.
    for (size_t q = 1; q < len; q++) {
        size_t lo = q, hi = q;
        if (!bset[q]) {
            while (!bset[lo]) lo--;
            while (!bset[hi]) hi++;
            ASSERT_EQ_INT((long)wlx_text_unit_prev(s, len, q), (long)lo);
            ASSERT_EQ_INT((long)wlx_text_unit_next(s, len, q), (long)hi);
            ASSERT_EQ_INT((long)wlx_text_normalize_cursor_offset(s, len, q), (long)lo);
        } else {
            ASSERT_EQ_INT((long)wlx_text_unit_next(s, len, wlx_text_unit_prev(s, len, q)), (long)q);
            ASSERT_EQ_INT((long)wlx_text_unit_prev(s, len, wlx_text_unit_next(s, len, q)), (long)q);
            ASSERT_EQ_INT((long)wlx_text_normalize_cursor_offset(s, len, q), (long)q);
        }
    }
    ASSERT_EQ_INT((long)wlx_text_normalize_cursor_offset(s, len, len + 40), (long)len);
}

TEST(gr_malformed_input_steps_like_codepoints) {
    const char *cases[] = { "\xC3\x41", "\x80\x41", "\xFF\xFE", "\xE2\x82", "A\x80\x80",
        "\xF0\x9F\x91", "\xC3\xA9\xA9" };
    for (size_t c = 0; c < wlx_array_len(cases); c++) {
        const char *s = cases[c];
        size_t len = strlen(s);
        for (size_t pos = 0; pos <= len; pos++) {
            ASSERT_EQ_INT((long)wlx_text_unit_next(s, len, pos),
                (long)wlx_text_codepoint_next(s, len, pos));
            ASSERT_EQ_INT((long)wlx_text_unit_prev(s, len, pos),
                (long)wlx_text_codepoint_prev(s, len, pos));
        }
        // The codepoint steps are inverses over the same input.
        for (size_t pos = 0; pos < len; pos = wlx_text_codepoint_next(s, len, pos)) {
            size_t next = wlx_text_codepoint_next(s, len, pos);
            ASSERT_EQ_INT((long)wlx_text_codepoint_prev(s, len, next), (long)pos);
        }
    }
}

TEST(gr_codepoint_helpers_unchanged) {
    // wlx_text_codepoint_next / wlx_utf8_prev stay codepoint-level.
    ASSERT_EQ_INT(1, (long)wlx_text_codepoint_next(GR_EACUTE, 3, 0));
    ASSERT_EQ_INT(3, (long)wlx_text_codepoint_next(GR_EACUTE, 3, 1));
    ASSERT_EQ_INT(1, (long)wlx_utf8_prev(GR_EACUTE, 3));
    ASSERT_EQ_INT(4, (long)wlx_text_codepoint_next(GR_FAMILY, 18, 0));
    ASSERT_EQ_INT(14, (long)wlx_utf8_prev(GR_FAMILY, 18));
    ASSERT_EQ_INT(5, (long)wlx_utf8_slicelen(GR_FAMILY, 18));
}

// ============================================================================
// Consumers: hit-test, wrap cut, insert truncation
// ============================================================================

TEST(gr_hit_test_returns_unit_boundaries) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Rect rect = {0, 0, 200, 100};
    WLX_Text_Style ts = { .font_size = 10 };
    const char *text = "ab" GR_EACUTE "cd";
    size_t len = 7;

    test_frame_begin(&ctx, 0, 0, false, false);
    // "ab" spans 0..10, the cluster 10..25 (midpoint 17.5), "c" 25..30.
    ASSERT_EQ_INT(2, (int)wlx_text_offset_at_point(&ctx, rect, text, len, ts, WLX_TOP_LEFT, false, 16.0f, 5.0f));
    ASSERT_EQ_INT(5, (int)wlx_text_offset_at_point(&ctx, rect, text, len, ts, WLX_TOP_LEFT, false, 18.0f, 5.0f));
    for (int px = -5; px <= 40; px++) {
        size_t off = wlx_text_offset_at_point(&ctx, rect, text, len, ts, WLX_TOP_LEFT, false, (float)px, 5.0f);
        ASSERT_TRUE(off != 3 && off != 4);
        ASSERT_TRUE(wlx_text_unit_boundary(text, len, off));
    }
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// Build records straight through the kernel (as test_word_wrap.c does).
static size_t gr_build(WLX_Context *ctx, const char *text, size_t len, float row_w,
    bool strict_ws, WLX_Text_Line_Record *recs, size_t cap)
{
    WLX_Text_Style ts = { .font_size = 10, .color = {255, 255, 255, 255} };
    test_frame_begin(ctx, 0, 0, false, false);
    WLX_Text_Build_Inputs in = {
        .ctx = ctx,
        .text = text,
        .length = len,
        .style = ts,
        .rect = wlx_rect(0, 0, row_w, 100),
        .wrap = true,
        .wrap_strict_ws = strict_ws,
        .line_h = wlx_text_line_height(ctx, ts, NULL),
        .text_unit_cap = (size_t)WLX_TEXT_RUN_MAX_UNITS,
        .tab_advance = 0,
    };
    WLX_Text_Build_Cursor cur = { .text_unit_count = 0 };
    size_t n = wlx_text_build_lines(&in, &cur, recs, cap);
    test_frame_end(ctx);
    return n;
}

TEST(gr_wrap_cuts_at_unit_boundaries) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 200);
    // Twenty families, no whitespace: the over-wide fallback cuts at units.
    // A 200 px row holds 40 bytes, so two families (36 bytes) per row.
    static char text[20 * 18];
    for (int i = 0; i < 20; i++) memcpy(text + i * 18, GR_FAMILY, 18);
    WLX_Text_Line_Record recs[32];
    for (int strict = 0; strict < 2; strict++) {
        size_t n = gr_build(&ctx, text, sizeof(text), 200, strict == 1, recs, 32);
        ASSERT_EQ_INT((int)n, 10);
        for (size_t r = 0; r < n; r++) {
            ASSERT_EQ_INT((long)recs[r].visible_start, (long)(r * 36));
            ASSERT_EQ_INT((long)recs[r].visible_end, (long)(r * 36 + 36));
            ASSERT_EQ_F(recs[r].measured_w, 180.0f, 0.01f);
        }
    }
    wlx_context_destroy(&ctx);
}

TEST(gr_insert_truncates_on_unit_boundary) {
    char buf[8];
    size_t len, cursor, anchor;
    WLX_Text_Edit_Span span;
    const char *src = "ab" GR_EACUTE;   // 5 bytes

    // Room for four bytes cuts inside the cluster: the base goes with it.
    memset(&span, 0, sizeof(span));
    len = 0; cursor = 0; anchor = 0;
    ASSERT_EQ_INT(2, (long)wlx_text_edit_insert(buf, 4, &len, &cursor, &anchor, src, 5, &span, NULL));
    ASSERT_EQ_INT(2, (long)len);
    ASSERT_EQ_INT(2, (long)cursor);
    ASSERT_TRUE(memcmp(buf, "ab", 2) == 0);

    // Room for all five bytes lands the whole slice.
    memset(&span, 0, sizeof(span));
    len = 0; cursor = 0; anchor = 0;
    ASSERT_EQ_INT(5, (long)wlx_text_edit_insert(buf, 5, &len, &cursor, &anchor, src, 5, &span, NULL));
    ASSERT_EQ_INT(5, (long)cursor);

    // A mark typed on its own is a unit of its own in the slice and joins
    // its base in the buffer.
    memset(&span, 0, sizeof(span));
    memcpy(buf, "e", 1);
    len = 1; cursor = 1; anchor = 1;
    ASSERT_EQ_INT(2, (long)wlx_text_edit_insert(buf, 3, &len, &cursor, &anchor, GR_MARK, 2, &span, NULL));
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_EQ_INT(3, (long)cursor);
    ASSERT_TRUE(memcmp(buf, GR_EACUTE, 3) == 0);
    ASSERT_EQ_INT(0, (long)wlx_text_unit_prev(buf, len, 3));

    // No room for the mark: nothing lands, nothing moves.
    memset(&span, 0, sizeof(span));
    len = 1; cursor = 1; anchor = 1;
    ASSERT_EQ_INT(0, (long)wlx_text_edit_insert(buf, 2, &len, &cursor, &anchor, GR_MARK, 2, &span, NULL));
    ASSERT_EQ_INT(1, (long)len);
}

TEST(gr_normalize_floors_to_cluster_start) {
    const char *text = "ab" GR_FAMILY "c";
    size_t len = 21;
    ASSERT_EQ_INT(2, (long)wlx_text_normalize_cursor_offset(text, len, 10));
    ASSERT_EQ_INT(2, (long)wlx_text_normalize_cursor_offset(text, len, 6));
    ASSERT_EQ_INT(2, (long)wlx_text_normalize_cursor_offset(text, len, 2));
    ASSERT_EQ_INT(1, (long)wlx_text_normalize_cursor_offset(text, len, 1));
    ASSERT_EQ_INT(20, (long)wlx_text_normalize_cursor_offset(text, len, 20));
    ASSERT_EQ_INT(21, (long)wlx_text_normalize_cursor_offset(text, len, 21));
    ASSERT_EQ_INT(21, (long)wlx_text_normalize_cursor_offset(text, len, 30));
    ASSERT_EQ_INT(0, (long)wlx_text_normalize_cursor_offset(NULL, 0, 3));
}

SUITE(grapheme) {
    RUN_TEST(gr_extend_marks_join_their_base);
    RUN_TEST(gr_zwj_sequences);
    RUN_TEST(gr_variation_selectors_and_modifiers);
    RUN_TEST(gr_regional_indicators_pair_by_parity);
    RUN_TEST(gr_controls_and_malformed_never_join);
    RUN_TEST(gr_stacked_marks_are_one_unit);
    RUN_TEST(gr_steppers_are_exact_inverses_over_corpus);
    RUN_TEST(gr_malformed_input_steps_like_codepoints);
    RUN_TEST(gr_codepoint_helpers_unchanged);
    RUN_TEST(gr_hit_test_returns_unit_boundaries);
    RUN_TEST(gr_wrap_cuts_at_unit_boundaries);
    RUN_TEST(gr_insert_truncates_on_unit_boundary);
    RUN_TEST(gr_normalize_floors_to_cluster_start);
}

// test_editor_pipeline.c - line-build pipeline entries for windowed text:
// wlx_text_build_lines_from window equivalence (offset-0 identity and
// tail-of-full-build at every hard line start), the non-wrap
// truncate-and-continue mode (per-record unit budget, invisible-tail skip,
// gap-free source tiling), and the wrap-mode per-hard-line budget (rows of
// one line share one budget, frozen-tail skip, fresh budget per line).

#define EP_MAX_LINES_ 64
#define EP_UNIT_CAP_ (1u << 20)

// Build the whole document with the offset-0 entry. rect_w bounds the
// visible width per record; the huge explicit unit cap keeps the global
// budget out of the way so only rect_w (or, in truncate mode, the
// per-record cap) truncates.
static size_t ep_build_full(WLX_Context *ctx, const char *text, size_t length, float rect_w,
    bool truncate_continue, WLX_Text_Line_Record *out, size_t cap) {
    WLX_Text_Build_Inputs inputs = {
        .ctx = ctx,
        .text = text,
        .length = length,
        .style = (WLX_Text_Style){ .font_size = 10 },
        .rect = (WLX_Rect){ 0, 0, rect_w, 10000.0f },
        .wrap = false,
        .line_h = 10.0f,
        .text_unit_cap = EP_UNIT_CAP_,
        .truncate_continue = truncate_continue,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    return wlx_text_build_lines(&inputs, &cursor, out, cap);
}

static size_t ep_build_from(WLX_Context *ctx, const char *text, size_t length, float rect_w,
    bool truncate_continue, size_t start_offset, WLX_Text_Line_Record *out, size_t cap) {
    WLX_Text_Build_Inputs inputs = {
        .ctx = ctx,
        .text = text,
        .length = length,
        .style = (WLX_Text_Style){ .font_size = 10 },
        .rect = (WLX_Rect){ 0, 0, rect_w, 10000.0f },
        .wrap = false,
        .line_h = 10.0f,
        .text_unit_cap = EP_UNIT_CAP_,
        .truncate_continue = truncate_continue,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    return wlx_text_build_lines_from(&inputs, &cursor, start_offset, out, cap);
}

// Wrapped build (per-line budget mode when truncate_continue is set) from
// an arbitrary hard line start, in either whitespace mode (strict_ws: the
// editable-build rule; off: display builds, whitespace hangs).
static size_t ep_build_wrap_from_mode(WLX_Context *ctx, const char *text, size_t length,
    float rect_w, bool truncate_continue, bool strict_ws, size_t start_offset,
    WLX_Text_Line_Record *out, size_t cap) {
    WLX_Text_Build_Inputs inputs = {
        .ctx = ctx,
        .text = text,
        .length = length,
        .style = (WLX_Text_Style){ .font_size = 10 },
        .rect = (WLX_Rect){ 0, 0, rect_w, 10000.0f },
        .wrap = true,
        .wrap_strict_ws = strict_ws,
        .line_h = 10.0f,
        .text_unit_cap = EP_UNIT_CAP_,
        .truncate_continue = truncate_continue,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    return wlx_text_build_lines_from(&inputs, &cursor, start_offset, out, cap);
}

static size_t ep_build_wrap_from(WLX_Context *ctx, const char *text, size_t length, float rect_w,
    bool truncate_continue, size_t start_offset, WLX_Text_Line_Record *out, size_t cap) {
    return ep_build_wrap_from_mode(ctx, text, length, rect_w, truncate_continue, false,
        start_offset, out, cap);
}

// Records are zero-initialized before population (wlx_zero_struct), so a
// byte-for-byte comparison covers every field including padding.
static bool ep_records_equal(const WLX_Text_Line_Record *a, const WLX_Text_Line_Record *b) {
    return memcmp(a, b, sizeof(*a)) == 0;
}

// Collect the hard line starts of text: offset 0 plus the end of every
// newline separator (CRLF counts as one separator).
static size_t ep_hard_line_starts(const char *text, size_t length, size_t *out, size_t cap) {
    size_t count = 0;
    if (cap == 0) return 0;
    out[count++] = 0;
    size_t off = 0;
    while (off < length) {
        size_t sep_end = 0;
        if (wlx_text_newline_at(text, length, off, &sep_end)) {
            if (count < cap) out[count++] = sep_end;
            off = sep_end;
        } else {
            off++;
        }
    }
    return count;
}

// The corpora shared by the equivalence and tail tests: LF, CRLF, mixed
// separators (incl. lone CR), UTF-8 multibyte at line boundaries, empty
// text, trailing newline, consecutive empty lines, and no-newline text.
static const char *ep_corpora[] = {
    "alpha\nbeta\ngamma",
    "alpha\r\nbeta\r\ngamma",
    "a\nbb\r\nccc\rdddd",
    "h\xC3\xB6la\nw\xC3\xB6rld\n\xC3\xB6",
    "",
    "trailing\n",
    "\n\n\n",
    "one line without newline",
};
#define EP_CORPUS_COUNT_ (sizeof(ep_corpora) / sizeof(ep_corpora[0]))

// ============================================================================
// Window equivalence: the offset-0 entry reproduces wlx_text_build_lines
// byte-for-byte on every corpus, wide and narrow.
// ============================================================================

TEST(editor_build_from_offset_zero_matches_full_build) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);
    float widths[2] = { 1000.0f, 20.0f };

    test_frame_begin(&ctx, 0, 0, false, false);
    for (size_t c = 0; c < EP_CORPUS_COUNT_; c++) {
        const char *text = ep_corpora[c];
        size_t length = strlen(text);
        for (size_t w = 0; w < 2; w++) {
            WLX_Text_Line_Record full[EP_MAX_LINES_], from[EP_MAX_LINES_];
            size_t full_count = ep_build_full(&ctx, text, length, widths[w], false, full, EP_MAX_LINES_);
            size_t from_count = ep_build_from(&ctx, text, length, widths[w], false, 0, from, EP_MAX_LINES_);

            ASSERT_EQ_INT((long)full_count, (long)from_count);
            for (size_t i = 0; i < full_count; i++) {
                ASSERT_TRUE(ep_records_equal(&full[i], &from[i]));
            }
        }
    }
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Tail equivalence: building from each hard line start yields exactly the
// tail of the full build. With a wide rect every record is one hard line,
// so hard line start k corresponds to full-build record k.
// ============================================================================

TEST(editor_build_from_each_hard_line_start_yields_tail) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    test_frame_begin(&ctx, 0, 0, false, false);
    for (size_t c = 0; c < EP_CORPUS_COUNT_; c++) {
        const char *text = ep_corpora[c];
        size_t length = strlen(text);

        size_t starts[EP_MAX_LINES_];
        size_t start_count = ep_hard_line_starts(text, length, starts, EP_MAX_LINES_);

        WLX_Text_Line_Record full[EP_MAX_LINES_];
        size_t full_count = ep_build_full(&ctx, text, length, 1000.0f, false, full, EP_MAX_LINES_);

        for (size_t k = 0; k < start_count; k++) {
            WLX_Text_Line_Record tail[EP_MAX_LINES_];
            size_t tail_count = ep_build_from(&ctx, text, length, 1000.0f, false, starts[k], tail, EP_MAX_LINES_);

            ASSERT_TRUE(full_count >= k);
            ASSERT_EQ_INT((long)(full_count - k), (long)tail_count);
            for (size_t i = 0; i < tail_count; i++) {
                ASSERT_TRUE(full[k + i].source_start == starts[k] || i > 0);
                ASSERT_TRUE(ep_records_equal(&full[k + i], &tail[i]));
            }
        }
    }
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// The tail equivalence also holds in truncate-and-continue mode, where
// truncated records still start on hard line boundaries.
TEST(editor_build_from_hard_line_start_yields_tail_truncate_mode) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    // Mock measure: 5 px per byte at font_size 10; rect 20 -> 4 visible units.
    const char *text = "abcdefgh\nxy\nlongerline\n";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t starts[EP_MAX_LINES_];
    size_t start_count = ep_hard_line_starts(text, length, starts, EP_MAX_LINES_);
    ASSERT_EQ_INT(4, (long)start_count);

    WLX_Text_Line_Record full[EP_MAX_LINES_];
    size_t full_count = ep_build_full(&ctx, text, length, 20.0f, true, full, EP_MAX_LINES_);
    ASSERT_EQ_INT(4, (long)full_count);

    for (size_t k = 0; k < start_count; k++) {
        WLX_Text_Line_Record tail[EP_MAX_LINES_];
        size_t tail_count = ep_build_from(&ctx, text, length, 20.0f, true, starts[k], tail, EP_MAX_LINES_);

        ASSERT_EQ_INT((long)(full_count - k), (long)tail_count);
        for (size_t i = 0; i < tail_count; i++) {
            ASSERT_TRUE(ep_records_equal(&full[k + i], &tail[i]));
        }
    }

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Truncate-and-continue: over-wide lines yield truncated records, the build
// continues past them, and record source ranges tile the text with no gaps.
// ============================================================================

TEST(editor_truncate_continue_builds_past_overwide_lines) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    const char *text = "abcdefgh\nxy\nlongerline\n";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    // Without the mode the first over-wide line ends the build.
    WLX_Text_Line_Record stopped[EP_MAX_LINES_];
    size_t stopped_count = ep_build_full(&ctx, text, length, 20.0f, false, stopped, EP_MAX_LINES_);
    ASSERT_EQ_INT(1, (long)stopped_count);

    // With it the build reaches the trailing empty line.
    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_full(&ctx, text, length, 20.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(4, (long)count);

    // Record 0: "abcd" visible, tail "efgh" skipped, separator consumed.
    ASSERT_EQ_INT(0, (long)lines[0].source_start);
    ASSERT_EQ_INT(4, (long)lines[0].visible_end);
    ASSERT_EQ_INT(8, (long)lines[0].separator_start);
    ASSERT_EQ_INT(9, (long)lines[0].separator_end);
    ASSERT_EQ_INT(9, (long)lines[0].source_end);
    ASSERT_TRUE(lines[0].ended_by_newline);
    ASSERT_EQ_F(lines[0].measured_w, 20.0f, 0.1f);

    // Record 1: "xy" fits untruncated.
    ASSERT_EQ_INT(9, (long)lines[1].source_start);
    ASSERT_EQ_INT(11, (long)lines[1].visible_end);
    ASSERT_EQ_INT(12, (long)lines[1].source_end);

    // Record 2: "long" visible out of "longerline".
    ASSERT_EQ_INT(12, (long)lines[2].source_start);
    ASSERT_EQ_INT(16, (long)lines[2].visible_end);
    ASSERT_EQ_INT(22, (long)lines[2].separator_start);
    ASSERT_EQ_INT(23, (long)lines[2].source_end);

    // Record 3: trailing empty line after the final separator.
    ASSERT_EQ_INT((long)length, (long)lines[3].source_start);
    ASSERT_TRUE(lines[3].empty_visual);

    // Tiling: consecutive source ranges, first at 0, last ending at length.
    ASSERT_EQ_INT(0, (long)lines[0].source_start);
    for (size_t i = 1; i < count; i++) {
        ASSERT_EQ_INT((long)lines[i - 1].source_end, (long)lines[i].source_start);
    }
    ASSERT_EQ_INT((long)length, (long)lines[count - 1].source_end);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_truncate_continue_no_trailing_newline_reaches_eof) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    const char *text = "abcdefgh";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_full(&ctx, text, length, 20.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(1, (long)count);

    // The invisible tail runs to end-of-text; the record covers all of it.
    ASSERT_EQ_INT(0, (long)lines[0].source_start);
    ASSERT_EQ_INT(4, (long)lines[0].visible_end);
    ASSERT_EQ_INT((long)length, (long)lines[0].source_end);
    ASSERT_EQ_INT((long)length, (long)lines[0].separator_start);
    ASSERT_EQ_INT((long)length, (long)lines[0].separator_end);
    ASSERT_FALSE(lines[0].ended_by_newline);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_truncate_continue_crlf_tail_consumes_pair) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    const char *text = "abcdefgh\r\nxy";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_full(&ctx, text, length, 20.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);

    // The CRLF pair after the invisible tail is one separator.
    ASSERT_EQ_INT(4, (long)lines[0].visible_end);
    ASSERT_EQ_INT(8, (long)lines[0].separator_start);
    ASSERT_EQ_INT(10, (long)lines[0].separator_end);
    ASSERT_EQ_INT(10, (long)lines[0].source_end);
    ASSERT_EQ_INT(10, (long)lines[1].source_start);
    ASSERT_EQ_INT((long)length, (long)lines[1].source_end);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_truncate_continue_multibyte_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    // Five o-umlauts (2 bytes each, 10 px each under the mock): rect 20
    // truncates after two codepoints on a byte boundary inside the line.
    const char *text = "\xC3\xB6\xC3\xB6\xC3\xB6\xC3\xB6\xC3\xB6\nx";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_full(&ctx, text, length, 20.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);

    ASSERT_EQ_INT(4, (long)lines[0].visible_end);
    ASSERT_EQ_INT(10, (long)lines[0].separator_start);
    ASSERT_EQ_INT(11, (long)lines[0].source_end);
    ASSERT_EQ_INT(11, (long)lines[1].source_start);
    ASSERT_EQ_INT((long)length, (long)lines[1].source_end);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Per-record unit budget: each record measures up to
// WLX_EDITOR_MAX_LINE_UNITS units, then freezes; the budget resets for the
// next record instead of draining a shared build-wide cap.
// ============================================================================

TEST(editor_truncate_continue_per_record_budget_resets) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    // Two lines, each longer than the per-record cap, in one buffer:
    // "aaaa...\nbbbb..." with 'a'/'b' runs of cap + 76 units.
    enum { EP_LONG_ = WLX_EDITOR_MAX_LINE_UNITS + 76 };
    static char text[2 * EP_LONG_ + 2];
    memset(text, 'a', EP_LONG_);
    text[EP_LONG_] = '\n';
    memset(text + EP_LONG_ + 1, 'b', EP_LONG_);
    text[2 * EP_LONG_ + 1] = '\0';
    size_t length = 2 * EP_LONG_ + 1;

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    // rect wide enough that only the unit budget truncates.
    size_t count = ep_build_full(&ctx, text, length, 1e9f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);

    // Both records freeze at exactly the cap: the second record got a
    // fresh budget rather than the remainder of a shared one.
    ASSERT_EQ_INT(WLX_EDITOR_MAX_LINE_UNITS, (long)(lines[0].visible_end - lines[0].visible_start));
    ASSERT_EQ_INT(WLX_EDITOR_MAX_LINE_UNITS, (long)(lines[1].visible_end - lines[1].visible_start));
    ASSERT_EQ_F(lines[0].measured_w, (float)WLX_EDITOR_MAX_LINE_UNITS * 5.0f, 0.5f);
    ASSERT_EQ_F(lines[1].measured_w, (float)WLX_EDITOR_MAX_LINE_UNITS * 5.0f, 0.5f);

    // Tiling still holds across the skipped tails.
    ASSERT_EQ_INT(0, (long)lines[0].source_start);
    ASSERT_EQ_INT((long)(EP_LONG_ + 1), (long)lines[0].source_end);
    ASSERT_EQ_INT((long)(EP_LONG_ + 1), (long)lines[1].source_start);
    ASSERT_EQ_INT((long)length, (long)lines[1].source_end);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// The mode leaves the global build-wide cap semantics alone: without it, an
// exhausted shared budget still ends the build.
TEST(editor_default_mode_keeps_shared_unit_cap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    const char *text = "abcdef\nghijkl";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Build_Inputs inputs = {
        .ctx = &ctx,
        .text = text,
        .length = length,
        .style = (WLX_Text_Style){ .font_size = 10 },
        .rect = (WLX_Rect){ 0, 0, 1000.0f, 10000.0f },
        .wrap = false,
        .line_h = 10.0f,
        .text_unit_cap = 8,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = wlx_text_build_lines(&inputs, &cursor, lines, EP_MAX_LINES_);

    // Line one takes 6 units; line two truncates at the shared cap (2 more).
    ASSERT_EQ_INT(2, (long)count);
    ASSERT_EQ_INT(6, (long)(lines[0].visible_end - lines[0].visible_start));
    ASSERT_EQ_INT(2, (long)(lines[1].visible_end - lines[1].visible_start));
    ASSERT_EQ_INT(8, (long)cursor.text_unit_count);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_truncate_continue_empty_text_builds_nothing) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_full(&ctx, "", 0, 20.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(0, (long)count);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Wrap-mode window equivalence: with the per-line budget active, building
// from any hard line start reproduces the corresponding tail of a full
// wrapped build - wrap decisions never cross a hard newline, so the first
// row of every hard line is a safe re-entry point.
// ============================================================================

TEST(editor_wrap_build_from_hard_line_start_yields_tail) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    // The shared corpora plus tab-bearing text; the narrow widths force
    // multi-row wrapping (7 px: one 5 px unit per row, multibyte units
    // over-wide on their own row).
    const char *corpora[EP_CORPUS_COUNT_ + 3];
    for (size_t c = 0; c < EP_CORPUS_COUNT_; c++) corpora[c] = ep_corpora[c];
    corpora[EP_CORPUS_COUNT_] = "a\tbb\tccc\nx\ty";
    // Spaced prose: doubled spaces, an over-wide word, a tab, a
    // spaces-only line and a trailing space, so word cuts and hanging
    // whitespace re-enter identically too.
    corpora[EP_CORPUS_COUNT_ + 1] = "so  many  spaces\nshort zzzzzzzzzzzzz after\ntab\tstop x\n   \nend ";
    // Grapheme clusters (corpus macros from test_grapheme.c, same TU): at
    // 7 px every cluster is an over-wide unit on a row of its own.
    corpora[EP_CORPUS_COUNT_ + 2] = "ab " GR_EACUTE " " GR_FLAG "\n" GR_FAMILY GR_FAMILY " x\n"
        GR_HEART "\n" GR_THUMBS;
    float widths[3] = { 1000.0f, 20.0f, 7.0f };

    test_frame_begin(&ctx, 0, 0, false, false);
    // Both whitespace modes: display (hanging) and editable (strict).
    for (int mode = 0; mode < 2; mode++)
    for (size_t c = 0; c < EP_CORPUS_COUNT_ + 3; c++) {
        const char *text = corpora[c];
        size_t length = strlen(text);
        bool strict = mode == 1;

        size_t starts[EP_MAX_LINES_];
        size_t start_count = ep_hard_line_starts(text, length, starts, EP_MAX_LINES_);

        for (size_t w = 0; w < 3; w++) {
            WLX_Text_Line_Record full[EP_MAX_LINES_];
            size_t full_count = ep_build_wrap_from_mode(&ctx, text, length, widths[w], true,
                strict, 0, full, EP_MAX_LINES_);

            for (size_t k = 0; k < start_count; k++) {
                WLX_Text_Line_Record tail[EP_MAX_LINES_];
                size_t tail_count = ep_build_wrap_from_mode(&ctx, text, length, widths[w], true,
                    strict, starts[k], tail, EP_MAX_LINES_);

                if (full_count == 0) {
                    ASSERT_EQ_INT(0, (long)tail_count);
                    continue;
                }
                // The first full-build record of hard line k: rows are
                // offset-ordered and continuation rows start mid-line, so
                // it is the first record starting at starts[k].
                size_t j = 0;
                while (j < full_count && full[j].source_start != starts[k]) j++;
                ASSERT_TRUE(j < full_count);
                ASSERT_EQ_INT((long)(full_count - j), (long)tail_count);
                for (size_t i = 0; i < tail_count; i++) {
                    ASSERT_TRUE(ep_records_equal(&full[j + i], &tail[i]));
                }
            }
        }
    }
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Per-hard-line budget under wrap: a line exhausting the budget freezes -
// its final row skips the unmeasured tail - and the next line starts with
// a fresh budget.
// ============================================================================

TEST(editor_wrap_line_budget_freezes_tail_and_continues) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    enum { EP_WLONG_ = WLX_EDITOR_MAX_LINE_UNITS + 76 };
    static char text[EP_WLONG_ + 3 + 1];
    memset(text, 'a', EP_WLONG_);
    text[EP_WLONG_] = '\n';
    text[EP_WLONG_ + 1] = 'x';
    text[EP_WLONG_ + 2] = 'y';
    text[EP_WLONG_ + 3] = '\0';
    size_t length = EP_WLONG_ + 3;

    test_frame_begin(&ctx, 0, 0, false, false);

    // Wide rect: only the per-line budget truncates, so the long line is
    // one frozen row and the build continues to the next line.
    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = ep_build_wrap_from(&ctx, text, length, 1e9f, true, 0, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);

    ASSERT_EQ_INT(WLX_EDITOR_MAX_LINE_UNITS,
        (long)(lines[0].visible_end - lines[0].visible_start));
    ASSERT_EQ_INT((long)EP_WLONG_, (long)lines[0].separator_start);
    ASSERT_EQ_INT((long)(EP_WLONG_ + 1), (long)lines[0].separator_end);
    ASSERT_EQ_INT((long)(EP_WLONG_ + 1), (long)lines[0].source_end);
    ASSERT_TRUE(lines[0].ended_by_newline);

    ASSERT_EQ_INT((long)(EP_WLONG_ + 1), (long)lines[1].source_start);
    ASSERT_EQ_INT((long)length, (long)lines[1].source_end);
    ASSERT_EQ_INT(2, (long)(lines[1].visible_end - lines[1].visible_start));

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_line_budget_exhausts_across_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    // Narrow rect (4 units per row): the long line spends its budget over
    // WLX_EDITOR_MAX_LINE_UNITS / 4 rows, the budget runs out exactly at a
    // row boundary, so the last in-budget row carries the tail skip and
    // the next hard line still gets a fresh budget.
    enum { EP_WROWU_ = 4, EP_WTAIL_ = 30 };
    enum { EP_WROWS_ = WLX_EDITOR_MAX_LINE_UNITS / EP_WROWU_ };
    enum { EP_WLEN_ = WLX_EDITOR_MAX_LINE_UNITS + EP_WTAIL_ };
    static char text[EP_WLEN_ + 3 + 1];
    memset(text, 'a', EP_WLEN_);
    text[EP_WLEN_] = '\n';
    text[EP_WLEN_ + 1] = 'x';
    text[EP_WLEN_ + 2] = 'y';
    text[EP_WLEN_ + 3] = '\0';
    size_t length = EP_WLEN_ + 3;

    test_frame_begin(&ctx, 0, 0, false, false);

    static WLX_Text_Line_Record lines[EP_WROWS_ + 8];
    size_t count = ep_build_wrap_from(&ctx, text, length, 20.0f, true, 0,
        lines, EP_WROWS_ + 8);
    ASSERT_EQ_INT((long)(EP_WROWS_ + 1), (long)count);

    // In-budget rows: 4 units each, tiling forward, no separators.
    for (size_t i = 0; i + 2 < count; i++) {
        ASSERT_EQ_INT(EP_WROWU_, (long)(lines[i].visible_end - lines[i].visible_start));
        ASSERT_FALSE(lines[i].ended_by_newline);
        ASSERT_EQ_INT((long)lines[i].source_end, (long)lines[i + 1].source_start);
    }

    // The budget-spending row carries the tail skip and the separator.
    const WLX_Text_Line_Record *last_row = &lines[EP_WROWS_ - 1];
    ASSERT_EQ_INT(EP_WROWU_, (long)(last_row->visible_end - last_row->visible_start));
    ASSERT_EQ_INT((long)EP_WLEN_, (long)last_row->separator_start);
    ASSERT_EQ_INT((long)(EP_WLEN_ + 1), (long)last_row->source_end);
    ASSERT_TRUE(last_row->ended_by_newline);

    // The next hard line wraps with a fresh budget.
    ASSERT_EQ_INT((long)(EP_WLEN_ + 1), (long)lines[EP_WROWS_].source_start);
    ASSERT_EQ_INT(2, (long)(lines[EP_WROWS_].visible_end - lines[EP_WROWS_].visible_start));
    ASSERT_EQ_INT((long)length, (long)lines[EP_WROWS_].source_end);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// Without the mode flag, wrap keeps the shared build-wide cap semantics: an
// exhausted budget still freezes the build (the note-scale inputbox model).
TEST(editor_wrap_without_truncate_keeps_shared_cap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);

    const char *text = "abcdef\nghijkl";
    size_t length = strlen(text);

    test_frame_begin(&ctx, 0, 0, false, false);

    WLX_Text_Build_Inputs inputs = {
        .ctx = &ctx,
        .text = text,
        .length = length,
        .style = (WLX_Text_Style){ .font_size = 10 },
        .rect = (WLX_Rect){ 0, 0, 1000.0f, 10000.0f },
        .wrap = true,
        .line_h = 10.0f,
        .text_unit_cap = 8,
    };
    WLX_Text_Build_Cursor cursor = { .text_unit_count = 0 };
    WLX_Text_Line_Record lines[EP_MAX_LINES_];
    size_t count = wlx_text_build_lines(&inputs, &cursor, lines, EP_MAX_LINES_);

    // Line one takes 6 units; line two truncates at the shared cap and the
    // build ends there instead of skipping to further lines.
    ASSERT_EQ_INT(2, (long)count);
    ASSERT_EQ_INT(6, (long)(lines[0].visible_end - lines[0].visible_start));
    ASSERT_EQ_INT(2, (long)(lines[1].visible_end - lines[1].visible_start));
    ASSERT_EQ_INT(8, (long)cursor.text_unit_count);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hard line start predicate: the scan-side newline semantics used by the
// from-offset debug contract (CRLF is one separator, lone CR ends a line).
// ============================================================================

TEST(editor_hard_line_start_predicate) {
    const char *text = "a\nb\r\nc\rd\xC3\xB6";
    size_t length = strlen(text);

    ASSERT_TRUE(wlx_text_hard_line_start_at(text, length, 0));
    ASSERT_FALSE(wlx_text_hard_line_start_at(text, length, 1));   // at '\n'
    ASSERT_TRUE(wlx_text_hard_line_start_at(text, length, 2));    // after '\n'
    ASSERT_FALSE(wlx_text_hard_line_start_at(text, length, 4));   // inside "\r\n"
    ASSERT_TRUE(wlx_text_hard_line_start_at(text, length, 5));    // after "\r\n"
    ASSERT_TRUE(wlx_text_hard_line_start_at(text, length, 7));    // after lone '\r'
    ASSERT_FALSE(wlx_text_hard_line_start_at(text, length, 8));   // mid-word
    ASSERT_FALSE(wlx_text_hard_line_start_at(text, length, length));

    ASSERT_TRUE(wlx_text_hard_line_start_at("x\n", 2, 2));        // trailing LF
    ASSERT_TRUE(wlx_text_hard_line_start_at("x\r", 2, 2));        // trailing CR
    ASSERT_FALSE(wlx_text_hard_line_start_at("xy", 2, 3));        // past the end
    ASSERT_TRUE(wlx_text_hard_line_start_at(NULL, 0, 0));
}

// ============================================================================
// Measurement-path coverage: the heights split and the semantic batch
// gate, pinned so the scan-loop implementation cannot silently unify
// them (the standard mock's height equals line_h and its advances are
// bit-identical to prefix measures, which hides both).
// ============================================================================

TEST(editor_batch_heights_line_h_per_unit_heights_measured) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);
    test_install_mock_tall_measure(&ctx);
    test_frame_begin(&ctx, 0, 0, false, false);

    const char *text = "alpha\nbeta";
    size_t length = strlen(text);
    WLX_Text_Line_Record lines[EP_MAX_LINES_];

    // Per-unit fills store the backend-measured height of the visible
    // prefix (tall mock: font_size + bytes), not the uniform line_h.
    size_t count = ep_build_full(&ctx, text, length, 1000.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);
    ASSERT_EQ_F(15.0f, lines[0].measured_h, 0.001f);
    ASSERT_EQ_F(14.0f, lines[1].measured_h, 0.001f);

    // Batch fills substitute the build's uniform line_h: advances report
    // x geometry only - the documented heights split.
    test_install_mock_advances(&ctx);
    count = ep_build_full(&ctx, text, length, 1000.0f, true, lines, EP_MAX_LINES_);
    ASSERT_EQ_INT(2, (long)count);
    ASSERT_EQ_F(10.0f, lines[0].measured_h, 0.001f);
    ASSERT_EQ_F(10.0f, lines[1].measured_h, 0.001f);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(editor_non_editor_builds_never_batch) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 400);
    test_frame_begin(&ctx, 0, 0, false, false);

    const char *text = "alpha\nbeta gamma delta";
    size_t length = strlen(text);
    WLX_Text_Line_Record base[EP_MAX_LINES_];
    size_t base_count = ep_build_full(&ctx, text, length, 40.0f, false, base, EP_MAX_LINES_);

    // The batch gate is semantic, not capability-based: installing the
    // advances callback must not reroute a non-editor build (no
    // truncate_continue) - records stay byte-identical and the callback
    // is never asked.
    test_install_mock_advances(&ctx);
    test_reset_mock_advances_calls();
    WLX_Text_Line_Record with[EP_MAX_LINES_];
    size_t with_count = ep_build_full(&ctx, text, length, 40.0f, false, with, EP_MAX_LINES_);
    ASSERT_EQ_INT((long)base_count, (long)with_count);
    for (size_t i = 0; i < base_count; i++) {
        ASSERT_TRUE(ep_records_equal(&base[i], &with[i]));
    }
    ASSERT_EQ_INT(0, (long)test_mock_advances_calls());

    // Sanity for the counter itself: the editor-mode build does batch.
    ep_build_full(&ctx, text, length, 40.0f, true, with, EP_MAX_LINES_);
    ASSERT_TRUE(test_mock_advances_calls() > 0);

    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

SUITE(editor_pipeline) {
    RUN_TEST(editor_build_from_offset_zero_matches_full_build);
    RUN_TEST(editor_build_from_each_hard_line_start_yields_tail);
    RUN_TEST(editor_build_from_hard_line_start_yields_tail_truncate_mode);
    RUN_TEST(editor_truncate_continue_builds_past_overwide_lines);
    RUN_TEST(editor_truncate_continue_no_trailing_newline_reaches_eof);
    RUN_TEST(editor_truncate_continue_crlf_tail_consumes_pair);
    RUN_TEST(editor_truncate_continue_multibyte_boundary);
    RUN_TEST(editor_truncate_continue_per_record_budget_resets);
    RUN_TEST(editor_default_mode_keeps_shared_unit_cap);
    RUN_TEST(editor_truncate_continue_empty_text_builds_nothing);
    RUN_TEST(editor_wrap_build_from_hard_line_start_yields_tail);
    RUN_TEST(editor_wrap_line_budget_freezes_tail_and_continues);
    RUN_TEST(editor_wrap_line_budget_exhausts_across_rows);
    RUN_TEST(editor_wrap_without_truncate_keeps_shared_cap);
    RUN_TEST(editor_hard_line_start_predicate);
    RUN_TEST(editor_batch_heights_line_h_per_unit_heights_measured);
    RUN_TEST(editor_non_editor_builds_never_batch);
}

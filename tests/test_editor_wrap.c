// test_editor_wrap.c - wlx_editor wrapped mode, view path: rows drawn at
// the band width, row-space wheel scrolling (anchor first_line/first_row),
// the overflow probe for few heavily wrapped lines, top and bottom clamps
// (backfilled exact end), thumb drag with track-end-means-document-end,
// wrap toggling carrying the anchor, anchor row clamping on document
// change, and first-row-only gutter numbering.
//
// Geometry model (mock backend): char width = font_size/2 = 5 px at
// font_size 10, line height 10. Fixture: 400x100 context, content_padding
// 4, border 0 -> input_rect {4,4,392,92}, band {9,4,383,92} before strips;
// the vertical strip narrows the band to 373 (74 chars per wrapped row).
// Reuses the ev_* fixture from test_editor_view.c.

// Editor call with wrap controls; same fixture geometry as ev_editor.
static bool ew_editor(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t revision,
                      bool wrap, bool line_numbers) {
    bool focused = false;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .revision = revision, .out_focused = &focused,
            .wrap = wrap, .line_numbers = line_numbers),
        __FILE__, __LINE__);
    return focused;
}

static bool ew_frame_full(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                          bool wrap, bool line_numbers,
                          int mx, int my, bool down, bool clicked, float wheel) {
    test_frame_begin_full(ctx, mx, my, down, clicked, down, wheel, NULL, NULL, NULL, 0, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ew_editor(ctx, buf, cap, len, rev, wrap, line_numbers);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool ew_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev) {
    return ew_frame_full(ctx, buf, cap, len, rev, true, false, 200, 50, false, false, 0.0f);
}

static bool ew_frame_wheel(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                           float wheel) {
    return ew_frame_full(ctx, buf, cap, len, rev, true, false, 200, 50, false, false, wheel);
}

// Fill buf with one long line of n_a 'a' bytes, a newline, "xy", a newline,
// then extra short lines.
static size_t ew_fill_long_first(char *buf, size_t cap, size_t n_a, size_t extra_lines) {
    size_t off = 0;
    for (size_t i = 0; i < n_a && off + 1 < cap; i++) buf[off++] = 'a';
    buf[off++] = '\n';
    buf[off++] = 'x';
    buf[off++] = 'y';
    buf[off++] = '\n';
    for (size_t i = 0; i < extra_lines && off + 4 < cap; i++) {
        buf[off++] = 'l';
        buf[off++] = (char)('0' + (i / 10) % 10);
        buf[off++] = (char)('0' + i % 10);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// ============================================================================
// Wrapped rows render at the band width
// ============================================================================

TEST(editor_wrap_draws_band_wide_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text = _ev_capture_draw_text;

    // 200 'a's wrap into 3 rows at ~74 chars per row, then "xy".
    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 0);

    _ev_reset_captures();
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Rows stack at x = band.x, successive y (band.y = 4, line_h 10); the
    // wrapped line yields three rows and "xy" follows on the fourth.
    ASSERT_TRUE(_ev_capture_count >= 4);
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ_F(_ev_captures[i].x, 9.0f, 0.01f);
        ASSERT_EQ_F(_ev_captures[i].y, 4.0f + 10.0f * (float)i, 0.01f);
    }
    ASSERT_TRUE(strcmp(_ev_captures[3].text, "xy") == 0);
    // No row measures wider than the band in characters.
    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(strlen(_ev_captures[i].text) <= 76);
        ASSERT_TRUE(_ev_captures[i].text[0] == 'a');
    }

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Wheel moves visual rows, crossing hard line boundaries
// ============================================================================

TEST(editor_wrap_wheel_scrolls_rows_not_lines) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Line 0 wraps into 3 rows; then "xy" and 20 short lines.
    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);

    // One notch = 20 px = 2 rows: stays inside the wrapped line.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -1.0f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(2, (long)state->first_row);

    // Two more notches = 4 rows: row 3 of line 0 does not exist (3 rows),
    // so the anchor walks into the following hard lines.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -2.0f);
    ASSERT_EQ_INT(4, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);

    // Back up 5 rows: 4 line starts back, then one wrapped row up ends on
    // the long line's last row.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, 2.5f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(1, (long)state->first_row);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Overflow probe: few hard lines, many rows
// ============================================================================

TEST(editor_wrap_overflow_probe_detects_heavy_wrapping) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Three 300-char lines: 4 rows each at ~74 chars per row - 12 rows in a
    // 9-row band while the hard line count (4 with the trailing empty
    // line) stays under it, so only the probe can see the overflow.
    static char buf[1024];
    size_t off = 0;
    for (int l = 0; l < 3; l++) {
        for (int i = 0; i < 300; i++) buf[off++] = (char)('a' + l);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    size_t len = off;

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // The wheel is consumed and moves rows: overflow was detected.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -1.0f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(2, (long)state->first_row);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_no_overflow_leaves_anchor_alone) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[64];
    size_t len = ev_fill_lines(buf, sizeof(buf), 3);

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -1.0f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.0f, 0.001f);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Clamps: the top stops at (0,0,0), the bottom backfills the exact end
// ============================================================================

TEST(editor_wrap_bottom_clamp_lands_exact_end) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // 30 one-row lines plus the trailing empty line = 31 rows; the band
    // shows 9.2 rows, so the bottom anchor backfills ceil(9.2) = 10 rows
    // from the end: anchor line 21, fraction 0.8 of a row cut off above.
    static char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 30);

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Wheel far past the end in one gesture.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -100.0f);
    ASSERT_EQ_INT(21, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.8f, 0.01f);

    // And back up past the start.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, 100.0f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.0f, 0.001f);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_thumb_drag_to_track_end_reaches_document_end) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 30);

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Press on the thumb (top of the track at anchor 0), then drag far
    // below the track: the position clamps to the track end, which always
    // means the document end - the same backfilled anchor as the wheel
    // clamp.
    float sb_w = ctx.theme->scrollbar.width > 0.0f ? ctx.theme->scrollbar.width : 10.0f;
    WLX_Rect thumb = wlx_scrollbar_rect((WLX_Rect){ 4, 4, 392, 92 }, 310.0f, 0.0f, sb_w);
    int tx = (int)(thumb.x + thumb.w * 0.5f);
    int ty = (int)(thumb.y + thumb.h * 0.5f);

    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, tx, ty, true, true, 0.0f);
    ASSERT_TRUE(state->caret.dragging_scrollbar);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, tx, 500, true, false, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, tx, 500, false, false, 0.0f);

    ASSERT_EQ_INT(21, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.8f, 0.01f);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Mode transitions carry the anchor and reset the foreign-mode state
// ============================================================================

TEST(editor_wrap_toggle_carries_anchor_and_resets) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);

    // Scroll two wrapped rows in.
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -1.0f);
    ASSERT_EQ_INT(2, (long)state->first_row);
    state->caret.preferred_x = 33.0f;
    state->caret.preferred_x_valid = true;

    // Leaving wrap zeroes the row and drops the sticky column; the anchor
    // line survives.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, false, false, 200, 50, false, false, 0.0f);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_FALSE(state->caret.preferred_x_valid);

    // Scroll sideways in no-wrap mode, then re-enter wrap: scroll_x resets.
    state->scroll_x = 120.0f;
    state->caret.preferred_x_valid = true;
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_F(state->scroll_x, 0.0f, 0.001f);
    ASSERT_FALSE(state->caret.preferred_x_valid);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Anchor row clamps when the document changes under the anchor
// ============================================================================

TEST(editor_wrap_anchor_row_clamps_on_document_change) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -1.0f);
    ASSERT_EQ_INT(2, (long)state->first_row);

    // The long first line shrinks to one row (external mutation, revision
    // bump): the anchor row must clamp inside the line's new row count.
    len = ev_fill_lines(buf, sizeof(buf), 30);
    ew_frame(&ctx, buf, sizeof(buf), &len, 1);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Gutter numbers only the first row of each hard line
// ============================================================================

TEST(editor_wrap_gutter_numbers_first_rows_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text = _ev_capture_draw_text;

    // Line 1 wraps into 3 rows, line 2 is "xy", line 3 is the trailing
    // empty line: the gutter draws "1", "2", "3" exactly once each.
    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 0);

    _ev_reset_captures();
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, true, 200, 50, false, false, 0.0f);

    int ones = 0, twos = 0, threes = 0;
    for (int i = 0; i < _ev_capture_count; i++) {
        if (strcmp(_ev_captures[i].text, "1") == 0) ones++;
        if (strcmp(_ev_captures[i].text, "2") == 0) twos++;
        if (strcmp(_ev_captures[i].text, "3") == 0) threes++;
    }
    ASSERT_EQ_INT(1, ones);
    ASSERT_EQ_INT(1, twos);
    ASSERT_EQ_INT(1, threes);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Caret in rows: click, UP/DOWN across rows and line boundaries, paging
// ============================================================================

static bool ew_frame_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                         WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 50, false, false, false, 0.0f, NULL, keys_pressed,
        NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ew_editor(ctx, buf, cap, len, 0, true, false);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

TEST(editor_wrap_click_and_vertical_motion_step_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Line 0 wraps into 3 rows; enough extra lines for vertical overflow.
    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Click on row 0, column 2 (band x 9 + 11 px -> boundary midpoint puts
    // the caret at offset 2), which also focuses the editor.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    ASSERT_EQ_INT(2, (long)state->caret.cursor_pos);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, false, false, 0.0f);

    // DOWN moves one visual row inside the same hard line, keeping the
    // column; a second DOWN repeats it. The row width in characters is
    // unknown here, but both steps must advance by exactly one row:
    // c1 = width + 2, c2 = 2 * width + 2 -> c2 - c1 == c1 - 2.
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    size_t c1 = state->caret.cursor_pos;
    ASSERT_TRUE(c1 > 2 && c1 < 200);
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    size_t c2 = state->caret.cursor_pos;
    ASSERT_EQ_INT((long)(c1 - 2), (long)(c2 - c1));

    // UP twice returns to the click position.
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ASSERT_EQ_INT(2, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_vertical_motion_crosses_line_boundaries) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Click at the start of the "xy" row (row 3: the long line holds rows
    // 0-2): caret lands on the line start, offset 201.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 38, true, true, 0.0f);
    ASSERT_EQ_INT(201, (long)state->caret.cursor_pos);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 38, false, false, 0.0f);

    // UP lands on the long line's LAST row (not its start), at column 0 of
    // that row: an offset strictly inside the line, past two full rows.
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    size_t on_last_row = state->caret.cursor_pos;
    ASSERT_TRUE(on_last_row > 100 && on_last_row < 200);

    // DOWN returns to the "xy" line start.
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(201, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_page_keys_step_band_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 30);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Focus at the document start.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 8, true, true, 0.0f);
    ASSERT_EQ_INT(0, (long)state->caret.cursor_pos);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 8, false, false, 0.0f);

    // One-row lines: PageDown steps the caret a band of rows (92 px / 10),
    // 9 lines of 4 bytes each.
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_PAGE_DOWN, 0);
    ASSERT_EQ_INT(36, (long)state->caret.cursor_pos);
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_PAGE_UP, 0);
    ASSERT_EQ_INT(0, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Caret-follow: near moves scroll by rows, far jumps re-anchor exactly
// ============================================================================

TEST(editor_wrap_ctrl_end_bottom_aligns_and_home_returns) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 30);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Focus, then Ctrl+End: the caret jumps to the document end and the
    // view bottom-aligns through the backward fill - the same anchor the
    // bottom clamp derives: line 21, fraction 0.8.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 8, true, true, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 8, false, false, 0.0f);
    uint32_t command = 0;
#if defined(__APPLE__)
    command = WLX_MOD_SUPER;
#else
    command = WLX_MOD_CTRL;
#endif
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, command);
    ASSERT_EQ_INT((long)len, (long)state->caret.cursor_pos);
    ASSERT_EQ_INT(21, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.8f, 0.01f);

    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, command);
    ASSERT_EQ_INT(0, (long)state->caret.cursor_pos);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);
    ASSERT_EQ_F(state->y_frac, 0.0f, 0.001f);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_drag_select_auto_scrolls_rows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Press on row 0 column 2, then hold the pointer below the band: the
    // view scrolls down by rows while the selection grows toward it.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    size_t anchor = state->caret.selection_anchor;
    ASSERT_EQ_INT(2, (long)anchor);

    for (int i = 0; i < 10; i++) {
        ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 200, true, false, 0.0f);
    }
    ASSERT_TRUE(state->first_line > 0 || state->first_row > 0);
    ASSERT_EQ_INT((long)anchor, (long)state->caret.selection_anchor);
    ASSERT_TRUE(state->caret.cursor_pos > 201);
    ASSERT_TRUE(state->caret.mouse_selecting);

    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 200, false, false, 0.0f);
    ASSERT_FALSE(state->caret.mouse_selecting);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Caret draw: on its row's y, only when the row is inside the window
// ============================================================================

#define EW_MAX_LINES_CAP_ 16
static struct { float x; float y0; float y1; } _ew_lines[EW_MAX_LINES_CAP_];
static int _ew_line_count = 0;

static void _ew_capture_draw_line(float x1, float y1, float x2, float y2, float thick,
                                  WLX_Color c) {
    (void)thick; (void)c;
    if (_ew_line_count < EW_MAX_LINES_CAP_) {
        _ew_lines[_ew_line_count].x = x1;
        _ew_lines[_ew_line_count].y0 = y1;
        _ew_lines[_ew_line_count].y1 = y2;
        (void)x2;
        _ew_line_count++;
    }
}

TEST(editor_wrap_caret_draws_on_its_row) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_line = _ew_capture_draw_line;

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Click row 0 column 2, DOWN to row 1: the caret line must draw inside
    // the second row's y band (14..24) at the sticky column x (band 9 +
    // 10 px prefix + cursor padding).
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, false, false, 0.0f);
    _ew_line_count = 0;
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);

    ASSERT_TRUE(_ew_line_count >= 1);
    bool found = false;
    for (int i = 0; i < _ew_line_count; i++) {
        if (_ew_lines[i].y0 >= 14.0f && _ew_lines[i].y1 <= 24.0f
            && _ew_lines[i].x > 9.0f && _ew_lines[i].x < 30.0f) found = true;
    }
    ASSERT_TRUE(found);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Editing under wrap: reflow, splits, frozen tails, select-all replace,
// and mode toggles with live state (offsets are mode-independent)
// ============================================================================

static bool ew_frame_text(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                          const char *text) {
    test_frame_begin_full(ctx, 200, 50, false, false, false, 0.0f, NULL, NULL, NULL, 0, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ew_editor(ctx, buf, cap, len, 0, true, false);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

TEST(editor_wrap_typing_inserts_at_row_caret) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Click row 0 column 2, type one character: it lands at offset 2 and
    // the caret advances with it.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    ASSERT_EQ_INT(2, (long)state->caret.cursor_pos);
    size_t before = len;
    ew_frame_text(&ctx, buf, sizeof(buf), &len, "Z");
    ASSERT_EQ_INT((long)(before + 1), (long)len);
    ASSERT_TRUE(buf[2] == 'Z');
    ASSERT_EQ_INT(3, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_enter_splits_wrapped_line) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(state != NULL && idx != NULL);
    size_t lines_before = idx->count;

    // Caret one row down (row 1 start region), then Enter: the wrapped
    // line splits into two hard lines and the index grows by one.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 10, 18, true, true, 0.0f);
    size_t at = state->caret.cursor_pos;
    ASSERT_TRUE(at > 2 && at < 200);
    size_t before = len;
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ASSERT_EQ_INT((long)(before + 1), (long)len);
    ASSERT_TRUE(buf[at] == '\n');
    ASSERT_EQ_INT((long)(at + 1), (long)state->caret.cursor_pos);
    ASSERT_EQ_INT((long)(lines_before + 1), (long)idx->count);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_frozen_tail_caret_pins_and_appends) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // One hard line longer than the per-line budget: its tail is frozen
    // out of geometry, but the caret can still reach the document end and
    // edits there work.
    enum { EW_FROZEN_ = WLX_EDITOR_MAX_LINE_UNITS + 50 };
    static char buf[EW_FROZEN_ + 64];
    memset(buf, 'a', EW_FROZEN_);
    buf[EW_FROZEN_] = '\0';
    size_t len = EW_FROZEN_;

    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, false, false, 0.0f);
    uint32_t command = 0;
#if defined(__APPLE__)
    command = WLX_MOD_SUPER;
#else
    command = WLX_MOD_CTRL;
#endif
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, command);
    ASSERT_EQ_INT((long)len, (long)state->caret.cursor_pos);

    ew_frame_text(&ctx, buf, sizeof(buf), &len, "Z");
    ASSERT_EQ_INT((long)(EW_FROZEN_ + 1), (long)len);
    ASSERT_TRUE(buf[EW_FROZEN_] == 'Z');
    ASSERT_EQ_INT((long)len, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_select_all_replace_clamps_anchor) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 30);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(state != NULL && idx != NULL);

    // Scroll to the bottom, then select-all and type: the document
    // collapses to one character and the anchor clamps back to the top.
    ew_frame_wheel(&ctx, buf, sizeof(buf), &len, 0, -100.0f);
    ASSERT_TRUE(state->first_line > 0);

    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    uint32_t command = 0;
#if defined(__APPLE__)
    command = WLX_MOD_SUPER;
#else
    command = WLX_MOD_CTRL;
#endif
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_A, command);
    ASSERT_EQ_INT(0, (long)state->caret.selection_anchor);
    ASSERT_EQ_INT((long)len, (long)state->caret.cursor_pos);

    ew_frame_text(&ctx, buf, sizeof(buf), &len, "Q");
    ASSERT_EQ_INT(1, (long)len);
    ASSERT_TRUE(buf[0] == 'Q');
    ASSERT_EQ_INT(1, (long)state->caret.cursor_pos);
    ASSERT_EQ_INT(1, (long)idx->count);
    ASSERT_EQ_INT(0, (long)state->first_line);
    ASSERT_EQ_INT(0, (long)state->first_row);

    wlx_context_destroy(&ctx);
}

TEST(editor_wrap_toggle_preserves_caret_and_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 20);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);

    // Select from row 0 column 2 down one row by dragging.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 8, true, true, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 18, true, false, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, false, 20, 18, false, false, 0.0f);
    size_t anchor = state->caret.selection_anchor;
    size_t cursor = state->caret.cursor_pos;
    ASSERT_EQ_INT(2, (long)anchor);
    ASSERT_TRUE(cursor > anchor);

    // Byte offsets are mode-independent: both survive a round trip
    // through no-wrap mode.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, false, false, 200, 50, false, false, 0.0f);
    ASSERT_EQ_INT((long)anchor, (long)state->caret.selection_anchor);
    ASSERT_EQ_INT((long)cursor, (long)state->caret.cursor_pos);
    ew_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_INT((long)anchor, (long)state->caret.selection_anchor);
    ASSERT_EQ_INT((long)cursor, (long)state->caret.cursor_pos);

    wlx_context_destroy(&ctx);
}

// Mimic the SDL_ttf backends' zero-length hazard: TTF text APIs treat a
// zero length as "NUL-terminated", so an empty-slice measure that leaks
// through returns the width of everything up to the next NUL. The wrapped
// caret x must never measure an empty prefix - on such a backend a
// column-0 caret teleports to the row's end (or past the band, where the
// gate culls it entirely).
static void ew_poison_measure(const char *text, size_t len, WLX_Text_Style ts,
                              float *w, float *h) {
    (void)text;
    int fs = ts.font_size > 0 ? ts.font_size : 10;
    if (len == 0) { if (w) *w = 9999.0f; if (h) *h = (float)fs; return; }
    // Non-empty spans keep the mock's metric (half font_size per byte).
    if (w) *w = (float)len * (float)fs * 0.5f;
    if (h) *h = (float)fs;
}

TEST(editor_wrap_caret_col0_draws_at_row_start) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_line = _ew_capture_draw_line;
    ctx.backend.measure_text_slice = ew_poison_measure;

    static char buf[512];
    size_t len = ew_fill_long_first(buf, sizeof(buf), 200, 5);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, true, 200, 50, false, false, 0.0f);

    // Focus with a click, then HOME: caret at column 0 of the wrapped line.
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, true, 40, 8, true, true, 0.0f);
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, true, 40, 8, false, false, 0.0f);
    ew_frame_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);
    WLX_Editor_State *state = ev_state(&ctx);
    ASSERT_TRUE(state != NULL);
    ASSERT_EQ_INT(0, (long)state->caret.cursor_pos);

    // The caret must draw at the row's start, not at a measured-empty-prefix
    // offset near (or past) the band's right edge.
    _ew_line_count = 0;
    ew_frame_full(&ctx, buf, sizeof(buf), &len, 0, true, true, 200, 50, false, false, 0.0f);
    ASSERT_TRUE(_ew_line_count >= 1);
    for (int i = 0; i < _ew_line_count; i++) {
        ASSERT_TRUE(_ew_lines[i].x < 60.0f);
    }
}

SUITE(editor_wrap) {
    RUN_TEST(editor_wrap_draws_band_wide_rows);
    RUN_TEST(editor_wrap_wheel_scrolls_rows_not_lines);
    RUN_TEST(editor_wrap_overflow_probe_detects_heavy_wrapping);
    RUN_TEST(editor_wrap_no_overflow_leaves_anchor_alone);
    RUN_TEST(editor_wrap_bottom_clamp_lands_exact_end);
    RUN_TEST(editor_wrap_thumb_drag_to_track_end_reaches_document_end);
    RUN_TEST(editor_wrap_toggle_carries_anchor_and_resets);
    RUN_TEST(editor_wrap_anchor_row_clamps_on_document_change);
    RUN_TEST(editor_wrap_gutter_numbers_first_rows_only);
    RUN_TEST(editor_wrap_click_and_vertical_motion_step_rows);
    RUN_TEST(editor_wrap_vertical_motion_crosses_line_boundaries);
    RUN_TEST(editor_wrap_page_keys_step_band_rows);
    RUN_TEST(editor_wrap_ctrl_end_bottom_aligns_and_home_returns);
    RUN_TEST(editor_wrap_drag_select_auto_scrolls_rows);
    RUN_TEST(editor_wrap_caret_draws_on_its_row);
    RUN_TEST(editor_wrap_typing_inserts_at_row_caret);
    RUN_TEST(editor_wrap_enter_splits_wrapped_line);
    RUN_TEST(editor_wrap_frozen_tail_caret_pins_and_appends);
    RUN_TEST(editor_wrap_select_all_replace_clamps_anchor);
    RUN_TEST(editor_wrap_toggle_preserves_caret_and_selection);
    RUN_TEST(editor_wrap_caret_col0_draws_at_row_start);
}

// test_editor_edit.c - wlx_editor editing: typing, Enter, Tab, Backspace/
// Delete with word variants, clipboard cut/copy/paste through the shared
// transport, the explicit length in/out contract (full-buffer rejection,
// UTF-8 boundary truncation, opportunistic trailing NUL), index rebuild on
// every widget edit (including same-length replaces the length guard cannot
// see), revision-guard interplay, read-only rejection, and next-tab-stop
// geometry.
//
// Reuses ev_state / ev_index / ev_fill_lines / _ev_capture* from
// test_editor_view.c and ec_command_mod from test_editor_caret.c (same
// translation unit). Frame helpers are local: state identity is the
// call-site, so a test must drive all its frames from one helper.

static bool ed_editor(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                      uint32_t revision, bool read_only) {
    bool focused = false;
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .revision = revision, .read_only = read_only,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool ed_read_only = false;

static bool ed_frame_ex(WLX_Context *ctx, char *buf, size_t cap, size_t *len, uint32_t rev,
                        int mx, int my, bool down, bool clicked,
                        WLX_Key_Code key, uint32_t mods, const char *text) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, mx, my, down, clicked, down, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = ed_editor(ctx, buf, cap, len, rev, ed_read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static bool ed_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len) {
    return ed_frame_ex(ctx, buf, cap, len, 0, 0, 0, false, false, WLX_KEY_NONE, 0, NULL);
}

static bool ed_click(WLX_Context *ctx, char *buf, size_t cap, size_t *len, int mx, int my) {
    return ed_frame_ex(ctx, buf, cap, len, 0, mx, my, true, true, WLX_KEY_NONE, 0, NULL);
}

static bool ed_key(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                   WLX_Key_Code key, uint32_t mods) {
    return ed_frame_ex(ctx, buf, cap, len, 0, 200, 50, false, false, key, mods, NULL);
}

static bool ed_type(WLX_Context *ctx, char *buf, size_t cap, size_t *len, const char *text) {
    return ed_frame_ex(ctx, buf, cap, len, 0, 200, 50, false, false, WLX_KEY_NONE, 0, text);
}

// ============================================================================
// Insert / delete classes
// ============================================================================

TEST(edit_typing_inserts_at_caret_and_rebuilds) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "abc\ndef", 8);
    size_t len = 7;
    ed_frame(&ctx, buf, sizeof(buf), &len);

    // Caret after 'a' (content x 5 -> column 1 on line 0).
    ed_click(&ctx, buf, sizeof(buf), &len, 14, 9);
    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);
    uint32_t rebuilds_before = idx->rebuilds;

    ed_type(&ctx, buf, sizeof(buf), &len, "XY");
    ASSERT_EQ_INT(9, (long)len);
    ASSERT_TRUE(memcmp(buf, "aXYbc\ndef", 9) == 0);
    ASSERT_EQ_INT(0, buf[9]); // opportunistic trailing NUL
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT((long)(rebuilds_before + 1), (long)idx->rebuilds);
    ASSERT_EQ_INT(2, (long)idx->count);
    wlx_context_destroy(&ctx);
}

TEST(edit_enter_inserts_newline_and_grows_index) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "abcd", 5);
    size_t len = 4;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 19, 9); // caret at column 2

    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(1, (long)idx->count);

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab\ncd", 5) == 0);
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(2, (long)idx->count);
    wlx_context_destroy(&ctx);
}

TEST(edit_backspace_delete_codepoint_and_word) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "alpha beta", 11);
    size_t len = 10;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 200, 9); // past the text -> line end

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(10, (long)st->caret.cursor_pos);

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(9, (long)len); // "alpha bet"

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, WLX_MOD_CTRL); // word
    ASSERT_EQ_INT(6, (long)len);
    ASSERT_TRUE(memcmp(buf, "alpha ", 6) == 0);

    // Forward deletes from the document start.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, ec_command_mod());
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DELETE, 0);
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "lpha ", 5) == 0);

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DELETE, WLX_MOD_CTRL); // word
    ASSERT_EQ_INT(1, (long)len);
    ASSERT_TRUE(buf[0] == ' ');
    wlx_context_destroy(&ctx);
}

TEST(edit_same_length_replace_still_rebuilds_index) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "ab\ncd", 6);
    size_t len = 5;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 9, 9); // caret at 0

    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);

    // Select "a", replace with "x": the length does not change, so only the
    // explicit edit trigger can rebuild the index.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    uint32_t rebuilds_before = idx->rebuilds;
    ed_type(&ctx, buf, sizeof(buf), &len, "x");
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "xb\ncd", 5) == 0);
    ASSERT_EQ_INT((long)(rebuilds_before + 1), (long)idx->rebuilds);
    wlx_context_destroy(&ctx);
}

TEST(edit_crlf_pairs_delete_bytewise_and_by_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "a\r\nb", 5);
    size_t len = 4;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 9, 19); // line 1 start = offset 3

    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(2, (long)idx->count);

    // Byte-wise backspace eats the '\n' first (the lone '\r' still
    // separates), then the '\r' merges the lines.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_EQ_INT(2, (long)idx->count);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(2, (long)len);
    ASSERT_EQ_INT(1, (long)idx->count);
    ASSERT_TRUE(memcmp(buf, "ab", 2) == 0);

    // A selection spanning the whole CRLF pair deletes it atomically.
    memcpy(buf, "a\r\nb", 5);
    len = 4;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    st->caret.selection_anchor = 1;
    st->caret.cursor_pos = 3;
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(2, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab", 2) == 0);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(edit_select_all_replace_whole_document) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 10);
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 20, 20);

    bool keys[WLX_KEY_COUNT] = {0};
    keys[WLX_KEY_A] = true;
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, WLX_KEY_A, ec_command_mod(), NULL);
    ed_type(&ctx, buf, sizeof(buf), &len, "z");

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(1, (long)len);
    ASSERT_TRUE(buf[0] == 'z');
    ASSERT_EQ_INT(1, (long)idx->count);
    (void)keys;
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Clipboard
// ============================================================================

TEST(edit_paste_multiline_block) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "ab", 3);
    size_t len = 2;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 14, 9); // caret at 1

    test_set_clipboard("one\ntwo\nthree");
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_V, ec_command_mod());

    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);
    ASSERT_EQ_INT(15, (long)len);
    ASSERT_TRUE(memcmp(buf, "aone\ntwo\nthreeb", 15) == 0);
    ASSERT_EQ_INT(14, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(3, (long)idx->count);
    wlx_context_destroy(&ctx);
}

TEST(edit_cut_across_window_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40); // 160 bytes, 41 lines
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 9, 9);

    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);

    // Select from line 2 to line 30 (far outside the 9-line window).
    st->caret.selection_anchor = 8;
    st->caret.cursor_pos = 120;
    test_set_clipboard("");
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_X, ec_command_mod());

    ASSERT_EQ_INT(112, (long)strlen(test_get_clipboard()));
    ASSERT_TRUE(memcmp(test_get_clipboard(), "l02\n", 4) == 0);
    ASSERT_EQ_INT(48, (long)len);
    ASSERT_EQ_INT(8, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(13, (long)idx->count); // 12 lines + trailing empty line
    wlx_context_destroy(&ctx);
}

TEST(edit_read_only_rejects_mutations_allows_copy) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "secret", 7);
    size_t len = 6;
    ed_read_only = true;

    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 20, 9);
    ed_type(&ctx, buf, sizeof(buf), &len, "x");
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ASSERT_EQ_INT(6, (long)len);
    ASSERT_TRUE(memcmp(buf, "secret", 6) == 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->caret.selection_anchor = 0;
    st->caret.cursor_pos = 6;
    test_set_clipboard("");
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_C, ec_command_mod());
    ASSERT_EQ_STR(test_get_clipboard(), "secret");

    ed_read_only = false;
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Length contract
// ============================================================================

TEST(edit_full_buffer_rejects_and_truncates_on_utf8_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Capacity 3, two bytes used: one byte of room.
    static char buf[3];
    memcpy(buf, "aa", 2);
    size_t len = 2;
    ed_frame(&ctx, buf, 3, &len);
    ed_click(&ctx, buf, 3, &len, 200, 9); // caret at line end

    // A two-byte codepoint cannot land whole: rejected entirely.
    ed_type(&ctx, buf, 3, &len, "\xC3\xB6");
    ASSERT_EQ_INT(2, (long)len);

    // One ASCII byte fits exactly (no room left for the trailing NUL, which
    // is opportunistic, not required).
    ed_type(&ctx, buf, 3, &len, "b");
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_TRUE(memcmp(buf, "aab", 3) == 0);

    // A full buffer rejects further input; the length is stable.
    ed_type(&ctx, buf, 3, &len, "c");
    ASSERT_EQ_INT(3, (long)len);
    wlx_context_destroy(&ctx);
}

TEST(edit_revision_guard_interplay_with_edits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "one\ntwo", 8);
    size_t len = 7;
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 1, 0, 0, false, false, WLX_KEY_NONE, 0, NULL);
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 1, 200, 50, true, true, WLX_KEY_NONE, 0, NULL);

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(1, (long)idx->rebuilds);

    // Widget edit under a constant revision rebuilds via the edit trigger.
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 1, 200, 50, false, false, WLX_KEY_NONE, 0, "x");
    ASSERT_EQ_INT(2, (long)idx->rebuilds);

    // A later external mutation with a revision bump still rebuilds.
    buf[0] = '\n';
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 2, 0, 0, false, false, WLX_KEY_NONE, 0, NULL);
    ASSERT_EQ_INT(3, (long)idx->rebuilds);
    ASSERT_EQ_INT(3, (long)idx->count);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Tabs: insertion and next-tab-stop geometry
// ============================================================================

TEST(edit_tab_prefix_measure_hits_next_stop) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // tab_advance 20 (4 columns x 5px space). Mock: 5px per byte.
    WLX_Text_Style ts = { .font_size = 10 };
    float w = 0.0f, h = 0.0f;

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_TRUE(wlx_text_measure_prefix_tabs(&ctx, "\tX", 2, 0, 1, ts, 20.0f, &w, &h));
    ASSERT_EQ_F(w, 20.0f, 0.01f);
    ASSERT_TRUE(wlx_text_measure_prefix_tabs(&ctx, "\tX", 2, 0, 2, ts, 20.0f, &w, &h));
    ASSERT_EQ_F(w, 25.0f, 0.01f);

    // "aaaaa" is 25px: the following tab advances to 40, not 45.
    ASSERT_TRUE(wlx_text_measure_prefix_tabs(&ctx, "aaaaa\tbb", 8, 0, 7, ts, 20.0f, &w, &h));
    ASSERT_EQ_F(w, 45.0f, 0.01f);
    ASSERT_TRUE(wlx_text_measure_prefix_tabs(&ctx, "aaaaa\tbb", 8, 0, 6, ts, 20.0f, &w, &h));
    ASSERT_EQ_F(w, 40.0f, 0.01f);

    // Passthrough (tab_advance 0) measures the raw backend width.
    ASSERT_TRUE(wlx_text_measure_prefix_tabs(&ctx, "\tX", 2, 0, 2, ts, 0.0f, &w, &h));
    ASSERT_EQ_F(w, 10.0f, 0.01f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(edit_tab_key_inserts_and_segments_draw_at_stops) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_text = _ev_capture_draw_text;

    char buf[64];
    memcpy(buf, "ab", 3);
    size_t len = 2;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 9, 9); // caret at 0

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_TAB, 0);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_TRUE(memcmp(buf, "\tab", 3) == 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);

    // The segment after the tab draws at the tab stop: band.x + 20 = 29.
    _ev_reset_captures();
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ASSERT_TRUE(_ev_capture_count > 0);
    ASSERT_EQ_STR(_ev_captures[0].text, "ab");
    ASSERT_EQ_F(_ev_captures[0].x, 29.0f, 0.01f);

    // UP/DOWN sticky column and hit-tests agree with the expanded x: a
    // click at the tab stop lands the caret after the tab.
    ed_click(&ctx, buf, sizeof(buf), &len, 9 + 19, 9);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

SUITE(editor_edit) {
    RUN_TEST(edit_typing_inserts_at_caret_and_rebuilds);
    RUN_TEST(edit_enter_inserts_newline_and_grows_index);
    RUN_TEST(edit_backspace_delete_codepoint_and_word);
    RUN_TEST(edit_same_length_replace_still_rebuilds_index);
    RUN_TEST(edit_crlf_pairs_delete_bytewise_and_by_selection);
    RUN_TEST(edit_select_all_replace_whole_document);
    RUN_TEST(edit_paste_multiline_block);
    RUN_TEST(edit_cut_across_window_boundary);
    RUN_TEST(edit_read_only_rejects_mutations_allows_copy);
    RUN_TEST(edit_full_buffer_rejects_and_truncates_on_utf8_boundary);
    RUN_TEST(edit_revision_guard_interplay_with_edits);
    RUN_TEST(edit_tab_prefix_measure_hits_next_stop);
    RUN_TEST(edit_tab_key_inserts_and_segments_draw_at_stops);
}

// test_editor_edit.c - wlx_editor editing: typing, Enter, Tab, Backspace/
// Delete with word variants, clipboard cut/copy/paste through the shared
// transport, the explicit length in/out contract (full-buffer rejection,
// UTF-8 boundary truncation, opportunistic trailing NUL), the line index
// patched from every widget edit's span (including same-length replaces the
// length guard cannot see) and its equivalence with a full rebuild,
// revision-guard interplay, read-only rejection, and next-tab-stop geometry.
//
// Reuses ev_state / ev_index / ev_fill_lines / _ev_capture* from
// test_editor_view.c and test_command_mod from test_mock_backend.h (same
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

TEST(edit_typing_inserts_at_caret_and_patches) {
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
    uint32_t patches_before = idx->patches;

    ed_type(&ctx, buf, sizeof(buf), &len, "XY");
    ASSERT_EQ_INT(9, (long)len);
    ASSERT_TRUE(memcmp(buf, "aXYbc\ndef", 9) == 0);
    ASSERT_EQ_INT(0, buf[9]); // opportunistic trailing NUL
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    // A widget edit patches the index from its span; the full rescan is
    // for the guard.
    ASSERT_EQ_INT((long)rebuilds_before, (long)idx->rebuilds);
    ASSERT_EQ_INT((long)(patches_before + 1), (long)idx->patches);
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
    uint32_t rebuilds_before = idx->rebuilds;
    uint32_t patches_before = idx->patches;

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_ENTER, 0);
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab\ncd", 5) == 0);
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(2, (long)idx->count);
    ASSERT_EQ_INT((long)rebuilds_before, (long)idx->rebuilds);
    ASSERT_EQ_INT((long)(patches_before + 1), (long)idx->patches);
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
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, test_command_mod());
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DELETE, 0);
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "lpha ", 5) == 0);

    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DELETE, WLX_MOD_CTRL); // word
    ASSERT_EQ_INT(1, (long)len);
    ASSERT_TRUE(buf[0] == ' ');
    wlx_context_destroy(&ctx);
}

TEST(edit_same_length_replace_still_patches_index) {
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
    // explicit edit trigger can update the index, and it patches.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    uint32_t rebuilds_before = idx->rebuilds;
    uint32_t patches_before = idx->patches;
    ed_type(&ctx, buf, sizeof(buf), &len, "x");
    ASSERT_EQ_INT(5, (long)len);
    ASSERT_TRUE(memcmp(buf, "xb\ncd", 5) == 0);
    ASSERT_EQ_INT((long)rebuilds_before, (long)idx->rebuilds);
    ASSERT_EQ_INT((long)(patches_before + 1), (long)idx->patches);
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
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, WLX_KEY_A, test_command_mod(), NULL);
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
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_V, test_command_mod());

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
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_X, test_command_mod());

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
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_C, test_command_mod());
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

    // Widget edit under a constant revision patches via the edit trigger;
    // the rescan is not run.
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 1, 200, 50, false, false, WLX_KEY_NONE, 0, "x");
    ASSERT_EQ_INT(1, (long)idx->rebuilds);
    ASSERT_EQ_INT(1, (long)idx->patches);

    // A later external mutation with a revision bump rebuilds.
    buf[0] = '\n';
    ed_frame_ex(&ctx, buf, sizeof(buf), &len, 2, 0, 0, false, false, WLX_KEY_NONE, 0, NULL);
    ASSERT_EQ_INT(2, (long)idx->rebuilds);
    ASSERT_EQ_INT(1, (long)idx->patches);
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

// ============================================================================
// Grapheme clusters (corpus macros from test_grapheme.c, same TU)
// ============================================================================

TEST(edit_cluster_keys_step_and_delete_whole) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    size_t len = 0;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 200, 9);   // focus, caret at 0
    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Typed one cluster per frame: "ab" + family + "c" = 21 bytes.
    ed_type(&ctx, buf, sizeof(buf), &len, "ab");
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FAMILY);
    ed_type(&ctx, buf, sizeof(buf), &len, "c");
    ASSERT_EQ_INT(21, (long)len);
    ASSERT_EQ_INT(21, (long)st->caret.cursor_pos);

    // Left and Right step over the family as one unit.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(20, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(20, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(21, (long)st->caret.cursor_pos);

    // Backspace removes "c", then the whole family.
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(20, (long)len);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(2, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab", 2) == 0);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);

    // Delete at the family's start removes it whole.
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FAMILY);
    ed_type(&ctx, buf, sizeof(buf), &len, "c");
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_DELETE, 0);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_TRUE(memcmp(buf, "abc", 3) == 0);

    // Shift+Left twice from the end selects "c" and the family; Backspace
    // removes the selection.
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FAMILY);      // "ab" + family + "c"
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ASSERT_EQ_INT(21, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, WLX_MOD_SHIFT);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, WLX_MOD_SHIFT);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(21, (long)st->caret.selection_anchor);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(2, (long)len);

    // A word delete takes the family as part of its word (the separator
    // class is ASCII whitespace, unchanged).
    ed_type(&ctx, buf, sizeof(buf), &len, " ");
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FAMILY);
    ASSERT_EQ_INT(21, (long)len);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, WLX_MOD_CTRL);
    ASSERT_EQ_INT(3, (long)len);
    ASSERT_TRUE(memcmp(buf, "ab ", 3) == 0);

    // A flag and a decomposed accent step and delete the same way.
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FLAG);
    ed_type(&ctx, buf, sizeof(buf), &len, GR_EACUTE);
    ASSERT_EQ_INT(14, (long)len);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(11, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(11, (long)len);
    ed_key(&ctx, buf, sizeof(buf), &len, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_INT(3, (long)len);

    // An app-set offset inside a cluster reads back at the cluster's start
    // after one frame.
    ed_type(&ctx, buf, sizeof(buf), &len, GR_FAMILY);
    st->caret.cursor_pos = 10;
    st->caret.selection_anchor = 10;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ASSERT_EQ_INT(3, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(3, (long)st->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

TEST(edit_same_frame_in_place_mutation_rebuilds_not_patches) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "abc\ndef\nghi", 12);
    size_t len = 11;
    ed_frame(&ctx, buf, sizeof(buf), &len);
    ed_click(&ctx, buf, sizeof(buf), &len, 200, 9); // focus, caret at the end of line 0

    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(idx != NULL);
    ASSERT_EQ_INT(3, (long)idx->count);
    uint32_t rebuilds_before = idx->rebuilds;
    uint32_t patches_before = idx->patches;

    // The application overwrites a separator in place between frames: same
    // length, no revision bump. The sampled probe sees it before the keys
    // run, so the widget edit on this frame takes the full rescan instead
    // of patching an index that is already wrong.
    buf[3] = 'x';
    ed_type(&ctx, buf, sizeof(buf), &len, "z");
    ASSERT_EQ_INT(12, (long)len);
    ASSERT_EQ_INT((long)(rebuilds_before + 1), (long)idx->rebuilds);
    ASSERT_EQ_INT((long)patches_before, (long)idx->patches);
    ASSERT_EQ_INT(2, (long)idx->count);
    ASSERT_EQ_INT(9, (long)idx->offsets[1]);   // "abczxdef\nghi": one separator left
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Index patch equivalence: a widget edit's span patch yields exactly the
// array a rebuild of the same bytes yields - over the pipeline corpora
// with every span and six replacements, the named separator shapes the
// patch's bounds were derived from, and a randomised separator-dense
// alphabet with CR at both document ends
// ============================================================================

static size_t ed_apply_edit(char *buf, size_t len, size_t start, size_t old_end,
                            const char *rep, size_t rep_len) {
    memmove(buf + start + rep_len, buf + old_end, len - old_end);
    memcpy(buf + start, rep, rep_len);
    return len - (old_end - start) + rep_len;
}

static void ed_print_doc(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\n') printf("\\n");
        else if (buf[i] == '\r') printf("\\r");
        else putchar(buf[i]);
    }
}

// Apply the edit, patch one index, rebuild the other; true when equal. A
// mismatch prints the document, the span and both arrays.
static bool ed_patch_matches_rebuild(WLX_Editor_Line_Index *patched, WLX_Editor_Line_Index *ref,
                                     char *buf, size_t *len, size_t start, size_t old_end,
                                     const char *rep, size_t rep_len) {
    *len = ed_apply_edit(buf, *len, start, old_end, rep, rep_len);
    size_t new_end = start + rep_len;
    if (!wlx_editor_index_patch(patched, buf, *len, start, old_end, new_end)) {
        printf("  patch refused: start=%zu old_end=%zu new_end=%zu len=%zu\n",
            start, old_end, new_end, *len);
        return false;
    }
    if (!wlx_editor_index_rebuild(ref, buf, *len)) return false;
    bool equal = patched->count == ref->count;
    for (size_t i = 0; equal && i < ref->count; i++) equal = patched->offsets[i] == ref->offsets[i];
    if (equal) return true;
    printf("  mismatch: start=%zu old_end=%zu new_end=%zu len=%zu\n  doc: ",
        start, old_end, new_end, *len);
    ed_print_doc(buf, *len);
    printf("\n  patched:");
    for (size_t i = 0; i < patched->count; i++) printf(" %zu", patched->offsets[i]);
    printf("\n  rebuilt:");
    for (size_t i = 0; i < ref->count; i++) printf(" %zu", ref->offsets[i]);
    printf("\n");
    return false;
}

static uint64_t ed_rng_state = 0x9E3779B97F4A7C15ULL;
static uint64_t ed_rng(void) {
    ed_rng_state ^= ed_rng_state << 13;
    ed_rng_state ^= ed_rng_state >> 7;
    ed_rng_state ^= ed_rng_state << 17;
    return ed_rng_state;
}

TEST(editor_index_patch_equals_rebuild) {
    static const char *reps[] = { "", "x", "\n", "\r", "\r\n", "x\r\ny\n" };
    const size_t rep_count = sizeof(reps) / sizeof(reps[0]);
    char buf[256];
    WLX_Editor_Line_Index patched = {0}, ref = {0};
    bool ok = true;

    // Corpus half: every (start, old_end) pair of every pipeline corpus with
    // every replacement, the index rebuilt fresh before each edit.
    for (size_t c = 0; ok && c < EP_CORPUS_COUNT_; c++) {
        size_t base_len = strlen(ep_corpora[c]);
        for (size_t start = 0; ok && start <= base_len; start++) {
            for (size_t old_end = start; ok && old_end <= base_len; old_end++) {
                for (size_t r = 0; ok && r < rep_count; r++) {
                    memcpy(buf, ep_corpora[c], base_len);
                    size_t len = base_len;
                    ok = wlx_editor_index_rebuild(&patched, buf, len)
                        && ed_patch_matches_rebuild(&patched, &ref, buf, &len,
                            start, old_end, reps[r], strlen(reps[r]));
                }
            }
        }
    }

    // Named cases: the separator shapes the bounds were derived from.
    static const struct { const char *doc; size_t start, old_end; const char *rep; } named[] = {
        { "abc\ndef", 0, 0, "\n" },        // Enter at offset 0
        { "abc\ndef", 2, 2, "x\r\ny\n" }, // a paste containing separators
        { "ab\r\ncd", 1, 4, "" },          // delete across a CRLF pair
        { "ab\r\ncd", 3, 3, "x" },         // split a CR from its LF
        { "ab\rx\ncd", 3, 4, "" },         // re-join the pair by deleting the byte between
        { "ab\ncd", 0, 5, "" },            // the document emptied
        { "abc", 3, 3, "\n" },             // trailing separator added
        { "abc\n", 3, 4, "" },             // trailing separator removed
        { "abc\ndef", 7, 7, "x" },         // edit at the document end
        { "abc\r", 4, 4, "\n" },           // lone CR at the end joined by an appended LF
        { "\nabc", 0, 0, "x" },            // insert at offset 0 of a document starting with LF
        { "a\r\nb", 2, 2, "\r" },          // a CR inserted between a CR and its LF
        { "a\nb", 1, 1, "\r" },            // a CR inserted before an LF forms a pair
        { "a\r\nb", 1, 2, "" },            // the CR of a pair deleted, the LF stays a separator
        { "a\r\nb", 2, 3, "" },            // the LF of a pair deleted, the CR stays a separator
    };
    for (size_t i = 0; ok && i < sizeof(named) / sizeof(named[0]); i++) {
        size_t len = strlen(named[i].doc);
        memcpy(buf, named[i].doc, len);
        ok = wlx_editor_index_rebuild(&patched, buf, len)
            && ed_patch_matches_rebuild(&patched, &ref, buf, &len, named[i].start,
                named[i].old_end, named[i].rep, strlen(named[i].rep));
        if (!ok) printf("  named case %zu\n", i);
    }

    // Randomised half: fixed seed, an alphabet dense in separators, four
    // sequential edits per document so a patch operates on a patched index.
    static const char alphabet[] = "ab\n\r\r\n\nx\r";
    const size_t alen = sizeof(alphabet) - 1;
    ed_rng_state = 0x9E3779B97F4A7C15ULL;
    for (size_t it = 0; ok && it < 50000; it++) {
        size_t len = ed_rng() % 40;
        for (size_t i = 0; i < len; i++) buf[i] = alphabet[ed_rng() % alen];
        ok = wlx_editor_index_rebuild(&patched, buf, len);
        for (int e = 0; ok && e < 4; e++) {
            size_t start = ed_rng() % (len + 1);
            size_t old_end = start + ed_rng() % (len - start + 1);
            char rep[8];
            size_t rep_len = ed_rng() % 7;
            for (size_t i = 0; i < rep_len; i++) rep[i] = alphabet[ed_rng() % alen];
            ok = ed_patch_matches_rebuild(&patched, &ref, buf, &len, start, old_end, rep, rep_len);
            if (!ok) printf("  random document %zu edit %d\n", it, e);
        }
    }

    wlx_free(patched.offsets);
    wlx_free(ref.offsets);
    ASSERT_TRUE(ok);
}

SUITE(editor_edit) {
    RUN_TEST(edit_cluster_keys_step_and_delete_whole);
    RUN_TEST(edit_typing_inserts_at_caret_and_patches);
    RUN_TEST(edit_enter_inserts_newline_and_grows_index);
    RUN_TEST(edit_backspace_delete_codepoint_and_word);
    RUN_TEST(edit_same_length_replace_still_patches_index);
    RUN_TEST(edit_crlf_pairs_delete_bytewise_and_by_selection);
    RUN_TEST(edit_select_all_replace_whole_document);
    RUN_TEST(edit_paste_multiline_block);
    RUN_TEST(edit_cut_across_window_boundary);
    RUN_TEST(edit_read_only_rejects_mutations_allows_copy);
    RUN_TEST(edit_full_buffer_rejects_and_truncates_on_utf8_boundary);
    RUN_TEST(edit_revision_guard_interplay_with_edits);
    RUN_TEST(edit_same_frame_in_place_mutation_rebuilds_not_patches);
    RUN_TEST(editor_index_patch_equals_rebuild);
    RUN_TEST(edit_tab_prefix_measure_hits_next_stop);
    RUN_TEST(edit_tab_key_inserts_and_segments_draw_at_stops);
}

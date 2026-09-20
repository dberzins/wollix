// test_editor_caret.c - wlx_editor caret, selection, and hit-testing: mouse
// caret placement in windows scrolled on both axes, multi-click (word /
// select-all), drag-select with dual-axis auto-scroll, the keyboard motion
// vocabulary (arrows, word motion, HOME/END, UP/DOWN sticky column across
// window edges, caret-coupled paging, Ctrl+Home/End, Ctrl+A), caret-follow
// on both axes, and scrollbar-strip press exclusion.
//
// Reuses the ev_* fixture from test_editor_view.c (same translation unit):
// 400x100 context, content_padding 4, border 0, font_size 10 -> band
// {9,4,383,92} before strips, line_h 10, mock char width 5.

static bool ec_frame_key_mod(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                             WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    return ev_frame_full(ctx, buf, cap, len, 0, 200, 50, false, false, 0.0f, mods, keys_pressed);
}

// ============================================================================
// Mouse caret
// ============================================================================

TEST(caret_click_places_in_scrolled_window) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    st->first_line = 10;
    st->y_frac = 0.0f;

    // Click at band-relative (11, 25): scroll 100 + 25 -> line 12; content
    // x 11 rounds to column 2 (boundaries at 10 and 15, midpoint 12.5).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 29, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(12 * 4 + 2, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(12 * 4 + 2, (long)st->caret.selection_anchor);
    // The caret line was already visible, so the view did not move.
    ASSERT_EQ_INT(10, (long)st->first_line);
    wlx_context_destroy(&ctx);
}

TEST(caret_click_places_in_horizontally_scrolled_window) {
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
    st->scroll_x = 50.0f;

    // Click 2px into the band: content x = 52 -> column 10 (midpoint 52.5).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 11, 20, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(10, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(caret_multi_click_word_then_select_all) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "hello world\nfoo bar", 20);
    size_t len = 19;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Click 1 inside "world" (content x 41 -> column 8).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 9, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(8, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(8, (long)st->caret.selection_anchor);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 9, false, false, 0.0f, 0, NULL);

    // Click 2 on the same spot selects the word.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 9, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(6, (long)st->caret.selection_anchor);
    ASSERT_EQ_INT(11, (long)st->caret.cursor_pos);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 9, false, false, 0.0f, 0, NULL);

    // Click 3 selects the whole document.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 9, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(0, (long)st->caret.selection_anchor);
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Drag-select with dual-axis auto-scroll
// ============================================================================

TEST(caret_drag_select_auto_scrolls_down) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Press on line 4, then hold below the band: the view scrolls toward
    // the pointer (overshoot capped at 60px -> 15px per 1/60s frame) while
    // the selection grows to the bottom edge line.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 50, true, true, 0.0f, 0, NULL);
    size_t anchor = st->caret.selection_anchor;
    ASSERT_EQ_INT(4 * 4 + 2, (long)anchor);

    for (int i = 0; i < 10; i++) {
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 200, true, false, 0.0f, 0, NULL);
    }
    ASSERT_TRUE(st->first_line > 0);
    ASSERT_EQ_INT((long)anchor, (long)st->caret.selection_anchor);
    ASSERT_TRUE(st->caret.cursor_pos > anchor + 40); // grew many lines down
    ASSERT_TRUE(st->caret.mouse_selecting);

    // Release ends the gesture.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 200, false, false, 0.0f, 0, NULL);
    ASSERT_FALSE(st->caret.mouse_selecting);
    wlx_context_destroy(&ctx);
}

TEST(caret_drag_select_auto_scrolls_right) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    memset(buf, 'a', 200);
    buf[200] = '\0';
    size_t len = 200;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Press near the right edge, then hold past it: scroll_x grows and the
    // selection keeps extending to the (moving) edge column.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 300, 20, true, true, 0.0f, 0, NULL);
    size_t anchor = st->caret.selection_anchor;
    ASSERT_TRUE(anchor > 0);

    for (int i = 0; i < 20; i++) {
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 399, 20, true, false, 0.0f, 0, NULL);
    }
    ASSERT_TRUE(st->scroll_x > 0.0f);
    ASSERT_TRUE(st->caret.cursor_pos > anchor);
    ASSERT_EQ_INT((long)anchor, (long)st->caret.selection_anchor);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Keyboard motion
// ============================================================================

TEST(caret_arrows_word_home_end_vocabulary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "alpha beta\ngamma delta", 23);
    size_t len = 22;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Focus with a click at the document start.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 10, 5, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_CTRL); // word
    ASSERT_EQ_INT(5, (long)st->caret.cursor_pos);

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ASSERT_EQ_INT(10, (long)st->caret.cursor_pos); // end of "alpha beta"

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(11, (long)st->caret.cursor_pos); // over the newline

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);
    ASSERT_EQ_INT(11, (long)st->caret.cursor_pos); // line 2 start (already there)

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos); // DOWN on last line -> end

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, test_command_mod());
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos); // Ctrl+HOME -> document start

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, test_command_mod());
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos); // Ctrl+END -> document end

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, WLX_MOD_SHIFT);
    ASSERT_EQ_INT(0, (long)st->caret.selection_anchor); // SHIFT extends
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

TEST(caret_up_down_sticky_column_survives_short_lines) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Line starts: 0, 11, 13, 24, 26 (widths 10, 1, 10, 1, 10 chars).
    char buf[64];
    memcpy(buf, "aaaaaaaaaa\nb\ncccccccccc\nd\neeeeeeeeee", 37);
    size_t len = 36;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Click column 8 on line 0 (content x 41).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 50, 5, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(8, (long)st->caret.cursor_pos);

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(12, (long)st->caret.cursor_pos); // short line: clamps to its end

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(13 + 8, (long)st->caret.cursor_pos); // sticky column returns

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(25, (long)st->caret.cursor_pos); // short line again

    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(26 + 8, (long)st->caret.cursor_pos); // and back to column 8
    wlx_context_destroy(&ctx);
}

TEST(caret_left_right_drop_sticky_column_only_on_change) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[64];
    memcpy(buf, "aaaaaaaaaa\nb\ncccccccccc", 23);
    size_t len = 23;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Focus and place the caret at the document start.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 10, 5, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);

    // Latch a sticky column, then press LEFT at the start: the caret cannot
    // move, so the latched column survives.
    st->caret.preferred_x = 33.0f;
    st->caret.preferred_x_valid = true;
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_LEFT, 0);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);
    ASSERT_TRUE(st->caret.preferred_x_valid);

    // RIGHT moves the caret: the horizontal change drops the column.
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT(1, (long)st->caret.cursor_pos);
    ASSERT_FALSE(st->caret.preferred_x_valid);

    // RIGHT at the document end is a no-op again: the column survives.
    st->caret.cursor_pos = len;
    st->caret.selection_anchor = len;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    st->caret.preferred_x = 33.0f;
    st->caret.preferred_x_valid = true;
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_RIGHT, 0);
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
    ASSERT_TRUE(st->caret.preferred_x_valid);
    wlx_context_destroy(&ctx);
}

TEST(caret_up_down_across_window_edges_follows) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Focus, then park the caret far below the window; the next frame's
    // caret-follow jumps the view to it (line 30: scroll = 310 - 92 = 218).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 50, true, true, 0.0f, 0, NULL);
    st->caret.cursor_pos = 30 * 4 + 1;
    st->caret.selection_anchor = st->caret.cursor_pos;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_INT(21, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.8f, 0.01f);

    // Ten UPs cross the window's top edge; follow keeps the caret line at
    // the top: line 20 -> scroll 200.
    for (int i = 0; i < 10; i++) {
        ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    }
    ASSERT_EQ_INT(20 * 4 + 1, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT(20, (long)st->first_line);
    ASSERT_EQ_F(st->y_frac, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(caret_ctrl_a_selects_all) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    size_t len = ev_fill_lines(buf, sizeof(buf), 10);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 50, true, true, 0.0f, 0, NULL);

    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_A] = true;
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, 0.0f,
        test_command_mod(), keys_pressed);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (long)st->caret.selection_anchor);
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

// Caret draw capture: every editor draw_line is a caret; remember the x
// together with the left edge of the scissor clip it was drawn under.
#define EC_CARET_LINES_CAP 8
static struct { float x; float clip_x; } ec_caret_lines[EC_CARET_LINES_CAP];
static int ec_caret_line_count = 0;
static WLX_Rect ec_last_clip;

static void ec_capture_begin_scissor(WLX_Rect r, void *user) {
    (void)user; ec_last_clip = r; }
static void ec_capture_draw_line(float x1, float y1, float x2, float y2,
                                 float thick, WLX_Color c, void *user) {
    (void)user;
    (void)y1; (void)x2; (void)y2; (void)thick; (void)c;
    if (ec_caret_line_count < EC_CARET_LINES_CAP) {
        ec_caret_lines[ec_caret_line_count].x = x1;
        ec_caret_lines[ec_caret_line_count].clip_x = ec_last_clip.x;
        ec_caret_line_count++;
    }
}

TEST(caret_column0_draws_fully_inside_band) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    ctx.backend.draw_line = ec_capture_draw_line;
    ctx.backend.begin_scissor = ec_capture_begin_scissor;

    char buf[64];
    memcpy(buf, "abc\ndef", 7);
    size_t len = 7;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    // Focus, then HOME: the caret lands on column 0, whose x is exactly the
    // band's left edge.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 30, 8, true, true, 0.0f, 0, NULL);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);

    // The caret line must keep its whole width inside the clip it draws
    // under: an edge-flush line loses half its width to the scissor, and a
    // thin-line backend loses the surviving pixel to driver edge rounding.
    ec_caret_line_count = 0;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_TRUE(ec_caret_line_count >= 1);
    for (int i = 0; i < ec_caret_line_count; i++) {
        ASSERT_TRUE(ec_caret_lines[i].x
            >= ec_caret_lines[i].clip_x + WLX_TEXT_CARET_WIDTH * 0.5f);
    }
    wlx_context_destroy(&ctx);
}

TEST(caret_ctrl_a_snaps_view_back_to_parked_caret) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Focus, caret to the document end (the view follows to the bottom).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 50, true, true, 0.0f, 0, NULL);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, test_command_mod());
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
    size_t bottom_line = st->first_line;
    ASSERT_TRUE(bottom_line > 0);

    // Wheel the view back to the top; the caret stays parked at the end.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false, 16.0f, 0, NULL);
    ASSERT_EQ_INT(0, (long)st->first_line);

    // Select-all changes only the anchor (the caret is already at the end);
    // caret-follow must still snap the view back to the caret.
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_A, test_command_mod());
    ASSERT_EQ_INT(0, (long)st->caret.selection_anchor);
    ASSERT_EQ_INT((long)len, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT((long)bottom_line, (long)st->first_line);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Caret-follow, horizontal
// ============================================================================

TEST(caret_follow_horizontal_on_end_and_home) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    memset(buf, 'a', 200);
    buf[200] = '\0';
    size_t len = 200;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 10, 10, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);
    ASSERT_EQ_F(st->scroll_x, 0.0f, 0.001f);

    // END: caret x = 1000; follow puts it at the band's right edge with the
    // caret margin (cursor width 2 + padding 2): 1000 + 4 - 383 = 621.
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ASSERT_EQ_INT(200, (long)st->caret.cursor_pos);
    ASSERT_EQ_F(st->scroll_x, 621.0f, 0.5f);

    // HOME: follow returns to the left edge.
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_HOME, 0);
    ASSERT_EQ_INT(0, (long)st->caret.cursor_pos);
    ASSERT_EQ_F(st->scroll_x, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(caret_follow_end_of_line_survives_idle_and_wheel_reaches_it) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[256];
    memset(buf, 'a', 200);
    buf[200] = '\0';
    size_t len = 200;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Focus, END: follow parks the view at 1000 + 4 - 383 = 621.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 10, 10, true, true, 0.0f, 0, NULL);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_END, 0);
    ASSERT_EQ_F(st->scroll_x, 621.0f, 0.5f);

    // Idle frames must not clamp the follow target away: the horizontal
    // limit reserves the caret margin past the longest measured line, so
    // the end-of-line caret keeps its band position instead of parking
    // just past the right edge (drawn only on follow frames, culled and
    // unreachable after).
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ASSERT_EQ_F(st->scroll_x, 621.0f, 0.5f);

    // Scroll home, then Shift+wheel right until it stops: the wheel's max
    // reach lands on the same caret-visible scroll, not 4px short of it.
    st->scroll_x = 0.0f;
    for (int i = 0; i < 40; i++)
        ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 50, false, false,
                      -2.0f, WLX_MOD_SHIFT, NULL);
    ASSERT_EQ_F(st->scroll_x, 621.0f, 0.5f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Scrollbar strip exclusion (presses never touch caret or selection)
// ============================================================================

TEST(caret_scrollbar_press_excluded) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    char buf[512];
    size_t len = ev_fill_lines(buf, sizeof(buf), 40);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);

    WLX_Editor_State *st = ev_state(&ctx);
    ASSERT_TRUE(st != NULL);

    // Place the caret, then press on the vertical scrollbar strip (track
    // right edge, below the thumb): caret and selection stay put.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 29, true, true, 0.0f, 0, NULL);
    size_t caret_before = st->caret.cursor_pos;
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 20, 29, false, false, 0.0f, 0, NULL);

    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 390, 60, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT((long)caret_before, (long)st->caret.cursor_pos);
    ASSERT_EQ_INT((long)caret_before, (long)st->caret.selection_anchor);

    // A press on the thumb starts the drag gesture, still without touching
    // the caret (thumb spans y 4..24.6 at scroll 0).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 390, 60, false, false, 0.0f, 0, NULL);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 390, 10, true, true, 0.0f, 0, NULL);
    ASSERT_TRUE(st->caret.dragging_scrollbar);
    ASSERT_EQ_INT((long)caret_before, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Grapheme clusters (corpus macros from test_grapheme.c, same TU)
// ============================================================================

TEST(caret_cluster_click_and_vertical_motion_land_on_boundaries) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);

    // Line 0: "ab" + family + "c" (21 bytes; the family is one 90 px unit
    // spanning content x 10..100, midpoint 55); line 1: "xyz".
    char buf[64];
    memcpy(buf, "ab" GR_FAMILY "c\nxyz", 25);
    size_t len = 25;
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    ev_frame(&ctx, buf, sizeof(buf), &len, 0);
    WLX_Editor_State *st = ev_state(&ctx);
    WLX_Editor_Line_Index *idx = ev_index(&ctx);
    ASSERT_TRUE(st != NULL && idx != NULL);

    // The hit test a click resolves through, swept across the family and
    // its neighbours: only 2 or 20 come back, split at the midpoint.
    WLX_Text_Style ts = { .font_size = 10 };
    for (float cx = 8.0f; cx <= 102.0f; cx += 1.0f) {
        size_t off = wlx_editor_offset_at_x(&ctx, buf, len, ts, idx->geom.env.line_h,
            idx->geom.env.tab_advance, idx, &idx->geom, 0, 1000.0f, 0.0f, cx);
        size_t want = cx < 55.0f ? 2 : 20;
        ASSERT_EQ_INT((long)want, (long)off);
    }
    // Two real clicks, one on each side of the midpoint (screen x is
    // content x + 9; each resolves elsewhere than the last, so no
    // double-click forms).
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 40, 9, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 100, 9, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(20, (long)st->caret.cursor_pos);

    // Up from the end of "xyz" (x 15) aims inside the family and lands on
    // its start; Down returns to the latched column.
    ev_frame_full(&ctx, buf, sizeof(buf), &len, 0, 200, 19, true, true, 0.0f, 0, NULL);
    ASSERT_EQ_INT(25, (long)st->caret.cursor_pos);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_UP, 0);
    ASSERT_EQ_INT(2, (long)st->caret.cursor_pos);
    ec_frame_key_mod(&ctx, buf, sizeof(buf), &len, WLX_KEY_DOWN, 0);
    ASSERT_EQ_INT(25, (long)st->caret.cursor_pos);
    wlx_context_destroy(&ctx);
}

SUITE(editor_caret) {
    RUN_TEST(caret_cluster_click_and_vertical_motion_land_on_boundaries);
    RUN_TEST(caret_click_places_in_scrolled_window);
    RUN_TEST(caret_click_places_in_horizontally_scrolled_window);
    RUN_TEST(caret_multi_click_word_then_select_all);
    RUN_TEST(caret_drag_select_auto_scrolls_down);
    RUN_TEST(caret_drag_select_auto_scrolls_right);
    RUN_TEST(caret_arrows_word_home_end_vocabulary);
    RUN_TEST(caret_up_down_sticky_column_survives_short_lines);
    RUN_TEST(caret_left_right_drop_sticky_column_only_on_change);
    RUN_TEST(caret_up_down_across_window_edges_follows);
    RUN_TEST(caret_ctrl_a_selects_all);
    RUN_TEST(caret_column0_draws_fully_inside_band);
    RUN_TEST(caret_ctrl_a_snaps_view_back_to_parked_caret);
    RUN_TEST(caret_follow_horizontal_on_end_and_home);
    RUN_TEST(caret_follow_end_of_line_survives_idle_and_wheel_reaches_it);
    RUN_TEST(caret_scrollbar_press_excluded);
}

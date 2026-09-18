// test_dropdown.c - wlx_dropdown: closed face + overlay row list.
//
// The face toggles a below-anchored overlay list on the next layer; choosing
// a row writes *selected, closes, and returns true. The list closes on
// Escape and on a press whose owner lies outside the dropdown (press-owner
// claim delta, not rect math). Arbitration is previous-frame based, so every
// flow warms candidates first and allows one settle frame after a close
// before probing what the pointer reaches.
//
// Geometry (ctx 400x300, VERT slots 30/150/30 so widgets fill their slots
// exactly): face 0..30, open list rows 30..60 / 60..90 / 90..120,
// base "outside" button 180..210, empty space 120..180 and below 210.

static const char *_dd_options[3] = { "alpha", "beta", "gamma" };
static int _dd_sel = 0;
static bool _dd_outside_clicked = false;

static WLX_Rect _dd_last_after_dropdown;

static bool _dd_frame_ex(WLX_Context *ctx, int mx, int my,
                         bool down, bool clicked, bool escape) {
    bool keys[WLX_KEY_COUNT] = {0};
    if (escape) keys[WLX_KEY_ESCAPE] = true;
    test_frame_begin_ex(ctx, mx, my, down, clicked, down, 0.0f,
                        escape ? keys : NULL, escape ? keys : NULL, NULL);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_PX(150), WLX_SLOT_PX(30)),
        .padding = 0, .gap = 0);
    bool changed = wlx_dropdown(ctx, "pick", &_dd_sel, _dd_options, 3,
                                .height = 30, .row_height = 30);
    _dd_last_after_dropdown = wlx_last_rect(ctx);
    _dd_outside_clicked |= wlx_button(ctx, "outside", .height = 30, .pos = 2);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return changed;
}

static bool _dd_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    return _dd_frame_ex(ctx, mx, my, down, clicked, false);
}

// Press + release at a point; returns whether the release frame reported a
// selection change.
static bool _dd_click(WLX_Context *ctx, int mx, int my) {
    _dd_frame(ctx, mx, my, true, true);
    return _dd_frame(ctx, mx, my, false, false);
}

static void _dd_reset(WLX_Context *ctx) {
    _dd_sel = 0;
    _dd_outside_clicked = false;
    test_ctx_init(ctx, 400, 300);
    _dd_frame(ctx, 200, 250, false, false);   // warm the candidate list
}

TEST(dropdown_open_choose_changes_and_closes) {
    WLX_Context ctx;
    _dd_reset(&ctx);

    ASSERT_TRUE(!_dd_click(&ctx, 10, 15));    // open via the face; no change yet
    ASSERT_TRUE(_dd_click(&ctx, 10, 75));     // row 1 ("beta")
    ASSERT_EQ_INT(1, _dd_sel);

    // Closed: after a settle frame the old row position reaches nothing,
    // and the base button below is clickable again.
    _dd_frame(&ctx, 10, 45, false, false);
    ASSERT_TRUE(!_dd_click(&ctx, 10, 45));
    ASSERT_EQ_INT(1, _dd_sel);
    _dd_click(&ctx, 10, 195);
    ASSERT_TRUE(_dd_outside_clicked);

    wlx_context_destroy(&ctx);
}

TEST(dropdown_reopens_and_selection_persists) {
    WLX_Context ctx;
    _dd_reset(&ctx);

    _dd_click(&ctx, 10, 15);                  // open
    ASSERT_TRUE(_dd_click(&ctx, 10, 105));    // row 2 ("gamma")
    ASSERT_EQ_INT(2, _dd_sel);

    _dd_frame(&ctx, 10, 15, false, false);    // idle frames: selection holds
    _dd_frame(&ctx, 10, 15, false, false);
    ASSERT_EQ_INT(2, _dd_sel);

    _dd_click(&ctx, 10, 15);                  // reopen
    ASSERT_TRUE(_dd_click(&ctx, 10, 45));     // row 0 ("alpha")
    ASSERT_EQ_INT(0, _dd_sel);

    wlx_context_destroy(&ctx);
}

// An outside press closes the list AND still reaches its own target: the
// same press latches the base button, whose release clicks it.
TEST(dropdown_outside_press_closes) {
    WLX_Context ctx;
    _dd_reset(&ctx);

    _dd_click(&ctx, 10, 15);                  // open
    _dd_click(&ctx, 10, 195);                 // press the base button
    ASSERT_TRUE(_dd_outside_clicked);         // press was not eaten
    _dd_frame(&ctx, 10, 45, false, false);    // settle
    ASSERT_TRUE(!_dd_click(&ctx, 10, 45));    // old row position: nothing
    ASSERT_EQ_INT(0, _dd_sel);

    // Reopen, then press empty space (owner is nobody): also closes.
    _dd_click(&ctx, 10, 15);
    _dd_click(&ctx, 200, 250);
    _dd_frame(&ctx, 10, 45, false, false);    // settle
    ASSERT_TRUE(!_dd_click(&ctx, 10, 45));
    ASSERT_EQ_INT(0, _dd_sel);

    wlx_context_destroy(&ctx);
}

// wlx_last_rect after the dropdown call reports the face even while the
// list is open (its rows are internal) - the tooltip anchor stays stable.
TEST(dropdown_last_rect_is_the_face_while_open) {
    WLX_Context ctx;
    _dd_reset(&ctx);

    _dd_click(&ctx, 10, 15);                  // open: rows built this frame
    ASSERT_TRUE(_dd_last_after_dropdown.y == 0.0f);
    ASSERT_TRUE(_dd_last_after_dropdown.h == 30.0f);

    wlx_context_destroy(&ctx);
}

TEST(dropdown_escape_closes) {
    WLX_Context ctx;
    _dd_reset(&ctx);

    _dd_click(&ctx, 10, 15);                  // open
    // Sanity: the list is really open - row 0 is the hover owner.
    _dd_frame(&ctx, 10, 45, false, false);
    ASSERT_TRUE(ctx.interaction.hot_id != 0);

    _dd_frame_ex(&ctx, 200, 250, false, false, true);   // Escape
    _dd_frame(&ctx, 10, 45, false, false);    // settle
    ASSERT_TRUE(!_dd_click(&ctx, 10, 45));    // list is gone
    ASSERT_EQ_INT(0, _dd_sel);

    wlx_context_destroy(&ctx);
}

// Two dropdowns: pressing the second one's face is an outside press for the
// first (closes it) and the same press opens the second on release.
// Geometry: A face 0..30 (list 30..120), B face 180..210 (list 210..300).
static int _dd2_sel_a = 0, _dd2_sel_b = 0;
static bool _dd2_changed_a = false, _dd2_changed_b = false;

static void _dd2_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_PX(150), WLX_SLOT_PX(30)),
        .padding = 0, .gap = 0);
    _dd2_changed_a |= wlx_dropdown(ctx, "aa", &_dd2_sel_a, _dd_options, 3,
                                   .height = 30, .row_height = 30);
    _dd2_changed_b |= wlx_dropdown(ctx, "bb", &_dd2_sel_b, _dd_options, 3,
                                   .height = 30, .row_height = 30, .pos = 2);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void _dd2_click(WLX_Context *ctx, int mx, int my) {
    _dd2_frame(ctx, mx, my, true, true);
    _dd2_frame(ctx, mx, my, false, false);
}

TEST(dropdown_opening_one_closes_the_other) {
    WLX_Context ctx;
    _dd2_sel_a = 0; _dd2_sel_b = 0;
    _dd2_changed_a = false; _dd2_changed_b = false;
    test_ctx_init(&ctx, 400, 300);
    _dd2_frame(&ctx, 200, 250, false, false);   // warm

    _dd2_click(&ctx, 10, 15);                   // open A
    _dd2_click(&ctx, 10, 195);                  // press B's face: closes A, opens B
    _dd2_frame(&ctx, 10, 45, false, false);     // settle

    _dd2_click(&ctx, 10, 255);                  // B is open: row 1 at 240..270
    ASSERT_TRUE(_dd2_changed_b);
    ASSERT_EQ_INT(1, _dd2_sel_b);

    // A closed when B's face took the press: its old row 0 reaches nothing
    // (any press there would have chosen "alpha" were A still open).
    _dd2_frame(&ctx, 10, 45, false, false);     // settle after B closed
    _dd2_click(&ctx, 10, 45);
    ASSERT_TRUE(!_dd2_changed_a);
    ASSERT_EQ_INT(0, _dd2_sel_a);

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Dropdown declared INSIDE a scroll panel - the common placement. The open
// list escapes the base panel for drawing and input. Geometry (ctx 400x300):
// panel viewport 0..150 (PX(150) slot); face at 70..100 inside it; open rows
// 100..130 / 130..160 / 160..190 - the last row lies outside the viewport.
// ---------------------------------------------------------------------------

static int _ddp_sel = 0;
static WLX_Rect _ddp_scissors[32];
static int _ddp_scissor_count = 0;
static void _ddp_rec_scissor(WLX_Rect r, void *user) {
    (void)user;
    if (_ddp_scissor_count < 32) _ddp_scissors[_ddp_scissor_count++] = r;
}

static bool _ddp_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(150), WLX_SLOT_PX(150)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 100.0f, .padding = 0);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(70), WLX_SLOT_PX(30)), .padding = 0, .gap = 0);
    wlx_widget(ctx, .pos = 0, .height = 70);
    bool changed = wlx_dropdown(ctx, "pick", &_ddp_sel, _dd_options, 3,
                                .height = 30, .row_height = 30, .pos = 1);
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return changed;
}

static bool _ddp_click(WLX_Context *ctx, int mx, int my) {
    _ddp_frame(ctx, mx, my, true, true);
    return _ddp_frame(ctx, mx, my, false, false);
}

static void _ddp_reset(WLX_Context *ctx) {
    _ddp_sel = 0;
    _ddp_scissor_count = 0;
    test_ctx_init(ctx, 400, 300);
    _ddp_frame(ctx, 200, 250, false, false);   // warm the candidate list
}

TEST(dropdown_inside_scroll_panel_rows_below_viewport_select) {
    WLX_Context ctx;
    _ddp_reset(&ctx);

    _ddp_click(&ctx, 10, 85);                 // open via the face (inside the viewport)
    _ddp_frame(&ctx, 10, 85, false, false);   // settle
    ASSERT_TRUE(_ddp_click(&ctx, 10, 145));   // row 2 (130..160): inside the viewport
    ASSERT_TRUE(_ddp_sel == 1);

    _ddp_click(&ctx, 10, 85);                 // reopen
    _ddp_frame(&ctx, 10, 85, false, false);
    ASSERT_TRUE(_ddp_click(&ctx, 10, 175));   // row 3 (160..190): OUTSIDE the base viewport
    ASSERT_TRUE(_ddp_sel == 2);

    wlx_context_destroy(&ctx);
}

// The open list's own scroll panel scissor is the list rect, not the list
// rect intersected with the base panel viewport.
TEST(dropdown_inside_scroll_panel_inner_scissor_is_the_list_rect) {
    WLX_Context ctx;
    _ddp_reset(&ctx);
    ctx.backend.begin_scissor = _ddp_rec_scissor;

    _ddp_click(&ctx, 10, 85);                 // open
    _ddp_scissor_count = 0;
    _ddp_frame(&ctx, 10, 85, false, false);   // settle: list built and replayed

    // List rect: x 0, y 100, w 400, h 90. Its scissor must be recorded
    // unclipped (h 90), not cut at the viewport bottom (h 50).
    bool full = false, cut = false;
    for (int i = 0; i < _ddp_scissor_count; i++) {
        WLX_Rect r = _ddp_scissors[i];
        if (r.y == 100.0f && r.h == 90.0f) full = true;
        if (r.y == 100.0f && r.h == 50.0f) cut = true;
    }
    ASSERT_TRUE(full);
    ASSERT_TRUE(!cut);

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// The dropdown face is the button face: with the same options in the same
// slot, a closed dropdown and a wlx_button record the same draw stream
// (box, border, text) - hovered and plain - so a face feature can only land
// once.
// ---------------------------------------------------------------------------

static Test_Stream _fe_record(WLX_Context *ctx, bool dropdown, int mx, int my) {
    static const char *opts[2] = { "alpha", "beta" };
    static int sel = -1;                       // -1: the label shows on the face
    test_frame_begin(ctx, mx, my, false, false);
    test_stream_reset();
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    if (dropdown) {
        (void)wlx_dropdown(ctx, "pick", &sel, opts, 2, .height = 40,
            .font_size = 20, .border_width = 2, .roundness = 0,
            .content_padding = 6, .hover_brightness = 0.3f);
    } else {
        (void)wlx_button(ctx, "pick", .height = 40,
            .font_size = 20, .border_width = 2, .roundness = 0,
            .content_padding = 6, .hover_brightness = 0.3f);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return test_stream_take();
}

TEST(dropdown_face_draws_like_a_button) {
    WLX_Context a, b;
    test_ctx_init(&a, 400, 300); test_stream_install(&a);
    test_ctx_init(&b, 400, 300); test_stream_install(&b);

    // Plain (pointer away), two frames so candidates exist for hover.
    _fe_record(&a, true, 300, 250);  _fe_record(&b, false, 300, 250);
    Test_Stream da = _fe_record(&a, true, 300, 250);
    Test_Stream db = _fe_record(&b, false, 300, 250);
    ASSERT_TRUE(da.count >= 2);                  // box + text at least
    ASSERT_TRUE(test_stream_equal(&da, &db));

    // Hovered: the tint path runs in both.
    _fe_record(&a, true, 20, 20);  _fe_record(&b, false, 20, 20);
    Test_Stream ha = _fe_record(&a, true, 20, 20);
    Test_Stream hb = _fe_record(&b, false, 20, 20);
    ASSERT_TRUE(test_stream_equal(&ha, &hb));
    ASSERT_TRUE(!test_stream_equal(&da, &ha));   // hover did change the fill

    wlx_context_destroy(&a);
    wlx_context_destroy(&b);
}

SUITE(dropdown) {
    RUN_TEST(dropdown_open_choose_changes_and_closes);
    RUN_TEST(dropdown_reopens_and_selection_persists);
    RUN_TEST(dropdown_outside_press_closes);
    RUN_TEST(dropdown_escape_closes);
    RUN_TEST(dropdown_last_rect_is_the_face_while_open);
    RUN_TEST(dropdown_opening_one_closes_the_other);
    RUN_TEST(dropdown_inside_scroll_panel_rows_below_viewport_select);
    RUN_TEST(dropdown_inside_scroll_panel_inner_scissor_is_the_list_rect);
    RUN_TEST(dropdown_face_draws_like_a_button);
}

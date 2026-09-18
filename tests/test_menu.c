// test_menu.c - wlx_menu_begin / wlx_menu_item / wlx_menu_end.
//
// A point-anchored overlay list of click items; opening is caller-owned
// (*open), closing happens at wlx_menu_end on item click, Escape, or an
// outside press (press-owner claim delta). One nested wlx_menu_begin
// opens a submenu on the next layer, beating parent items under the
// pointer. Chrome height follows the previous frame's item count.
//
// Geometry (ctx 400x300): base button 0..30; menu at (50,50) w=120 rows=30
// -> "copy" 50..80, "paste" 80..110, "more" (keep_open submenu trigger)
// 110..140; submenu at (100,60) -> "sub one" 60..90 x 100..220,
// overlapping "copy"/"paste" in x >= 100.

static bool _mn_open = false, _mn_sub_open = false;
static bool _mn_copy = false, _mn_paste = false, _mn_sub_one = false;
static bool _mn_base_clicked = false;

static WLX_Rect _mn_rects[32];
static int _mn_rect_count = 0;
static void _mn_rec_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user;
    (void)c;
    if (_mn_rect_count < 32) _mn_rects[_mn_rect_count++] = r;
}

static void _mn_frame_ex(WLX_Context *ctx, int mx, int my,
                         bool down, bool clicked, bool escape) {
    bool keys[WLX_KEY_COUNT] = {0};
    if (escape) keys[WLX_KEY_ESCAPE] = true;
    test_frame_begin_ex(ctx, mx, my, down, clicked, down, 0.0f,
                        escape ? keys : NULL, escape ? keys : NULL, NULL);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(30)), .padding = 0);
    _mn_base_clicked |= wlx_button(ctx, "base", .height = 30);
    if (wlx_menu_begin(ctx, &_mn_open, 50, 50, .width = 120, .row_height = 30)) {
        _mn_copy  |= wlx_menu_item(ctx, "copy");
        _mn_paste |= wlx_menu_item(ctx, "paste");
        if (wlx_menu_item(ctx, "more", .keep_open = true)) {
            _mn_sub_open = !_mn_sub_open;
        }
        if (wlx_menu_begin(ctx, &_mn_sub_open, 100, 60,
                           .width = 120, .row_height = 30)) {
            _mn_sub_one |= wlx_menu_item(ctx, "sub one");
            wlx_menu_end(ctx);
        }
        wlx_menu_end(ctx);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void _mn_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    _mn_frame_ex(ctx, mx, my, down, clicked, false);
}

static void _mn_click(WLX_Context *ctx, int mx, int my) {
    _mn_frame(ctx, mx, my, true, true);
    _mn_frame(ctx, mx, my, false, false);
}

// Reset flags and warm two frames (bootstrap + candidate build).
static void _mn_reset(WLX_Context *ctx, bool open, bool sub_open) {
    _mn_open = open; _mn_sub_open = sub_open;
    _mn_copy = false; _mn_paste = false; _mn_sub_one = false;
    _mn_base_clicked = false;
    _mn_rect_count = 0;
    test_ctx_init(ctx, 400, 300);
    _mn_frame(ctx, 300, 200, false, false);
    _mn_frame(ctx, 300, 200, false, false);
}

TEST(menu_item_fires_and_closes) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, false);

    _mn_click(&ctx, 60, 95);                  // "paste" row
    ASSERT_TRUE(_mn_paste);
    ASSERT_TRUE(!_mn_copy);
    ASSERT_TRUE(!_mn_open);                   // item click closed the menu

    _mn_frame(&ctx, 60, 95, false, false);    // settle
    _mn_click(&ctx, 60, 95);                  // menu gone: nothing fires
    ASSERT_TRUE(!_mn_copy && !_mn_open);

    wlx_context_destroy(&ctx);
}

// An outside press closes the menu AND still reaches its own target.
TEST(menu_outside_press_closes) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, false);

    _mn_click(&ctx, 10, 15);                  // press the base button
    ASSERT_TRUE(!_mn_open);
    ASSERT_TRUE(_mn_base_clicked);            // press was not eaten
    ASSERT_TRUE(!_mn_copy && !_mn_paste);

    wlx_context_destroy(&ctx);
}

TEST(menu_escape_closes) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, false);

    _mn_frame_ex(&ctx, 300, 200, false, false, true);
    ASSERT_TRUE(!_mn_open);

    wlx_context_destroy(&ctx);
}

// A submenu draws and arbitrates on the next layer: its item under the
// pointer beats the parent item beneath, and choosing it dismisses the
// whole menu chain (a leaf activation closes every ancestor too).
TEST(menu_submenu_wins_over_parent_and_closes_the_chain) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, true);

    _mn_click(&ctx, 110, 70);                 // "sub one" over "copy"
    ASSERT_TRUE(_mn_sub_one);
    ASSERT_TRUE(!_mn_copy);
    ASSERT_TRUE(!_mn_sub_open);               // submenu closed by its item
    ASSERT_TRUE(!_mn_open);                   // ...and the parent with it

    wlx_context_destroy(&ctx);
}

// A keep_open item (the submenu trigger) leaves its menu open when
// clicked: the submenu it summoned survives past the click frame and its
// items are reachable afterwards.
TEST(menu_keep_open_trigger_leaves_parent_open) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, false);

    _mn_click(&ctx, 60, 125);                 // click "more"
    ASSERT_TRUE(_mn_open);                    // parent did NOT close
    ASSERT_TRUE(_mn_sub_open);                // trigger opened the submenu

    _mn_frame(&ctx, 110, 70, false, false);   // settle: submenu candidates live
    _mn_click(&ctx, 110, 70);                 // submenu leaf: fires and...
    ASSERT_TRUE(_mn_sub_one);
    ASSERT_TRUE(!_mn_open);                   // ...dismisses the whole chain

    wlx_context_destroy(&ctx);
}

// Chrome height adapts to the item count recorded last frame.
TEST(menu_chrome_follows_item_count) {
    WLX_Context ctx;
    _mn_reset(&ctx, true, false);
    ctx.backend.draw_rect = _mn_rec_rect;

    _mn_rect_count = 0;
    _mn_frame(&ctx, 300, 200, false, false);  // two items known by now

    bool found = false;
    for (int i = 0; i < _mn_rect_count; i++) {
        WLX_Rect r = _mn_rects[i];
        if (r.x == 50.0f && r.y == 50.0f && r.w == 120.0f && r.h == 90.0f) {
            found = true;
        }
    }
    ASSERT_TRUE(found);                       // panel chrome spans all three rows

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// wlx_menu_button_begin: a face button that toggles the menu and anchors
// the list below itself. The face sits inside the menu's press-claim
// window, so pressing it while open closes cleanly on release instead of
// fighting an outside-close on the press (the close-reopen flicker).
//
// Geometry (ctx 400x300, VERT slots 30/200/30): face 0..30 full width,
// list rows 30..60 / 60..90, base button 230..260, empty space around.
// ---------------------------------------------------------------------------

static bool _mb_open = false;
static bool _mb_a = false, _mb_b = false;
static bool _mb_base_clicked = false;
static WLX_Rect _mb_last_after_block;

static void _mb_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(30), WLX_SLOT_PX(200), WLX_SLOT_PX(30)),
        .padding = 0, .gap = 0);
    if (wlx_menu_button_begin(ctx, "open", &_mb_open,
            .height = 30, .row_height = 30)) {
        _mb_a |= wlx_menu_item(ctx, "item a");
        _mb_b |= wlx_menu_item(ctx, "item b");
        wlx_menu_end(ctx);
    }
    _mb_last_after_block = wlx_last_rect(ctx);
    _mb_base_clicked |= wlx_button(ctx, "base", .height = 30, .pos = 2);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void _mb_click(WLX_Context *ctx, int mx, int my) {
    _mb_frame(ctx, mx, my, true, true);
    _mb_frame(ctx, mx, my, false, false);
}

static void _mb_reset(WLX_Context *ctx) {
    _mb_open = false;
    _mb_a = false; _mb_b = false;
    _mb_base_clicked = false;
    _mn_rect_count = 0;
    test_ctx_init(ctx, 400, 300);
    _mb_frame(ctx, 200, 150, false, false);
    _mb_frame(ctx, 200, 150, false, false);
}

// The reported bug pin: the second click on the face closes the menu and
// does NOT reopen it.
TEST(menu_button_toggles_open_and_closed) {
    WLX_Context ctx;
    _mb_reset(&ctx);

    _mb_click(&ctx, 10, 15);                  // first click: opens
    ASSERT_TRUE(_mb_open);

    _mb_frame(&ctx, 10, 15, false, false);    // settle: list candidates live
    _mb_click(&ctx, 10, 15);                  // second click: closes
    ASSERT_TRUE(!_mb_open);

    _mb_frame(&ctx, 10, 45, false, false);    // settle
    ASSERT_TRUE(!_mb_open);                   // ...and stays closed
    _mb_click(&ctx, 10, 45);                  // old item position: nothing
    ASSERT_TRUE(!_mb_a);
    ASSERT_TRUE(!_mb_open);

    wlx_context_destroy(&ctx);
}

// A menu button is one widget: after the block, wlx_last_rect reports the
// face (y 0, h 30), not the last item row (y 60..90) - the dropdown rule.
TEST(menu_button_last_rect_is_the_face_while_open) {
    WLX_Context ctx;
    _mb_reset(&ctx);

    _mb_click(&ctx, 10, 15);                  // open: items built this frame
    ASSERT_TRUE(_mb_open);
    _mb_frame(&ctx, 10, 15, false, false);    // settle with the list open
    ASSERT_TRUE(_mb_last_after_block.y == 0.0f);
    ASSERT_TRUE(_mb_last_after_block.h == 30.0f);

    wlx_context_destroy(&ctx);
}

TEST(menu_button_item_fires_and_closes) {
    WLX_Context ctx;
    _mb_reset(&ctx);

    _mb_click(&ctx, 10, 15);                  // open
    _mb_frame(&ctx, 10, 45, false, false);    // settle
    _mb_click(&ctx, 10, 45);                  // "item a"
    ASSERT_TRUE(_mb_a);
    ASSERT_TRUE(!_mb_open);

    wlx_context_destroy(&ctx);
}

TEST(menu_button_outside_press_closes) {
    WLX_Context ctx;
    _mb_reset(&ctx);

    _mb_click(&ctx, 10, 15);                  // open
    _mb_frame(&ctx, 10, 15, false, false);    // settle
    _mb_click(&ctx, 10, 245);                 // press the base button
    ASSERT_TRUE(!_mb_open);
    ASSERT_TRUE(_mb_base_clicked);            // press was not eaten

    wlx_context_destroy(&ctx);
}

// The list hangs from the face's bottom edge at face width; its chrome
// spans the item count recorded last frame.
TEST(menu_button_anchors_below_face) {
    WLX_Context ctx;
    _mb_reset(&ctx);
    ctx.backend.draw_rect = _mn_rec_rect;

    _mb_click(&ctx, 10, 15);                  // open
    _mb_frame(&ctx, 10, 15, false, false);    // item count now 2
    _mn_rect_count = 0;
    _mb_frame(&ctx, 10, 15, false, false);

    bool found = false;
    for (int i = 0; i < _mn_rect_count; i++) {
        WLX_Rect r = _mn_rects[i];
        if (r.x == 0.0f && r.y == 30.0f && r.w == 400.0f && r.h == 60.0f) {
            found = true;
        }
    }
    ASSERT_TRUE(found);

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// wlx_submenu_begin: no coordinates - the list anchors flush to the parent
// panel's right edge at the trigger item's row, inherits the parent's
// styling, and shares the parent's press scope so the trigger toggles it
// without the close-on-press / reopen-on-release flicker.
//
// Geometry: parent menu at (50,50) w=120 rows=30 -> "one" 50..80, trigger
// "more" 80..110; submenu (inherits w=120, rh=30) at (170,80) ->
// "rename" 80..110, "delete" 110..140, all in x 170..290.
// ---------------------------------------------------------------------------

static bool _sb_open = false, _sb_sub = false;
static bool _sb_one = false, _sb_rename = false;

static void _sb_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(30)),
        .padding = 0, .gap = 0);
    wlx_widget(ctx, .height = 30);
    if (wlx_menu_begin(ctx, &_sb_open, 50, 50, .width = 120, .row_height = 30)) {
        _sb_one |= wlx_menu_item(ctx, "one");
        if (wlx_menu_item(ctx, "more", .keep_open = true)) {
            _sb_sub = !_sb_sub;
        }
        if (wlx_submenu_begin(ctx, &_sb_sub)) {
            _sb_rename |= wlx_menu_item(ctx, "rename");
            (void)wlx_menu_item(ctx, "delete");
            wlx_menu_end(ctx);
        }
        wlx_menu_end(ctx);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void _sb_click(WLX_Context *ctx, int mx, int my) {
    _sb_frame(ctx, mx, my, true, true);
    _sb_frame(ctx, mx, my, false, false);
}

static void _sb_reset(WLX_Context *ctx) {
    _sb_open = true; _sb_sub = false;
    _sb_one = false; _sb_rename = false;
    _mn_rect_count = 0;
    test_ctx_init(ctx, 400, 300);
    _sb_frame(ctx, 300, 250, false, false);
    _sb_frame(ctx, 300, 250, false, false);
}

TEST(submenu_anchors_beside_trigger_row) {
    WLX_Context ctx;
    _sb_reset(&ctx);
    ctx.backend.draw_rect = _mn_rec_rect;

    _sb_click(&ctx, 60, 95);                  // click "more": submenu opens
    ASSERT_TRUE(_sb_sub);
    _sb_frame(&ctx, 60, 95, false, false);    // item count now 2
    _mn_rect_count = 0;
    _sb_frame(&ctx, 60, 95, false, false);

    // Chrome flush to the parent's right edge (50+120) at the trigger row
    // (50 + 1*30), two inherited 30px rows tall.
    bool found = false;
    for (int i = 0; i < _mn_rect_count; i++) {
        WLX_Rect r = _mn_rects[i];
        if (r.x == 170.0f && r.y == 80.0f && r.w == 120.0f && r.h == 60.0f) {
            found = true;
        }
    }
    ASSERT_TRUE(found);

    wlx_context_destroy(&ctx);
}

// The bug pin: clicking the trigger of an OPEN submenu closes it cleanly -
// no outside-close on the press followed by a reopening toggle on release.
TEST(submenu_trigger_toggles_without_flicker) {
    WLX_Context ctx;
    _sb_reset(&ctx);

    _sb_click(&ctx, 60, 95);                  // open the submenu
    ASSERT_TRUE(_sb_sub);
    _sb_frame(&ctx, 60, 95, false, false);    // settle

    _sb_click(&ctx, 60, 95);                  // click "more" again
    ASSERT_TRUE(!_sb_sub);                    // closed...
    ASSERT_TRUE(_sb_open);                    // ...parent untouched by the toggle
    _sb_frame(&ctx, 200, 95, false, false);   // settle
    ASSERT_TRUE(!_sb_sub);                    // ...and stays closed
    // The old "rename" position is empty space now: nothing fires there
    // (that empty-space press closes the parent too - by design).
    _sb_click(&ctx, 200, 95);
    ASSERT_TRUE(!_sb_rename);

    wlx_context_destroy(&ctx);
}

TEST(submenu_item_click_closes_the_whole_chain) {
    WLX_Context ctx;
    _sb_reset(&ctx);

    _sb_click(&ctx, 60, 95);                  // open the submenu
    _sb_frame(&ctx, 200, 95, false, false);   // settle
    _sb_click(&ctx, 200, 95);                 // "rename"
    ASSERT_TRUE(_sb_rename);
    ASSERT_TRUE(!_sb_sub);                    // submenu closed by its item
    ASSERT_TRUE(!_sb_open);                   // ...and its parent with it

    wlx_context_destroy(&ctx);
}

// A parent leaf declared BEFORE the submenu closes the parent; the submenu's
// caller-owned flag must not survive (the core clears it - consumers no
// longer need `if (!open) sub = false;`), and a parent that re-opens with a
// stale submenu flag shows no stale submenu.
TEST(submenu_flag_clears_when_parent_item_closes_chain) {
    WLX_Context ctx;
    _sb_reset(&ctx);

    _sb_click(&ctx, 60, 95);                  // open the submenu via "more"
    ASSERT_TRUE(_sb_sub);
    _sb_frame(&ctx, 60, 65, false, false);    // settle over "one"
    _sb_click(&ctx, 60, 65);                  // "one": a leaf before the submenu
    ASSERT_TRUE(_sb_one);
    ASSERT_TRUE(!_sb_open);                   // parent closed by its item...
    ASSERT_TRUE(!_sb_sub);                    // ...and the submenu flag with it

    // Stale flag on reopen: the parent's first frame clears it. (One closed
    // frame first - that is what every real close/reopen sequence has.)
    _sb_frame(&ctx, 300, 250, false, false);
    _sb_open = true; _sb_sub = true;
    _sb_frame(&ctx, 300, 250, false, false);
    ASSERT_TRUE(_sb_open);
    ASSERT_TRUE(!_sb_sub);

    wlx_context_destroy(&ctx);
}

// A menu button declared INSIDE a scroll panel: items below the base
// viewport still take the press (the list escapes the panel for input).
// Geometry: panel viewport 0..150; face 70..100; items 100..130 / 130..160
// / 160..190 (the last outside the viewport).
static bool _mbp_open = false, _mbp_c = false;

static void _mbp_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(150), WLX_SLOT_PX(150)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 100.0f, .padding = 0);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(70), WLX_SLOT_PX(30)), .padding = 0, .gap = 0);
    wlx_widget(ctx, .pos = 0, .height = 70);
    if (wlx_menu_button_begin(ctx, "open", &_mbp_open,
            .height = 30, .row_height = 30, .pos = 1)) {
        (void)wlx_menu_item(ctx, "a");
        (void)wlx_menu_item(ctx, "b");
        _mbp_c |= wlx_menu_item(ctx, "c");
        wlx_menu_end(ctx);
    }
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(menu_inside_scroll_panel_item_below_viewport_clicks) {
    WLX_Context ctx;
    _mbp_open = false; _mbp_c = false;
    test_ctx_init(&ctx, 400, 300);
    _mbp_frame(&ctx, 200, 250, false, false);   // warm
    _mbp_frame(&ctx, 200, 250, false, false);

    _mbp_frame(&ctx, 10, 85, true, true);        // open via the face
    _mbp_frame(&ctx, 10, 85, false, false);
    ASSERT_TRUE(_mbp_open);
    _mbp_frame(&ctx, 10, 175, false, false);     // settle over "c" (outside the viewport)
    _mbp_frame(&ctx, 10, 175, true, true);
    _mbp_frame(&ctx, 10, 175, false, false);
    ASSERT_TRUE(_mbp_c);
    ASSERT_TRUE(!_mbp_open);                     // the leaf closed the menu

    wlx_context_destroy(&ctx);
}

// The menu-button face is the button face: same options, same slot, same
// draw stream as a wlx_button (closed menu), plain and hovered.
static Test_Stream _mfe_record(WLX_Context *ctx, bool menu_button, int mx, int my) {
    static bool open = false;
    test_frame_begin(ctx, mx, my, false, false);
    test_stream_reset();
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    if (menu_button) {
        if (wlx_menu_button_begin(ctx, "open", &open, .height = 40,
                .font_size = 20, .border_width = 2, .roundness = 0,
                .content_padding = 6, .hover_brightness = 0.3f)) {
            wlx_menu_end(ctx);
        }
    } else {
        (void)wlx_button(ctx, "open", .height = 40,
            .font_size = 20, .border_width = 2, .roundness = 0,
            .content_padding = 6, .hover_brightness = 0.3f);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return test_stream_take();
}

TEST(menu_button_face_draws_like_a_button) {
    WLX_Context a, b;
    test_ctx_init(&a, 400, 300); test_stream_install(&a);
    test_ctx_init(&b, 400, 300); test_stream_install(&b);

    _mfe_record(&a, true, 300, 250);  _mfe_record(&b, false, 300, 250);
    Test_Stream da = _mfe_record(&a, true, 300, 250);
    Test_Stream db = _mfe_record(&b, false, 300, 250);
    ASSERT_TRUE(da.count >= 2);
    ASSERT_TRUE(test_stream_equal(&da, &db));

    _mfe_record(&a, true, 20, 20);  _mfe_record(&b, false, 20, 20);
    Test_Stream ha = _mfe_record(&a, true, 20, 20);
    Test_Stream hb = _mfe_record(&b, false, 20, 20);
    ASSERT_TRUE(test_stream_equal(&ha, &hb));
    ASSERT_TRUE(!test_stream_equal(&da, &ha));

    wlx_context_destroy(&a);
    wlx_context_destroy(&b);
}

// The menu stack is heap-backed: NULL until a menu opens, allocated once
// (the pointer never changes - frame pointers stay valid), and released by
// wlx_context_destroy (leak-checked under ASan).
TEST(menu_stack_allocated_once_and_freed) {
    WLX_Context ctx;
    _mn_reset(&ctx, false, false);            // two warm frames, menu closed
    ASSERT_TRUE(ctx.menu_stack == NULL);      // no menu body yet

    _mn_open = true;
    _mn_frame(&ctx, 300, 200, false, false);  // menu body built: stack allocated
    WLX_Menu_Frame *stack = ctx.menu_stack;
    ASSERT_TRUE(stack != NULL);
    ASSERT_TRUE(ctx.menu_stack_count == 0);   // balanced after the frame

    _mn_click(&ctx, 60, 125);                 // "more": submenu opens (nesting)
    _mn_frame(&ctx, 60, 125, false, false);
    ASSERT_TRUE(_mn_sub_open);
    ASSERT_TRUE(ctx.menu_stack == stack);     // same block, no reallocation

    _mn_open = false; _mn_sub_open = false;
    _mn_frame(&ctx, 300, 200, false, false);  // closed again: block retained
    ASSERT_TRUE(ctx.menu_stack == stack);

    wlx_context_destroy(&ctx);               // frees it (ASan leak check)
    ASSERT_TRUE(ctx.menu_stack == NULL);
}

SUITE(menu) {
    RUN_TEST(menu_item_fires_and_closes);
    RUN_TEST(menu_outside_press_closes);
    RUN_TEST(menu_escape_closes);
    RUN_TEST(menu_submenu_wins_over_parent_and_closes_the_chain);
    RUN_TEST(menu_keep_open_trigger_leaves_parent_open);
    RUN_TEST(menu_chrome_follows_item_count);
    RUN_TEST(menu_button_toggles_open_and_closed);
    RUN_TEST(menu_button_last_rect_is_the_face_while_open);
    RUN_TEST(menu_button_item_fires_and_closes);
    RUN_TEST(menu_button_outside_press_closes);
    RUN_TEST(menu_button_anchors_below_face);
    RUN_TEST(submenu_anchors_beside_trigger_row);
    RUN_TEST(submenu_trigger_toggles_without_flicker);
    RUN_TEST(submenu_item_click_closes_the_whole_chain);
    RUN_TEST(submenu_flag_clears_when_parent_item_closes_chain);
    RUN_TEST(menu_inside_scroll_panel_item_below_viewport_clicks);
    RUN_TEST(menu_button_face_draws_like_a_button);
    RUN_TEST(menu_stack_allocated_once_and_freed);
}

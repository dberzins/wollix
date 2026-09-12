// test_overlay.c - layered command replay.
//
// Ranges carry the layer active at open; replay dispatches layers in
// ascending order (one pass per layer) so higher layers draw over
// everything below, and each pass leaves no scissor open. Frames with
// every range on layer 0 take the flat fast path, pinned by the existing
// replay suites staying unchanged.

static WLX_Rect _ov_rects[16];
static int _ov_rect_count = 0;
static int _ov_scissor_open = 0;
static int _ov_scissor_begins = 0;
static int _ov_scissor_ends = 0;
static int _ov_rects_inside_scissor = 0;

static void _ov_rec_rect(WLX_Rect r, WLX_Color c) {
    (void)c;
    if (_ov_rect_count < 16) _ov_rects[_ov_rect_count++] = r;
    if (_ov_scissor_open > 0) _ov_rects_inside_scissor++;
}

static void _ov_rec_scissor_begin(WLX_Rect r) {
    (void)r;
    _ov_scissor_begins++;
    _ov_scissor_open++;
}

static void _ov_rec_scissor_end(void) {
    _ov_scissor_ends++;
    _ov_scissor_open--;
}

static void _ov_reset(void) {
    _ov_rect_count = 0;
    _ov_scissor_open = 0;
    _ov_scissor_begins = 0;
    _ov_scissor_ends = 0;
    _ov_rects_inside_scissor = 0;
}

// A widget recorded while current_layer == 1 must replay after every
// base-layer command, regardless of recording order.
TEST(layered_replay_dispatches_ascending) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 3, WLX_VERT);
    wlx_widget(&ctx, .height = 10, .back_color = fill);   // base
    ctx.current_layer = 1;                                // overlay scope stand-in
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .height = 20, .back_color = fill);   // layer 1
    wlx_layout_end(&ctx);
    ctx.current_layer = 0;
    wlx_widget(&ctx, .height = 30, .back_color = fill);   // base again
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(3, _ov_rect_count);
    ASSERT_TRUE(_ov_rects[0].h == 10.0f);   // base pass, recording order
    ASSERT_TRUE(_ov_rects[1].h == 30.0f);
    ASSERT_TRUE(_ov_rects[2].h == 20.0f);   // layer-1 pass replays last

    wlx_context_destroy(&ctx);
}

// A clip inside a layer-1 subtree scissors exactly that layer's content:
// balanced within the pass, never cropping the base layer's commands.
TEST(layered_replay_keeps_scissors_within_their_layer) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    ctx.backend.begin_scissor = _ov_rec_scissor_begin;
    ctx.backend.end_scissor = _ov_rec_scissor_end;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 3, WLX_VERT);
    wlx_widget(&ctx, .height = 10, .back_color = fill);   // base, unclipped
    ctx.current_layer = 1;
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true);
    wlx_widget(&ctx, .height = 20, .back_color = fill);   // layer 1, clipped
    wlx_layout_end(&ctx);
    ctx.current_layer = 0;
    wlx_widget(&ctx, .height = 30, .back_color = fill);   // base, unclipped
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(3, _ov_rect_count);
    ASSERT_TRUE(_ov_scissor_begins >= 1);
    ASSERT_EQ_INT(_ov_scissor_begins, _ov_scissor_ends);  // balanced
    ASSERT_EQ_INT(0, _ov_scissor_open);                   // nothing dangling
    ASSERT_EQ_INT(1, _ov_rects_inside_scissor);           // only the layer-1 rect

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Press/hover arbitration: the previous frame's topmost candidate under the
// pointer (highest layer, then latest query) owns the press and the hover.
// ---------------------------------------------------------------------------

static const WLX_Rect ab_a_rect = { 0, 0, 100, 40 };     // base, earlier
static const WLX_Rect ab_b_rect = { 50, 0, 100, 40 };    // base, later, overlaps A
static const WLX_Rect ab_x_rect = { 0, 100, 100, 40 };   // appears mid-test

#define AB_A(ctx) wlx_get_interaction((ctx), ab_a_rect, \
    WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, "ab_a", 1)
#define AB_B(ctx) wlx_get_interaction((ctx), ab_b_rect, \
    WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, "ab_b", 1)
#define AB_X(ctx) wlx_get_interaction((ctx), ab_x_rect, \
    WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, "ab_x", 1)

// The review probe, inverted: with two overlapping CLICK widgets, the later
// (visually topmost) one receives the press and the click.
TEST(arbitration_topmost_click_wins) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 75, 20, false, false);   // warm the candidates
    AB_A(&ctx); AB_B(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 75, 20, true, true);     // press in the overlap
    WLX_Interaction a = AB_A(&ctx);
    WLX_Interaction b = AB_B(&ctx);
    ASSERT_TRUE(!a.active);
    ASSERT_TRUE(b.active && b.pressed);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 75, 20, false, false);   // release
    a = AB_A(&ctx);
    b = AB_B(&ctx);
    ASSERT_TRUE(!a.clicked);
    ASSERT_TRUE(b.clicked);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// The review probe, inverted: exactly one widget hovers under overlap.
TEST(arbitration_single_hover_under_overlap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 75, 20, false, false);   // warm
    AB_A(&ctx); AB_B(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 75, 20, false, false);
    WLX_Interaction a = AB_A(&ctx);
    WLX_Interaction b = AB_B(&ctx);
    ASSERT_TRUE(!a.hover);
    ASSERT_TRUE(b.hover);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// A higher layer beats a later base-layer query.
TEST(arbitration_higher_layer_beats_later_seq) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    for (int frame = 0; frame < 2; frame++) {
        bool press = (frame == 1);
        test_frame_begin(&ctx, 75, 20, press, press);
        ctx.current_layer = 1;
        WLX_Interaction b = AB_B(&ctx);              // layer 1, earlier query
        ctx.current_layer = 0;
        WLX_Interaction a = AB_A(&ctx);              // base, later query
        if (press) {
            ASSERT_TRUE(b.active && b.pressed);
            ASSERT_TRUE(!a.active);
        }
        test_frame_end(&ctx);
    }

    wlx_context_destroy(&ctx);
}

// A widget that was not queried last frame cannot own this frame's press.
TEST(arbitration_first_frame_appearance_cannot_own) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 50, 120, false, false);  // warm: only A exists
    AB_A(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, true, true);    // press where X appears
    AB_A(&ctx);
    WLX_Interaction x = AB_X(&ctx);
    ASSERT_TRUE(!x.active);                          // no previous candidate
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, false, false);  // release; X is warm now
    AB_A(&ctx); AB_X(&ctx);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 50, 120, true, true);    // second press: X owns
    AB_A(&ctx);
    x = AB_X(&ctx);
    ASSERT_TRUE(x.active && x.pressed);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// The first frame of a context has no arbitration data: query-time capture
// applies, so a frame-one press still activates (the bootstrap contract).
TEST(arbitration_bootstrap_first_frame_acquires) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 20, 20, true, true);
    WLX_Interaction a = AB_A(&ctx);
    ASSERT_TRUE(a.active && a.pressed);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// wlx_overlay: absolutely positioned subtree on the next layer.
// ---------------------------------------------------------------------------

static int _ov_warns = 0;
static void _ov_warn_sink(const char *file, int line, const char *msg, void *user) {
    (void)file; (void)line; (void)msg; (void)user;
    _ov_warns++;
}

TEST(overlay_draws_over_later_base_content) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_widget(&ctx, .height = 10, .back_color = fill);         // base, before
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 50, 50, 120, 80 }),
        .back_color = fill);
    wlx_widget(&ctx, .height = 20, .back_color = fill);         // overlay body
    wlx_overlay_end(&ctx);
    wlx_widget(&ctx, .height = 30, .back_color = fill);         // base, after
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(4, _ov_rect_count);
    ASSERT_TRUE(_ov_rects[0].h == 10.0f);   // base pass first
    ASSERT_TRUE(_ov_rects[1].h == 30.0f);
    ASSERT_TRUE(_ov_rects[2].w == 120.0f);  // overlay chrome
    ASSERT_TRUE(_ov_rects[3].h == 20.0f);   // overlay body content

    wlx_context_destroy(&ctx);
}

// The headline probe: content inside an overlay wins the press over the
// base widget beneath it.
TEST(overlay_press_beats_base_beneath) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    bool base_clicked = false, over_clicked = false;
    for (int frame = 0; frame < 3; frame++) {
        bool down    = (frame == 1);
        bool clicked = (frame == 1);
        test_frame_begin(&ctx, 50, 20, down, clicked);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        base_clicked |= wlx_button(&ctx, "base", .height = 40);
        wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 20, 0, 100, 40 }));
        over_clicked |= wlx_button(&ctx, "over");
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(over_clicked);
    ASSERT_TRUE(!base_clicked);
    wlx_context_destroy(&ctx);
}

TEST(overlay_scissor_contains_its_layer) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    ctx.backend.begin_scissor = _ov_rec_scissor_begin;
    ctx.backend.end_scissor = _ov_rec_scissor_end;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_widget(&ctx, .height = 10, .back_color = fill);         // base
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 50, 50, 120, 80 }));
    wlx_widget(&ctx, .height = 20, .back_color = fill);         // clipped body
    wlx_overlay_end(&ctx);
    wlx_widget(&ctx, .height = 30, .back_color = fill);         // base
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_ov_scissor_begins >= 1);
    ASSERT_EQ_INT(_ov_scissor_begins, _ov_scissor_ends);
    ASSERT_EQ_INT(0, _ov_scissor_open);
    ASSERT_EQ_INT(1, _ov_rects_inside_scissor);   // only the overlay body

    wlx_context_destroy(&ctx);
}

TEST(overlay_nested_draws_over_parent_overlay) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .height = 10, .back_color = fill);          // base
    wlx_overlay_begin(&ctx, 2, ((WLX_Rect){ 40, 40, 200, 160 }));
    wlx_widget(&ctx, .height = 20, .back_color = fill);          // layer 1
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 60, 60, 100, 60 }));
    wlx_widget(&ctx, .height = 25, .back_color = fill);          // layer 2
    wlx_overlay_end(&ctx);
    wlx_overlay_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(3, _ov_rect_count);
    ASSERT_TRUE(_ov_rects[0].h == 10.0f);
    ASSERT_TRUE(_ov_rects[1].h == 20.0f);
    ASSERT_TRUE(_ov_rects[2].h == 25.0f);   // deepest overlay replays last

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Wheel routing: the wheel belongs to the pointer's layer. A base panel
// under an overlay must not scroll; a panel inside the overlay must.
// ---------------------------------------------------------------------------

// One frame: base scroll panel filling the window, optionally covered by an
// overlay with a button under the pointer. Returns the offset observed in
// the panel body (wheel applies at panel end, so reads lag one frame).
static float _ov_base_panel_frame(WLX_Context *ctx, bool with_overlay,
                                  int mx, int my, float wheel) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel,
                        NULL, NULL, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_scroll_panel_begin(ctx, 900);              // scrollable in 300px viewport
    wlx_button(ctx, "row");
    float off = wlx_get_scroll_panel_offset(ctx);
    wlx_scroll_panel_end(ctx);
    if (with_overlay) {
        wlx_overlay_begin(ctx, 1, ((WLX_Rect){ 20, 0, 100, 40 }));
        wlx_button(ctx, "over");
        wlx_overlay_end(ctx);
    }
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return off;
}

TEST(wheel_over_overlay_leaves_base_panel_still) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    _ov_base_panel_frame(&ctx, true, 50, 20, 0.0f);            // warm
    _ov_base_panel_frame(&ctx, true, 50, 20, -1.0f);           // wheel over overlay
    ASSERT_TRUE(ctx.input.wheel_delta == -1.0f);               // not consumed below
    float off = _ov_base_panel_frame(&ctx, true, 50, 20, 0.0f);
    ASSERT_TRUE(off == 0.0f);                                  // base panel still

    // Overlay dismissed: the pointer's owner drops back to the base layer
    // and the same wheel scrolls the panel again.
    _ov_base_panel_frame(&ctx, false, 50, 20, 0.0f);           // re-warm
    _ov_base_panel_frame(&ctx, false, 50, 20, -1.0f);
    ASSERT_TRUE(ctx.input.wheel_delta == 0.0f);                // consumed
    off = _ov_base_panel_frame(&ctx, false, 50, 20, 0.0f);
    ASSERT_TRUE(off > 0.0f);

    wlx_context_destroy(&ctx);
}

// One frame: scroll panel living inside an overlay, pointer over its content.
static float _ov_overlay_panel_frame(WLX_Context *ctx, int mx, int my,
                                     float wheel) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel,
                        NULL, NULL, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT);
    wlx_widget(ctx, .height = 40);
    wlx_overlay_begin(ctx, 1, ((WLX_Rect){ 20, 20, 200, 120 }));
    wlx_scroll_panel_begin(ctx, 900);              // scrollable in 120px viewport
    wlx_button(ctx, "item");
    float off = wlx_get_scroll_panel_offset(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_overlay_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return off;
}

TEST(wheel_scrolls_panel_inside_overlay) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    _ov_overlay_panel_frame(&ctx, 60, 60, 0.0f);               // warm
    _ov_overlay_panel_frame(&ctx, 60, 60, -1.0f);              // wheel over item
    ASSERT_TRUE(ctx.input.wheel_delta == 0.0f);                // consumed on layer 1
    float off = _ov_overlay_panel_frame(&ctx, 60, 60, 0.0f);
    ASSERT_TRUE(off > 0.0f);

    wlx_context_destroy(&ctx);
}

// Opt-in offscreen culling must not drop overlay content lying outside an
// enclosing base-layer clip: those scissors do not apply to the overlay's
// replay pass. (The dashboard regression: an unclipped menu reaching below
// its clipped module card lost its row rects but kept text, which is never
// culled - items rendered without hover highlights.)
TEST(overlay_rects_escape_base_clip_culling) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    wlx_set_cull_offscreen(&ctx, true);
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)),
        .padding = 0, .clip = true);            // base clip: 0..40
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 0, 200, 100, 40 }),
        .clip = false);                          // entirely below the clip
    wlx_widget(&ctx, .height = 20, .back_color = fill);
    wlx_overlay_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    bool found = false;
    for (int i = 0; i < _ov_rect_count; i++) {
        if (_ov_rects[i].h == 20.0f && _ov_rects[i].y >= 200.0f) found = true;
    }
    ASSERT_TRUE(found);

    wlx_context_destroy(&ctx);
}

TEST(overlay_immediate_mode_warns_and_draws_in_place) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    wlx_dbg_init(&ctx);
    ctx.dbg->warn_cb = _ov_warn_sink;
    _ov_reset();
    _ov_warns = 0;

    WLX_Color fill = { 9, 9, 9, 255 };
    wlx_begin_immediate(&ctx, wlx_rect(0, 0, 400, 300), _test_input_handler);
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 50, 50, 120, 80 }));
    wlx_widget(&ctx, .height = 20, .back_color = fill);
    wlx_overlay_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(1, _ov_warns);            // once-per-site diagnostic
    ASSERT_EQ_INT(1, _ov_rect_count);       // body drew in place
    ASSERT_TRUE(_ov_rects[0].h == 20.0f);

    wlx_context_destroy(&ctx);
}

// ---------------------------------------------------------------------------
// Overlays declared INSIDE a scroll panel (the common popup placement):
// the popup escapes the base panel for drawing AND input. Geometry (ctx
// 400x300): panel viewport 0..150 (PX(150) slot); overlay at y 100..220, so
// its lower half lies outside the base viewport.
// ---------------------------------------------------------------------------

static bool _ov_in_panel_clicked = false;

static void _ov_in_panel_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(150), WLX_SLOT_PX(150)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 100.0f, .padding = 0);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(100)), .padding = 0);
    wlx_widget(ctx, .height = 100);                      // base body
    wlx_overlay_begin(ctx, 2, ((WLX_Rect){ 20, 100, 200, 120 }));
    wlx_widget(ctx, .height = 60);                       // 100..160 (inside vp)
    _ov_in_panel_clicked |= wlx_button(ctx, "below", .height = 60);  // 160..220 (outside vp)
    wlx_overlay_end(ctx);
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(overlay_inside_scroll_panel_button_below_viewport_is_clickable) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ov_in_panel_clicked = false;

    _ov_in_panel_frame(&ctx, 300, 280, false, false);   // warm candidates
    _ov_in_panel_frame(&ctx, 60, 190, true, true);      // press on the button (y 190 > 150)
    _ov_in_panel_frame(&ctx, 60, 190, false, false);    // release -> click
    ASSERT_TRUE(_ov_in_panel_clicked);

    wlx_context_destroy(&ctx);
}

// Regression guard: the base context is unchanged while an overlay is open
// elsewhere - a base button scrolled out of its panel is still unclickable.
static bool _ov_base_hidden_clicked = false;
static bool _ov_base_shown_clicked = false;

static void _ov_base_clip_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_PX(200)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 200.0f, .padding = 0);  // viewport 0..100, content 200
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_PX(100)), .padding = 0, .gap = 0);
    _ov_base_shown_clicked  |= wlx_button(ctx, "shown",  .height = 100);  // 0..100
    _ov_base_hidden_clicked |= wlx_button(ctx, "hidden", .height = 100);  // 100..200, scrolled away
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_overlay_begin(ctx, 1, ((WLX_Rect){ 300, 250, 80, 40 }));      // open elsewhere
    wlx_widget(ctx, .height = 40);
    wlx_overlay_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(base_widgets_under_open_overlay_stay_clipped) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ov_base_hidden_clicked = false;
    _ov_base_shown_clicked = false;

    _ov_base_clip_frame(&ctx, 200, 290, false, false);  // warm
    _ov_base_clip_frame(&ctx, 50, 150, true, true);     // press where "hidden" sits (outside vp)
    _ov_base_clip_frame(&ctx, 50, 150, false, false);
    ASSERT_TRUE(!_ov_base_hidden_clicked);
    _ov_base_clip_frame(&ctx, 50, 50, true, true);      // control: "shown" inside the viewport
    _ov_base_clip_frame(&ctx, 50, 50, false, false);
    ASSERT_TRUE(_ov_base_shown_clicked);

    wlx_context_destroy(&ctx);
}

// Nested overlays: after the inner overlay ends, the outer overlay's clip
// context is back (the outer's own rect clips, not the base panel) - a
// button in the outer overlay below the base viewport still takes the press.
static bool _ov_nested_outer_clicked = false;

static void _ov_nested_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(150), WLX_SLOT_PX(150)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 100.0f, .padding = 0);  // viewport 0..150
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(100)), .padding = 0);
    wlx_widget(ctx, .height = 100);
    wlx_overlay_begin(ctx, 3, ((WLX_Rect){ 20, 100, 200, 180 }));   // outer: 100..280
    wlx_widget(ctx, .height = 60);                                  // 100..160
    wlx_overlay_begin(ctx, 1, ((WLX_Rect){ 240, 0, 100, 40 }));     // inner, elsewhere
    wlx_widget(ctx, .height = 40);
    wlx_overlay_end(ctx);                                           // back to outer context
    _ov_nested_outer_clicked |= wlx_button(ctx, "outer-below", .height = 60, .pos = 2); // 220..280
    wlx_overlay_end(ctx);
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

TEST(nested_overlay_restores_enclosing_base) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _ov_nested_outer_clicked = false;

    _ov_nested_frame(&ctx, 380, 290, false, false);    // warm
    _ov_nested_frame(&ctx, 60, 250, true, true);       // y 250: outside the base viewport
    _ov_nested_frame(&ctx, 60, 250, false, false);
    ASSERT_TRUE(_ov_nested_outer_clicked);

    wlx_context_destroy(&ctx);
}

// Deferred mode: an overlay inside a clip layout leaves the base pass's
// scissor stream untouched - exactly one BEGIN/END pair for the base clip
// and one for the overlay, no re-arm BEGIN after the overlay (the base pass
// never lost its clip; layers replay separately).
TEST(overlay_end_emits_no_stray_scissor_in_deferred_mode) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    ctx.backend.begin_scissor = _ov_rec_scissor_begin;
    ctx.backend.end_scissor = _ov_rec_scissor_end;
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(40)),
        .padding = 0, .gap = 0, .clip = true);              // base clip
    wlx_widget(&ctx, .height = 40, .back_color = fill);
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 50, 100, 120, 40 }));   // clip = true by default
    wlx_widget(&ctx, .height = 20, .back_color = fill);
    wlx_overlay_end(&ctx);
    wlx_widget(&ctx, .height = 40, .back_color = fill);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(2, _ov_scissor_begins);     // base clip + overlay, no re-arm
    ASSERT_EQ_INT(_ov_scissor_begins, _ov_scissor_ends);
    ASSERT_EQ_INT(0, _ov_scissor_open);

    wlx_context_destroy(&ctx);
}

// With offscreen culling on, a rect drawn on a popup layer entirely outside
// the overlay's own clip rect is culled (the layer's clip context is the
// overlay rect), while the overlay's body still records.
TEST(overlay_rects_outside_own_clip_are_culled) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_rect = _ov_rec_rect;
    wlx_set_cull_offscreen(&ctx, true);
    _ov_reset();

    WLX_Color fill = { 9, 9, 9, 255 };
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 0, 200, 100, 40 }));    // clip = true
    wlx_widget(&ctx, .height = 20, .back_color = fill);              // inside: recorded
    wlx_draw_rect(&ctx, ((WLX_Rect){ 300, 0, 50, 50 }), fill);       // outside: culled
    wlx_overlay_end(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    bool inside = false, outside = false;
    for (int i = 0; i < _ov_rect_count; i++) {
        if (_ov_rects[i].h == 20.0f && _ov_rects[i].y >= 200.0f) inside = true;
        if (_ov_rects[i].x == 300.0f && _ov_rects[i].y == 0.0f) outside = true;
    }
    ASSERT_TRUE(inside);
    ASSERT_TRUE(!outside);

    wlx_context_destroy(&ctx);
}

SUITE(overlay) {
    RUN_TEST(layered_replay_dispatches_ascending);
    RUN_TEST(layered_replay_keeps_scissors_within_their_layer);
    RUN_TEST(overlay_draws_over_later_base_content);
    RUN_TEST(overlay_press_beats_base_beneath);
    RUN_TEST(overlay_scissor_contains_its_layer);
    RUN_TEST(overlay_nested_draws_over_parent_overlay);
    RUN_TEST(arbitration_topmost_click_wins);
    RUN_TEST(arbitration_single_hover_under_overlap);
    RUN_TEST(arbitration_higher_layer_beats_later_seq);
    RUN_TEST(arbitration_first_frame_appearance_cannot_own);
    RUN_TEST(arbitration_bootstrap_first_frame_acquires);
    RUN_TEST(wheel_over_overlay_leaves_base_panel_still);
    RUN_TEST(wheel_scrolls_panel_inside_overlay);
    RUN_TEST(overlay_rects_escape_base_clip_culling);
    RUN_TEST(overlay_immediate_mode_warns_and_draws_in_place);
    RUN_TEST(overlay_inside_scroll_panel_button_below_viewport_is_clickable);
    RUN_TEST(base_widgets_under_open_overlay_stay_clipped);
    RUN_TEST(nested_overlay_restores_enclosing_base);
    RUN_TEST(overlay_end_emits_no_stray_scissor_in_deferred_mode);
    RUN_TEST(overlay_rects_outside_own_clip_are_culled);
}

// test_tooltip.c - wlx_tooltip_for: pointer-anchored draw-only tip.
//
// A per-id timer accumulates ctx->frame_dt while the pointer rests on the
// anchor (button up, pointer on the anchor's layer); past the delay the tip
// draws on the next layer near the pointer. Draw-only is the load-bearing
// pin: the tip never appends interaction candidates, so it cannot steal
// hover or the press from the widget it describes.

static float _tt_dt = 0.0f;
static float _tt_get_frame_time(void *user) {
    (void)user; return _tt_dt; }

static WLX_Rect _tt_rects[16];
static int _tt_rect_count = 0;
static void _tt_rec_rect(WLX_Rect r, WLX_Color c, void *user) {
    (void)user;
    (void)c;
    if (_tt_rect_count < 16) _tt_rects[_tt_rect_count++] = r;
}

static bool _tt_anchor_clicked = false;

// One frame: a 40px anchor button across the top, tooltip on its rect.
static bool _tt_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    _tt_anchor_clicked |= wlx_button(ctx, "anchor", .height = 40);
    bool vis = wlx_tooltip_for(ctx, ((WLX_Rect){ 0, 0, 400, 40 }), "hint",
                               .delay = 0.5f);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return vis;
}

static void _tt_reset(WLX_Context *ctx) {
    _tt_anchor_clicked = false;
    _tt_rect_count = 0;
    test_ctx_init(ctx, 400, 300);
    ctx->backend.get_frame_time = _tt_get_frame_time;
    _tt_dt = 0.2f;
    _tt_frame(ctx, 300, 200, false, false);   // warm, pointer off the anchor
}

TEST(tooltip_appears_after_delay_and_resets) {
    WLX_Context ctx;
    _tt_reset(&ctx);

    ASSERT_TRUE(!_tt_frame(&ctx, 50, 20, false, false));   // 0.2s
    ASSERT_TRUE(!_tt_frame(&ctx, 50, 20, false, false));   // 0.4s
    ASSERT_TRUE(_tt_frame(&ctx, 50, 20, false, false));    // 0.6s >= 0.5s
    ASSERT_TRUE(_tt_frame(&ctx, 50, 20, false, false));    // stays up

    ASSERT_TRUE(!_tt_frame(&ctx, 300, 200, false, false)); // pointer leaves
    ASSERT_TRUE(!_tt_frame(&ctx, 50, 20, false, false));   // timer restarted

    wlx_context_destroy(&ctx);
}

TEST(tooltip_hides_while_pressed_and_press_still_lands) {
    WLX_Context ctx;
    _tt_reset(&ctx);

    _tt_frame(&ctx, 50, 20, false, false);
    _tt_frame(&ctx, 50, 20, false, false);
    ASSERT_TRUE(_tt_frame(&ctx, 50, 20, false, false));    // visible

    ASSERT_TRUE(!_tt_frame(&ctx, 50, 20, true, true));     // press hides it
    ASSERT_TRUE(!_tt_frame(&ctx, 50, 20, false, false));   // release restarts delay
    ASSERT_TRUE(_tt_anchor_clicked);                        // the press was the anchor's

    wlx_context_destroy(&ctx);
}

TEST(tooltip_is_draw_only) {
    WLX_Context ctx;
    _tt_reset(&ctx);

    _tt_frame(&ctx, 50, 20, false, false);
    _tt_frame(&ctx, 50, 20, false, false);
    ASSERT_TRUE(_tt_frame(&ctx, 50, 20, false, false));    // visible

    // The frame's candidate list holds exactly the anchor button; the tip
    // added nothing, and the anchor still owns hover.
    ASSERT_EQ_INT(1, (int)ctx.cands[ctx.cand_frame & 1].count);
    ASSERT_TRUE(ctx.interaction.hot_id != 0);

    wlx_context_destroy(&ctx);
}

TEST(tooltip_draws_over_base_content) {
    WLX_Context ctx;
    _tt_reset(&ctx);
    ctx.backend.draw_rect = _tt_rec_rect;

    _tt_frame(&ctx, 50, 20, false, false);
    _tt_frame(&ctx, 50, 20, false, false);
    _tt_rect_count = 0;
    ASSERT_TRUE(_tt_frame(&ctx, 50, 20, false, false));    // visible

    // The tip chrome replays after every base-layer rect, at pointer +
    // offset (50 + 12, 20 + 18).
    ASSERT_TRUE(_tt_rect_count >= 2);
    ASSERT_TRUE(_tt_rects[_tt_rect_count - 1].x == 62.0f);
    ASSERT_TRUE(_tt_rects[_tt_rect_count - 1].y == 38.0f);

    wlx_context_destroy(&ctx);
}

// A 100px top bar over a scroll panel; the panel holds a 40px anchor button
// with a tooltip on wlx_last_rect, above 560px of filler. The wheel scrolls
// the panel when the pointer is inside it.
static bool _tt_scrolled_frame(WLX_Context *ctx, int mx, int my, float wheel) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel, NULL, NULL, NULL);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_FLEX(1)),
                       .padding = 0, .gap = 0);
        wlx_button(ctx, "top bar", .height = 100);
        wlx_scroll_panel_begin(ctx, 600, .wheel_scroll_speed = 20.0f);
            wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(560)),
                               .padding = 0, .gap = 0);
                wlx_button(ctx, "anchor", .height = 40);
                bool vis = wlx_tooltip_for(ctx, wlx_last_rect(ctx), "hint", .delay = 0.5f);
            wlx_layout_end(ctx);
        wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return vis;
}

// The anchor scrolled out of its panel sits under the top bar: its raw rect
// still contains the pointer there, but the viewport does not, so the tip
// must stay down - exactly like the widget's own hover.
TEST(tooltip_ignores_anchor_scrolled_out_of_its_panel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.get_frame_time = _tt_get_frame_time;
    _tt_dt = 0.2f;

    // Control: unscrolled, the anchor at y 100..140 is visible and tips.
    _tt_scrolled_frame(&ctx, 300, 250, 0.0f);                 // warm, pointer off
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 120, 0.0f));     // 0.2s
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 120, 0.0f));     // 0.4s
    ASSERT_TRUE(_tt_scrolled_frame(&ctx, 50, 120, 0.0f));      // 0.6s: visible

    // Scroll the panel 60px: the anchor now spans y 40..80, under the bar.
    _tt_scrolled_frame(&ctx, 300, 250, -3.0f);
    // Pointer rests where the anchor would be, for well past the delay.
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 60, 0.0f));
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 60, 0.0f));
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 60, 0.0f));
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 60, 0.0f));
    ASSERT_TRUE(!_tt_scrolled_frame(&ctx, 50, 60, 0.0f));

    wlx_context_destroy(&ctx);
}

// wlx_last_rect reports the widget just placed - the tooltip anchor idiom.
TEST(last_rect_reports_the_widget_just_placed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Rect before = wlx_last_rect(&ctx);
    ASSERT_TRUE(before.w == 0.0f && before.h == 0.0f);   // nothing placed yet
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_button(&ctx, "anchor", .height = 40);
    WLX_Rect r = wlx_last_rect(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(r.x == 0.0f && r.y == 0.0f);
    ASSERT_TRUE(r.w == 400.0f && r.h == 40.0f);

    wlx_context_destroy(&ctx);
}

// A tooltip whose anchor sits at the bottom of a scroll panel: the tip is
// drawn below the panel viewport with its own scissor (the tip rect), not
// cut to the base viewport.
static WLX_Rect _ttp_scissors[16];
static int _ttp_scissor_count = 0;
static void _ttp_rec_scissor(WLX_Rect r, void *user) {
    (void)user;
    if (_ttp_scissor_count < 16) _ttp_scissors[_ttp_scissor_count++] = r;
}

static bool _ttp_frame(WLX_Context *ctx, int mx, int my) {
    test_frame_begin(ctx, mx, my, false, false);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(150), WLX_SLOT_PX(150)), .padding = 0, .gap = 0);
    wlx_scroll_panel_begin(ctx, 100.0f, .padding = 0);            // viewport 0..150
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(110), WLX_SLOT_PX(40)), .padding = 0, .gap = 0);
    wlx_widget(ctx, .height = 110);
    wlx_button(ctx, "anchor", .height = 40);                     // 110..150
    bool vis = wlx_tooltip_for(ctx, wlx_last_rect(ctx), "hint", .delay = 0.5f);
    wlx_layout_end(ctx);
    wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return vis;
}

TEST(tooltip_anchor_inside_scroll_panel_tip_escapes_viewport) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.get_frame_time = _tt_get_frame_time;
    ctx.backend.begin_scissor = _ttp_rec_scissor;
    _tt_dt = 0.2f;

    _ttp_frame(&ctx, 300, 250);               // warm, pointer off
    _ttp_frame(&ctx, 50, 140);                // 0.2s on the anchor
    _ttp_frame(&ctx, 50, 140);                // 0.4s
    _ttp_scissor_count = 0;
    ASSERT_TRUE(_ttp_frame(&ctx, 50, 140));   // 0.6s: visible; tip at y 140 + 18 = 158

    // A scissor starting below the base viewport (y >= 150) with positive
    // height exists: the tip's own clip, not the intersection with 0..150.
    bool found = false;
    for (int i = 0; i < _ttp_scissor_count; i++) {
        if (_ttp_scissors[i].y >= 150.0f && _ttp_scissors[i].h > 0.0f) found = true;
    }
    ASSERT_TRUE(found);

    wlx_context_destroy(&ctx);
}

SUITE(tooltip) {
    RUN_TEST(tooltip_appears_after_delay_and_resets);
    RUN_TEST(tooltip_hides_while_pressed_and_press_still_lands);
    RUN_TEST(tooltip_is_draw_only);
    RUN_TEST(tooltip_draws_over_base_content);
    RUN_TEST(tooltip_ignores_anchor_scrolled_out_of_its_panel);
    RUN_TEST(last_rect_reports_the_widget_just_placed);
    RUN_TEST(tooltip_anchor_inside_scroll_panel_tip_escapes_viewport);
}

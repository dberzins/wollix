// test_layout_clip.c - opt-in layout clip (scissor stream + pointer gating)
// + segmented progress containment
//
// Included from test_main.c (single TU build) AFTER test_cmd_replay.c so the
// crec_* recording backend (crec_ctx_init, crec_frame_begin, crec_find, the
// MARK_* colors, and the _crec_log buffer) is available here.

// ============================================================================
// Helpers
// ============================================================================

// True when `inner` lies entirely within `outer` (within a small epsilon).
static inline bool clip_rect_contains(WLX_Rect outer, WLX_Rect inner) {
    float eps = 0.01f;
    return inner.x >= outer.x - eps &&
           inner.y >= outer.y - eps &&
           inner.x + inner.w <= outer.x + outer.w + eps &&
           inner.y + inner.h <= outer.y + outer.h + eps;
}

// Count recorded scissor commands of a given type.
static inline int clip_count_cmd(WLX_Cmd_Type type) {
    int n = 0;
    for (size_t i = 0; i < _crec_count; i++) {
        if (_crec_log[i].type == type) n++;
    }
    return n;
}

// Index of the last recorded command of a given type, -1 when none.
static inline int clip_last_cmd(WLX_Cmd_Type type) {
    int last = -1;
    for (size_t i = 0; i < _crec_count; i++) {
        if (_crec_log[i].type == type) last = (int)i;
    }
    return last;
}

// ============================================================================
// Layout clip: scissor recording
// ============================================================================

// A .clip = true layout records exactly one SCISSOR_BEGIN whose rect equals the
// layout content rect (post-padding) and one matching SCISSOR_END.
TEST(layout_clip_emits_scissor_on_content_rect) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true, .padding = 10);
        // The top layout rect is the post-padding content rect = scissor rect.
        WLX_Rect content = wlx_get_parent_rect(&ctx);
        crec_marker(&ctx, MARK_A, 40);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(1, clip_count_cmd(WLX_CMD_SCISSOR_BEGIN));
    ASSERT_EQ_INT(1, clip_count_cmd(WLX_CMD_SCISSOR_END));

    int sb = crec_find(WLX_CMD_SCISSOR_BEGIN, 0);
    ASSERT_TRUE(sb >= 0);
    ASSERT_EQ_RECT(_crec_log[sb].rect, content, 0.5f);

    // The SCISSOR_END follows the SCISSOR_BEGIN.
    int se = crec_find(WLX_CMD_SCISSOR_END, (size_t)sb + 1);
    ASSERT_TRUE(se >= 0);
    wlx_context_destroy(&ctx);
}

// A default (no-clip) layout records no scissor commands at all.
TEST(layout_no_clip_emits_no_scissor) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        crec_marker(&ctx, MARK_A, 40);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(-1, crec_find(WLX_CMD_SCISSOR_BEGIN, 0));
    ASSERT_EQ_INT(-1, crec_find(WLX_CMD_SCISSOR_END, 0));
    wlx_context_destroy(&ctx);
}

// A clip layout nested inside a scroll panel records a scissor that stays
// within the scroll panel's own clip rect (the panel scissor is recorded
// first; the layout clip rect is contained by it).
TEST(layout_clip_nested_intersects_scroll_panel) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 600);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_scroll_panel_begin(&ctx, 200.0f, .height = 120.0f);
            wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true, .padding = 8);
                crec_marker(&ctx, MARK_A, 40);
            wlx_layout_end(&ctx);
        wlx_scroll_panel_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    // First scissor: the scroll panel viewport. Second: the clip layout.
    int sb_panel = crec_find(WLX_CMD_SCISSOR_BEGIN, 0);
    ASSERT_TRUE(sb_panel >= 0);
    int sb_layout = crec_find(WLX_CMD_SCISSOR_BEGIN, (size_t)sb_panel + 1);
    ASSERT_TRUE(sb_layout >= 0);

    ASSERT_TRUE(clip_rect_contains(_crec_log[sb_panel].rect, _crec_log[sb_layout].rect));
    wlx_context_destroy(&ctx);
}

// After the refactor, wlx_panel_begin(.clip = true) still emits exactly one
// balanced scissor pair on the panel content rect (single release owner).
TEST(panel_clip_parity_after_refactor) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_panel_begin(&ctx, .clip = true);
            // No title -> the top layout rect is the panel content rect.
            WLX_Rect content = wlx_get_parent_rect(&ctx);
            crec_marker(&ctx, MARK_A, 40);
        wlx_panel_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(1, clip_count_cmd(WLX_CMD_SCISSOR_BEGIN));
    ASSERT_EQ_INT(1, clip_count_cmd(WLX_CMD_SCISSOR_END));

    int sb = crec_find(WLX_CMD_SCISSOR_BEGIN, 0);
    ASSERT_TRUE(sb >= 0);
    ASSERT_EQ_RECT(_crec_log[sb].rect, content, 0.5f);
    wlx_context_destroy(&ctx);
}

// A panel without .clip emits no scissor (the refactor must not leak a scissor).
TEST(panel_no_clip_emits_no_scissor) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 300);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_panel_begin(&ctx);
            crec_marker(&ctx, MARK_A, 40);
        wlx_panel_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(-1, crec_find(WLX_CMD_SCISSOR_BEGIN, 0));
    ASSERT_EQ_INT(-1, crec_find(WLX_CMD_SCISSOR_END, 0));
    wlx_context_destroy(&ctx);
}

// Nested clip layouts restore their enclosing scissor on release. Backends end
// clipping by disabling the scissor test outright (raylib's EndScissorMode)
// rather than popping to the previous region, so each inner clip that has an
// enclosing scissor re-arms it after its own SCISSOR_END. For three nested
// clips A>B>C that is: 3 enter begins + 2 re-arm begins (after C and after B)
// = 5 begins, and 3 ends (one per layer). The outermost (A) has no enclosing
// scissor, so it ends with a plain SCISSOR_END and the frame finishes with the
// scissor disabled (no leak).
TEST(layout_clip_stack_balanced) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 600);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT, .clip = true);
        crec_marker(&ctx, MARK_A, 40);
        wlx_layout_begin(&ctx, 2, WLX_VERT, .clip = true, .padding = 6);
            crec_marker(&ctx, MARK_B, 40);
            wlx_layout_begin(&ctx, 1, WLX_VERT, .clip = true, .padding = 4);
                crec_marker(&ctx, MARK_C, 40);
            wlx_layout_end(&ctx);
        wlx_layout_end(&ctx);
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    ASSERT_EQ_INT(5, clip_count_cmd(WLX_CMD_SCISSOR_BEGIN));
    ASSERT_EQ_INT(3, clip_count_cmd(WLX_CMD_SCISSOR_END));
    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);

    // No leak: the final scissor command in the buffer is a SCISSOR_END, so
    // replay finishes with clipping disabled.
    ASSERT_TRUE(clip_last_cmd(WLX_CMD_SCISSOR_END) > clip_last_cmd(WLX_CMD_SCISSOR_BEGIN));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Layout clip: pointer gating
// ============================================================================
// A .clip layout bounds the pointer as well as the drawing: a child's hit
// zone is its rect intersected with every enclosing clip of its layer, the
// rule scroll panel viewports and overlay roots already follow.

// One frame: a clip layout filling slot 0 (0..40) of a 40/200 parent, holding
// a 0..100 interaction rect queried with `flags`; `down` presses the button.
// (A root clip layout would clip to the whole context rect, so the clip
// layout must sit in a bounded parent slot.)
static WLX_Interaction _lc_probe(WLX_Context *ctx, int mx, int my, bool down, uint32_t flags) {
    test_frame_begin(ctx, mx, my, down, down);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(200)),
                       .padding = 0, .gap = 0);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .clip = true);      // clip 0..40
    WLX_Interaction s = wlx_get_interaction(ctx, wlx_rect(0, 0, 100, 100), flags, "lc_probe", 1);
    wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return s;
}

// Hover stops at the clip rect on the bootstrap frame and on the arbitrated
// frame alike; inside the clip rect the widget is hot as before.
TEST(layout_clip_gates_hover) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Interaction s = _lc_probe(&ctx, 50, 80, false, WLX_INTERACT_HOVER); // bootstrap: containment
    ASSERT_FALSE(s.hover);
    s = _lc_probe(&ctx, 50, 80, false, WLX_INTERACT_HOVER);                 // arbitrated: last frame's candidates
    ASSERT_FALSE(s.hover);

    (void)_lc_probe(&ctx, 50, 20, false, WLX_INTERACT_HOVER);               // control: inside the clip
    s = _lc_probe(&ctx, 50, 20, false, WLX_INTERACT_HOVER);
    ASSERT_TRUE(s.hover);
    wlx_context_destroy(&ctx);
}

// A press on the cropped part of a widget does not acquire the drag, after
// a warm frame (arbitrated) and on a fresh context (bootstrap); a press
// inside the clip rect does.
TEST(layout_clip_drag_not_acquired_outside) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    const uint32_t drag = WLX_INTERACT_HOVER | WLX_INTERACT_DRAG;
    (void)_lc_probe(&ctx, 50, 80, false, drag);              // warm
    WLX_Interaction s = _lc_probe(&ctx, 50, 80, true, drag); // press outside the clip
    ASSERT_FALSE(s.active);
    ASSERT_EQ_INT((int)ctx.interaction.active_id, 0);
    wlx_context_destroy(&ctx);

    test_ctx_init(&ctx, 400, 300);
    s = _lc_probe(&ctx, 50, 80, true, drag);                 // bootstrap press outside
    ASSERT_FALSE(s.active);
    wlx_context_destroy(&ctx);

    test_ctx_init(&ctx, 400, 300);
    s = _lc_probe(&ctx, 50, 20, true, drag);                 // control: inside the clip
    ASSERT_TRUE(s.active);
    wlx_context_destroy(&ctx);
}

// Parent slots 0..40 and 40..240. "below" is declared first into slot 1;
// the clip layout in slot 0 holds "big", which overflows to 0..100, so
// 40..100 of it is cropped and sits over "below".
static bool _lc_big_clicked = false;
static bool _lc_below_clicked = false;
static void _lc_click_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(200)),
                       .padding = 0, .gap = 0);
        _lc_below_clicked |= wlx_button(ctx, "below", .pos = 1, .height = 200);   // 40..240
        wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(100)),
                           .pos = 0, .padding = 0, .clip = true);                 // clip 0..40
            _lc_big_clicked |= wlx_button(ctx, "big", .height = 100);            // 0..100
        wlx_layout_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// The visible, earlier-declared sibling receives the click; the clipped-away
// part of the later-declared widget does not (arbitration sees the clipped
// candidate rect). Inside the clip rect the clipped widget still wins.
TEST(layout_clip_gates_click_visible_sibling_wins) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _lc_big_clicked = false;
    _lc_below_clicked = false;

    _lc_click_frame(&ctx, 200, 290, false, false);   // warm
    _lc_click_frame(&ctx, 50, 80, true, true);       // press: cropped "big", visible "below"
    _lc_click_frame(&ctx, 50, 80, false, false);     // release
    ASSERT_TRUE(_lc_below_clicked);
    ASSERT_FALSE(_lc_big_clicked);

    _lc_big_clicked = false;
    _lc_below_clicked = false;
    _lc_click_frame(&ctx, 50, 20, true, true);       // control: inside the clip
    _lc_click_frame(&ctx, 50, 20, false, false);
    ASSERT_TRUE(_lc_big_clicked);
    ASSERT_FALSE(_lc_below_clicked);
    wlx_context_destroy(&ctx);
}

// A scroll panel inside a clip layout: its scissor is the intersection with
// the clip, and the clip is re-armed after the panel ends so a sibling
// declared after the panel stays clipped. Expected stream: BEGIN(clip),
// BEGIN(panel, within clip), END, BEGIN(clip re-arm), MARK_B, END, MARK_C.
TEST(layout_clip_scroll_panel_scissor_intersects_and_rearms) {
    WLX_Context ctx;
    crec_ctx_init(&ctx, 400, 600);

    crec_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(60), WLX_SLOT_PX(300)),
                       .padding = 0, .gap = 0);
        wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(100), WLX_SLOT_PX(40)),
                           .padding = 0, .gap = 0, .clip = true);                 // clip 0..60
            wlx_scroll_panel_begin(&ctx, 500.0f, .height = 100.0f, .padding = 0); // 0..100 overflows
                crec_marker(&ctx, MARK_A, 40);
            wlx_scroll_panel_end(&ctx);
            crec_marker(&ctx, MARK_B, 40);                                        // 100..140, still inside the clip layout
        wlx_layout_end(&ctx);
        crec_marker(&ctx, MARK_C, 40);                                            // after the clip layout
    wlx_layout_end(&ctx);
    wlx_end(&ctx);

    // The panel's scissor lies within the clip layout's.
    int sb_clip = crec_find(WLX_CMD_SCISSOR_BEGIN, 0);
    ASSERT_TRUE(sb_clip >= 0);
    int sb_panel = crec_find(WLX_CMD_SCISSOR_BEGIN, (size_t)sb_clip + 1);
    ASSERT_TRUE(sb_panel >= 0);
    ASSERT_TRUE(clip_rect_contains(_crec_log[sb_clip].rect, _crec_log[sb_panel].rect));

    // After the panel's END, the clip rect is re-armed before MARK_B draws.
    int se_panel = crec_find(WLX_CMD_SCISSOR_END, (size_t)sb_panel + 1);
    ASSERT_TRUE(se_panel >= 0);
    int sb_rearm = crec_find(WLX_CMD_SCISSOR_BEGIN, (size_t)se_panel + 1);
    int mb = crec_find_color(MARK_B, 0);
    ASSERT_TRUE(mb >= 0);
    ASSERT_TRUE(sb_rearm >= 0 && sb_rearm < mb);
    ASSERT_EQ_F(_crec_log[sb_rearm].rect.y, _crec_log[sb_clip].rect.y, 0.01f);
    ASSERT_EQ_F(_crec_log[sb_rearm].rect.h, _crec_log[sb_clip].rect.h, 0.01f);

    // Balanced: clip enter, panel enter, re-arm = 3 begins; panel and clip
    // ends = 2 ends; the frame finishes with the scissor disabled.
    ASSERT_EQ_INT(3, clip_count_cmd(WLX_CMD_SCISSOR_BEGIN));
    ASSERT_EQ_INT(2, clip_count_cmd(WLX_CMD_SCISSOR_END));
    ASSERT_EQ_INT(0, (int)ctx.arena.layouts.count);
    ASSERT_TRUE(clip_last_cmd(WLX_CMD_SCISSOR_END) > clip_last_cmd(WLX_CMD_SCISSOR_BEGIN));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Layout clip: wheel gating
// ============================================================================

static float _lc_inner_y = 0.0f, _lc_sibling_y = 0.0f;

// Slot 0 (0..60) holds a clip layout with a scroll panel overflowing to
// 0..100; slot 1 (60..300) holds a sibling scroll panel. Both have room to
// scroll. Records each panel's content rect y right after its begin.
static void _lc_wheel_frame(WLX_Context *ctx, int mx, int my, float wheel) {
    test_frame_begin_ex(ctx, mx, my, false, false, false, wheel, NULL, NULL, NULL);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(60), WLX_SLOT_PX(240)),
                       .padding = 0, .gap = 0);
        wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .clip = true);            // clip 0..60
            wlx_scroll_panel_begin(ctx, 500.0f, .height = 100.0f, .padding = 0);   // 0..100
                _lc_inner_y = wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1].rect.y;
                wlx_widget(ctx, .height = 500);
            wlx_scroll_panel_end(ctx);
        wlx_layout_end(ctx);
        wlx_scroll_panel_begin(ctx, 800.0f, .padding = 0);                         // 60..300
            _lc_sibling_y = wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1].rect.y;
            wlx_widget(ctx, .height = 800);
        wlx_scroll_panel_end(ctx);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// A wheel notch over the cropped part of the inner panel scrolls the visible
// sibling under the pointer, not the cropped panel; over the visible part of
// the inner panel it scrolls that panel alone.
TEST(layout_clip_gates_wheel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    _lc_wheel_frame(&ctx, 50, 80, 0.0f);       // warm
    _lc_wheel_frame(&ctx, 50, 80, -1.0f);      // wheel over the cropped part (60..100)
    _lc_wheel_frame(&ctx, 50, 80, 0.0f);       // read back
    ASSERT_EQ_F(_lc_inner_y, 0.0f, 0.01f);     // cropped panel did not scroll
    ASSERT_TRUE(_lc_sibling_y < 60.0f);         // the visible sibling did
    wlx_context_destroy(&ctx);

    test_ctx_init(&ctx, 400, 300);             // control: wheel over the visible part
    _lc_wheel_frame(&ctx, 50, 30, 0.0f);
    _lc_wheel_frame(&ctx, 50, 30, -1.0f);
    _lc_wheel_frame(&ctx, 50, 30, 0.0f);
    ASSERT_TRUE(_lc_inner_y < 0.0f);
    ASSERT_EQ_F(_lc_sibling_y, 60.0f, 0.01f);
    wlx_context_destroy(&ctx);
}

SUITE(layout_clip) {
    RUN_TEST(layout_clip_emits_scissor_on_content_rect);
    RUN_TEST(layout_no_clip_emits_no_scissor);
    RUN_TEST(layout_clip_nested_intersects_scroll_panel);
    RUN_TEST(panel_clip_parity_after_refactor);
    RUN_TEST(panel_no_clip_emits_no_scissor);
    RUN_TEST(layout_clip_stack_balanced);
    RUN_TEST(layout_clip_gates_hover);
    RUN_TEST(layout_clip_drag_not_acquired_outside);
    RUN_TEST(layout_clip_gates_click_visible_sibling_wins);
    RUN_TEST(layout_clip_scroll_panel_scissor_intersects_and_rearms);
    RUN_TEST(layout_clip_gates_wheel);
}

// ============================================================================
// Segmented progress containment (wlx_progress_cell_rect)
// ============================================================================

// At a track narrower than segments + (segments-1)*gap, every cell stays
// within the track: x >= track.x and x + w <= track.x + track.w, w >= 0.
TEST(progress_cells_within_track_narrow) {
    WLX_Rect track = { 10.0f, 5.0f, 20.0f, 8.0f };
    int segments = 8;
    float gap = 4.0f; // 8 + 7*4 = 36 > 20 -> gap compression + clamp engage
    float right = track.x + track.w;

    for (int i = 0; i < segments; i++) {
        WLX_Rect cell = wlx_progress_cell_rect(track, segments, gap, i);
        ASSERT_TRUE(cell.x >= track.x - 0.01f);
        ASSERT_TRUE(cell.x + cell.w <= right + 0.01f);
        ASSERT_TRUE(cell.w >= 0.0f);
    }
}

// An extreme degenerate width (gaps almost fill the track, cells floored to
// 1px) still produces only in-bounds cells; trailing cells collapse to zero.
TEST(progress_cells_clamp_degenerate) {
    WLX_Rect track = { 0.0f, 0.0f, 9.0f, 6.0f };
    int segments = 5;
    float gap = 2.0f;
    float right = track.x + track.w;

    for (int i = 0; i < segments; i++) {
        WLX_Rect cell = wlx_progress_cell_rect(track, segments, gap, i);
        ASSERT_TRUE(cell.x >= track.x - 0.01f);
        ASSERT_TRUE(cell.x + cell.w <= right + 0.01f);
        ASSERT_TRUE(cell.w >= 0.0f);
    }
}

// At a comfortable width, cells tile the track without exceeding it, each cell
// keeps at least 1px, and the nominal gap is preserved between cells.
TEST(progress_cells_within_track_normal) {
    WLX_Rect track = { 0.0f, 0.0f, 200.0f, 10.0f };
    int segments = 5;
    float gap = 4.0f;
    float right = track.x + track.w;

    WLX_Rect prev = {0};
    for (int i = 0; i < segments; i++) {
        WLX_Rect cell = wlx_progress_cell_rect(track, segments, gap, i);
        ASSERT_TRUE(cell.x >= track.x - 0.01f);
        ASSERT_TRUE(cell.x + cell.w <= right + 0.01f);
        ASSERT_TRUE(cell.w >= 1.0f);
        if (i > 0) {
            float actual_gap = cell.x - (prev.x + prev.w);
            ASSERT_EQ_F(actual_gap, gap, 0.5f);
        }
        prev = cell;
    }

    // The last cell ends exactly at the track's right edge.
    WLX_Rect last = wlx_progress_cell_rect(track, segments, gap, segments - 1);
    ASSERT_EQ_F(last.x + last.w, right, 0.5f);
}

SUITE(progress_bounds) {
    RUN_TEST(progress_cells_within_track_narrow);
    RUN_TEST(progress_cells_clamp_degenerate);
    RUN_TEST(progress_cells_within_track_normal);
}

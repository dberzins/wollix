// test_layout_clip.c - opt-in layout clip + segmented progress containment
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
    int last_begin = -1, last_end = -1;
    for (size_t i = 0; i < _crec_count; i++) {
        if (_crec_log[i].type == WLX_CMD_SCISSOR_BEGIN) last_begin = (int)i;
        if (_crec_log[i].type == WLX_CMD_SCISSOR_END)   last_end   = (int)i;
    }
    ASSERT_TRUE(last_end > last_begin);
}

SUITE(layout_clip) {
    RUN_TEST(layout_clip_emits_scissor_on_content_rect);
    RUN_TEST(layout_no_clip_emits_no_scissor);
    RUN_TEST(layout_clip_nested_intersects_scroll_panel);
    RUN_TEST(panel_clip_parity_after_refactor);
    RUN_TEST(panel_no_clip_emits_no_scissor);
    RUN_TEST(layout_clip_stack_balanced);
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

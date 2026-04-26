// test_rounding.c - tests for roundness resolution, separator widget,
// and layout background drawing.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs for draw-call verification
// ============================================================================

static int _rec_draw_line_count;
static float _rec_draw_line_x1, _rec_draw_line_y1;
static float _rec_draw_line_x2, _rec_draw_line_y2;

static void rec_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    (void)thick; (void)c;
    _rec_draw_line_count++;
    _rec_draw_line_x1 = x1; _rec_draw_line_y1 = y1;
    _rec_draw_line_x2 = x2; _rec_draw_line_y2 = y2;
}

static int _rec_draw_rect_count;
static WLX_Rect _rec_draw_rect_last;

static void rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)c;
    _rec_draw_rect_count++;
    _rec_draw_rect_last = r;
}

static int _rec_draw_rect_rounded_count;
static float _rec_draw_rect_rounded_last_rn;

static void rec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)r; (void)segments; (void)c;
    _rec_draw_rect_rounded_count++;
    _rec_draw_rect_rounded_last_rn = roundness;
}

static int _rec_draw_rect_rounded_lines_count;

static void rec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    (void)r; (void)roundness; (void)segments; (void)thick; (void)c;
    _rec_draw_rect_rounded_lines_count++;
}

static void rec_reset(void) {
    _rec_draw_line_count = 0;
    _rec_draw_rect_count = 0;
    _rec_draw_rect_rounded_count = 0;
    _rec_draw_rect_rounded_lines_count = 0;
}

static WLX_Backend rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_line = rec_draw_line;
    b.draw_rect = rec_draw_rect;
    b.draw_rect_rounded = rec_draw_rect_rounded;
    b.draw_rect_rounded_lines = rec_draw_rect_rounded_lines;
    return b;
}

// ============================================================================
// wlx_resolve_roundness tests
// ============================================================================

TEST(roundness_sentinel_resolves_to_theme) {
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0.25f;
    theme.rounded_segments = 12;

    float rn = -1.0f;
    int rs = -1;
    wlx_resolve_roundness(&theme, &rn, &rs);

    ASSERT_EQ_F(rn, 0.25f, 0.001f);
    ASSERT_EQ_INT(rs, 12);
}

TEST(roundness_zero_stays_sharp) {
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0.25f;
    theme.rounded_segments = 12;

    float rn = 0.0f;
    int rs = 0;
    wlx_resolve_roundness(&theme, &rn, &rs);

    ASSERT_EQ_F(rn, 0.0f, 0.001f);
    ASSERT_EQ_INT(rs, 0);
}

TEST(roundness_explicit_overrides_theme) {
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0.25f;
    theme.rounded_segments = 12;

    float rn = 0.5f;
    int rs = 16;
    wlx_resolve_roundness(&theme, &rn, &rs);

    ASSERT_EQ_F(rn, 0.5f, 0.001f);
    ASSERT_EQ_INT(rs, 16);
}

// ============================================================================
// wlx_separator tests
// ============================================================================

TEST(separator_horizontal) {
    // In a VERT layout, the separator slot is wider than tall -> horizontal
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_separator(&ctx, .height = 4);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_rec_draw_line_count, 1);
    // Horizontal: y1 == y2 (centered in 4px slot)
    ASSERT_EQ_F(_rec_draw_line_y1, _rec_draw_line_y2, 0.001f);
    // Spans full width
    ASSERT_EQ_F(_rec_draw_line_x1, 0.0f, 0.5f);
    ASSERT_TRUE(_rec_draw_line_x2 > 100.0f);

    wlx_context_destroy(&ctx);
}

TEST(separator_vertical) {
    // In a HORZ layout with a narrow slot, h > w -> vertical
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 3, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_FLEX(1), WLX_SLOT_PX(4), WLX_SLOT_FLEX(1) });
    wlx_widget(&ctx, .color = (WLX_Color){0,0,0,255});
    wlx_separator(&ctx);
    wlx_widget(&ctx, .color = (WLX_Color){0,0,0,255});
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // The 4px-wide slot is 300px tall -> vertical line
    ASSERT_TRUE(_rec_draw_line_count >= 1);
    // Vertical: x1 == x2
    ASSERT_EQ_F(_rec_draw_line_x1, _rec_draw_line_x2, 0.001f);

    wlx_context_destroy(&ctx);
}

TEST(separator_default_color_uses_theme_border) {
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_separator(&ctx, .height = 2);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_rec_draw_line_count, 1);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Layout background tests
// ============================================================================

TEST(layout_background_draws_when_back_color_set) {
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = (WLX_Color){30, 30, 30, 255});
    wlx_widget(&ctx, .color = (WLX_Color){50, 50, 50, 255}, .roundness = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Layout bg (flat, roundness=0) + widget (flat, roundness=0)
    ASSERT_TRUE(_rec_draw_rect_count >= 2);
    wlx_context_destroy(&ctx);
}

TEST(layout_background_not_drawn_when_zero) {
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    // No back_color set -> no background drawing
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .color = (WLX_Color){50, 50, 50, 255}, .roundness = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Only 1 draw_rect for the widget, none for layout bg
    ASSERT_EQ_INT(_rec_draw_rect_count, 1);
    wlx_context_destroy(&ctx);
}

TEST(layout_background_rounded) {
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = (WLX_Color){30, 30, 30, 255},
        .border_color = (WLX_Color){60, 60, 60, 255},
        .border_width = 1.0f,
        .roundness = 0.1f,
        .rounded_segments = 8);
    wlx_widget(&ctx, .color = (WLX_Color){50, 50, 50, 255}, .roundness = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Layout bg rounded draw
    ASSERT_TRUE(_rec_draw_rect_rounded_count >= 1);
    ASSERT_EQ_F(_rec_draw_rect_rounded_last_rn, 0.1f, 0.001f);
    // Rounded border drawn
    ASSERT_TRUE(_rec_draw_rect_rounded_lines_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(grid_background_draws) {
    rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_grid_begin(&ctx, 1, 1,
        .back_color = (WLX_Color){30, 30, 30, 255});
    wlx_grid_cell(&ctx, 0, 0);
    wlx_widget(&ctx, .color = (WLX_Color){50, 50, 50, 255}, .roundness = 0);
    wlx_grid_end(&ctx);
    test_frame_end(&ctx);

    // Grid bg (flat) + widget (flat)
    ASSERT_TRUE(_rec_draw_rect_count >= 2);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(rounding) {
    RUN_TEST(roundness_sentinel_resolves_to_theme);
    RUN_TEST(roundness_zero_stays_sharp);
    RUN_TEST(roundness_explicit_overrides_theme);
    RUN_TEST(separator_horizontal);
    RUN_TEST(separator_vertical);
    RUN_TEST(separator_default_color_uses_theme_border);
    RUN_TEST(layout_background_draws_when_back_color_set);
    RUN_TEST(layout_background_not_drawn_when_zero);
    RUN_TEST(layout_background_rounded);
    RUN_TEST(grid_background_draws);
}

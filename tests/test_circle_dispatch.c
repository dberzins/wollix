// test_circle_dispatch.c - tests for core circle/ring dispatch routing.
// Included from test_main.c (single TU build).

// ============================================================================
// Local recording stubs for dispatch verification
// ============================================================================

static int _cd_circle_count;
static float _cd_last_circle_cx;
static float _cd_last_circle_cy;
static float _cd_last_circle_radius;

static void cd_rec_draw_circle(float cx, float cy, float radius, int segments, WLX_Color c) {
    _cd_circle_count++;
    _cd_last_circle_cx = cx;
    _cd_last_circle_cy = cy;
    _cd_last_circle_radius = radius;
    (void)segments; (void)c;
}

static int _cd_ring_count;
static float _cd_last_ring_inner_r;
static float _cd_last_ring_outer_r;

static void cd_rec_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color c) {
    _cd_ring_count++;
    (void)cx; (void)cy; (void)segments; (void)c;
    _cd_last_ring_inner_r = inner_r;
    _cd_last_ring_outer_r = outer_r;
}

static int _cd_rounded_count;

static void cd_rec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    _cd_rounded_count++;
    (void)r; (void)roundness; (void)segments; (void)c;
}

static int _cd_rounded_lines_count;

static void cd_rec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    _cd_rounded_lines_count++;
    (void)r; (void)roundness; (void)segments; (void)thick; (void)c;
}

static void cd_reset(void) {
    _cd_circle_count = 0;
    _cd_ring_count = 0;
    _cd_rounded_count = 0;
    _cd_rounded_lines_count = 0;
    _cd_last_circle_cx = 0;
    _cd_last_circle_cy = 0;
    _cd_last_circle_radius = 0;
    _cd_last_ring_inner_r = 0;
    _cd_last_ring_outer_r = 0;
}

// ============================================================================
// Tests
// ============================================================================

TEST(circle_routing_calls_draw_circle) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = cd_rec_draw_circle;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;

    WLX_Rect sq = {10, 20, 30, 30};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded(&ctx, sq, 1.0f, 16, white);

    ASSERT_EQ_INT(1, _cd_circle_count);
    ASSERT_EQ_INT(0, _cd_rounded_count);
    ASSERT_EQ_F(25.0f, _cd_last_circle_cx, 0.01f);
    ASSERT_EQ_F(35.0f, _cd_last_circle_cy, 0.01f);
    ASSERT_EQ_F(15.0f, _cd_last_circle_radius, 0.01f);
}

TEST(ring_routing_calls_draw_ring) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_ring = cd_rec_draw_ring;
    ctx.backend.draw_rect_rounded_lines = cd_rec_draw_rect_rounded_lines;

    WLX_Rect sq = {10, 20, 40, 40};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded_lines(&ctx, sq, 1.0f, 16, 2.0f, white);

    ASSERT_EQ_INT(1, _cd_ring_count);
    ASSERT_EQ_INT(0, _cd_rounded_lines_count);
    ASSERT_EQ_F(18.0f, _cd_last_ring_inner_r, 0.01f);
    ASSERT_EQ_F(20.0f, _cd_last_ring_outer_r, 0.01f);
}

TEST(null_fallback_circle_uses_rect_rounded) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = NULL;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;

    WLX_Rect sq = {0, 0, 20, 20};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded(&ctx, sq, 1.0f, 16, white);

    ASSERT_EQ_INT(0, _cd_circle_count);
    ASSERT_EQ_INT(1, _cd_rounded_count);
}

TEST(null_fallback_ring_uses_rect_rounded_lines) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_ring = NULL;
    ctx.backend.draw_rect_rounded_lines = cd_rec_draw_rect_rounded_lines;

    WLX_Rect sq = {0, 0, 20, 20};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded_lines(&ctx, sq, 1.0f, 16, 1.0f, white);

    ASSERT_EQ_INT(0, _cd_ring_count);
    ASSERT_EQ_INT(1, _cd_rounded_lines_count);
}

TEST(nonsquare_rect_bypasses_circle) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = cd_rec_draw_circle;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;

    WLX_Rect wide = {0, 0, 40, 20};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded(&ctx, wide, 1.0f, 16, white);

    ASSERT_EQ_INT(0, _cd_circle_count);
    ASSERT_EQ_INT(1, _cd_rounded_count);
}

TEST(low_roundness_bypasses_circle) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.immediate_mode = true;
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = cd_rec_draw_circle;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;

    WLX_Rect sq = {0, 0, 20, 20};
    WLX_Color white = {255, 255, 255, 255};
    wlx_draw_rect_rounded(&ctx, sq, 0.5f, 16, white);

    ASSERT_EQ_INT(0, _cd_circle_count);
    ASSERT_EQ_INT(1, _cd_rounded_count);
}

TEST(toggle_routes_thumb_through_draw_circle) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = cd_rec_draw_circle;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = true;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "Test", &val);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_cd_circle_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(radio_routes_ring_through_draw_ring) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_ring = cd_rec_draw_ring;
    ctx.backend.draw_rect_rounded_lines = cd_rec_draw_rect_rounded_lines;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "A", &active, 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_cd_ring_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(toggle_fallback_without_circle) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_circle = NULL;
    ctx.backend.draw_rect_rounded = cd_rec_draw_rect_rounded;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = true;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "Test", &val);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _cd_circle_count);
    ASSERT_TRUE(_cd_rounded_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(radio_fallback_without_ring) {
    cd_reset();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_ring = NULL;
    ctx.backend.draw_rect_rounded_lines = cd_rec_draw_rect_rounded_lines;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "A", &active, 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(0, _cd_ring_count);
    ASSERT_TRUE(_cd_rounded_lines_count >= 1);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(circle_dispatch) {
    RUN_TEST(circle_routing_calls_draw_circle);
    RUN_TEST(ring_routing_calls_draw_ring);
    RUN_TEST(null_fallback_circle_uses_rect_rounded);
    RUN_TEST(null_fallback_ring_uses_rect_rounded_lines);
    RUN_TEST(nonsquare_rect_bypasses_circle);
    RUN_TEST(low_roundness_bypasses_circle);
    RUN_TEST(toggle_routes_thumb_through_draw_circle);
    RUN_TEST(radio_routes_ring_through_draw_ring);
    RUN_TEST(toggle_fallback_without_circle);
    RUN_TEST(radio_fallback_without_ring);
}

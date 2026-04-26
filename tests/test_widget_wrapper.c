// test_widget_wrapper.c - wlx_widget wrapper contract tests (Phase 1 coverage).
// Included from test_main.c (single TU build).
//
// Tests 1-4: document existing behaviour; expected GREEN against current code.
// Test 5: pins the F1 hover-brightness bug; expected RED against current code.
//   Phase 1 exit criteria: tests 1-4 pass, test 5 fails naming the missing
//   hover-brightness application on opt.border_color in wlx_widget_impl.

// ============================================================================
// Local recording stubs
// ============================================================================

static WLX_Color _ww_last_fill_color;
static WLX_Color _ww_last_border_color;

static void ww_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    _ww_last_fill_color = c;
}

static void ww_rec_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)r; (void)thick;
    _ww_last_border_color = c;
}

static WLX_Backend ww_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect       = ww_rec_draw_rect;
    b.draw_rect_lines = ww_rec_draw_rect_lines;
    return b;
}

static void ww_reset(void) {
    _ww_last_fill_color   = (WLX_Color){0};
    _ww_last_border_color = (WLX_Color){0};
}

// ============================================================================
// Test 1: .id = "swatch" - id stack is balanced after the call
// ============================================================================

TEST(widget_wrapper_id_balanced) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    size_t count_before = ctx.arena.id_stack.count;
    wlx_widget(&ctx, .id = "swatch");
    size_t count_after = ctx.arena.id_stack.count;
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT((int)count_before, (int)count_after);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Test 2: .id = NULL - id stack is not touched
// ============================================================================

TEST(widget_wrapper_id_null_no_push) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    size_t count_before = ctx.arena.id_stack.count;
    wlx_widget(&ctx, .id = NULL);
    size_t count_after = ctx.arena.id_stack.count;
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT((int)count_before, (int)count_after);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Test 3: .opacity = 0.5 applies to the recorded fill color alpha
// ============================================================================

TEST(widget_wrapper_opacity_fill) {
    ww_reset();
    WLX_Context ctx = {0};
    ctx.backend = ww_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness        = 0;
    theme.rounded_segments = 0;
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .color = WLX_RGBA(200, 100, 50, 255), .opacity = 0.5f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 255 * 0.5 = 127.5 -> rounds to 128; allow +/-1 for integer rounding
    int expected_a = (int)roundf(255.0f * 0.5f);
    ASSERT_TRUE(abs((int)_ww_last_fill_color.a - expected_a) <= 1);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Test 4: .opacity = 0.5 applies to the recorded border color alpha
// ============================================================================

TEST(widget_wrapper_opacity_border) {
    ww_reset();
    WLX_Context ctx = {0};
    ctx.backend = ww_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness        = 0;
    theme.rounded_segments = 0;
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx,
        .color        = WLX_RGBA(200, 100, 50, 255),
        .border_color = WLX_RGBA(100, 50, 200, 255),
        .border_width = 2.0f,
        .opacity      = 0.5f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 255 * 0.5 = 127.5 -> rounds to 128; allow +/-1 for integer rounding
    int expected_a = (int)roundf(255.0f * 0.5f);
    ASSERT_TRUE(abs((int)_ww_last_border_color.a - expected_a) <= 1);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Test 5: hover brightness applies to both fill AND border color
//
// Expected RED against current code: wlx_widget_impl only brightens opt.color
// on hover; opt.border_color is drawn raw, so the assertion below fails.
// Phase 2 fixes this by applying hover brightness to opt.border_color.
// ============================================================================

TEST(widget_wrapper_hover_brightness_border) {
    ww_reset();
    WLX_Context ctx = {0};
    ctx.backend = ww_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness        = 0;
    theme.rounded_segments = 0;
    theme.hover_brightness = 0.2f;
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    const WLX_Color base_border = WLX_RGBA(100, 50, 200, 255);

    // Mouse inside the single full-screen slot so inter.hover == true
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx,
        .color        = WLX_RGBA(80, 80, 80, 255),
        .border_color = base_border,
        .border_width = 2.0f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Hover brightness (factor=0.2) should have been applied to border too.
    // wlx_color_brightness: r=100+(255-100)*0.2=131, g=50+(255-50)*0.2=91,
    //                        b=200+(255-200)*0.2=211, a=255 (unchanged).
    WLX_Color expected_border = wlx_color_brightness(base_border, 0.2f);

    // This assertion FAILS against current code because hover brightness is
    // only applied to the fill color in wlx_widget_impl, not opt.border_color.
    ASSERT_EQ_COLOR(_ww_last_border_color, expected_border);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(widget_wrapper) {
    RUN_TEST(widget_wrapper_id_balanced);
    RUN_TEST(widget_wrapper_id_null_no_push);
    RUN_TEST(widget_wrapper_opacity_fill);
    RUN_TEST(widget_wrapper_opacity_border);
    RUN_TEST(widget_wrapper_hover_brightness_border);
}

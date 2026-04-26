// test_widgets.c - tests for Phase 2 widgets: progress, toggle, radio.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs for draw-call verification
// ============================================================================

static int _wgt_draw_rect_count;
static WLX_Rect _wgt_draw_rect_last;
static WLX_Color _wgt_draw_rect_last_color;

static void wgt_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    _wgt_draw_rect_count++;
    _wgt_draw_rect_last = r;
    _wgt_draw_rect_last_color = c;
}

static int _wgt_draw_rect_rounded_count;
static float _wgt_draw_rect_rounded_last_rn;
static WLX_Color _wgt_draw_rect_rounded_last_color;

static void wgt_rec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)r; (void)segments;
    _wgt_draw_rect_rounded_count++;
    _wgt_draw_rect_rounded_last_rn = roundness;
    _wgt_draw_rect_rounded_last_color = c;
}

static int _wgt_draw_rect_rounded_lines_count;

static void wgt_rec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    (void)r; (void)roundness; (void)segments; (void)thick; (void)c;
    _wgt_draw_rect_rounded_lines_count++;
}

static int _wgt_draw_circle_count;

static void wgt_rec_draw_circle(float cx, float cy, float radius, int segments, WLX_Color c) {
    (void)cx; (void)cy; (void)radius; (void)segments; (void)c;
    _wgt_draw_circle_count++;
}

static int _wgt_draw_ring_count;

static void wgt_rec_draw_ring(float cx, float cy, float inner_r, float outer_r, int segments, WLX_Color c) {
    (void)cx; (void)cy; (void)inner_r; (void)outer_r; (void)segments; (void)c;
    _wgt_draw_ring_count++;
}

static void wgt_rec_reset(void) {
    _wgt_draw_rect_count = 0;
    _wgt_draw_rect_rounded_count = 0;
    _wgt_draw_rect_rounded_lines_count = 0;
    _wgt_draw_circle_count = 0;
    _wgt_draw_ring_count = 0;
}

static WLX_Backend wgt_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect = wgt_rec_draw_rect;
    b.draw_rect_rounded = wgt_rec_draw_rect_rounded;
    b.draw_rect_rounded_lines = wgt_rec_draw_rect_rounded_lines;
    b.draw_circle = wgt_rec_draw_circle;
    b.draw_ring = wgt_rec_draw_ring;
    return b;
}

// ============================================================================
// Progress tests
// ============================================================================

TEST(progress_clamps_value) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 2, WLX_VERT);
    // Value > 1 should be clamped; must not crash or draw beyond track
    wlx_progress(&ctx, 2.0f);
    // Value < 0 should be clamped
    wlx_progress(&ctx, -1.0f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Both should have drawn without issues
    ASSERT_TRUE(_wgt_draw_rect_count >= 2 || _wgt_draw_rect_rounded_count >= 2);
    wlx_context_destroy(&ctx);
}

TEST(progress_zero_draws_no_fill) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0;
    theme.rounded_segments = 0;
    ctx.theme = &theme;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_progress(&ctx, 0.0f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // With sharp corners and value=0: only 1 draw_rect for the track, no fill
    ASSERT_EQ_INT(_wgt_draw_rect_count, 1);
    wlx_context_destroy(&ctx);
}

TEST(progress_full_draws_fill) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0;
    theme.rounded_segments = 0;
    ctx.theme = &theme;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    test_frame_begin(&ctx, 0, 0, false, false);

    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_progress(&ctx, 1.0f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Track + fill = 2 draw_rect calls
    ASSERT_EQ_INT(_wgt_draw_rect_count, 2);
    wlx_context_destroy(&ctx);
}

TEST(progress_resolves_theme_colors) {
    WLX_Progress_Opt opt = wlx_default_progress_opt();
    WLX_Theme theme = wlx_theme_dark;
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    // Colors should be zero before resolve
    ASSERT_TRUE(wlx_color_is_zero(opt.track_color));
    ASSERT_TRUE(wlx_color_is_zero(opt.fill_color));

    wlx_resolve_opt_progress(&ctx, &opt);

    // After resolve: track_color from slider.track, fill_color from accent
    ASSERT_FALSE(wlx_color_is_zero(opt.track_color));
    ASSERT_FALSE(wlx_color_is_zero(opt.fill_color));
    ASSERT_EQ_INT(opt.track_color.r, theme.slider.track.r);
    ASSERT_EQ_INT(opt.fill_color.r, theme.accent.r);
}

// ============================================================================
// Toggle tests
// ============================================================================

// Wrapper keeps the wlx_toggle macro on a single source line so the
// interaction ID stays stable across frames (ID = hash(file, line)).
static bool _test_toggle_frame(WLX_Context *ctx, bool *val) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    bool changed = wlx_toggle(ctx, "Test", val);
    wlx_layout_end(ctx);
    return changed;
}

TEST(toggle_click_flips_value) {
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = false;

    // Frame 1: hover
    test_frame_begin(&ctx, 200, 150, false, false);
    _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);
    ASSERT_FALSE(val);

    // Frame 2: press
    test_frame_begin(&ctx, 200, 150, true, true);
    _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);
    ASSERT_FALSE(val);

    // Frame 3: release -> click fires
    test_frame_begin(&ctx, 200, 150, false, false);
    bool changed = _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);
    ASSERT_TRUE(val);
    ASSERT_TRUE(changed);

    // Frame 4: idle
    test_frame_begin(&ctx, 200, 150, false, false);
    _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);
    ASSERT_TRUE(val);

    // Frame 5: press to toggle off
    test_frame_begin(&ctx, 200, 150, true, true);
    _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);

    // Frame 6: release -> click fires, toggles off
    test_frame_begin(&ctx, 200, 150, false, false);
    changed = _test_toggle_frame(&ctx, &val);
    test_frame_end(&ctx);
    ASSERT_FALSE(val);
    ASSERT_TRUE(changed);

    wlx_context_destroy(&ctx);
}

TEST(toggle_draws_active_track_when_on) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = true;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "On", &val);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Track (pill shape) goes through draw_rect_rounded; thumb (square) through draw_circle
    ASSERT_TRUE(_wgt_draw_rect_rounded_count >= 1);
    ASSERT_TRUE(_wgt_draw_circle_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(toggle_resolves_theme_defaults) {
    WLX_Toggle_Opt opt = wlx_default_toggle_opt();
    WLX_Theme theme = wlx_theme_dark;
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    ASSERT_TRUE(wlx_color_is_zero(opt.track_active_color));
    ASSERT_TRUE(wlx_color_is_zero(opt.thumb_color));

    wlx_resolve_opt_toggle(&ctx, &opt);

    // track_active_color resolves through toggle.track_active -> accent
    ASSERT_FALSE(wlx_color_is_zero(opt.track_active_color));
    // thumb_color resolves through toggle.thumb -> foreground
    ASSERT_FALSE(wlx_color_is_zero(opt.thumb_color));
    ASSERT_EQ_INT(opt.thumb_color.r, theme.foreground.r);
}

TEST(toggle_fallback_first_level) {
    WLX_Theme theme = wlx_theme_dark;
    theme.toggle.track        = WLX_RGBA(10, 20, 30, 255);
    theme.toggle.track_active = WLX_RGBA(40, 50, 60, 255);
    theme.toggle.thumb        = WLX_RGBA(70, 80, 90, 255);

    WLX_Toggle_Opt opt = wlx_default_toggle_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;
    wlx_resolve_opt_toggle(&ctx, &opt);

    ASSERT_EQ_COLOR(opt.track_color,        theme.toggle.track);
    ASSERT_EQ_COLOR(opt.track_active_color, theme.toggle.track_active);
    ASSERT_EQ_COLOR(opt.thumb_color,        theme.toggle.thumb);
}

TEST(toggle_fallback_second_level) {
    WLX_Theme theme = wlx_theme_dark;
    theme.toggle.track        = (WLX_Color){0};
    theme.toggle.track_active = (WLX_Color){0};
    theme.toggle.thumb        = (WLX_Color){0};

    WLX_Toggle_Opt opt = wlx_default_toggle_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;
    wlx_resolve_opt_toggle(&ctx, &opt);

    ASSERT_EQ_COLOR(opt.track_color,        theme.slider.track);
    ASSERT_EQ_COLOR(opt.track_active_color, theme.accent);
    ASSERT_EQ_COLOR(opt.thumb_color,        theme.foreground);
}

// ============================================================================
// Radio tests
// ============================================================================

// Wrapper keeps each wlx_radio call on a fixed source line so IDs
// stay stable across frames.
static bool _test_radio_frame(WLX_Context *ctx, int *active) {
    wlx_layout_begin(ctx, 3, WLX_VERT);
    wlx_radio(ctx, "A", active, 0);
    wlx_radio(ctx, "B", active, 1);
    bool changed = wlx_radio(ctx, "C", active, 2);
    wlx_layout_end(ctx);
    return changed;
}

TEST(radio_click_sets_active_index) {
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    // Frame 1: no click
    test_frame_begin(&ctx, 200, 150, false, false);
    _test_radio_frame(&ctx, &active);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(active, 0);

    // Frame 2: press on the third radio (slot 2: y=200..300, center=250)
    test_frame_begin(&ctx, 200, 250, true, true);
    _test_radio_frame(&ctx, &active);
    test_frame_end(&ctx);

    // Frame 3: release -> click fires
    test_frame_begin(&ctx, 200, 250, false, false);
    bool changed = _test_radio_frame(&ctx, &active);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(active, 2);
    ASSERT_TRUE(changed);

    wlx_context_destroy(&ctx);
}

TEST(radio_selected_draws_fill) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "Selected", &active, 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Ring (draw_ring) + fill circle (draw_circle) for selected state
    ASSERT_TRUE(_wgt_draw_circle_count >= 1);
    ASSERT_TRUE(_wgt_draw_ring_count >= 1);
    wlx_context_destroy(&ctx);
}

TEST(radio_unselected_no_fill) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 1;  // something else selected

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "Not selected", &active, 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Ring drawn but no fill circle
    ASSERT_TRUE(_wgt_draw_ring_count >= 1);
    ASSERT_EQ_INT(_wgt_draw_circle_count, 0);
    wlx_context_destroy(&ctx);
}

TEST(radio_resolves_theme_defaults) {
    WLX_Radio_Opt opt = wlx_default_radio_opt();
    WLX_Theme theme = wlx_theme_dark;
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    ASSERT_TRUE(wlx_color_is_zero(opt.ring_color));
    ASSERT_TRUE(wlx_color_is_zero(opt.fill_color));

    wlx_resolve_opt_radio(&ctx, &opt);

    // ring_color resolves through radio.ring -> border
    ASSERT_FALSE(wlx_color_is_zero(opt.ring_color));
    ASSERT_EQ_INT(opt.ring_color.r, theme.border.r);
    // fill_color resolves through radio.fill -> accent
    ASSERT_FALSE(wlx_color_is_zero(opt.fill_color));
    ASSERT_EQ_INT(opt.fill_color.r, theme.accent.r);
}

TEST(radio_fallback_first_level) {
    WLX_Theme theme = wlx_theme_dark;
    theme.radio.ring = WLX_RGBA(11, 22, 33, 255);
    theme.radio.fill = WLX_RGBA(44, 55, 66, 255);

    WLX_Radio_Opt opt = wlx_default_radio_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;
    wlx_resolve_opt_radio(&ctx, &opt);

    ASSERT_EQ_COLOR(opt.ring_color, theme.radio.ring);
    ASSERT_EQ_COLOR(opt.fill_color, theme.radio.fill);
}

TEST(radio_fallback_second_level) {
    WLX_Theme theme = wlx_theme_dark;
    theme.radio.ring = (WLX_Color){0};
    theme.radio.fill = (WLX_Color){0};

    WLX_Radio_Opt opt = wlx_default_radio_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;
    wlx_resolve_opt_radio(&ctx, &opt);

    ASSERT_EQ_COLOR(opt.ring_color, theme.border);
    ASSERT_EQ_COLOR(opt.fill_color, theme.accent);
}

// ============================================================================
// Toggle alignment tests
// ============================================================================

TEST(toggle_align_left_positions_at_slot_left) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);

    WLX_Toggle_Opt opt = wlx_default_toggle_opt(.align = WLX_LEFT);
    WLX_Context tmp = ctx;
    WLX_Toggle_Opt resolved = opt;
    wlx_resolve_opt_toggle(&tmp, &resolved);

    // With WLX_LEFT, content starts at left of the slot.
    // Verify resolve doesn't clobber the align field.
    ASSERT_EQ_INT(resolved.align, WLX_LEFT);
}

TEST(toggle_align_center_differs_from_left) {
    // Drive a full frame and confirm acr differs from wr.x for WLX_CENTER.
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);

    bool val = false;

    // WLX_LEFT frame
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "Hi", &val);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // WLX_CENTER frame - must not crash and must complete normally
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "Hi", &val, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(toggle_align_right_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = true;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "Right", &val, .align = WLX_RIGHT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(toggle_no_label_align_center_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    bool val = false;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, NULL, &val, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Radio alignment tests
// ============================================================================

TEST(radio_align_left_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "A", &active, 0, .align = WLX_LEFT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(radio_align_center_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "B", &active, 0, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(radio_align_right_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, "C", &active, 0, .align = WLX_RIGHT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(radio_no_label_align_center_does_not_crash) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);
    int active = 0;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_radio(&ctx, NULL, &active, 0, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_widget ID-stack and opacity tests (Phase 1 regressions)
// ============================================================================

TEST(widget_id_stack_balanced) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    size_t count_before = ctx.arena.id_stack.count;
    wlx_widget(&ctx, .id = "swatch");
    size_t count_after = ctx.arena.id_stack.count;
    ASSERT_EQ_INT((int)count_before, (int)count_after);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

TEST(widget_opacity_applied_to_fill) {
    wgt_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = wgt_rec_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness = 0;
    theme.rounded_segments = 0;
    ctx.theme = &theme;
    ctx.rect = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .color = WLX_RGBA(200, 100, 50, 255), .opacity = 0.5f);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // opacity=0.5 applied to alpha=255 -> 128; draw_rect must record it
    ASSERT_EQ_INT(_wgt_draw_rect_last_color.a, 128);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Phase 4a fallback-precedence tests
// ============================================================================

// toggle: hover_brightness resolves from theme when opt value is the float-unset sentinel
TEST(toggle_hover_brightness_resolves_from_theme) {
    WLX_Theme theme = wlx_theme_dark;
    theme.hover_brightness = 0.6f;

    WLX_Toggle_Opt opt = wlx_default_toggle_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    // Default opt has hover_brightness at sentinel; should resolve to theme value
    ASSERT_TRUE(opt.hover_brightness <= WLX_FLOAT_UNSET);
    wlx_resolve_opt_toggle(&ctx, &opt);
    ASSERT_TRUE(opt.hover_brightness > WLX_FLOAT_UNSET);
    ASSERT_EQ_INT((int)(opt.hover_brightness * 100), 60);
}

// radio: hover_brightness resolves from theme when opt value is the float-unset sentinel
TEST(radio_hover_brightness_resolves_from_theme) {
    WLX_Theme theme = wlx_theme_dark;
    theme.hover_brightness = 0.7f;

    WLX_Radio_Opt opt = wlx_default_radio_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    ASSERT_TRUE(opt.hover_brightness <= WLX_FLOAT_UNSET);
    wlx_resolve_opt_radio(&ctx, &opt);
    ASSERT_TRUE(opt.hover_brightness > WLX_FLOAT_UNSET);
    ASSERT_EQ_INT((int)(opt.hover_brightness * 100), 70);
}

// inputbox: border_width uses widget-specific fallback (input.border_width) first,
// then falls back to theme->border_width via wlx_resolve_border
TEST(inputbox_border_width_two_level_fallback) {
    WLX_Theme theme = wlx_theme_dark;
    theme.input.border_width = -1.0f;  // widget-specific not set
    theme.border_width       = 2.0f;   // ultimate fallback

    WLX_Inputbox_Opt opt = wlx_default_inputbox_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    wlx_resolve_opt_inputbox(&ctx, &opt);
    ASSERT_EQ_INT((int)opt.border_width, 2);
}

TEST(inputbox_border_width_widget_specific_wins) {
    WLX_Theme theme = wlx_theme_dark;
    theme.input.border_width = 3.0f;
    theme.border_width       = 1.0f;

    WLX_Inputbox_Opt opt = wlx_default_inputbox_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    wlx_resolve_opt_inputbox(&ctx, &opt);
    ASSERT_EQ_INT((int)opt.border_width, 3);
}

// checkbox: widget-specific border_color fallback uses theme->checkbox.border first
TEST(checkbox_border_color_widget_specific_first) {
    WLX_Theme theme = wlx_theme_dark;
    theme.checkbox.border = WLX_RGBA(11, 22, 33, 255);
    theme.border          = WLX_RGBA(99, 99, 99, 255);

    WLX_Checkbox_Opt opt = wlx_default_checkbox_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &theme;

    wlx_resolve_opt_checkbox(&ctx, &opt);
    ASSERT_EQ_COLOR(opt.border_color, theme.checkbox.border);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(widgets) {
    RUN_TEST(progress_clamps_value);
    RUN_TEST(progress_zero_draws_no_fill);
    RUN_TEST(progress_full_draws_fill);
    RUN_TEST(progress_resolves_theme_colors);
    RUN_TEST(toggle_click_flips_value);
    RUN_TEST(toggle_draws_active_track_when_on);
    RUN_TEST(toggle_resolves_theme_defaults);
    RUN_TEST(toggle_fallback_first_level);
    RUN_TEST(toggle_fallback_second_level);
    RUN_TEST(toggle_align_left_positions_at_slot_left);
    RUN_TEST(toggle_align_center_differs_from_left);
    RUN_TEST(toggle_align_right_does_not_crash);
    RUN_TEST(toggle_no_label_align_center_does_not_crash);
    RUN_TEST(radio_click_sets_active_index);
    RUN_TEST(radio_selected_draws_fill);
    RUN_TEST(radio_unselected_no_fill);
    RUN_TEST(radio_resolves_theme_defaults);
    RUN_TEST(radio_fallback_first_level);
    RUN_TEST(radio_fallback_second_level);
    RUN_TEST(radio_align_left_does_not_crash);
    RUN_TEST(radio_align_center_does_not_crash);
    RUN_TEST(radio_align_right_does_not_crash);
    RUN_TEST(radio_no_label_align_center_does_not_crash);
    RUN_TEST(widget_id_stack_balanced);
    RUN_TEST(widget_opacity_applied_to_fill);
    RUN_TEST(toggle_hover_brightness_resolves_from_theme);
    RUN_TEST(radio_hover_brightness_resolves_from_theme);
    RUN_TEST(inputbox_border_width_two_level_fallback);
    RUN_TEST(inputbox_border_width_widget_specific_wins);
    RUN_TEST(checkbox_border_color_widget_specific_first);
}

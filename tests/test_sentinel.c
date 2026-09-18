// test_sentinel.c - option default and sentinel contract: explicit zero is
// zero, an unset field resolves to its documented fallback, request tokens
// are not sentinels, and the enumerated v0.9 default changes hold.
// Included from test_main.c (single TU build); the resolvers are reachable
// directly, so most cases pin resolution without running a frame.

// Custom theme whose values differ from every literal default, so an
// inherited value is distinguishable from an explicit zero and from the
// bundled presets.
static WLX_Theme sentinel_theme(void) {
    WLX_Theme t = wlx_theme_dark;
    t.border_width     = 3.0f;
    t.roundness        = 0.5f;
    t.rounded_segments = 8;
    t.hover_brightness = 0.25f;
    t.padding          = 7.0f;
    return t;
}

// ============================================================================
// Rule U: explicit zero is zero, unset inherits
// ============================================================================

TEST(sentinel_explicit_zero_is_zero) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;

    WLX_Label_Opt label = wlx_default_label_opt(.border_width = 0, .roundness = 0,
                                                .rounded_segments = 0);
    wlx_resolve_opt_label(&ctx, &label);
    ASSERT_EQ_F(label.border_width, 0.0f, 0.0001f);
    ASSERT_EQ_F(label.roundness, 0.0f, 0.0001f);
    ASSERT_EQ_INT(label.rounded_segments, 0);

    WLX_Button_Opt button = wlx_default_button_opt(.content_padding = 0);
    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING(&ctx, button);
    ASSERT_EQ_F(rp.top, 0.0f, 0.0001f);
    ASSERT_EQ_F(rp.left, 0.0f, 0.0001f);

    // Layout gap is a literal: 0 means the slots abut, 5 means 5 px apart.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_PX(50)),
                       .padding = 0, .gap = 0);
    WLX_Layout *la = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    WLX_Rect a0 = wlx_get_slot_rect(&ctx, la, -1, 1);
    WLX_Rect a1 = wlx_get_slot_rect(&ctx, la, -1, 1);
    wlx_layout_end(&ctx);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(50), WLX_SLOT_PX(50)),
                       .padding = 0, .gap = 5);
    WLX_Layout *lb = &wlx_pool_layouts(&ctx)[ctx.arena.layouts.count - 1];
    WLX_Rect b0 = wlx_get_slot_rect(&ctx, lb, -1, 1);
    WLX_Rect b1 = wlx_get_slot_rect(&ctx, lb, -1, 1);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_F(a1.y - a0.y, 50.0f, 0.0001f);
    ASSERT_EQ_F(b1.y - b0.y, 55.0f, 0.0001f);
}

TEST(sentinel_unset_inherits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;

    // Omitted fields carry the sentinel through the macro.
    WLX_Label_Opt omitted = wlx_default_label_opt();
    wlx_resolve_opt_label(&ctx, &omitted);
    ASSERT_EQ_F(omitted.border_width, 3.0f, 0.0001f);
    ASSERT_EQ_F(omitted.roundness, 0.5f, 0.0001f);
    ASSERT_EQ_INT(omitted.rounded_segments, 8);

    // The sentinel written explicitly means the same as omission.
    WLX_Label_Opt explicit_unset = wlx_default_label_opt(.border_width = WLX_UNSET,
                                                         .roundness = WLX_UNSET,
                                                         .rounded_segments = WLX_UNSET);
    wlx_resolve_opt_label(&ctx, &explicit_unset);
    ASSERT_EQ_F(explicit_unset.border_width, 3.0f, 0.0001f);
    ASSERT_EQ_F(explicit_unset.roundness, 0.5f, 0.0001f);
    ASSERT_EQ_INT(explicit_unset.rounded_segments, 8);

    // Leaf-widget content padding: unset resolves to 0, not to the theme.
    WLX_Button_Opt button = wlx_default_button_opt();
    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING(&ctx, button);
    ASSERT_EQ_F(rp.top, 0.0f, 0.0001f);
    ASSERT_EQ_F(rp.right, 0.0f, 0.0001f);
}

// Brightness shifts are the one signed-domain Rule U family: -1 is the
// sentinel, any value above it (negative included) is explicit.
TEST(sentinel_brightness_domain) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;

    WLX_Slider_Opt omitted = wlx_default_slider_opt();
    wlx_resolve_opt_slider(&ctx, &omitted);
    ASSERT_EQ_F(omitted.hover_brightness, 0.25f, 0.0001f);
    ASSERT_EQ_F(omitted.thumb_hover_brightness, 0.125f, 0.0001f);

    WLX_Slider_Opt minus_one = wlx_default_slider_opt(.hover_brightness = WLX_UNSET,
                                                      .thumb_hover_brightness = -1.0f);
    wlx_resolve_opt_slider(&ctx, &minus_one);
    ASSERT_EQ_F(minus_one.hover_brightness, 0.25f, 0.0001f);
    ASSERT_EQ_F(minus_one.thumb_hover_brightness, 0.125f, 0.0001f);

    WLX_Slider_Opt legacy = wlx_default_slider_opt(.hover_brightness = WLX_FLOAT_UNSET);
    wlx_resolve_opt_slider(&ctx, &legacy);
    ASSERT_EQ_F(legacy.hover_brightness, 0.25f, 0.0001f);

    WLX_Slider_Opt darken = wlx_default_slider_opt(.hover_brightness = -0.5f);
    wlx_resolve_opt_slider(&ctx, &darken);
    ASSERT_EQ_F(darken.hover_brightness, -0.5f, 0.0001f);

    WLX_Slider_Opt none = wlx_default_slider_opt(.hover_brightness = 0.0f);
    wlx_resolve_opt_slider(&ctx, &none);
    ASSERT_EQ_F(none.hover_brightness, 0.0f, 0.0001f);
}

// WLX_PADDING_USE_THEME is a request token, distinct from unset.
TEST(sentinel_padding_theme_token) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;

    WLX_Button_Opt button = wlx_default_button_opt(.content_padding = WLX_PADDING_USE_THEME);
    WLX_Resolved_Padding rp = WLX_RESOLVE_CONTENT_PADDING(&ctx, button);
    ASSERT_EQ_F(rp.top, 7.0f, 0.0001f);
    ASSERT_EQ_F(rp.bottom, 7.0f, 0.0001f);

    // A per-side value wins over the token; an unset side falls back to it.
    WLX_Button_Opt mixed = wlx_default_button_opt(.content_padding = WLX_PADDING_USE_THEME,
                                                  .content_padding_left = 1.0f);
    rp = WLX_RESOLVE_CONTENT_PADDING(&ctx, mixed);
    ASSERT_EQ_F(rp.left, 1.0f, 0.0001f);
    ASSERT_EQ_F(rp.right, 7.0f, 0.0001f);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(sentinel) {
    RUN_TEST(sentinel_explicit_zero_is_zero);
    RUN_TEST(sentinel_unset_inherits);
    RUN_TEST(sentinel_brightness_domain);
    RUN_TEST(sentinel_padding_theme_token);
}

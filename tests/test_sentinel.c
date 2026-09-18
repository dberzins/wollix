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
    wlx_context_destroy(&ctx);
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
// The v0.9 enumerated default changes
// ============================================================================

// Panel chrome inherits theme roundness like every widget; explicit 0 stays
// sharp. Observed through the rounded-rect recorder of test_rounding.c.
TEST(panel_roundness_inherits_theme) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend = rec_backend();
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;
    WLX_Color fill = WLX_RGBA(40, 40, 40, 255);

    rec_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_panel_begin(&ctx, .back_color = fill);
    wlx_panel_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(_rec_draw_rect_rounded_count >= 1);

    rec_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_panel_begin(&ctx, .back_color = fill, .roundness = 0);
    wlx_panel_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(_rec_draw_rect_rounded_count, 0);
    wlx_context_destroy(&ctx);
}

// The layout at the top of the pool, and the nearest enclosing HORZ one.
static WLX_Layout *sentinel_top_layout(WLX_Context *ctx) {
    return &wlx_pool_layouts(ctx)[ctx->arena.layouts.count - 1];
}
static WLX_Layout *sentinel_horz_layout(WLX_Context *ctx) {
    for (size_t i = ctx->arena.layouts.count; i > 0; i--) {
        WLX_Layout *l = &wlx_pool_layouts(ctx)[i - 1];
        if (wlx_layout_is_horz(l)) return l;
    }
    return NULL;
}

// Split gap is a literal like every container gap: omission and 0 agree.
TEST(split_gap_literal_zero) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_split_begin(&ctx);
    float omitted = sentinel_horz_layout(&ctx)->gap;
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_split_begin(&ctx, .gap = 0);
    float zero = sentinel_horz_layout(&ctx)->gap;
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_split_begin(&ctx, .gap = 5);
    float five = sentinel_horz_layout(&ctx)->gap;
    test_frame_end(&ctx);

    ASSERT_EQ_F(omitted, 0.0f, 0.0001f);
    ASSERT_EQ_F(zero, 0.0f, 0.0001f);
    ASSERT_EQ_F(five, 5.0f, 0.0001f);
    wlx_context_destroy(&ctx);
}

// Menu width follows Rule U: unset resolves to the default, 0 is 0.
TEST(menu_width_rule_u) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Theme theme = sentinel_theme();
    ctx.theme = &theme;

    WLX_Menu_Opt omitted = wlx_default_menu_opt();
    wlx_resolve_opt_menu(&ctx, &omitted);
    ASSERT_EQ_F(omitted.width, 180.0f, 0.0001f);

    WLX_Menu_Opt zero = wlx_default_menu_opt(.width = 0);
    wlx_resolve_opt_menu(&ctx, &zero);
    ASSERT_EQ_F(zero.width, 0.0f, 0.0001f);

    WLX_Menu_Opt explicit_unset = wlx_default_menu_opt(.width = WLX_UNSET);
    wlx_resolve_opt_menu(&ctx, &explicit_unset);
    ASSERT_EQ_F(explicit_unset.width, 180.0f, 0.0001f);
}

// Text recorder: x of the last text drawn in a frame.
static float _sentinel_last_text_x = 0.0f;
static void sentinel_rec_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)y; (void)style;
    _sentinel_last_text_x = x;
}

static float sentinel_panel_title_x(WLX_Context *ctx, WLX_Panel_Opt opt) {
    test_frame_begin(ctx, 0, 0, false, false);
    wlx_panel_begin_impl(ctx, opt, __FILE__, __LINE__);
    wlx_panel_end(ctx);
    test_frame_end(ctx);
    return _sentinel_last_text_x;
}

// An explicit WLX_ALIGN_NONE on the panel title is honoured (raw placement,
// the same x as WLX_LEFT); the centred default lives in the macro.
TEST(panel_title_align_none_honoured) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = sentinel_rec_text;

    float centred = sentinel_panel_title_x(&ctx, wlx_default_panel_opt(.title = "T"));
    float left    = sentinel_panel_title_x(&ctx, wlx_default_panel_opt(.title = "T",
                                                     .title_align = WLX_LEFT));
    float none    = sentinel_panel_title_x(&ctx, wlx_default_panel_opt(.title = "T",
                                                     .title_align = WLX_ALIGN_NONE));
    ASSERT_TRUE(centred > left);
    ASSERT_EQ_F(none, left, 0.0001f);
    wlx_context_destroy(&ctx);
}

// An explicit WLX_SLOT_AUTO split size reaches the inner layout as AUTO
// (weight 1 beside the FLEX(1) second pane: two equal panes), instead of
// being mistaken for the omitted 280 px default.
TEST(split_explicit_auto_honoured) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_split_begin(&ctx);
    WLX_Layout *h = sentinel_horz_layout(&ctx);
    WLX_Rect d0 = wlx_get_slot_rect(&ctx, h, 0, 1);
    test_frame_end(&ctx);
    ASSERT_EQ_F(d0.w, 280.0f, 0.0001f);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_split_begin(&ctx, .first_size = WLX_SLOT_AUTO);
    h = sentinel_horz_layout(&ctx);
    WLX_Rect a0 = wlx_get_slot_rect(&ctx, h, 0, 1);
    WLX_Rect a1 = wlx_get_slot_rect(&ctx, h, 1, 1);
    test_frame_end(&ctx);
    ASSERT_EQ_F(a0.w, a1.w, 0.0001f);
    ASSERT_TRUE(a0.w > 280.0f);
    wlx_context_destroy(&ctx);
}

// On a compound widget the uniform content padding written as WLX_UNSET
// means the widget default, exactly like omission (panel: 2 px).
TEST(content_padding_unset_equals_omission) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_panel_begin(&ctx);
    float omitted_x = sentinel_top_layout(&ctx)->rect.x;
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_panel_begin(&ctx, .content_padding = WLX_UNSET);
    float unset_x = sentinel_top_layout(&ctx)->rect.x;
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_panel_begin(&ctx, .content_padding = 0);
    float zero_x = sentinel_top_layout(&ctx)->rect.x;
    test_frame_end(&ctx);

    ASSERT_EQ_F(omitted_x, 2.0f, 0.0001f);
    ASSERT_EQ_F(unset_x, 2.0f, 0.0001f);
    ASSERT_EQ_F(zero_x, 0.0f, 0.0001f);
    wlx_context_destroy(&ctx);
}

// Tooltip fixture: fixed 1 s frames so the default 0.5 s delay elapses on
// the second hovered frame; the tip background is the last rect replayed.
static float sentinel_tt_frame_time(void) { return 1.0f; }
static WLX_Rect _sentinel_last_rect;
static void sentinel_rec_rect(WLX_Rect r, WLX_Color c) { (void)c; _sentinel_last_rect = r; }

static float sentinel_tip_width(WLX_Context *ctx, WLX_Tooltip_Opt opt) {
    for (int i = 0; i < 2; i++) {
        test_frame_begin(ctx, 50, 20, false, false);
        wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
        wlx_button(ctx, "anchor", .height = 40);
        wlx_tooltip_for_impl(ctx, (WLX_Rect){ 0, 0, 400, 40 }, "hint", opt, __FILE__, __LINE__);
        wlx_layout_end(ctx);
        test_frame_end(ctx);
    }
    return _sentinel_last_rect.w;
}

// Tooltip `padding` is renamed `content_padding` (the slot-inset name was
// wrong for an inner inset); the old name aliases the same storage for one
// minor version. Unset resolves to the tooltip's 6 px.
TEST(tooltip_content_padding_alias) {
    ASSERT_TRUE(offsetof(WLX_Tooltip_Opt, padding) == offsetof(WLX_Tooltip_Opt, content_padding));

    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.get_frame_time = sentinel_tt_frame_time;
    ctx.backend.draw_rect = sentinel_rec_rect;
    float text_w = 0.0f, text_h = 0.0f;
    mock_measure_text("hint", (WLX_Text_Style){ .font_size = wlx_theme_dark.font_size },
                      &text_w, &text_h);

    // Pointer parked off the anchor first so the hover timer starts fresh.
    test_frame_begin(&ctx, 300, 200, false, false);
    test_frame_end(&ctx);
    ASSERT_EQ_F(sentinel_tip_width(&ctx, wlx_default_tooltip_opt()), text_w + 12.0f, 0.001f);
    ASSERT_EQ_F(sentinel_tip_width(&ctx, wlx_default_tooltip_opt(.content_padding = 0)),
                text_w, 0.001f);
    ASSERT_EQ_F(sentinel_tip_width(&ctx, wlx_default_tooltip_opt(.padding = 2)),
                text_w + 4.0f, 0.001f);
    ASSERT_EQ_F(sentinel_tip_width(&ctx, wlx_default_tooltip_opt(.content_padding = WLX_UNSET)),
                text_w + 12.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Align aliases: .content_align / .align, .slot_align / .widget_align
// ============================================================================

// The renamed fields share storage with their pre-0.9 names (anonymous
// unions), so old and new spellings place identically.
TEST(align_aliases_share_storage) {
    ASSERT_TRUE(offsetof(WLX_Label_Opt, align) == offsetof(WLX_Label_Opt, content_align));
    ASSERT_TRUE(offsetof(WLX_Label_Opt, widget_align) == offsetof(WLX_Label_Opt, slot_align));
    ASSERT_TRUE(offsetof(WLX_Button_Opt, align) == offsetof(WLX_Button_Opt, content_align));
    ASSERT_TRUE(offsetof(WLX_Image_Opt, align) == offsetof(WLX_Image_Opt, content_align));
    ASSERT_TRUE(offsetof(WLX_Editor_Opt, align) == offsetof(WLX_Editor_Opt, content_align));
    ASSERT_TRUE(offsetof(WLX_Editor_Opt, widget_align) == offsetof(WLX_Editor_Opt, slot_align));

    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = sentinel_rec_text;
    ctx.backend.draw_rect = sentinel_rec_rect;

    float x_new, x_old;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(&ctx, "right", .content_align = WLX_RIGHT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    x_new = _sentinel_last_text_x;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(&ctx, "right", .align = WLX_RIGHT);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    x_old = _sentinel_last_text_x;
    ASSERT_EQ_F(x_new, x_old, 0.0001f);
    ASSERT_TRUE(x_new > 200.0f);

    WLX_Rect r_new, r_old;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_button(&ctx, "b", .slot_align = WLX_CENTER, .width = 50);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    r_new = _sentinel_last_rect;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_button(&ctx, "b", .widget_align = WLX_CENTER, .width = 50);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    r_old = _sentinel_last_rect;
    ASSERT_EQ_F(r_new.x, r_old.x, 0.0001f);
    ASSERT_EQ_F(r_new.w, 50.0f, 0.0001f);
    ASSERT_EQ_F(r_new.x, 175.0f, 0.0001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(sentinel) {
    RUN_TEST(sentinel_explicit_zero_is_zero);
    RUN_TEST(sentinel_unset_inherits);
    RUN_TEST(sentinel_brightness_domain);
    RUN_TEST(sentinel_padding_theme_token);
    RUN_TEST(panel_roundness_inherits_theme);
    RUN_TEST(split_gap_literal_zero);
    RUN_TEST(menu_width_rule_u);
    RUN_TEST(panel_title_align_none_honoured);
    RUN_TEST(split_explicit_auto_honoured);
    RUN_TEST(content_padding_unset_equals_omission);
    RUN_TEST(tooltip_content_padding_alias);
    RUN_TEST(align_aliases_share_storage);
}

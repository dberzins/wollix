// test_padding_alignment.c - cross-widget invariants for the unified
// WLX_CONTENT_PADDING_FIELDS shape. Asserts that every chrome widget
// (label, button, split, panel, inputbox) consumes per-side padding
// through wlx_resolve_content_padding with consistent defaults and
// theme opt-in semantics.

static WLX_Theme align_theme_with_padding(float padding) {
    WLX_Theme t = wlx_theme_dark;
    t.padding = padding;
    return t;
}

// ----- Default-value invariants on opt structs ------------------------------

TEST(padding_alignment_label_defaults_sentinel) {
    WLX_Label_Opt opt = wlx_default_label_opt();
    ASSERT_EQ_F(-1.0f, opt.content_padding, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_top, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_right, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_bottom, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_left, 0.001f);
}

TEST(padding_alignment_button_defaults_sentinel) {
    WLX_Button_Opt opt = wlx_default_button_opt();
    ASSERT_EQ_F(-1.0f, opt.content_padding, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_top, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_right, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_bottom, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_left, 0.001f);
}

TEST(padding_alignment_split_default_uniform_4) {
    WLX_Split_Opt opt = wlx_default_split_opt();
    ASSERT_EQ_F(4.0f, opt.content_padding, 0.001f);
}

TEST(padding_alignment_panel_default_uniform_2) {
    WLX_Panel_Opt opt = wlx_default_panel_opt();
    ASSERT_EQ_F(2.0f, opt.content_padding, 0.001f);
}

TEST(padding_alignment_inputbox_default_uniform_10) {
    WLX_Inputbox_Opt opt = wlx_default_inputbox_opt();
    ASSERT_EQ_F(10.0f, opt.content_padding, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_top, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_right, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_bottom, 0.001f);
    ASSERT_EQ_F(-1.0f, opt.content_padding_left, 0.001f);
}

// ----- Resolver invariants --------------------------------------------------
//
// Each widget routes through wlx_resolve_content_padding with a shared theme,
// so the same input sentinel produces the same per-side resolution.

TEST(padding_alignment_uniform_resolves_to_all_sides) {
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(
        NULL, 12.0f, -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_EQ_F(12.0f, rp.top,    0.001f);
    ASSERT_EQ_F(12.0f, rp.right,  0.001f);
    ASSERT_EQ_F(12.0f, rp.bottom, 0.001f);
    ASSERT_EQ_F(12.0f, rp.left,   0.001f);
}

TEST(padding_alignment_per_side_overrides_uniform) {
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(
        NULL, 8.0f, 0.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_EQ_F(0.0f, rp.top,    0.001f);
    ASSERT_EQ_F(8.0f, rp.right,  0.001f);
    ASSERT_EQ_F(8.0f, rp.bottom, 0.001f);
    ASSERT_EQ_F(8.0f, rp.left,   0.001f);
}

TEST(padding_alignment_theme_opt_in_uses_theme_padding) {
    WLX_Theme t = align_theme_with_padding(6.0f);
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(
        &t, WLX_PADDING_USE_THEME, -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_EQ_F(6.0f, rp.top,    0.001f);
    ASSERT_EQ_F(6.0f, rp.right,  0.001f);
    ASSERT_EQ_F(6.0f, rp.bottom, 0.001f);
    ASSERT_EQ_F(6.0f, rp.left,   0.001f);
}

TEST(padding_alignment_default_sentinel_resolves_to_zero) {
    WLX_Resolved_Padding rp = wlx_resolve_content_padding(
        NULL, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_EQ_F(0.0f, rp.top,    0.001f);
    ASSERT_EQ_F(0.0f, rp.right,  0.001f);
    ASSERT_EQ_F(0.0f, rp.bottom, 0.001f);
    ASSERT_EQ_F(0.0f, rp.left,   0.001f);
}

// ----- Clamp invariants -----------------------------------------------------

TEST(padding_alignment_clamp_scales_oversized_horizontal_pair) {
    WLX_Resolved_Padding rp = { .top = 0, .right = 12, .bottom = 0, .left = 12 };
    wlx_clamp_resolved_padding(&rp, 20.0f, 100.0f);
    ASSERT_TRUE(rp.left + rp.right <= 20.0f + 0.001f);
    ASSERT_TRUE(rp.left  >= 0.0f);
    ASSERT_TRUE(rp.right >= 0.0f);
}

TEST(padding_alignment_clamp_scales_oversized_vertical_pair) {
    WLX_Resolved_Padding rp = { .top = 20, .right = 0, .bottom = 30, .left = 0 };
    wlx_clamp_resolved_padding(&rp, 100.0f, 10.0f);
    ASSERT_TRUE(rp.top + rp.bottom <= 10.0f + 0.001f);
    ASSERT_TRUE(rp.top    >= 0.0f);
    ASSERT_TRUE(rp.bottom >= 0.0f);
}

TEST(padding_alignment_clamp_preserves_in_bounds_values) {
    WLX_Resolved_Padding rp = { .top = 4, .right = 6, .bottom = 4, .left = 6 };
    wlx_clamp_resolved_padding(&rp, 100.0f, 100.0f);
    ASSERT_EQ_F(4.0f, rp.top,    0.001f);
    ASSERT_EQ_F(6.0f, rp.right,  0.001f);
    ASSERT_EQ_F(4.0f, rp.bottom, 0.001f);
    ASSERT_EQ_F(6.0f, rp.left,   0.001f);
}

SUITE(padding_alignment) {
    RUN_TEST(padding_alignment_label_defaults_sentinel);
    RUN_TEST(padding_alignment_button_defaults_sentinel);
    RUN_TEST(padding_alignment_split_default_uniform_4);
    RUN_TEST(padding_alignment_panel_default_uniform_2);
    RUN_TEST(padding_alignment_inputbox_default_uniform_10);
    RUN_TEST(padding_alignment_uniform_resolves_to_all_sides);
    RUN_TEST(padding_alignment_per_side_overrides_uniform);
    RUN_TEST(padding_alignment_theme_opt_in_uses_theme_padding);
    RUN_TEST(padding_alignment_default_sentinel_resolves_to_zero);
    RUN_TEST(padding_alignment_clamp_scales_oversized_horizontal_pair);
    RUN_TEST(padding_alignment_clamp_scales_oversized_vertical_pair);
    RUN_TEST(padding_alignment_clamp_preserves_in_bounds_values);
}

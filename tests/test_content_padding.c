// test_content_padding.c - tests for the new content_padding* fields on
// WLX_Label_Opt and WLX_Button_Opt. The inset shrinks only the content
// (text + image) rect; chrome (background, border) and the hit rect stay
// at the full widget rect.
//
// Cases:
//   1. Default opts            - no padding applied (content_rect == wr).
//   2. Uniform .content_padding = 12 - all sides inset by 12.
//   3. Asymmetric per-side     - .content_padding_top/right/bottom/left.
//   4. Uniform + per-side override - side override beats uniform.
//   5. Theme opt-in            - .content_padding = WLX_PADDING_USE_THEME.
//   6. Clamp on tight rect     - left+right > rect.w -> scale, never negative.
//   7. Chrome unchanged        - draw_rect still receives wr.
//   8. Hit-rect unchanged      - click between wr and content_rect still hits.
//   9. Image-only path         - image dst inset by padding.
//  10. Image + text path       - both image and text rects computed in content_rect.
//  11. Slot + content interact - slot padding (outer) and content padding (inner) compose.
//
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs: capture texture, text, and rect draws.
// ============================================================================

static int       _cp_tex_count;
static WLX_Rect  _cp_tex_dst;
static int       _cp_text_count;
static float     _cp_text_x;
static float     _cp_text_y;
static int       _cp_rect_count;
static WLX_Rect  _cp_last_rect;

static void cp_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex; (void)src; (void)tint;
    _cp_tex_count++;
    _cp_tex_dst = dst;
}

static void cp_rec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)style;
    _cp_text_count++;
    _cp_text_x = x;
    _cp_text_y = y;
}

static void cp_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)c;
    _cp_rect_count++;
    _cp_last_rect = r;
}

static void cp_rec_reset(void) {
    _cp_tex_count   = 0;
    _cp_tex_dst     = (WLX_Rect){0};
    _cp_text_count  = 0;
    _cp_text_x      = 0;
    _cp_text_y      = 0;
    _cp_rect_count  = 0;
    _cp_last_rect   = (WLX_Rect){0};
}

static WLX_Backend cp_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture = cp_rec_draw_texture;
    b.draw_text    = cp_rec_draw_text;
    b.draw_rect    = cp_rec_draw_rect;
    return b;
}

// ============================================================================
// Test fixtures
// ============================================================================

static WLX_Texture cp_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

static void cp_ctx_init(WLX_Context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = cp_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, 200, 200);
}

// Begin a frame on a 200x200 root, single vertical slot, mouse out.
static void cp_begin(WLX_Context *ctx) {
    cp_rec_reset();
    cp_ctx_init(ctx);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void cp_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// Custom theme with non-zero padding so the WLX_PADDING_USE_THEME path is
// observable. Cloned from wlx_theme_dark at runtime.
static WLX_Theme cp_theme_padded(void) {
    WLX_Theme t = wlx_theme_dark;
    t.padding = 6.0f;
    return t;
}

// ============================================================================
// 1. Default: content_padding fields all sentinel -> resolved zero -> content
//    rect equals wr.
// ============================================================================

TEST(content_padding_label_default_zero_inset) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_label(&ctx, "", .texture = cp_make_texture(50, 50),
              .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    cp_end(&ctx);

    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(content_padding_button_default_zero_inset) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    cp_end(&ctx);

    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// 2. Uniform .content_padding = 12.
// ============================================================================

TEST(content_padding_uniform_insets_all_sides_equally) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding = 12);
    cp_end(&ctx);

    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){12, 12, 176, 176}), 0.001f);
}

// ============================================================================
// 3. Asymmetric per-side: top=4, right=16, bottom=4, left=16.
// ============================================================================

TEST(content_padding_asymmetric_per_side) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding_top = 4,
               .content_padding_right = 16,
               .content_padding_bottom = 4,
               .content_padding_left = 16);
    cp_end(&ctx);

    // wr = (0, 0, 200, 200); insets {4, 16, 4, 16} -> (16, 4, 168, 192).
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){16, 4, 168, 192}), 0.001f);
}

// ============================================================================
// 4. Uniform + per-side override: content_padding = 8 with top=0 means
//    top = 0, others = 8.
// ============================================================================

TEST(content_padding_per_side_overrides_uniform) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding = 8,
               .content_padding_top = 0);
    cp_end(&ctx);

    // insets {0, 8, 8, 8} -> (8, 0, 184, 192).
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){8, 0, 184, 192}), 0.001f);
}

// ============================================================================
// 5. Theme opt-in: content_padding = WLX_PADDING_USE_THEME resolves to the
//    theme's `padding` knob (6 in our custom theme).
// ============================================================================

TEST(content_padding_theme_opt_in_uses_theme_padding) {
    WLX_Context ctx;
    cp_rec_reset();
    cp_ctx_init(&ctx);
    WLX_Theme custom = cp_theme_padded();
    ctx.theme = &custom;
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding = WLX_PADDING_USE_THEME);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // theme.padding = 6 -> insets {6, 6, 6, 6} -> (6, 6, 188, 188).
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){6, 6, 188, 188}), 0.001f);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 6. Clamp: a 20px-wide cell with content_padding_left = 12, _right = 12
//    asks for 24 px of horizontal padding. Resolver clamps to fit.
// ============================================================================

TEST(content_padding_clamp_scales_oversized_pair) {
    WLX_Context ctx;
    cp_rec_reset();
    cp_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin_s(&ctx, WLX_HORZ, WLX_SIZES(WLX_SLOT_PX(20), WLX_SLOT_FLEX(1)));
    wlx_button(&ctx, "", .texture = cp_make_texture(20, 200),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding_left = 12,
               .content_padding_right = 12);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // wr = (0, 0, 20, 200). left+right = 24 > 20 -> scale by 20/24:
    //   left = 10, right = 10 -> content_rect = (10, 0, 0, 200).
    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_F(_cp_tex_dst.x, 10.0f, 0.001f);
    ASSERT_EQ_F(_cp_tex_dst.w, 0.0f, 0.001f);
    ASSERT_TRUE(_cp_tex_dst.w >= 0.0f);
    ASSERT_EQ_F(_cp_tex_dst.h, 200.0f, 0.001f);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 7. Chrome unchanged: draw_rect (background fill) still receives wr even
//    when content padding is non-zero. Tested on both label and button.
// ============================================================================

TEST(content_padding_label_chrome_rect_equals_wr) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_label(&ctx, "", .show_background = true,
              .back_color = WLX_RGBA(40, 40, 40, 255),
              .content_padding = 20);
    cp_end(&ctx);

    ASSERT_TRUE(_cp_rect_count >= 1);
    ASSERT_EQ_RECT(_cp_last_rect, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(content_padding_button_chrome_rect_equals_wr) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .content_padding = 20);
    cp_end(&ctx);

    // Button always fills (theme surface). Even with content_padding=20 the
    // chrome rect must still equal wr.
    ASSERT_TRUE(_cp_rect_count >= 1);
    ASSERT_EQ_RECT(_cp_last_rect, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// 8. Hit-rect unchanged: a click at (10, 10) on a 200x200 button with
//    content_padding = 40 is inside wr but outside content_rect (40..160).
//    The button must still report clicked. Both frames must call
//    wlx_button from the same source line so the widget ID is stable.
// ============================================================================

static bool cp_padded_button(WLX_Context *ctx) {
    wlx_layout_begin(ctx, 1, WLX_VERT);
    bool clicked = wlx_button(ctx, "Click", .content_padding = 40);
    wlx_layout_end(ctx);
    return clicked;
}

TEST(content_padding_hit_rect_covers_full_wr) {
    WLX_Context ctx;
    cp_ctx_init(&ctx);

    // Frame 1: press at (10, 10) - in wr, outside content_rect.
    cp_rec_reset();
    test_frame_begin(&ctx, 10, 10, true, true);
    bool press = cp_padded_button(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(press);

    // Frame 2: release at the same position. Click fires because the
    // interaction call uses wr, not content_rect.
    cp_rec_reset();
    test_frame_begin(&ctx, 10, 10, false, false);
    bool release = cp_padded_button(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(release);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 9. Image-only path: button with texture + no text + content_padding = 20.
//    Image-only branch uses content_rect as the target when image_size is
//    unset; STRETCH fills the full target.
// ============================================================================

TEST(content_padding_image_only_dst_inset_by_padding) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .content_padding = 20);
    cp_end(&ctx);

    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){20, 20, 160, 160}), 0.001f);
}

// ============================================================================
// 10. Image + text path: with image_size=50, gap=10, font_size=16,
//     text="Save" (32 wide), placement=LEFT, content_padding=10.
//     content_rect = (10, 10, 180, 180); block = (10, 75, 92, 50);
//     image_rect = (10, 75, 50, 50); FIT into 50x50 src -> dst == image_rect.
//     Text x then anchors at block.x + image_size + gap = 10 + 50 + 10 = 70.
// ============================================================================

TEST(content_padding_image_plus_text_uses_content_rect) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = cp_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16,
               .image_placement = WLX_IMAGE_PLACEMENT_LEFT,
               .content_padding = 10);
    cp_end(&ctx);

    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){10, 75, 50, 50}), 0.001f);
    ASSERT_TRUE(_cp_text_count >= 1);
    // Text draw x-origin reflects the padded image+gap offset.
    ASSERT_EQ_F(_cp_text_x, 70.0f, 0.001f);
}

// ============================================================================
// 11. Slot + content padding interact:
//     slot .padding = 4 shrinks the cell to (4, 4, 192, 192);
//     content_padding = 8 further insets content to (12, 12, 176, 176).
// ============================================================================

TEST(content_padding_composes_with_slot_padding) {
    WLX_Context ctx;
    cp_begin(&ctx);
    wlx_button(&ctx, "", .texture = cp_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .padding = 4,            // outer (slot) padding
               .content_padding = 8);   // inner (content) padding
    cp_end(&ctx);

    ASSERT_EQ_INT(_cp_tex_count, 1);
    ASSERT_EQ_RECT(_cp_tex_dst, ((WLX_Rect){12, 12, 176, 176}), 0.001f);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(content_padding) {
    RUN_TEST(content_padding_label_default_zero_inset);
    RUN_TEST(content_padding_button_default_zero_inset);
    RUN_TEST(content_padding_uniform_insets_all_sides_equally);
    RUN_TEST(content_padding_asymmetric_per_side);
    RUN_TEST(content_padding_per_side_overrides_uniform);
    RUN_TEST(content_padding_theme_opt_in_uses_theme_padding);
    RUN_TEST(content_padding_clamp_scales_oversized_pair);
    RUN_TEST(content_padding_label_chrome_rect_equals_wr);
    RUN_TEST(content_padding_button_chrome_rect_equals_wr);
    RUN_TEST(content_padding_hit_rect_covers_full_wr);
    RUN_TEST(content_padding_image_only_dst_inset_by_padding);
    RUN_TEST(content_padding_image_plus_text_uses_content_rect);
    RUN_TEST(content_padding_composes_with_slot_padding);
}

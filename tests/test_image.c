// test_image.c - tests for wlx_image: scale modes, alignment anchors, src
// defaults, tint resolution, and opacity-stack folding.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording draw_texture stub
// ============================================================================

static int       _img_draw_count;
static WLX_Rect  _img_last_src;
static WLX_Rect  _img_last_dst;
static WLX_Color _img_last_tint;
static int       _img_last_tex_w;
static int       _img_last_tex_h;

static void img_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    _img_draw_count++;
    _img_last_src    = src;
    _img_last_dst    = dst;
    _img_last_tint   = tint;
    _img_last_tex_w  = tex.width;
    _img_last_tex_h  = tex.height;
}

static void img_rec_reset(void) {
    _img_draw_count = 0;
    _img_last_src   = (WLX_Rect){0};
    _img_last_dst   = (WLX_Rect){0};
    _img_last_tint  = (WLX_Color){0};
    _img_last_tex_w = 0;
    _img_last_tex_h = 0;
}

static WLX_Backend img_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture = img_rec_draw_texture;
    return b;
}

// ============================================================================
// Test fixture
// ============================================================================

static WLX_Texture img_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

// Begin a frame on a square 200x200 root and open a single-slot vertical
// layout. The widget rect equals the root rect.
static void img_begin_square(WLX_Context *ctx) {
    img_rec_reset();
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = img_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, 200, 200);
    test_frame_begin(ctx, 0, 0, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void img_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// ============================================================================
// STRETCH + default src
// ============================================================================

TEST(image_stretch_dst_matches_widget_rect) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(64, 32));
    img_end(&ctx);

    ASSERT_EQ_INT(_img_draw_count, 1);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(image_default_src_covers_full_texture) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(64, 32));
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 64, 32}), 0.001f);
}

TEST(image_custom_src_subrect_passed_through) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(128, 128),
        .src = { 10, 20, 30, 40 });
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){10, 20, 30, 40}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// NONE alignment
// ============================================================================

TEST(image_none_top_left) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50),
        .scale = WLX_IMAGE_SCALE_NONE, .align = WLX_TOP_LEFT);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 50, 50}), 0.001f);
}

TEST(image_none_center) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50),
        .scale = WLX_IMAGE_SCALE_NONE, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){75, 75, 50, 50}), 0.001f);
}

TEST(image_none_bottom_right) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50),
        .scale = WLX_IMAGE_SCALE_NONE, .align = WLX_BOTTOM_RIGHT);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){150, 150, 50, 50}), 0.001f);
}

// ============================================================================
// FIT preserves aspect, letterboxes inside widget rect
// ============================================================================

TEST(image_fit_landscape_source_pillarboxes_vertically) {
    // 100x50 source, 200x200 widget: scale 2x to (200, 100), centered.
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 50),
        .scale = WLX_IMAGE_SCALE_FIT, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 100, 50}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 50, 200, 100}), 0.001f);
}

TEST(image_fit_portrait_source_letterboxes_horizontally) {
    // 50x100 source, 200x200 widget: scale 2x to (100, 200), centered.
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 100),
        .scale = WLX_IMAGE_SCALE_FIT, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 50, 100}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){50, 0, 100, 200}), 0.001f);
}

TEST(image_fit_align_top_left_anchors_to_origin) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 50),
        .scale = WLX_IMAGE_SCALE_FIT, .align = WLX_TOP_LEFT);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 100}), 0.001f);
}

// ============================================================================
// FILL crops src rect to widget aspect; dst always equals widget rect
// ============================================================================

TEST(image_fill_landscape_source_horizontal_crop_center) {
    // 200x100 source (aspect 2:1), 200x200 widget (aspect 1:1).
    // Narrow s.w to 100, anchor center -> src.x = 50.
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(200, 100),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){50, 0, 100, 100}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(image_fill_landscape_horizontal_crop_left_anchor) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(200, 100),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_LEFT);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 100, 100}), 0.001f);
}

TEST(image_fill_landscape_horizontal_crop_right_anchor) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(200, 100),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_RIGHT);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){100, 0, 100, 100}), 0.001f);
}

TEST(image_fill_portrait_source_vertical_crop_center) {
    // 100x200 source (aspect 1:2), 200x200 widget (aspect 1:1).
    // Narrow s.h to 100, anchor center -> src.y = 50.
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 200),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 50, 100, 100}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(image_fill_portrait_vertical_crop_top_anchor) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 200),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_TOP);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 100, 100}), 0.001f);
}

TEST(image_fill_portrait_vertical_crop_bottom_anchor) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 200),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_BOTTOM);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 100, 100, 100}), 0.001f);
}

TEST(image_fill_matching_aspect_no_crop) {
    // 100x100 source (aspect 1:1), 200x200 widget: src untouched.
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(100, 100),
        .scale = WLX_IMAGE_SCALE_FILL, .align = WLX_CENTER);
    img_end(&ctx);

    ASSERT_EQ_RECT(_img_last_src, ((WLX_Rect){0, 0, 100, 100}), 0.001f);
    ASSERT_EQ_RECT(_img_last_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// Empty / unloaded texture: no draw, no crash, frame still closes
// ============================================================================

TEST(image_empty_texture_emits_no_draw) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(0, 0));
    img_end(&ctx);

    ASSERT_EQ_INT(_img_draw_count, 0);
}

TEST(image_empty_texture_does_not_break_subsequent_widgets) {
    // Two-slot layout: empty image in slot 0, real image in slot 1.
    // The frame must close cleanly so the second widget gets a valid slot.
    img_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = img_rec_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_image(&ctx, img_make_texture(0, 0));
    wlx_image(&ctx, img_make_texture(50, 50));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_img_draw_count, 1);
    // Slot 1 is the bottom half of the root: y in [100, 200).
    ASSERT_EQ_F(_img_last_dst.y, 100.0f, 0.001f);
    ASSERT_EQ_F(_img_last_dst.h, 100.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Tint + opacity resolution
// ============================================================================

TEST(image_default_tint_resolves_to_white) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50));
    img_end(&ctx);

    // WLX_WHITE is (255, 255, 255, 255); default opacity 1.0 leaves it unchanged.
    ASSERT_EQ_INT(_img_last_tint.r, 255);
    ASSERT_EQ_INT(_img_last_tint.g, 255);
    ASSERT_EQ_INT(_img_last_tint.b, 255);
    ASSERT_EQ_INT(_img_last_tint.a, 255);
}

TEST(image_explicit_tint_passes_through) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50),
        .tint = WLX_RGBA(200, 100, 50, 255));
    img_end(&ctx);

    ASSERT_EQ_INT(_img_last_tint.r, 200);
    ASSERT_EQ_INT(_img_last_tint.g, 100);
    ASSERT_EQ_INT(_img_last_tint.b, 50);
    ASSERT_EQ_INT(_img_last_tint.a, 255);
}

TEST(image_opacity_stack_folds_into_tint_alpha) {
    img_rec_reset();
    WLX_Context ctx = {0};
    ctx.backend = img_rec_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_push_opacity(&ctx, 0.5f);
    wlx_image(&ctx, img_make_texture(50, 50));
    wlx_pop_opacity(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 255 * 0.5 -> 128 (roundf in wlx_color_apply_opacity rounds half away from zero).
    ASSERT_EQ_INT(_img_last_tint.a, 128);
    ASSERT_EQ_INT(_img_last_tint.r, 255);
    wlx_context_destroy(&ctx);
}

TEST(image_explicit_opacity_overrides_default) {
    WLX_Context ctx;
    img_begin_square(&ctx);
    wlx_image(&ctx, img_make_texture(50, 50),
        .tint = WLX_RGBA(255, 255, 255, 200), .opacity = 0.5f);
    img_end(&ctx);

    // Tint alpha 200 * opacity 0.5 -> 100.
    ASSERT_EQ_INT(_img_last_tint.a, 100);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(image) {
    RUN_TEST(image_stretch_dst_matches_widget_rect);
    RUN_TEST(image_default_src_covers_full_texture);
    RUN_TEST(image_custom_src_subrect_passed_through);

    RUN_TEST(image_none_top_left);
    RUN_TEST(image_none_center);
    RUN_TEST(image_none_bottom_right);

    RUN_TEST(image_fit_landscape_source_pillarboxes_vertically);
    RUN_TEST(image_fit_portrait_source_letterboxes_horizontally);
    RUN_TEST(image_fit_align_top_left_anchors_to_origin);

    RUN_TEST(image_fill_landscape_source_horizontal_crop_center);
    RUN_TEST(image_fill_landscape_horizontal_crop_left_anchor);
    RUN_TEST(image_fill_landscape_horizontal_crop_right_anchor);
    RUN_TEST(image_fill_portrait_source_vertical_crop_center);
    RUN_TEST(image_fill_portrait_vertical_crop_top_anchor);
    RUN_TEST(image_fill_portrait_vertical_crop_bottom_anchor);
    RUN_TEST(image_fill_matching_aspect_no_crop);

    RUN_TEST(image_empty_texture_emits_no_draw);
    RUN_TEST(image_empty_texture_does_not_break_subsequent_widgets);

    RUN_TEST(image_default_tint_resolves_to_white);
    RUN_TEST(image_explicit_tint_passes_through);
    RUN_TEST(image_opacity_stack_folds_into_tint_alpha);
    RUN_TEST(image_explicit_opacity_overrides_default);
}

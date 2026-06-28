// test_button_image.c - tests for image-capable wlx_button: text-only
// regression, image-only, image+text mixed content, placement variants,
// custom texture_src, scale-mode parity with wlx_image, empty-texture
// fallbacks, opacity folding into texture_tint, hover background brightness
// with no double-tinting, and one-slot consumption per call.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs: capture texture, text, and rect draws.
// ============================================================================

#define BI_MAX_DRAWS 8

static int       _bi_tex_count;
static WLX_Rect  _bi_tex_srcs[BI_MAX_DRAWS];
static WLX_Rect  _bi_tex_dsts[BI_MAX_DRAWS];
static WLX_Color _bi_tex_tints[BI_MAX_DRAWS];
static WLX_Rect  _bi_tex_src;
static WLX_Rect  _bi_tex_dst;
static WLX_Color _bi_tex_tint;

static int       _bi_text_count;

static int       _bi_rect_count;
static WLX_Color _bi_last_rect_color;

static void bi_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex;
    if (_bi_tex_count < BI_MAX_DRAWS) {
        _bi_tex_srcs[_bi_tex_count]  = src;
        _bi_tex_dsts[_bi_tex_count]  = dst;
        _bi_tex_tints[_bi_tex_count] = tint;
    }
    _bi_tex_count++;
    _bi_tex_src  = src;
    _bi_tex_dst  = dst;
    _bi_tex_tint = tint;
}

static void bi_rec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)x; (void)y; (void)style;
    _bi_text_count++;
}

static void bi_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    _bi_rect_count++;
    _bi_last_rect_color = c;
}

static void bi_rec_reset(void) {
    _bi_tex_count       = 0;
    memset(_bi_tex_srcs,  0, sizeof(_bi_tex_srcs));
    memset(_bi_tex_dsts,  0, sizeof(_bi_tex_dsts));
    memset(_bi_tex_tints, 0, sizeof(_bi_tex_tints));
    _bi_tex_src         = (WLX_Rect){0};
    _bi_tex_dst         = (WLX_Rect){0};
    _bi_tex_tint        = (WLX_Color){0};
    _bi_text_count      = 0;
    _bi_rect_count      = 0;
    _bi_last_rect_color = (WLX_Color){0};
}

static WLX_Backend bi_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture = bi_rec_draw_texture;
    b.draw_text    = bi_rec_draw_text;
    b.draw_rect    = bi_rec_draw_rect;
    return b;
}

// ============================================================================
// Test fixtures
// ============================================================================

static WLX_Texture bi_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

// Initialize a fresh 200x200 ctx without starting a frame.
static void bi_ctx_init(WLX_Context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = bi_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, 200, 200);
}

// Begin a frame on a 200x200 root, single-slot vertical layout, mouse at
// (-1, -1) so no widget is hovered.
static void bi_begin(WLX_Context *ctx) {
    bi_rec_reset();
    bi_ctx_init(ctx);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void bi_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// Helper that always invokes wlx_button from the same source line so
// successive frames share the same widget ID (the macro keys ID off of
// __FILE__/__LINE__).
static bool bi_button_image_only(WLX_Context *ctx, WLX_Texture tex) {
    return wlx_button(ctx, "", .texture = tex);
}

static bool bi_button_text_only(WLX_Context *ctx, const char *text) {
    return wlx_button(ctx, text);
}

// ============================================================================
// Text-only regression
// ============================================================================

TEST(button_image_text_only_emits_chrome_and_text) {
    WLX_Context ctx;
    bi_begin(&ctx);
    bool clicked = wlx_button(&ctx, "Save");
    bi_end(&ctx);

    ASSERT_FALSE(clicked);
    ASSERT_EQ_INT(_bi_tex_count, 0);
    ASSERT_TRUE(_bi_text_count >= 1);
    ASSERT_TRUE(_bi_rect_count >= 1);
}

// ============================================================================
// Image-only mode
// ============================================================================

TEST(button_image_image_only_emits_one_texture_no_text) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .align = WLX_CENTER);
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 1);
    ASSERT_EQ_INT(_bi_text_count, 0);
    ASSERT_TRUE(_bi_rect_count >= 1);
}

TEST(button_image_image_only_uses_full_button_rect_when_image_size_unset) {
    // image_size <= 0 means full button rect is the image target. With FIT
    // (default) and a 50x50 source in a 200x200 button, FIT scales to
    // 200x200 and produces dst == button rect.
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 1);
    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(button_image_image_only_explicit_size_anchors_via_align) {
    // .image_size = 50 with align = WLX_CENTER positions a 50x50 sub-rect
    // centered inside the 200x200 button.
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH,
               .image_size = 50, .align = WLX_CENTER);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){75, 75, 50, 50}), 0.001f);
}

TEST(button_image_image_only_click_returns_true_on_release) {
    WLX_Context ctx;
    bi_ctx_init(&ctx);
    WLX_Texture tex = bi_make_texture(50, 50);

    // Frame 1: press inside button.
    bi_rec_reset();
    test_frame_begin(&ctx, 100, 100, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool press = bi_button_image_only(&ctx, tex);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(press);

    // Frame 2: release while still hovering -> clicked.
    bi_rec_reset();
    test_frame_begin(&ctx, 100, 100, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool release = bi_button_image_only(&ctx, tex);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(release);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Image + text mode (LEFT default placement)
// ============================================================================

TEST(button_image_image_plus_text_emits_one_texture_and_text) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16);
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 1);
    ASSERT_TRUE(_bi_text_count >= 1);
}

// ============================================================================
// Placement: explicit image_size = 50, gap = 10, font_size = 16. Mock measures
// "Save" as 4 * 16 * 0.5 = 32 wide, 16 tall.
//
//   LEFT  : block 92x50, align=LEFT in 200x200 -> block (0, 75, 92, 50)
//           image rect (0, 75, 50, 50), text rect  (60, 75, 32, 50)
//   RIGHT : same block; text first, image right     image rect (42, 75, 50, 50)
//   TOP   : block 50x76, align=LEFT -> block (0, 62, 50, 76)
//           image rect (0, 62, 50, 50), text rect  (0, 122, 50, 16)
//   BOTTOM: same block; text first, image bottom    image rect (0, 88, 50, 50)
//
// With FIT and a 50x50 source in a 50x50 image rect, dst == image rect.
// ============================================================================

TEST(button_image_placement_left) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16,
               .image_placement = WLX_IMAGE_PLACEMENT_LEFT);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 75, 50, 50}), 0.001f);
}

TEST(button_image_placement_right) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16,
               .image_placement = WLX_IMAGE_PLACEMENT_RIGHT);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){42, 75, 50, 50}), 0.001f);
}

TEST(button_image_placement_top) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16,
               .image_placement = WLX_IMAGE_PLACEMENT_TOP);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 62, 50, 50}), 0.001f);
}

TEST(button_image_placement_bottom) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16,
               .image_placement = WLX_IMAGE_PLACEMENT_BOTTOM);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 88, 50, 50}), 0.001f);
}

// ============================================================================
// Squeezed image + text: the image keeps its requested size and crops at the
// content edge (widget drawing is not scissored, so the overflow is removed
// geometrically) -- the destination shrinks to the visible sub-rect and the
// source narrows proportionally so the image is not stretched.
// ============================================================================

TEST(button_image_squeezed_width_crops_image_at_content_edge) {
    // 13-wide button slot, image_size 18, gap 8, text 80 wide. The 18x18 glyph
    // sits at (0, 91) but content is only 13 wide, so the right 5px crop away:
    // dst keeps its 18px height and full size but x-extent clips to 13, and the
    // source narrows to the matching 13/18 of its width (50 * 13/18).
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_layout_begin(&ctx, 2, WLX_HORZ,
        .sizes = (WLX_Slot_Size[]){ WLX_SLOT_PX(13), WLX_SLOT_FLEX(1) });
    wlx_button(&ctx, "Launch Lab", .texture = bi_make_texture(50, 50),
               .image_size = 18, .image_text_gap = 8, .font_size = 16);
    wlx_layout_end(&ctx);
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 1);
    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 91, 13, 18}), 0.001f);
    ASSERT_EQ_RECT(_bi_tex_src, ((WLX_Rect){0, 0, 50.0f * 13.0f / 18.0f, 50}), 0.001f);
}

TEST(button_image_unsqueezed_image_not_cropped) {
    // Counterpart: when the widget has room, the crop is a no-op -- the source
    // stays the full texture and the destination is the laid-out image rect.
    // (200x200 button, 50x50 glyph, LEFT placement, block height 50 -> y=75.)
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(50, 50),
               .image_size = 50, .image_text_gap = 10, .font_size = 16);
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 1);
    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 75, 50, 50}), 0.001f);
    ASSERT_EQ_RECT(_bi_tex_src, ((WLX_Rect){0, 0, 50, 50}), 0.001f);
}

// ============================================================================
// Custom texture_src honored
// ============================================================================

TEST(button_image_custom_texture_src_passed_through) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(128, 128),
               .texture_src = { 10, 20, 30, 40 },
               .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    bi_end(&ctx);

    ASSERT_EQ_RECT(_bi_tex_src, ((WLX_Rect){10, 20, 30, 40}), 0.001f);
    ASSERT_EQ_RECT(_bi_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// Scale-mode parity with wlx_image: same target rect + same scale + same
// align must produce the same source/dest rects.
// ============================================================================

static void bi_capture_button_rects(WLX_Image_Scale scale, int tex_w, int tex_h,
                                    WLX_Rect *out_src, WLX_Rect *out_dst) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(tex_w, tex_h),
               .texture_scale = scale, .align = WLX_CENTER);
    bi_end(&ctx);
    *out_src = _bi_tex_src;
    *out_dst = _bi_tex_dst;
}

static void bi_capture_image_rects(WLX_Image_Scale scale, int tex_w, int tex_h,
                                   WLX_Rect *out_src, WLX_Rect *out_dst) {
    WLX_Context ctx;
    bi_rec_reset();
    bi_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_image(&ctx, bi_make_texture(tex_w, tex_h),
              .scale = scale, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    *out_src = _bi_tex_src;
    *out_dst = _bi_tex_dst;
    wlx_context_destroy(&ctx);
}

TEST(button_image_scale_stretch_parity_with_wlx_image) {
    WLX_Rect bs, bd, is, id;
    bi_capture_button_rects(WLX_IMAGE_SCALE_STRETCH, 100, 50, &bs, &bd);
    bi_capture_image_rects (WLX_IMAGE_SCALE_STRETCH, 100, 50, &is, &id);
    ASSERT_EQ_RECT(bs, is, 0.001f);
    ASSERT_EQ_RECT(bd, id, 0.001f);
}

TEST(button_image_scale_fit_parity_with_wlx_image) {
    WLX_Rect bs, bd, is, id;
    bi_capture_button_rects(WLX_IMAGE_SCALE_FIT, 100, 50, &bs, &bd);
    bi_capture_image_rects (WLX_IMAGE_SCALE_FIT, 100, 50, &is, &id);
    ASSERT_EQ_RECT(bs, is, 0.001f);
    ASSERT_EQ_RECT(bd, id, 0.001f);
}

TEST(button_image_scale_fill_parity_with_wlx_image) {
    WLX_Rect bs, bd, is, id;
    bi_capture_button_rects(WLX_IMAGE_SCALE_FILL, 200, 100, &bs, &bd);
    bi_capture_image_rects (WLX_IMAGE_SCALE_FILL, 200, 100, &is, &id);
    ASSERT_EQ_RECT(bs, is, 0.001f);
    ASSERT_EQ_RECT(bd, id, 0.001f);
}

TEST(button_image_scale_none_parity_with_wlx_image) {
    WLX_Rect bs, bd, is, id;
    bi_capture_button_rects(WLX_IMAGE_SCALE_NONE, 50, 50, &bs, &bd);
    bi_capture_image_rects (WLX_IMAGE_SCALE_NONE, 50, 50, &is, &id);
    ASSERT_EQ_RECT(bs, is, 0.001f);
    ASSERT_EQ_RECT(bd, id, 0.001f);
}

// ============================================================================
// Empty texture fallbacks
// ============================================================================

TEST(button_image_empty_texture_with_text_falls_back_to_text_only) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "Save", .texture = bi_make_texture(0, 0));
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 0);
    ASSERT_TRUE(_bi_text_count >= 1);
    ASSERT_TRUE(_bi_rect_count >= 1);
}

TEST(button_image_empty_texture_with_empty_text_emits_chrome_only) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(0, 0));
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 0);
    ASSERT_EQ_INT(_bi_text_count, 0);
    ASSERT_TRUE(_bi_rect_count >= 1);
}

TEST(button_image_empty_texture_empty_text_still_clickable) {
    WLX_Context ctx;
    bi_ctx_init(&ctx);

    // Press inside.
    bi_rec_reset();
    test_frame_begin(&ctx, 100, 100, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    (void)bi_button_text_only(&ctx, "");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // Release inside -> clicked.
    bi_rec_reset();
    test_frame_begin(&ctx, 100, 100, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool clicked = bi_button_text_only(&ctx, "");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(clicked);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Tint + opacity
// ============================================================================

TEST(button_image_default_texture_tint_resolves_to_white) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50));
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_tint.r, 255);
    ASSERT_EQ_INT(_bi_tex_tint.g, 255);
    ASSERT_EQ_INT(_bi_tex_tint.b, 255);
    ASSERT_EQ_INT(_bi_tex_tint.a, 255);
}

TEST(button_image_explicit_texture_tint_passes_through) {
    WLX_Context ctx;
    bi_begin(&ctx);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .texture_tint = WLX_RGBA(180, 90, 40, 255));
    bi_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_tint.r, 180);
    ASSERT_EQ_INT(_bi_tex_tint.g, 90);
    ASSERT_EQ_INT(_bi_tex_tint.b, 40);
    ASSERT_EQ_INT(_bi_tex_tint.a, 255);
}

TEST(button_image_opacity_stack_folds_into_texture_tint_alpha) {
    bi_rec_reset();
    WLX_Context ctx;
    bi_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_push_opacity(&ctx, 0.5f);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50));
    wlx_pop_opacity(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 255 * 0.5 -> 128 (roundf, half away from zero).
    ASSERT_EQ_INT(_bi_tex_tint.a, 128);
    ASSERT_EQ_INT(_bi_tex_tint.r, 255);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hover background brightness without double-tinting texture
// ============================================================================

TEST(button_image_hover_brightens_chrome_but_not_texture_tint) {
    // Frame 1: mouse outside button rect.
    WLX_Context ctx_a;
    bi_rec_reset();
    bi_ctx_init(&ctx_a);
    test_frame_begin(&ctx_a, -1, -1, false, false);
    wlx_layout_begin(&ctx_a, 1, WLX_VERT);
    wlx_button(&ctx_a, "", .texture = bi_make_texture(50, 50));
    wlx_layout_end(&ctx_a);
    test_frame_end(&ctx_a);
    WLX_Color tint_no_hover = _bi_tex_tint;
    WLX_Color rect_no_hover = _bi_last_rect_color;
    wlx_context_destroy(&ctx_a);

    // Frame 2: mouse inside button rect -> hover.
    WLX_Context ctx_b;
    bi_rec_reset();
    bi_ctx_init(&ctx_b);
    test_frame_begin(&ctx_b, 100, 100, false, false);
    wlx_layout_begin(&ctx_b, 1, WLX_VERT);
    wlx_button(&ctx_b, "", .texture = bi_make_texture(50, 50));
    wlx_layout_end(&ctx_b);
    test_frame_end(&ctx_b);
    WLX_Color tint_hover = _bi_tex_tint;
    WLX_Color rect_hover = _bi_last_rect_color;
    wlx_context_destroy(&ctx_b);

    // Chrome bg differs (hover brightens it).
    ASSERT_TRUE(rect_no_hover.r != rect_hover.r
                || rect_no_hover.g != rect_hover.g
                || rect_no_hover.b != rect_hover.b);

    // Texture tint is not double-modulated by hover.
    ASSERT_EQ_COLOR(tint_no_hover, tint_hover);
}

// ============================================================================
// Layout slot consumption: each image-capable button consumes exactly one slot.
// Two buttons in a 2-slot vertical layout must land in different halves.
// ============================================================================

TEST(button_image_consumes_one_slot_per_call) {
    WLX_Context ctx;
    bi_rec_reset();
    bi_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    wlx_button(&ctx, "", .texture = bi_make_texture(50, 50),
               .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_bi_tex_count, 2);
    // First button in top half; second in bottom half.
    ASSERT_EQ_F(_bi_tex_dsts[0].y, 0.0f, 0.001f);
    ASSERT_EQ_F(_bi_tex_dsts[0].h, 100.0f, 0.001f);
    ASSERT_EQ_F(_bi_tex_dsts[1].y, 100.0f, 0.001f);
    ASSERT_EQ_F(_bi_tex_dsts[1].h, 100.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(button_image) {
    RUN_TEST(button_image_text_only_emits_chrome_and_text);

    RUN_TEST(button_image_image_only_emits_one_texture_no_text);
    RUN_TEST(button_image_image_only_uses_full_button_rect_when_image_size_unset);
    RUN_TEST(button_image_image_only_explicit_size_anchors_via_align);
    RUN_TEST(button_image_image_only_click_returns_true_on_release);

    RUN_TEST(button_image_image_plus_text_emits_one_texture_and_text);

    RUN_TEST(button_image_placement_left);
    RUN_TEST(button_image_placement_right);
    RUN_TEST(button_image_placement_top);
    RUN_TEST(button_image_placement_bottom);

    RUN_TEST(button_image_squeezed_width_crops_image_at_content_edge);
    RUN_TEST(button_image_unsqueezed_image_not_cropped);

    RUN_TEST(button_image_custom_texture_src_passed_through);

    RUN_TEST(button_image_scale_stretch_parity_with_wlx_image);
    RUN_TEST(button_image_scale_fit_parity_with_wlx_image);
    RUN_TEST(button_image_scale_fill_parity_with_wlx_image);
    RUN_TEST(button_image_scale_none_parity_with_wlx_image);

    RUN_TEST(button_image_empty_texture_with_text_falls_back_to_text_only);
    RUN_TEST(button_image_empty_texture_with_empty_text_emits_chrome_only);
    RUN_TEST(button_image_empty_texture_empty_text_still_clickable);

    RUN_TEST(button_image_default_texture_tint_resolves_to_white);
    RUN_TEST(button_image_explicit_texture_tint_passes_through);
    RUN_TEST(button_image_opacity_stack_folds_into_texture_tint_alpha);

    RUN_TEST(button_image_hover_brightens_chrome_but_not_texture_tint);

    RUN_TEST(button_image_consumes_one_slot_per_call);
}

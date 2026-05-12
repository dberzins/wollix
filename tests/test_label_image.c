// test_label_image.c - tests for image-capable wlx_label: text-only
// regression (no chrome by default, optional chrome + hover when
// show_background=true), image-only edge case, image+text mixed content,
// placement variants, custom texture_src, scale-mode parity with wlx_image,
// empty-texture fallbacks, opacity folding into texture_tint, hover
// background brightness with no double-tinting, one-slot consumption per
// call, and the non-interactive contract that label clicks do not affect
// sibling widgets.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs: capture texture, text, and rect draws.
// ============================================================================

#define LI_MAX_DRAWS 8

static int       _li_tex_count;
static WLX_Rect  _li_tex_srcs[LI_MAX_DRAWS];
static WLX_Rect  _li_tex_dsts[LI_MAX_DRAWS];
static WLX_Color _li_tex_tints[LI_MAX_DRAWS];
static WLX_Rect  _li_tex_src;
static WLX_Rect  _li_tex_dst;
static WLX_Color _li_tex_tint;

static int       _li_text_count;

static int       _li_rect_count;
static WLX_Color _li_last_rect_color;

static void li_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex;
    if (_li_tex_count < LI_MAX_DRAWS) {
        _li_tex_srcs[_li_tex_count]  = src;
        _li_tex_dsts[_li_tex_count]  = dst;
        _li_tex_tints[_li_tex_count] = tint;
    }
    _li_tex_count++;
    _li_tex_src  = src;
    _li_tex_dst  = dst;
    _li_tex_tint = tint;
}

static void li_rec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)x; (void)y; (void)style;
    _li_text_count++;
}

static void li_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    _li_rect_count++;
    _li_last_rect_color = c;
}

static void li_rec_reset(void) {
    _li_tex_count       = 0;
    memset(_li_tex_srcs,  0, sizeof(_li_tex_srcs));
    memset(_li_tex_dsts,  0, sizeof(_li_tex_dsts));
    memset(_li_tex_tints, 0, sizeof(_li_tex_tints));
    _li_tex_src         = (WLX_Rect){0};
    _li_tex_dst         = (WLX_Rect){0};
    _li_tex_tint        = (WLX_Color){0};
    _li_text_count      = 0;
    _li_rect_count      = 0;
    _li_last_rect_color = (WLX_Color){0};
}

static WLX_Backend li_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture = li_rec_draw_texture;
    b.draw_text    = li_rec_draw_text;
    b.draw_rect    = li_rec_draw_rect;
    return b;
}

// ============================================================================
// Test fixtures
// ============================================================================

static WLX_Texture li_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

// Initialize a fresh 200x200 ctx without starting a frame.
static void li_ctx_init(WLX_Context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = li_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, 200, 200);
}

// Begin a frame on a 200x200 root, single-slot vertical layout, mouse at
// (-1, -1) so no widget is hovered.
static void li_begin(WLX_Context *ctx) {
    li_rec_reset();
    li_ctx_init(ctx);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void li_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// ============================================================================
// Text-only regression: default label draws text only (chrome inactive
// because show_background=false and theme border_width=0).
// ============================================================================

TEST(label_image_text_only_no_chrome_by_default) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Hello");
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 0);
    ASSERT_TRUE(_li_text_count >= 1);
    ASSERT_EQ_INT(_li_rect_count, 0);
}

TEST(label_image_text_only_with_show_background_emits_chrome) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Hello", .show_background = true);
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 0);
    ASSERT_TRUE(_li_text_count >= 1);
    ASSERT_TRUE(_li_rect_count >= 1);
}

// ============================================================================
// Image + text mode (default LEFT placement)
// ============================================================================

TEST(label_image_image_plus_text_emits_one_texture_and_text) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Saved", .texture = li_make_texture(50, 50),
              .image_size = 50, .image_text_gap = 10, .font_size = 16);
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 1);
    ASSERT_TRUE(_li_text_count >= 1);
}

// ============================================================================
// Placement: explicit image_size = 50, gap = 10, font_size = 16, align=LEFT.
// Mock measures "Save" as 4 * 16 * 0.5 = 32 wide, 16 tall.
//
//   LEFT  : block 92x50, align=LEFT in 200x200 -> block (0, 75, 92, 50)
//           image rect (0, 75, 50, 50)
//   RIGHT : same block; text first, image right -> image rect (42, 75, 50, 50)
//   TOP   : block 50x76, align=LEFT -> block (0, 62, 50, 76)
//           image rect (0, 62, 50, 50)
//   BOTTOM: same block; text first, image bottom -> image rect (0, 88, 50, 50)
//
// With FIT (default) and a 50x50 source in a 50x50 image rect, dst == image
// rect.
// ============================================================================

TEST(label_image_placement_left) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Save", .texture = li_make_texture(50, 50),
              .image_size = 50, .image_text_gap = 10, .font_size = 16,
              .image_placement = WLX_IMAGE_PLACEMENT_LEFT);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){0, 75, 50, 50}), 0.001f);
}

TEST(label_image_placement_right) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Save", .texture = li_make_texture(50, 50),
              .image_size = 50, .image_text_gap = 10, .font_size = 16,
              .image_placement = WLX_IMAGE_PLACEMENT_RIGHT);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){42, 75, 50, 50}), 0.001f);
}

TEST(label_image_placement_top) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Save", .texture = li_make_texture(50, 50),
              .image_size = 50, .image_text_gap = 10, .font_size = 16,
              .image_placement = WLX_IMAGE_PLACEMENT_TOP);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){0, 62, 50, 50}), 0.001f);
}

TEST(label_image_placement_bottom) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Save", .texture = li_make_texture(50, 50),
              .image_size = 50, .image_text_gap = 10, .font_size = 16,
              .image_placement = WLX_IMAGE_PLACEMENT_BOTTOM);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){0, 88, 50, 50}), 0.001f);
}

// Default image_text_gap is font_size * 0.5; with font_size=16, gap=8.
// "Save" -> 32 wide, image_size=50 -> block_w = 50 + 8 + 32 = 90, block_h=50
// align=LEFT -> block (0, 75, 90, 50); image rect (0, 75, 50, 50).
// (Image dst at (0, 75, 50, 50) is the same as the explicit gap case because
// the image still anchors at block.x for LEFT placement; verify text rect
// inferred via text draw existence and rect difference is on text side.)
TEST(label_image_image_text_gap_default_is_font_derived) {
    WLX_Context ctx;
    li_begin(&ctx);
    // Omit image_text_gap so the resolver derives it from font_size.
    wlx_label(&ctx, "Save", .texture = li_make_texture(50, 50),
              .image_size = 50, .font_size = 16,
              .image_placement = WLX_IMAGE_PLACEMENT_RIGHT);
    li_end(&ctx);

    // RIGHT placement: text first, image second. With derived gap=8,
    // available_text_w = 90 - 50 - 8 = 32, so image rect.x = 0 + 32 + 8 = 40.
    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){40, 75, 50, 50}), 0.001f);
}

// ============================================================================
// Image-only edge case
// ============================================================================

TEST(label_image_image_only_emits_one_texture_no_text) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .align = WLX_CENTER);
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 1);
    ASSERT_EQ_INT(_li_text_count, 0);
}

TEST(label_image_image_only_uses_full_label_rect_when_image_size_unset) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 1);
    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

TEST(label_image_image_only_explicit_size_anchors_via_align) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .texture_scale = WLX_IMAGE_SCALE_STRETCH,
              .image_size = 50, .align = WLX_CENTER);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){75, 75, 50, 50}), 0.001f);
}

// ============================================================================
// Custom texture_src honored
// ============================================================================

TEST(label_image_custom_texture_src_passed_through) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(128, 128),
              .texture_src = { 10, 20, 30, 40 },
              .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    li_end(&ctx);

    ASSERT_EQ_RECT(_li_tex_src, ((WLX_Rect){10, 20, 30, 40}), 0.001f);
    ASSERT_EQ_RECT(_li_tex_dst, ((WLX_Rect){0, 0, 200, 200}), 0.001f);
}

// ============================================================================
// Scale-mode parity with wlx_image: same target rect + same scale + same
// align must produce the same source/dest rects.
// ============================================================================

static void li_capture_label_rects(WLX_Image_Scale scale, int tex_w, int tex_h,
                                   WLX_Rect *out_src, WLX_Rect *out_dst) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(tex_w, tex_h),
              .texture_scale = scale, .align = WLX_CENTER);
    li_end(&ctx);
    *out_src = _li_tex_src;
    *out_dst = _li_tex_dst;
}

static void li_capture_image_rects(WLX_Image_Scale scale, int tex_w, int tex_h,
                                   WLX_Rect *out_src, WLX_Rect *out_dst) {
    WLX_Context ctx;
    li_rec_reset();
    li_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_image(&ctx, li_make_texture(tex_w, tex_h),
              .scale = scale, .align = WLX_CENTER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    *out_src = _li_tex_src;
    *out_dst = _li_tex_dst;
    wlx_context_destroy(&ctx);
}

TEST(label_image_scale_stretch_parity_with_wlx_image) {
    WLX_Rect ls, ld, is, id;
    li_capture_label_rects(WLX_IMAGE_SCALE_STRETCH, 100, 50, &ls, &ld);
    li_capture_image_rects(WLX_IMAGE_SCALE_STRETCH, 100, 50, &is, &id);
    ASSERT_EQ_RECT(ls, is, 0.001f);
    ASSERT_EQ_RECT(ld, id, 0.001f);
}

TEST(label_image_scale_fit_parity_with_wlx_image) {
    WLX_Rect ls, ld, is, id;
    li_capture_label_rects(WLX_IMAGE_SCALE_FIT, 100, 50, &ls, &ld);
    li_capture_image_rects(WLX_IMAGE_SCALE_FIT, 100, 50, &is, &id);
    ASSERT_EQ_RECT(ls, is, 0.001f);
    ASSERT_EQ_RECT(ld, id, 0.001f);
}

TEST(label_image_scale_fill_parity_with_wlx_image) {
    WLX_Rect ls, ld, is, id;
    li_capture_label_rects(WLX_IMAGE_SCALE_FILL, 200, 100, &ls, &ld);
    li_capture_image_rects(WLX_IMAGE_SCALE_FILL, 200, 100, &is, &id);
    ASSERT_EQ_RECT(ls, is, 0.001f);
    ASSERT_EQ_RECT(ld, id, 0.001f);
}

TEST(label_image_scale_none_parity_with_wlx_image) {
    WLX_Rect ls, ld, is, id;
    li_capture_label_rects(WLX_IMAGE_SCALE_NONE, 50, 50, &ls, &ld);
    li_capture_image_rects(WLX_IMAGE_SCALE_NONE, 50, 50, &is, &id);
    ASSERT_EQ_RECT(ls, is, 0.001f);
    ASSERT_EQ_RECT(ld, id, 0.001f);
}

// ============================================================================
// Empty texture fallbacks
// ============================================================================

TEST(label_image_empty_texture_with_text_falls_back_to_text_only) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "Hello", .texture = li_make_texture(0, 0));
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 0);
    ASSERT_TRUE(_li_text_count >= 1);
}

TEST(label_image_empty_texture_with_empty_text_emits_no_content) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(0, 0));
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 0);
    ASSERT_EQ_INT(_li_text_count, 0);
    ASSERT_EQ_INT(_li_rect_count, 0);
}

TEST(label_image_empty_texture_empty_text_chrome_still_draws_when_configured) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(0, 0),
              .show_background = true);
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 0);
    ASSERT_EQ_INT(_li_text_count, 0);
    ASSERT_TRUE(_li_rect_count >= 1);
}

// ============================================================================
// Tint + opacity
// ============================================================================

TEST(label_image_default_texture_tint_resolves_to_white) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50));
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_tint.r, 255);
    ASSERT_EQ_INT(_li_tex_tint.g, 255);
    ASSERT_EQ_INT(_li_tex_tint.b, 255);
    ASSERT_EQ_INT(_li_tex_tint.a, 255);
}

TEST(label_image_explicit_texture_tint_passes_through) {
    WLX_Context ctx;
    li_begin(&ctx);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .texture_tint = WLX_RGBA(180, 90, 40, 255));
    li_end(&ctx);

    ASSERT_EQ_INT(_li_tex_tint.r, 180);
    ASSERT_EQ_INT(_li_tex_tint.g, 90);
    ASSERT_EQ_INT(_li_tex_tint.b, 40);
    ASSERT_EQ_INT(_li_tex_tint.a, 255);
}

TEST(label_image_opacity_stack_folds_into_texture_tint_alpha) {
    li_rec_reset();
    WLX_Context ctx;
    li_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_push_opacity(&ctx, 0.5f);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50));
    wlx_pop_opacity(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // 255 * 0.5 -> 128 (roundf, half away from zero).
    ASSERT_EQ_INT(_li_tex_tint.a, 128);
    ASSERT_EQ_INT(_li_tex_tint.r, 255);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hover background brightness only when show_background=true; texture tint
// is never double-modulated by hover.
// ============================================================================

TEST(label_image_hover_brightens_chrome_only_when_show_background_true) {
    // Frame A: mouse outside the label, show_background=true.
    WLX_Context ctx_a;
    li_rec_reset();
    li_ctx_init(&ctx_a);
    test_frame_begin(&ctx_a, -1, -1, false, false);
    wlx_layout_begin(&ctx_a, 1, WLX_VERT);
    wlx_label(&ctx_a, "", .texture = li_make_texture(50, 50),
              .show_background = true);
    wlx_layout_end(&ctx_a);
    test_frame_end(&ctx_a);
    WLX_Color rect_no_hover = _li_last_rect_color;
    WLX_Color tint_no_hover = _li_tex_tint;
    wlx_context_destroy(&ctx_a);

    // Frame B: mouse inside the label, show_background=true.
    WLX_Context ctx_b;
    li_rec_reset();
    li_ctx_init(&ctx_b);
    test_frame_begin(&ctx_b, 100, 100, false, false);
    wlx_layout_begin(&ctx_b, 1, WLX_VERT);
    wlx_label(&ctx_b, "", .texture = li_make_texture(50, 50),
              .show_background = true);
    wlx_layout_end(&ctx_b);
    test_frame_end(&ctx_b);
    WLX_Color rect_hover = _li_last_rect_color;
    WLX_Color tint_hover = _li_tex_tint;
    wlx_context_destroy(&ctx_b);

    // Chrome background differs between hover states.
    ASSERT_TRUE(rect_no_hover.r != rect_hover.r
                || rect_no_hover.g != rect_hover.g
                || rect_no_hover.b != rect_hover.b);

    // Texture tint is not double-modulated by hover.
    ASSERT_EQ_COLOR(tint_no_hover, tint_hover);
}

TEST(label_image_no_chrome_when_show_background_false) {
    // Even with mouse over the label, no chrome draws when show_background
    // is false and theme border_width is 0.
    WLX_Context ctx;
    li_rec_reset();
    li_ctx_init(&ctx);
    test_frame_begin(&ctx, 100, 100, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_label(&ctx, "Hover me", .texture = li_make_texture(50, 50));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_li_rect_count, 0);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Layout slot consumption: each image-capable label consumes exactly one
// slot. Two labels in a 2-slot vertical layout must land in different halves.
// ============================================================================

TEST(label_image_consumes_one_slot_per_call) {
    WLX_Context ctx;
    li_rec_reset();
    li_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    wlx_label(&ctx, "", .texture = li_make_texture(50, 50),
              .texture_scale = WLX_IMAGE_SCALE_STRETCH);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(_li_tex_count, 2);
    // First label in top half; second in bottom half.
    ASSERT_EQ_F(_li_tex_dsts[0].y, 0.0f, 0.001f);
    ASSERT_EQ_F(_li_tex_dsts[0].h, 100.0f, 0.001f);
    ASSERT_EQ_F(_li_tex_dsts[1].y, 100.0f, 0.001f);
    ASSERT_EQ_F(_li_tex_dsts[1].h, 100.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Non-interactive contract: an image-capable label sitting in front of a
// button does not steal that button's click. The button still reports
// `clicked` on release while the cursor is inside it.
//
// wlx_label and wlx_button derive their widget IDs from __FILE__/__LINE__,
// so both frames must invoke them at the same source location. The helper
// below guarantees that.
// ============================================================================

static bool li_label_then_button(WLX_Context *ctx, WLX_Texture tex) {
    wlx_layout_begin(ctx, 2, WLX_VERT);
    wlx_label(ctx, "Label", .texture = tex);
    bool clicked = wlx_button(ctx, "Click");
    wlx_layout_end(ctx);
    return clicked;
}

TEST(label_image_does_not_consume_clicks_from_sibling_button) {
    WLX_Context ctx;
    li_ctx_init(&ctx);
    WLX_Texture tex = li_make_texture(50, 50);

    // Two-slot vertical layout: label in the top half (y in [0, 100)),
    // button in the bottom half (y in [100, 200)). Cursor at (100, 150)
    // is over the button, not the label.

    // Frame 1: press inside the button.
    li_rec_reset();
    test_frame_begin(&ctx, 100, 150, true, true);
    bool press = li_label_then_button(&ctx, tex);
    test_frame_end(&ctx);
    ASSERT_FALSE(press);

    // Frame 2: release while cursor still over the button.
    li_rec_reset();
    test_frame_begin(&ctx, 100, 150, false, false);
    bool release = li_label_then_button(&ctx, tex);
    test_frame_end(&ctx);
    ASSERT_TRUE(release);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(label_image) {
    RUN_TEST(label_image_text_only_no_chrome_by_default);
    RUN_TEST(label_image_text_only_with_show_background_emits_chrome);

    RUN_TEST(label_image_image_plus_text_emits_one_texture_and_text);

    RUN_TEST(label_image_placement_left);
    RUN_TEST(label_image_placement_right);
    RUN_TEST(label_image_placement_top);
    RUN_TEST(label_image_placement_bottom);
    RUN_TEST(label_image_image_text_gap_default_is_font_derived);

    RUN_TEST(label_image_image_only_emits_one_texture_no_text);
    RUN_TEST(label_image_image_only_uses_full_label_rect_when_image_size_unset);
    RUN_TEST(label_image_image_only_explicit_size_anchors_via_align);

    RUN_TEST(label_image_custom_texture_src_passed_through);

    RUN_TEST(label_image_scale_stretch_parity_with_wlx_image);
    RUN_TEST(label_image_scale_fit_parity_with_wlx_image);
    RUN_TEST(label_image_scale_fill_parity_with_wlx_image);
    RUN_TEST(label_image_scale_none_parity_with_wlx_image);

    RUN_TEST(label_image_empty_texture_with_text_falls_back_to_text_only);
    RUN_TEST(label_image_empty_texture_with_empty_text_emits_no_content);
    RUN_TEST(label_image_empty_texture_empty_text_chrome_still_draws_when_configured);

    RUN_TEST(label_image_default_texture_tint_resolves_to_white);
    RUN_TEST(label_image_explicit_texture_tint_passes_through);
    RUN_TEST(label_image_opacity_stack_folds_into_texture_tint_alpha);

    RUN_TEST(label_image_hover_brightens_chrome_only_when_show_background_true);
    RUN_TEST(label_image_no_chrome_when_show_background_false);

    RUN_TEST(label_image_consumes_one_slot_per_call);

    RUN_TEST(label_image_does_not_consume_clicks_from_sibling_button);
}

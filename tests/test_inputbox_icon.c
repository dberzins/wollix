// test_inputbox_icon.c - tests for the inputbox inner icon: text-only
// regression (no texture emitted), leading/trailing band placement, vertical
// centering independent of opt.align, default white tint, explicit tint
// pass-through, and a narrow-field guard that the band clamp never crashes or
// produces a negative-width text band. Included from test_main.c (single TU
// build).
//
// Geometry baseline: a 200x200 root with a single vertical slot and the
// inputbox default content_padding of 10 yields an interior input_rect of
// { x=10, y=10, w=180, h=180 }. A square texture + square icon cell means the
// SCALE_FIT destination equals the cell, so the recorded texture dst is exact.

// ============================================================================
// Recording stubs: capture texture and text draws.
// ============================================================================

#define IBI_MAX_DRAWS 8

static int       _ibi_tex_count;
static WLX_Rect  _ibi_tex_dst;
static WLX_Color _ibi_tex_tint;
static int       _ibi_text_count;
static int       _ibi_rect_count;

static void ibi_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex; (void)src;
    _ibi_tex_count++;
    _ibi_tex_dst  = dst;
    _ibi_tex_tint = tint;
}

static void ibi_rec_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)text; (void)x; (void)y; (void)style;
    _ibi_text_count++;
}

static void ibi_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r; (void)c;
    _ibi_rect_count++;
}

static void ibi_rec_reset(void) {
    _ibi_tex_count  = 0;
    _ibi_tex_dst    = (WLX_Rect){0};
    _ibi_tex_tint   = (WLX_Color){0};
    _ibi_text_count = 0;
    _ibi_rect_count = 0;
}

static WLX_Backend ibi_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture = ibi_rec_draw_texture;
    b.draw_text    = ibi_rec_draw_text;
    b.draw_rect    = ibi_rec_draw_rect;
    return b;
}

// ============================================================================
// Test fixtures
// ============================================================================

static WLX_Texture ibi_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

static void ibi_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = ibi_rec_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, w, h);
}

static void ibi_begin(WLX_Context *ctx, float w, float h) {
    ibi_rec_reset();
    ibi_ctx_init(ctx, w, h);
    test_frame_begin(ctx, -1, -1, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void ibi_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// ============================================================================
// Text-only regression: no icon -> no texture, chrome still drawn
// ============================================================================

TEST(inputbox_icon_unset_emits_no_texture) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, "", buf, sizeof(buf), .font_size = 16);
    ibi_end(&ctx);

    ASSERT_EQ_INT(0, _ibi_tex_count);
}

TEST(inputbox_no_regression_text_only) {
    WLX_Context ctx;
    char buf[64] = "hello";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, "", buf, sizeof(buf), .font_size = 16);
    ibi_end(&ctx);

    // No icon texture, the box chrome still draws, and the text path runs.
    ASSERT_EQ_INT(0, _ibi_tex_count);
    ASSERT_TRUE(_ibi_rect_count > 0);
    ASSERT_TRUE(_ibi_text_count > 0);
}

// ============================================================================
// Leading / trailing band placement
// ============================================================================

TEST(inputbox_icon_leading_anchors_left) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, NULL, buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20),
                 .image_size = 20, .image_text_gap = 8,
                 .image_placement = WLX_IMAGE_PLACEMENT_LEFT);
    ibi_end(&ctx);

    // input_rect.x = 10, leading inset = WLX_INPUTBOX_TEXT_INSET (5) ->
    // icon dst.x = 15; vertically centered: y = 10 + (180 - 20)/2 = 90.
    ASSERT_EQ_INT(1, _ibi_tex_count);
    ASSERT_EQ_RECT(_ibi_tex_dst, ((WLX_Rect){15, 90, 20, 20}), 0.001f);
}

TEST(inputbox_icon_trailing_anchors_right) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, NULL, buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20),
                 .image_size = 20, .image_text_gap = 8,
                 .image_placement = WLX_IMAGE_PLACEMENT_RIGHT);
    ibi_end(&ctx);

    // trailing: dst.x = input_rect.x + input_rect.w - INSET - size
    //         = 10 + 180 - 5 - 20 = 165; y centered = 90.
    ASSERT_EQ_INT(1, _ibi_tex_count);
    ASSERT_EQ_RECT(_ibi_tex_dst, ((WLX_Rect){165, 90, 20, 20}), 0.001f);
}

// ============================================================================
// Vertical centering is independent of opt.align
// ============================================================================

TEST(inputbox_icon_centered_vertically_top_align) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, NULL, buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20),
                 .image_size = 20, .image_text_gap = 8,
                 .align = WLX_TOP_LEFT);
    ibi_end(&ctx);

    // Icon y stays centered (90) regardless of the text alignment.
    ASSERT_EQ_F(90.0f, _ibi_tex_dst.y, 0.001f);
}

TEST(inputbox_icon_centered_vertically_bottom_align) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, NULL, buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20),
                 .image_size = 20, .image_text_gap = 8,
                 .align = WLX_BOTTOM_LEFT);
    ibi_end(&ctx);

    ASSERT_EQ_F(90.0f, _ibi_tex_dst.y, 0.001f);
}

// ============================================================================
// Tint resolution
// ============================================================================

TEST(inputbox_icon_default_tint_white) {
    WLX_Context ctx;
    char buf[64] = "";
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, "", buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20), .image_size = 20);
    ibi_end(&ctx);

    ASSERT_EQ_INT(1, _ibi_tex_count);
    ASSERT_EQ_COLOR(_ibi_tex_tint, WLX_WHITE);
}

TEST(inputbox_icon_explicit_tint_passes_through) {
    WLX_Context ctx;
    char buf[64] = "";
    WLX_Color want = (WLX_Color){ 200, 100, 50, 255 };
    ibi_begin(&ctx, 200, 200);
    wlx_inputbox(&ctx, "", buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20), .image_size = 20,
                 .texture_tint = want);
    ibi_end(&ctx);

    ASSERT_EQ_COLOR(_ibi_tex_tint, want);
}

// ============================================================================
// Narrow-field guard: the band clamp must not crash or emit a negative-width
// destination. Runs under -DWLX_DEBUG so any internal assert would fail here.
// ============================================================================

TEST(inputbox_icon_tiny_field_no_negative_dims) {
    WLX_Context ctx;
    char buf[64] = "abcdef";
    ibi_begin(&ctx, 28, 28);
    wlx_inputbox(&ctx, "", buf, sizeof(buf), .font_size = 16,
                 .texture = ibi_make_texture(20, 20),
                 .image_size = 20, .image_text_gap = 8);
    ibi_end(&ctx);

    // The icon is clamped to the interior; the call completes and the recorded
    // destination has non-negative dimensions.
    ASSERT_TRUE(_ibi_tex_dst.w >= 0.0f);
    ASSERT_TRUE(_ibi_tex_dst.h >= 0.0f);
}

// ============================================================================
// Suite registration
// ============================================================================

SUITE(inputbox_icon) {
    RUN_TEST(inputbox_icon_unset_emits_no_texture);
    RUN_TEST(inputbox_no_regression_text_only);

    RUN_TEST(inputbox_icon_leading_anchors_left);
    RUN_TEST(inputbox_icon_trailing_anchors_right);

    RUN_TEST(inputbox_icon_centered_vertically_top_align);
    RUN_TEST(inputbox_icon_centered_vertically_bottom_align);

    RUN_TEST(inputbox_icon_default_tint_white);
    RUN_TEST(inputbox_icon_explicit_tint_passes_through);

    RUN_TEST(inputbox_icon_tiny_field_no_negative_dims);
}

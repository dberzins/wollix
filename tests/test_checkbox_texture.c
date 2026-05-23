// test_checkbox_texture.c - tests for image-capable wlx_checkbox texture mode:
// resolver tint defaults and opacity folding, both-textures-required activation
// helper, per-state source rect and tint selection, unset-src fallback to full
// texture, white-default tint with opacity, missing-texture fallback to the
// native path, hover-and-tint isolation, check_color isolation, and one-slot
// consumption.
// Included from test_main.c (single TU build).

// ============================================================================
// Recording stubs: capture texture, rect (native chrome fill), rect-rounded,
// rect-rounded-lines, and line (native checkmark) draws.
// ============================================================================

#define CBTX_MAX_DRAWS 8

static int       _cbtx_tex_count;
static WLX_Rect  _cbtx_tex_srcs[CBTX_MAX_DRAWS];
static WLX_Rect  _cbtx_tex_dsts[CBTX_MAX_DRAWS];
static WLX_Color _cbtx_tex_tints[CBTX_MAX_DRAWS];
static WLX_Rect  _cbtx_tex_src;
static WLX_Color _cbtx_tex_tint;

static int       _cbtx_rect_count;
static WLX_Color _cbtx_last_rect_color;
static int       _cbtx_rect_rounded_count;
static int       _cbtx_rect_rounded_lines_count;
static int       _cbtx_line_count;
static WLX_Color _cbtx_last_line_color;

static void cbtx_rec_draw_texture(WLX_Texture tex, WLX_Rect src, WLX_Rect dst, WLX_Color tint) {
    (void)tex;
    if (_cbtx_tex_count < CBTX_MAX_DRAWS) {
        _cbtx_tex_srcs[_cbtx_tex_count]  = src;
        _cbtx_tex_dsts[_cbtx_tex_count]  = dst;
        _cbtx_tex_tints[_cbtx_tex_count] = tint;
    }
    _cbtx_tex_count++;
    _cbtx_tex_src  = src;
    _cbtx_tex_tint = tint;
}

static void cbtx_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    _cbtx_rect_count++;
    _cbtx_last_rect_color = c;
}

static void cbtx_rec_draw_rect_rounded(WLX_Rect r, float roundness, int segments, WLX_Color c) {
    (void)r; (void)roundness; (void)segments; (void)c;
    _cbtx_rect_rounded_count++;
}

static void cbtx_rec_draw_rect_rounded_lines(WLX_Rect r, float roundness, int segments, float thick, WLX_Color c) {
    (void)r; (void)roundness; (void)segments; (void)thick; (void)c;
    _cbtx_rect_rounded_lines_count++;
}

static void cbtx_rec_draw_line(float x1, float y1, float x2, float y2, float thick, WLX_Color c) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)thick;
    _cbtx_line_count++;
    _cbtx_last_line_color = c;
}

static void cbtx_rec_reset(void) {
    _cbtx_tex_count = 0;
    memset(_cbtx_tex_srcs,  0, sizeof(_cbtx_tex_srcs));
    memset(_cbtx_tex_dsts,  0, sizeof(_cbtx_tex_dsts));
    memset(_cbtx_tex_tints, 0, sizeof(_cbtx_tex_tints));
    _cbtx_tex_src  = (WLX_Rect){0};
    _cbtx_tex_tint = (WLX_Color){0};
    _cbtx_rect_count = 0;
    _cbtx_last_rect_color = (WLX_Color){0};
    _cbtx_rect_rounded_count = 0;
    _cbtx_rect_rounded_lines_count = 0;
    _cbtx_line_count = 0;
    _cbtx_last_line_color = (WLX_Color){0};
}

static WLX_Backend cbtx_rec_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_texture            = cbtx_rec_draw_texture;
    b.draw_rect               = cbtx_rec_draw_rect;
    b.draw_rect_rounded       = cbtx_rec_draw_rect_rounded;
    b.draw_rect_rounded_lines = cbtx_rec_draw_rect_rounded_lines;
    b.draw_line               = cbtx_rec_draw_line;
    return b;
}

// ============================================================================
// Test fixtures
// ============================================================================

static WLX_Texture cbtx_make_texture(int w, int h) {
    return (WLX_Texture){ .handle = 0, .width = w, .height = h };
}

// Sharp-corner theme so the native path uses draw_rect (not draw_rect_rounded).
// This keeps assertions on "native chrome happened" simple.
static WLX_Theme cbtx_sharp_theme;

static void cbtx_ctx_init(WLX_Context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = cbtx_rec_backend();
    cbtx_sharp_theme = wlx_theme_dark;
    cbtx_sharp_theme.roundness = 0;
    cbtx_sharp_theme.rounded_segments = 0;
    ctx->theme = &cbtx_sharp_theme;
    ctx->rect = wlx_rect(0, 0, 200, 200);
}

static void cbtx_begin(WLX_Context *ctx, int mx, int my) {
    cbtx_rec_reset();
    cbtx_ctx_init(ctx);
    test_frame_begin(ctx, mx, my, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);
}

static void cbtx_end(WLX_Context *ctx) {
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_context_destroy(ctx);
}

// ============================================================================
// Activation helper: both textures required.
// ============================================================================

TEST(checkbox_texture_mode_active_both_drawable) {
    ASSERT_TRUE(wlx_checkbox_texture_mode_active(
        cbtx_make_texture(32, 32),
        cbtx_make_texture(32, 32)));
}

TEST(checkbox_texture_mode_inactive_when_unchecked_missing) {
    ASSERT_FALSE(wlx_checkbox_texture_mode_active(
        cbtx_make_texture(32, 32),
        cbtx_make_texture(0, 0)));
}

TEST(checkbox_texture_mode_inactive_when_checked_missing) {
    ASSERT_FALSE(wlx_checkbox_texture_mode_active(
        cbtx_make_texture(0, 0),
        cbtx_make_texture(32, 32)));
}

TEST(checkbox_texture_mode_inactive_when_both_missing) {
    ASSERT_FALSE(wlx_checkbox_texture_mode_active(
        cbtx_make_texture(0, 0),
        cbtx_make_texture(0, 0)));
}

// ============================================================================
// Resolver: tints default to WLX_WHITE and fold opacity alongside other colors.
// ============================================================================

TEST(checkbox_texture_tints_default_to_white) {
    WLX_Checkbox_Opt opt = wlx_default_checkbox_opt();
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;

    ASSERT_TRUE(wlx_color_is_zero(opt.tex_checked_tint));
    ASSERT_TRUE(wlx_color_is_zero(opt.tex_unchecked_tint));

    wlx_resolve_opt_checkbox(&ctx, &opt);

    ASSERT_EQ_COLOR(opt.tex_checked_tint,   WLX_WHITE);
    ASSERT_EQ_COLOR(opt.tex_unchecked_tint, WLX_WHITE);
}

TEST(checkbox_texture_tints_fold_opacity) {
    WLX_Checkbox_Opt opt = wlx_default_checkbox_opt(.opacity = 0.5f);
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;

    wlx_resolve_opt_checkbox(&ctx, &opt);

    // opacity=0.5 applied to alpha=255 -> 128 for both tints.
    ASSERT_EQ_INT(opt.tex_checked_tint.a,   128);
    ASSERT_EQ_INT(opt.tex_unchecked_tint.a, 128);
}

TEST(checkbox_texture_explicit_tints_preserved_and_folded) {
    WLX_Checkbox_Opt opt = wlx_default_checkbox_opt(
        .tex_checked_tint   = WLX_RGBA(10, 20, 30, 200),
        .tex_unchecked_tint = WLX_RGBA(40, 50, 60, 200),
        .opacity            = 0.5f);
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;

    wlx_resolve_opt_checkbox(&ctx, &opt);

    // RGB preserved; alpha folded: 200 * 0.5 = 100.
    ASSERT_EQ_INT(opt.tex_checked_tint.r,   10);
    ASSERT_EQ_INT(opt.tex_checked_tint.g,   20);
    ASSERT_EQ_INT(opt.tex_checked_tint.b,   30);
    ASSERT_EQ_INT(opt.tex_checked_tint.a,   100);
    ASSERT_EQ_INT(opt.tex_unchecked_tint.r, 40);
    ASSERT_EQ_INT(opt.tex_unchecked_tint.a, 100);
}

// ============================================================================
// Texture draw path: per-state source rect and tint selection.
// ============================================================================

TEST(checkbox_texture_checked_state_uses_checked_src_and_tint) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    WLX_Texture atlas = cbtx_make_texture(64, 64);
    wlx_checkbox(&ctx, "Sync", &checked,
        .tex_checked       = atlas,
        .tex_unchecked     = atlas,
        .tex_checked_src   = (WLX_Rect){0,  0, 32, 32},
        .tex_unchecked_src = (WLX_Rect){32, 0, 32, 32},
        .tex_checked_tint  = WLX_RGBA(255, 100, 50, 255),
        .tex_unchecked_tint = WLX_RGBA(60, 60, 60, 255));
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    ASSERT_EQ_F(_cbtx_tex_src.x, 0.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.y, 0.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.w, 32.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.h, 32.0f, 0.001f);
    ASSERT_EQ_INT(_cbtx_tex_tint.r, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.g, 100);
    ASSERT_EQ_INT(_cbtx_tex_tint.b, 50);
}

TEST(checkbox_texture_unchecked_state_uses_unchecked_src_and_tint) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = false;
    WLX_Texture atlas = cbtx_make_texture(64, 64);
    wlx_checkbox(&ctx, "Sync", &checked,
        .tex_checked       = atlas,
        .tex_unchecked     = atlas,
        .tex_checked_src   = (WLX_Rect){0,  0, 32, 32},
        .tex_unchecked_src = (WLX_Rect){32, 0, 32, 32},
        .tex_checked_tint  = WLX_RGBA(255, 100, 50, 255),
        .tex_unchecked_tint = WLX_RGBA(60, 60, 60, 255));
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    ASSERT_EQ_F(_cbtx_tex_src.x, 32.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.y, 0.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.w, 32.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.h, 32.0f, 0.001f);
    ASSERT_EQ_INT(_cbtx_tex_tint.r, 60);
    ASSERT_EQ_INT(_cbtx_tex_tint.g, 60);
    ASSERT_EQ_INT(_cbtx_tex_tint.b, 60);
}

TEST(checkbox_texture_unset_src_falls_back_to_full_texture_checked) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    WLX_Texture tex_c = cbtx_make_texture(48, 24);
    WLX_Texture tex_u = cbtx_make_texture(48, 24);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = tex_c,
        .tex_unchecked = tex_u);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    ASSERT_EQ_F(_cbtx_tex_src.x, 0.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.y, 0.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.w, 48.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.h, 24.0f, 0.001f);
}

TEST(checkbox_texture_unset_src_falls_back_to_full_texture_unchecked) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = false;
    WLX_Texture tex_c = cbtx_make_texture(48, 24);
    WLX_Texture tex_u = cbtx_make_texture(20, 40);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = tex_c,
        .tex_unchecked = tex_u);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    ASSERT_EQ_F(_cbtx_tex_src.w, 20.0f, 0.001f);
    ASSERT_EQ_F(_cbtx_tex_src.h, 40.0f, 0.001f);
}

TEST(checkbox_texture_unset_tint_resolves_to_white) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    WLX_Texture tex = cbtx_make_texture(32, 32);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = tex,
        .tex_unchecked = tex);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    ASSERT_EQ_INT(_cbtx_tex_tint.r, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.g, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.b, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.a, 255);
}

TEST(checkbox_texture_opacity_folds_into_tint_alpha) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    WLX_Texture tex = cbtx_make_texture(32, 32);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = tex,
        .tex_unchecked = tex,
        .opacity       = 0.25f);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    // opacity=0.25 applied to default-white alpha=255 -> 64 (roundf(63.75))
    ASSERT_EQ_INT(_cbtx_tex_tint.a, 64);
}

// ============================================================================
// Missing-texture fallback: both states must be drawable for texture mode.
// ============================================================================

TEST(checkbox_missing_unchecked_falls_back_to_native) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = false;
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = cbtx_make_texture(32, 32),
        .tex_unchecked = cbtx_make_texture(0, 0));
    cbtx_end(&ctx);

    // No texture draw and the native box fill ran.
    ASSERT_EQ_INT(_cbtx_tex_count, 0);
    ASSERT_TRUE(_cbtx_rect_count >= 1);
}

TEST(checkbox_missing_checked_falls_back_to_native) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked   = cbtx_make_texture(0, 0),
        .tex_unchecked = cbtx_make_texture(32, 32));
    cbtx_end(&ctx);

    // No texture draw; native box + checkmark lines ran.
    ASSERT_EQ_INT(_cbtx_tex_count, 0);
    ASSERT_TRUE(_cbtx_rect_count >= 1);
    ASSERT_TRUE(_cbtx_line_count >= 1);
}

TEST(checkbox_both_textures_missing_uses_native) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    wlx_checkbox(&ctx, "Hi", &checked);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 0);
    ASSERT_TRUE(_cbtx_rect_count >= 1);
    ASSERT_TRUE(_cbtx_line_count >= 1);
}

// ============================================================================
// Hover and check_color isolation: texture branch reads only the per-state tint.
// ============================================================================

TEST(checkbox_texture_hover_does_not_modulate_tint) {
    WLX_Context ctx;
    // Mouse inside the widget rect so inter.hover resolves true.
    cbtx_begin(&ctx, 100, 100);
    bool checked = true;
    WLX_Texture tex = cbtx_make_texture(32, 32);
    WLX_Color explicit_tint = WLX_RGBA(123, 200, 77, 255);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked      = tex,
        .tex_unchecked    = tex,
        .tex_checked_tint = explicit_tint);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    // Tint must equal the resolved tex_checked_tint without hover modulation.
    ASSERT_EQ_INT(_cbtx_tex_tint.r, 123);
    ASSERT_EQ_INT(_cbtx_tex_tint.g, 200);
    ASSERT_EQ_INT(_cbtx_tex_tint.b, 77);
    ASSERT_EQ_INT(_cbtx_tex_tint.a, 255);
}

TEST(checkbox_texture_check_color_not_used_in_texture_branch) {
    WLX_Context ctx;
    cbtx_begin(&ctx, -1, -1);
    bool checked = true;
    WLX_Texture tex = cbtx_make_texture(32, 32);
    WLX_Color distinctive = WLX_RGBA(7, 8, 9, 255);
    wlx_checkbox(&ctx, "Hi", &checked,
        .tex_checked      = tex,
        .tex_unchecked    = tex,
        .tex_checked_tint = WLX_RGBA(255, 255, 255, 255),
        .check_color      = distinctive);
    cbtx_end(&ctx);

    ASSERT_EQ_INT(_cbtx_tex_count, 1);
    // Texture tint stays the per-state tint; check_color must not appear.
    ASSERT_EQ_INT(_cbtx_tex_tint.r, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.g, 255);
    ASSERT_EQ_INT(_cbtx_tex_tint.b, 255);
    // No native checkmark lines drawn while texture mode is active.
    ASSERT_EQ_INT(_cbtx_line_count, 0);
    // No native chrome rect drawn while texture mode is active.
    ASSERT_EQ_INT(_cbtx_rect_count, 0);
    ASSERT_EQ_INT(_cbtx_rect_rounded_count, 0);
}

// ============================================================================
// Layout: each texture-mode checkbox consumes exactly one slot.
// ============================================================================

TEST(checkbox_texture_mode_consumes_one_slot) {
    WLX_Context ctx;
    cbtx_rec_reset();
    cbtx_ctx_init(&ctx);
    test_frame_begin(&ctx, -1, -1, false, false);

    bool a = true;
    bool b = false;
    WLX_Texture tex = cbtx_make_texture(32, 32);

    wlx_layout_begin(&ctx, 2, WLX_VERT);
    wlx_checkbox(&ctx, "A", &a, .tex_checked = tex, .tex_unchecked = tex);
    wlx_checkbox(&ctx, "B", &b, .tex_checked = tex, .tex_unchecked = tex);
    wlx_layout_end(&ctx);

    test_frame_end(&ctx);

    // Both calls produced exactly one texture draw each: one slot per call.
    ASSERT_EQ_INT(_cbtx_tex_count, 2);

    // Two stacked slots in 200x200 -> y splits at 100. Dst rects must land in
    // the upper and lower halves respectively (proves separate slots, not
    // overlapping the same one).
    bool first_in_upper  = _cbtx_tex_dsts[0].y < 100.0f;
    bool second_in_lower = _cbtx_tex_dsts[1].y >= 100.0f;
    ASSERT_TRUE(first_in_upper);
    ASSERT_TRUE(second_in_lower);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(checkbox_texture) {
    RUN_TEST(checkbox_texture_mode_active_both_drawable);
    RUN_TEST(checkbox_texture_mode_inactive_when_unchecked_missing);
    RUN_TEST(checkbox_texture_mode_inactive_when_checked_missing);
    RUN_TEST(checkbox_texture_mode_inactive_when_both_missing);
    RUN_TEST(checkbox_texture_tints_default_to_white);
    RUN_TEST(checkbox_texture_tints_fold_opacity);
    RUN_TEST(checkbox_texture_explicit_tints_preserved_and_folded);
    RUN_TEST(checkbox_texture_checked_state_uses_checked_src_and_tint);
    RUN_TEST(checkbox_texture_unchecked_state_uses_unchecked_src_and_tint);
    RUN_TEST(checkbox_texture_unset_src_falls_back_to_full_texture_checked);
    RUN_TEST(checkbox_texture_unset_src_falls_back_to_full_texture_unchecked);
    RUN_TEST(checkbox_texture_unset_tint_resolves_to_white);
    RUN_TEST(checkbox_texture_opacity_folds_into_tint_alpha);
    RUN_TEST(checkbox_missing_unchecked_falls_back_to_native);
    RUN_TEST(checkbox_missing_checked_falls_back_to_native);
    RUN_TEST(checkbox_both_textures_missing_uses_native);
    RUN_TEST(checkbox_texture_hover_does_not_modulate_tint);
    RUN_TEST(checkbox_texture_check_color_not_used_in_texture_branch);
    RUN_TEST(checkbox_texture_mode_consumes_one_slot);
}

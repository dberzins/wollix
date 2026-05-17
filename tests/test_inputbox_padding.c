// test_inputbox_padding.c - per-side and visual-stability tests for the
// WLX_Inputbox_Opt content padding migration. Asserts that the input
// background rect matches the resolved per-side padding, that the
// historical default visual (uniform content_padding = 10 -> text inset 5)
// is preserved, and that a tight slot triggers the shared clamp.

static int      _ibp_box_count;
static WLX_Rect _ibp_first_box;

static void ibp_capture_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)thick; (void)c;
    if (_ibp_box_count == 0) _ibp_first_box = r;
    _ibp_box_count++;
}

static void ibp_reset(void) {
    _ibp_box_count = 0;
    _ibp_first_box = (WLX_Rect){0};
}

static void ibp_ctx_init(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.draw_rect_lines = ibp_capture_rect_lines;
}

// Build a single fixed-height slot, run one inputbox with caller-supplied
// opt fields (the helper supplies a non-zero border so draw_rect_lines is
// recorded for the input box rect).
typedef struct {
    float root_w;
    float root_h;
    float slot_h;
    float content_padding;
    float content_padding_top;
    float content_padding_right;
    float content_padding_bottom;
    float content_padding_left;
} Ibp_Run_Opts;

static void ibp_run(const Ibp_Run_Opts *o) {
    WLX_Context ctx;
    ibp_reset();
    ibp_ctx_init(&ctx, o->root_w, o->root_h);

    char buf[8] = "";
    WLX_Slot_Size sizes[1] = { WLX_SLOT_PX(o->slot_h) };

    test_frame_begin(&ctx, -1, -1, false, false);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .sizes = sizes, .padding = 0, .gap = 0);
            wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
                wlx_default_inputbox_opt(
                    .height = o->slot_h,
                    .font_size = 10,
                    .border_width = 1.0f,
                    .border_color = (WLX_Color){255, 255, 255, 255},
                    .roundness = 0.0f,
                    .content_padding        = o->content_padding,
                    .content_padding_top    = o->content_padding_top,
                    .content_padding_right  = o->content_padding_right,
                    .content_padding_bottom = o->content_padding_bottom,
                    .content_padding_left   = o->content_padding_left),
                __FILE__, __LINE__);
        wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ----- Default visual stability --------------------------------------------

TEST(inputbox_padding_default_uniform_10_preserves_input_rect) {
    WLX_Context ctx;
    ibp_reset();
    ibp_ctx_init(&ctx, 200, 100);

    char buf[8] = "";
    WLX_Slot_Size sizes[1] = { WLX_SLOT_PX(100) };

    test_frame_begin(&ctx, -1, -1, false, false);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .sizes = sizes, .padding = 0, .gap = 0);
            // Default opts: content_padding = 10 baked in by wlx_default_inputbox_opt.
            wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
                wlx_default_inputbox_opt(
                    .height = 100, .font_size = 10,
                    .border_width = 1.0f,
                    .border_color = (WLX_Color){255, 255, 255, 255},
                    .roundness = 0.0f),
                __FILE__, __LINE__);
        wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);

    // Defaults: rp = (10, 10, 10, 10)
    ASSERT_TRUE(_ibp_box_count >= 1);
    ASSERT_EQ_RECT(_ibp_first_box, ((WLX_Rect){10, 10, 180, 80}), 0.1f);
}

// ----- Per-side override ----------------------------------------------------

TEST(inputbox_padding_per_side_shifts_input_rect) {
    Ibp_Run_Opts o = {
        .root_w = 200, .root_h = 100, .slot_h = 100,
        .content_padding = 6.0f, .content_padding_left = 20.0f,
        .content_padding_top = -1.0f, .content_padding_right = -1.0f,
        .content_padding_bottom = -1.0f,
    };
    ibp_run(&o);

    // rp = (top=6, right=6, bottom=6, left=20)
    // input_x = 0 + 0 (no label) + 20 = 20
    // input_w = 200 - 20 - 6 = 174
    // input_y = 0 + 6 = 6
    // input_h = 100 - 12 = 88
    ASSERT_TRUE(_ibp_box_count >= 1);
    ASSERT_EQ_RECT(_ibp_first_box, ((WLX_Rect){20, 6, 174, 88}), 0.1f);
}

// ----- Theme opt-in ---------------------------------------------------------

TEST(inputbox_padding_theme_opt_in_matches_theme_padding) {
    WLX_Context ctx;
    ibp_reset();
    ibp_ctx_init(&ctx, 200, 100);

    WLX_Theme padded_theme = wlx_theme_dark;
    padded_theme.padding = 6.0f;
    ctx.theme = &padded_theme;

    char buf[8] = "";
    WLX_Slot_Size sizes[1] = { WLX_SLOT_PX(100) };

    test_frame_begin(&ctx, -1, -1, false, false);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .sizes = sizes, .padding = 0, .gap = 0);
            wlx_inputbox_impl(&ctx, NULL, buf, sizeof(buf),
                wlx_default_inputbox_opt(
                    .height = 100, .font_size = 10,
                    .border_width = 1.0f,
                    .border_color = (WLX_Color){255, 255, 255, 255},
                    .roundness = 0.0f,
                    .content_padding = WLX_PADDING_USE_THEME),
                __FILE__, __LINE__);
        wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);

    // rp = (6, 6, 6, 6) from theme.padding
    ASSERT_TRUE(_ibp_box_count >= 1);
    ASSERT_EQ_RECT(_ibp_first_box, ((WLX_Rect){6, 6, 188, 88}), 0.1f);
}

// ----- Clamp on tight slot --------------------------------------------------

TEST(inputbox_padding_clamp_on_tight_slot_never_negative) {
    Ibp_Run_Opts o = {
        .root_w = 200, .root_h = 30, .slot_h = 30,
        .content_padding = 30.0f, .content_padding_top = -1.0f,
        .content_padding_right = -1.0f, .content_padding_bottom = -1.0f,
        .content_padding_left = -1.0f,
    };
    ibp_run(&o);

    // wr.h is bumped to (font_size + 60 + 4) = 74, but slot fixed at 30,
    // so wr.h = 30. The shared clamp scales vertical pair down so the
    // input_rect never has negative dimensions even when padding would
    // otherwise overflow.
    ASSERT_TRUE(_ibp_box_count >= 1);
    ASSERT_TRUE(_ibp_first_box.w >= 0.0f);
    ASSERT_TRUE(_ibp_first_box.h >= 0.0f);
    ASSERT_TRUE(_ibp_first_box.w <= 200.0f);
    ASSERT_TRUE(_ibp_first_box.h <= 30.0f);
}

SUITE(inputbox_padding) {
    RUN_TEST(inputbox_padding_default_uniform_10_preserves_input_rect);
    RUN_TEST(inputbox_padding_per_side_shifts_input_rect);
    RUN_TEST(inputbox_padding_theme_opt_in_matches_theme_padding);
    RUN_TEST(inputbox_padding_clamp_on_tight_slot_never_negative);
}

// test_slice_backend.c - slice-only backend contract.
// Included from test_main.c (single TU build, after test_mock_backend.h).
//
// A backend that implements only the slice text callbacks (draw_text_slice /
// measure_text_slice, with the legacy NUL-terminated pair left NULL) must
// pass the readiness check and render text through both the immediate and
// the deferred (command replay) paths.

// ============================================================================
// Slice-only recording callbacks
// ============================================================================

static int    _slice_draw_calls = 0;
static int    _slice_measure_calls = 0;
static char   _slice_last_text[64];
static size_t _slice_last_len = 0;

static void slice_only_draw_text(const char *text, size_t len, float x, float y, WLX_Text_Style style) {
    (void)x; (void)y; (void)style;
    _slice_draw_calls++;
    _slice_last_len = len;
    size_t n = len < sizeof(_slice_last_text) - 1 ? len : sizeof(_slice_last_text) - 1;
    memcpy(_slice_last_text, text, n);
    _slice_last_text[n] = '\0';
}

// Same proportional model as mock_measure_text so layout stays deterministic.
static void slice_only_measure_text(const char *text, size_t len, WLX_Text_Style style, float *out_w, float *out_h) {
    (void)text;
    _slice_measure_calls++;
    int fs = style.font_size > 0 ? style.font_size : 10;
    if (out_w) *out_w = (float)len * (float)fs * 0.5f;
    if (out_h) *out_h = (float)fs;
}

static void slice_backend_ctx_init(WLX_Context *ctx, float w, float h) {
    test_ctx_init(ctx, w, h);
    ctx->backend.draw_text          = NULL;
    ctx->backend.measure_text       = NULL;
    ctx->backend.draw_text_slice    = slice_only_draw_text;
    ctx->backend.measure_text_slice = slice_only_measure_text;
    _slice_draw_calls = 0;
    _slice_measure_calls = 0;
    _slice_last_text[0] = '\0';
    _slice_last_len = 0;
}

// ============================================================================
// Tests
// ============================================================================

TEST(slice_only_backend_is_ready) {
    WLX_Context ctx;
    slice_backend_ctx_init(&ctx, 400, 300);
    ASSERT_TRUE(wlx_backend_is_ready(&ctx));
    wlx_context_destroy(&ctx);
}

// Legacy-only backends (the mock default) must stay ready too.
TEST(legacy_only_backend_is_ready) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text_slice = NULL;
    ctx.backend.measure_text_slice = NULL;
    ASSERT_TRUE(wlx_backend_is_ready(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(backend_without_any_text_callback_not_ready) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = NULL;
    ctx.backend.measure_text = NULL;
    ctx.backend.draw_text_slice = NULL;
    ctx.backend.measure_text_slice = NULL;
    ASSERT_FALSE(wlx_backend_is_ready(&ctx));
    wlx_context_destroy(&ctx);
}

// Deferred path: a label records a text command; wlx_end replays it through
// draw_text_slice with the exact byte span.
TEST(slice_only_backend_renders_deferred_text) {
    WLX_Context ctx;
    slice_backend_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_label(&ctx, "sliced");
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(_slice_measure_calls > 0);
    ASSERT_TRUE(_slice_draw_calls > 0);
    ASSERT_EQ_INT((int)_slice_last_len, 6);
    ASSERT_TRUE(strcmp(_slice_last_text, "sliced") == 0);
    wlx_context_destroy(&ctx);
}

// Immediate path: draws bypass the command buffer and hit the slice callback
// directly.
TEST(slice_only_backend_renders_immediate_text) {
    WLX_Context ctx;
    slice_backend_ctx_init(&ctx, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    ctx.immediate_mode = true;
    wlx_draw_text(&ctx, "now", 10.0f, 10.0f,
        (WLX_Text_Style){ .font_size = 12, .color = WLX_WHITE });
    ASSERT_EQ_INT(_slice_draw_calls, 1);
    ASSERT_EQ_INT((int)_slice_last_len, 3);
    ASSERT_TRUE(strcmp(_slice_last_text, "now") == 0);
    ctx.immediate_mode = false;
    test_frame_end(&ctx);

    wlx_context_destroy(&ctx);
}

SUITE(slice_backend) {
    RUN_TEST(slice_only_backend_is_ready);
    RUN_TEST(legacy_only_backend_is_ready);
    RUN_TEST(backend_without_any_text_callback_not_ready);
    RUN_TEST(slice_only_backend_renders_deferred_text);
    RUN_TEST(slice_only_backend_renders_immediate_text);
}

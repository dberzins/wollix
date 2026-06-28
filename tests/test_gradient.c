// test_gradient.c - vertical two-stop gradient fill primitive tests.
// Included from test_main.c (single TU build).
//
// Covers the gradient contract: the zero-stop sentinel (solid fill kept), the
// single-stop solid-via-gradient case, software-fallback band geometry and
// colours, opacity premultiply at the widget resolve step, render order (gradient
// in place of the fill, after effects, before borders), the gradient-only
// container decor gate, replay dispatch (native callback vs. software fallback),
// and the PERF histogram bucket.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// Recording backend (prefixed gv_) - captures fallback bands and native calls.
// ============================================================================

static int gv_rect_n, gv_native_n;
static WLX_Rect gv_band[512];
static WLX_Color gv_band_color[512];
static WLX_Color gv_native_top, gv_native_bottom;
static float gv_native_roundness;

static void gv_rec_rect(WLX_Rect r, WLX_Color c) {
    if (gv_rect_n < 512) { gv_band[gv_rect_n] = r; gv_band_color[gv_rect_n] = c; }
    gv_rect_n++;
}
static void gv_native_gradient(WLX_Rect r, WLX_Color top, WLX_Color bottom,
        float ro, int sg) {
    (void)r; (void)sg;
    gv_native_n++; gv_native_top = top; gv_native_bottom = bottom; gv_native_roundness = ro;
}
static void gv_reset(void) {
    gv_rect_n = gv_native_n = 0;
    gv_native_top = gv_native_bottom = (WLX_Color){0};
    gv_native_roundness = 0;
}

static int gv_find_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) return (int)i;
    return -1;
}
static int gv_count_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    int n = 0;
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) n++;
    return n;
}

// ============================================================================
// Sentinel and single-stop resolution
// ============================================================================

TEST(gradient_sentinel_keeps_solid_fill) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    // gradient_top = {0} -> no gradient; the solid fill renders unchanged.
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}});
    ASSERT_EQ_INT(0, gv_count_cmd(&ctx, WLX_CMD_GRADIENT_V));
    ASSERT_TRUE(gv_find_cmd(&ctx, WLX_CMD_RECT) >= 0);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(gradient_single_stop_is_uniform) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    // Only gradient_top set: gradient_bottom {0} resolves to gradient_top, so
    // the emitted command is a uniform fill expressed through the gradient path.
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .gradient_top = {10, 20, 30, 255}});
    int i = gv_find_cmd(&ctx, WLX_CMD_GRADIENT_V);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ_INT(1, gv_count_cmd(&ctx, WLX_CMD_GRADIENT_V));
    WLX_Cmd *g = &wlx_pool_commands(&ctx)[i];
    ASSERT_EQ_COLOR(g->data.gradient_v.top, ((WLX_Color){10, 20, 30, 255}));
    ASSERT_EQ_COLOR(g->data.gradient_v.bottom, ((WLX_Color){10, 20, 30, 255}));
    // No solid fill is emitted when the gradient is active.
    ASSERT_EQ_INT(0, gv_count_cmd(&ctx, WLX_CMD_RECT));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Software fallback band geometry / colours
// ============================================================================

TEST(gradient_fallback_band_geometry) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_rect = gv_rec_rect; // capture each band
    // draw_gradient_v left NULL -> software fallback.
    ctx.theme = &wlx_theme_dark;

    gv_reset();
    WLX_Color top = {0, 0, 0, 255}, bottom = {100, 0, 0, 255};
    // h = 40 -> bands = 40 / 4 = 10; band_h = 4; each band rect h = band_h + 1.
    wlx_render_gradient_v(&ctx, (WLX_Rect){10, 20, 50, 40}, top, bottom, 0.0f, 0);
    ASSERT_EQ_INT(10, gv_rect_n);
    ASSERT_EQ_RECT(gv_band[0], ((WLX_Rect){10, 20, 50, 5}), 0.001f);
    ASSERT_EQ_RECT(gv_band[9], ((WLX_Rect){10, 56, 50, 5}), 0.001f); // y = 20 + 9*4
    // Endpoints land exactly on the stops; the midpoint is the lerp.
    ASSERT_EQ_COLOR(gv_band_color[0], top);
    ASSERT_EQ_COLOR(gv_band_color[9], bottom);
    ASSERT_EQ_INT(56, gv_band_color[5].r); // round(100 * 5/9) = 56

    // A short rect still produces at least one band.
    gv_reset();
    wlx_render_gradient_v(&ctx, (WLX_Rect){0, 0, 10, 2}, top, bottom, 0.0f, 0);
    ASSERT_EQ_INT(1, gv_rect_n);
    ASSERT_EQ_COLOR(gv_band_color[0], top); // single band uses t = 0
}

// ============================================================================
// Opacity: effective widget opacity premultiplies both stops once
// ============================================================================

TEST(gradient_opacity_scales_both_stops) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_widget(&ctx, .width = 50, .height = 50, .opacity = 0.5f,
        .gradient_top = (WLX_Color){0, 200, 255, 200},
        .gradient_bottom = (WLX_Color){255, 0, 0, 100});
    int i = gv_find_cmd(&ctx, WLX_CMD_GRADIENT_V);
    ASSERT_TRUE(i >= 0);
    WLX_Cmd *g = &wlx_pool_commands(&ctx)[i];
    ASSERT_EQ_INT(100, g->data.gradient_v.top.a);    // 200 * 0.5
    ASSERT_EQ_INT(50,  g->data.gradient_v.bottom.a); // 100 * 0.5
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Render order: gradient replaces the fill, after effects, before borders
// ============================================================================

TEST(gradient_render_order_replaces_fill) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255},
        .border = {255, 255, 255, 255}, .border_width = 2,
        .shadow_color = {0, 0, 0, 200}, .glow_color = {0, 200, 255, 180},
        .gradient_top = {10, 20, 30, 255}, .gradient_bottom = {40, 50, 60, 255}});
    int si  = gv_find_cmd(&ctx, WLX_CMD_SHADOW);
    int gi  = gv_find_cmd(&ctx, WLX_CMD_GLOW);
    int gvi = gv_find_cmd(&ctx, WLX_CMD_GRADIENT_V);
    int bi  = gv_find_cmd(&ctx, WLX_CMD_RECT_LINES); // sharp border (roundness 0)
    ASSERT_TRUE(si >= 0 && gi >= 0 && gvi >= 0 && bi >= 0);
    ASSERT_TRUE(si < gvi && gi < gvi); // soft effects precede the gradient
    ASSERT_TRUE(gvi < bi);             // gradient precedes the border
    ASSERT_EQ_INT(0, gv_count_cmd(&ctx, WLX_CMD_RECT)); // no solid fill emitted
    ASSERT_EQ_INT(1, gv_count_cmd(&ctx, WLX_CMD_GRADIENT_V));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Decor gate: a gradient-only container still emits the command
// ============================================================================

TEST(gradient_container_decor_gate) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    // No back_color, no border - only a gradient. The decor gate must still fire.
    wlx_layout_begin(&ctx, 1, WLX_VERT, .gradient_top = (WLX_Color){10, 20, 30, 255});
    wlx_layout_end(&ctx);
    ASSERT_EQ_INT(1, gv_count_cmd(&ctx, WLX_CMD_GRADIENT_V));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Replay dispatch: software fallback vs. native callback
// ============================================================================

TEST(gradient_dispatch_fallback_then_native) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_rect = gv_rec_rect;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    // draw_gradient_v NULL -> fallback bands at replay (h = 40 -> 10 bands).
    gv_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_gradient_v(&ctx, (WLX_Rect){0, 0, 50, 40},
        (WLX_Color){0, 0, 0, 255}, (WLX_Color){100, 0, 0, 255}, 0.0f, 0);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(10, gv_rect_n);
    ASSERT_EQ_INT(0, gv_native_n);

    // Native callback set -> called once, no fallback bands; stops forwarded.
    ctx.backend.draw_gradient_v = gv_native_gradient;
    gv_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_gradient_v(&ctx, (WLX_Rect){0, 0, 50, 40},
        (WLX_Color){0, 0, 0, 255}, (WLX_Color){100, 0, 0, 255}, 0.0f, 0);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(1, gv_native_n);
    ASSERT_EQ_INT(0, gv_rect_n);
    ASSERT_EQ_COLOR(gv_native_top, ((WLX_Color){0, 0, 0, 255}));
    ASSERT_EQ_COLOR(gv_native_bottom, ((WLX_Color){100, 0, 0, 255}));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// PERF: one gradient command per element; fallback bands not double-counted
// ============================================================================

#ifdef WLX_PERF
TEST(gradient_perf_counts_one_per_element) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300); // mock backend -> draw_gradient_v NULL (fallback)

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT, .gradient_top = (WLX_Color){10, 20, 30, 255});
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    const WLX_Perf_Frame *f = wlx_perf_get_last_frame(&ctx);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_INT(1, (int)f->commands.by_type[WLX_CMD_GRADIENT_V]);
    // Fallback bands are immediate backend draws, not recorded commands.
    ASSERT_EQ_INT(0, (int)f->commands.by_type[WLX_CMD_RECT]);
    wlx_context_destroy(&ctx);
}
#endif

// ============================================================================
// Suite
// ============================================================================

SUITE(gradient) {
    RUN_TEST(gradient_sentinel_keeps_solid_fill);
    RUN_TEST(gradient_single_stop_is_uniform);
    RUN_TEST(gradient_fallback_band_geometry);
    RUN_TEST(gradient_opacity_scales_both_stops);
    RUN_TEST(gradient_render_order_replaces_fill);
    RUN_TEST(gradient_container_decor_gate);
    RUN_TEST(gradient_dispatch_fallback_then_native);
#ifdef WLX_PERF
    RUN_TEST(gradient_perf_counts_one_per_element);
#endif
}

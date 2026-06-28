// test_glow_shadow.c - first-class glow / shadow decoration tests.
// Included from test_main.c (single TU build).
//
// Covers the soft-effect contract: pure fallback geometry/alpha helpers and
// their parity with the dashboard demo, theme/explicit numeric-knob resolution,
// the zero-color sentinel, opacity premultiply at the resolve step, render
// order (effect command before fill), and replay dispatch (software fallback
// counts vs. native callback) including the immediate-mode path.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// Demo glow alpha helper, for the cross-parity check (header is guarded, so a
// later include from test_dashboard_effects.c is a no-op).
#include "demos/dashboard/dashboard_effects.h"

// ============================================================================
// Recording backend (prefixed gs_) - counts fallback draws and native calls.
// ============================================================================

static int gs_rr, gs_rl, gs_shadow_calls, gs_glow_calls;
static WLX_Color gs_last_shadow_color, gs_last_glow_color;
static int gs_last_shadow_layers, gs_last_glow_rings;

static void gs_rec_rr(WLX_Rect r, float ro, int s, WLX_Color c) {
    (void)r; (void)ro; (void)s; (void)c; gs_rr++;
}
static void gs_rec_rl(WLX_Rect r, float ro, int s, float t, WLX_Color c) {
    (void)r; (void)ro; (void)s; (void)t; (void)c; gs_rl++;
}
static void gs_native_shadow(WLX_Rect r, WLX_Color c, float ox, float oy,
        float bl, int ly, float ro, int sg) {
    (void)r; (void)ox; (void)oy; (void)bl; (void)ro; (void)sg;
    gs_shadow_calls++; gs_last_shadow_color = c; gs_last_shadow_layers = ly;
}
static void gs_native_glow(WLX_Rect r, WLX_Color c, float sp, int ri,
        float ro, int sg) {
    (void)r; (void)sp; (void)ro; (void)sg;
    gs_glow_calls++; gs_last_glow_color = c; gs_last_glow_rings = ri;
}
static void gs_reset(void) {
    gs_rr = gs_rl = gs_shadow_calls = gs_glow_calls = 0;
    gs_last_shadow_layers = gs_last_glow_rings = 0;
    gs_last_shadow_color = (WLX_Color){0};
    gs_last_glow_color = (WLX_Color){0};
}

static int gs_find_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) return (int)i;
    return -1;
}
static int gs_count_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    int n = 0;
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) n++;
    return n;
}

// ============================================================================
// Pure fallback geometry / alpha
// ============================================================================

TEST(glow_shadow_shadow_layer_rect_geometry) {
    WLX_Rect r = {10, 20, 100, 40};
    // offset (0,8), 4 layers: layer i shifts by t = i/4 of the offset.
    ASSERT_EQ_RECT(wlx_shadow_layer_rect(r, 0, 8, 4, 4), ((WLX_Rect){10, 28, 100, 40}), 0.001f);
    ASSERT_EQ_RECT(wlx_shadow_layer_rect(r, 0, 8, 4, 1), ((WLX_Rect){10, 22, 100, 40}), 0.001f);
    // offset_x participates too: i=2 -> t=0.5 -> +2,+4.
    ASSERT_EQ_RECT(wlx_shadow_layer_rect(r, 4, 8, 4, 2), ((WLX_Rect){12, 24, 100, 40}), 0.001f);
    // i clamps into [1..layers]; width/height never change.
    ASSERT_EQ_RECT(wlx_shadow_layer_rect(r, 0, 8, 4, 9), ((WLX_Rect){10, 28, 100, 40}), 0.001f);
}

TEST(glow_shadow_shadow_layer_alpha_formula) {
    // base=200, layers=4 -> round(200 * i / 16); alpha grows with i.
    ASSERT_EQ_INT(13, wlx_shadow_layer_alpha(200, 4, 1)); // 12.5 -> 13
    ASSERT_EQ_INT(25, wlx_shadow_layer_alpha(200, 4, 2));
    ASSERT_EQ_INT(38, wlx_shadow_layer_alpha(200, 4, 3)); // 37.5 -> 38
    ASSERT_EQ_INT(50, wlx_shadow_layer_alpha(200, 4, 4));
    ASSERT_TRUE(wlx_shadow_layer_alpha(200, 4, 1) < wlx_shadow_layer_alpha(200, 4, 4));
}

TEST(glow_shadow_glow_ring_rect_geometry) {
    WLX_Rect r = {10, 20, 100, 40};
    // spread=6, 3 rings: grow_i = 6 * (i+1)/3; ring grows outward on all sides.
    ASSERT_EQ_RECT(wlx_glow_ring_rect(r, 6, 3, 0), ((WLX_Rect){8, 18, 104, 44}), 0.001f);
    ASSERT_EQ_RECT(wlx_glow_ring_rect(r, 6, 3, 2), ((WLX_Rect){4, 14, 112, 52}), 0.001f);
    // outermost ring is the largest.
    WLX_Rect inner = wlx_glow_ring_rect(r, 6, 3, 0);
    WLX_Rect outer = wlx_glow_ring_rect(r, 6, 3, 2);
    ASSERT_TRUE(outer.w > inner.w && outer.h > inner.h);
}

TEST(glow_shadow_glow_ring_alpha_formula) {
    // base=180, rings=3 -> round(180 * (3-i)/3); ring 0 strongest.
    ASSERT_EQ_INT(180, wlx_glow_ring_alpha(180, 3, 0));
    ASSERT_EQ_INT(120, wlx_glow_ring_alpha(180, 3, 1));
    ASSERT_EQ_INT(60,  wlx_glow_ring_alpha(180, 3, 2));
    ASSERT_TRUE(wlx_glow_ring_alpha(180, 3, 0) > wlx_glow_ring_alpha(180, 3, 2));
}

TEST(glow_shadow_glow_alpha_parity_with_dashboard) {
    // The library glow alpha must equal the demo's dashboard_glow_ring_alpha
    // exactly (same falloff), proving the port preserved the look.
    for (int n = 1; n <= 6; n++)
        for (int i = 0; i < n; i++)
            for (int base = 0; base <= 255; base += 17)
                ASSERT_EQ_INT(dashboard_glow_ring_alpha(i, n, (uint8_t)base),
                              wlx_glow_ring_alpha((uint8_t)base, n, i));
}

// ============================================================================
// Numeric-knob resolution (recorded payload carries the resolved values)
// ============================================================================

TEST(glow_shadow_default_resolution) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark; // presets zero-init shadow/glow -> hard fallbacks
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .shadow_color = {0, 0, 0, 200}, .glow_color = {0, 200, 255, 180}});
    int si = gs_find_cmd(&ctx, WLX_CMD_SHADOW);
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    ASSERT_TRUE(si >= 0 && gi >= 0);
    WLX_Cmd *s = &wlx_pool_commands(&ctx)[si];
    WLX_Cmd *g = &wlx_pool_commands(&ctx)[gi];
    ASSERT_EQ_F(8.0f, s->data.shadow.blur, 0.001f);
    ASSERT_EQ_INT(4, s->data.shadow.layers);
    ASSERT_EQ_F(4.0f, g->data.glow.spread, 0.001f);
    ASSERT_EQ_INT(3, g->data.glow.rings);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_theme_resolution) {
    WLX_Theme th = wlx_theme_dark;
    th.shadow.blur = 5.0f; th.shadow.layers = 2;
    th.glow.spread = 7.0f; th.glow.rings = 6;
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &th;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .shadow_color = {0, 0, 0, 200}, .glow_color = {0, 200, 255, 180}});
    WLX_Cmd *s = &wlx_pool_commands(&ctx)[gs_find_cmd(&ctx, WLX_CMD_SHADOW)];
    WLX_Cmd *g = &wlx_pool_commands(&ctx)[gs_find_cmd(&ctx, WLX_CMD_GLOW)];
    ASSERT_EQ_F(5.0f, s->data.shadow.blur, 0.001f);
    ASSERT_EQ_INT(2, s->data.shadow.layers);
    ASSERT_EQ_F(7.0f, g->data.glow.spread, 0.001f);
    ASSERT_EQ_INT(6, g->data.glow.rings);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_explicit_override_wins) {
    WLX_Theme th = wlx_theme_dark;
    th.shadow.blur = 5.0f; th.shadow.layers = 2; // theme set, but per-call wins
    th.glow.spread = 7.0f; th.glow.rings = 6;
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &th;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .shadow_color = {0, 0, 0, 200}, .shadow_blur = 12.0f, .shadow_layers = 9,
        .glow_color = {0, 200, 255, 180}, .glow_spread = 3.0f, .glow_rings = 11});
    WLX_Cmd *s = &wlx_pool_commands(&ctx)[gs_find_cmd(&ctx, WLX_CMD_SHADOW)];
    WLX_Cmd *g = &wlx_pool_commands(&ctx)[gs_find_cmd(&ctx, WLX_CMD_GLOW)];
    ASSERT_EQ_F(12.0f, s->data.shadow.blur, 0.001f);
    ASSERT_EQ_INT(9, s->data.shadow.layers);
    ASSERT_EQ_F(3.0f, g->data.glow.spread, 0.001f);
    ASSERT_EQ_INT(11, g->data.glow.rings);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Sentinel: zero effect color emits no command
// ============================================================================

TEST(glow_shadow_sentinel_emits_nothing) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}}); // no effect colors set
    ASSERT_EQ_INT(0, gs_count_cmd(&ctx, WLX_CMD_SHADOW));
    ASSERT_EQ_INT(0, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Render order: effect commands precede the fill in the buffer
// ============================================================================

TEST(glow_shadow_render_order_before_fill) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255},
        .shadow_color = {0, 0, 0, 200}, .glow_color = {0, 200, 255, 180}});
    int si = gs_find_cmd(&ctx, WLX_CMD_SHADOW);
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    int fi = gs_find_cmd(&ctx, WLX_CMD_RECT); // sharp fill (roundness 0)
    ASSERT_TRUE(si >= 0 && gi >= 0 && fi >= 0);
    ASSERT_TRUE(si < fi); // shadow behind fill
    ASSERT_TRUE(gi < fi); // glow behind fill
    ASSERT_TRUE(si < gi); // shadow emitted before glow
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Opacity: effective opacity premultiplies the effect color at resolve step
// ============================================================================

TEST(glow_shadow_opacity_scales_effect_color) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 300);

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_button(&ctx, "X", .opacity = 0.5f,
        .shadow_color = (WLX_Color){0, 0, 0, 200},
        .glow_color = (WLX_Color){0, 200, 255, 180});
    int si = gs_find_cmd(&ctx, WLX_CMD_SHADOW);
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    ASSERT_TRUE(si >= 0 && gi >= 0);
    ASSERT_EQ_INT(100, wlx_pool_commands(&ctx)[si].data.shadow.color.a); // 200 * 0.5
    ASSERT_EQ_INT(90,  wlx_pool_commands(&ctx)[gi].data.glow.color.a);   // 180 * 0.5
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Replay dispatch: software fallback counts and native-callback routing
// ============================================================================

TEST(glow_shadow_fallback_replay_counts) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_rect_rounded = gs_rec_rr;
    ctx.backend.draw_rect_rounded_lines = gs_rec_rl;
    // draw_shadow / draw_glow left NULL -> software fallback.
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    gs_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_shadow(&ctx, (WLX_Rect){0, 0, 50, 50},
        (WLX_Color){0, 0, 0, 200}, 0, 8, 8, 4, 0, 8);
    test_frame_end(&ctx); // replay
    ASSERT_EQ_INT(4, gs_rr); // one rounded rect per shadow layer
    ASSERT_EQ_INT(0, gs_rl);

    gs_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_glow(&ctx, (WLX_Rect){0, 0, 50, 50},
        (WLX_Color){0, 200, 255, 180}, 4, 3, 0, 8);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(0, gs_rr);
    ASSERT_EQ_INT(3, gs_rl); // one rounded outline per glow ring
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_native_callback_used) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_rect_rounded = gs_rec_rr;
    ctx.backend.draw_rect_rounded_lines = gs_rec_rl;
    ctx.backend.draw_shadow = gs_native_shadow;
    ctx.backend.draw_glow = gs_native_glow;
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 200, 200);

    gs_reset();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_cmd_record_shadow(&ctx, (WLX_Rect){0, 0, 50, 50},
        (WLX_Color){0, 0, 0, 200}, 0, 8, 8, 4, 0, 8);
    wlx_cmd_record_glow(&ctx, (WLX_Rect){0, 0, 50, 50},
        (WLX_Color){0, 200, 255, 180}, 4, 3, 0, 8);
    test_frame_end(&ctx);
    ASSERT_EQ_INT(1, gs_shadow_calls); // exactly one native call each
    ASSERT_EQ_INT(1, gs_glow_calls);
    ASSERT_EQ_INT(0, gs_rr);           // and zero fallback draws
    ASSERT_EQ_INT(0, gs_rl);
    ASSERT_EQ_INT(4, gs_last_shadow_layers); // params forwarded intact
    ASSERT_EQ_INT(3, gs_last_glow_rings);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_immediate_mode_renders) {
    // Immediate mode has no replay pass: wlx_draw_box must render effects now.
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.backend.draw_rect_rounded = gs_rec_rr;
    ctx.backend.draw_rect_rounded_lines = gs_rec_rl;
    ctx.theme = &wlx_theme_dark;
    ctx.immediate_mode = true;

    gs_reset();
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 50, 50}, (WLX_Box_Style){
        .shadow_color = {0, 0, 0, 200}, .shadow_layers = 4,
        .glow_color = {0, 200, 255, 180}, .glow_rings = 3});
    ASSERT_EQ_INT(4, gs_rr); // shadow fallback drew immediately
    ASSERT_EQ_INT(3, gs_rl); // glow fallback drew immediately
}

// ============================================================================
// Per-widget effect targets: progress run / track, slider + toggle thumb.
// These widgets inherit the effect fields but decorate a semantic sub-rect
// (not a single full-element chrome box), so each is verified end to end.
// ============================================================================

static bool gs_rect_eq(WLX_Rect a, WLX_Rect b) {
    const float e = 0.01f;
    return fabsf(a.x - b.x) < e && fabsf(a.y - b.y) < e &&
           fabsf(a.w - b.w) < e && fabsf(a.h - b.h) < e;
}

// Index of the first RECT_ROUNDED whose rect matches `r`, at or after `from`.
static int gs_find_rect_rounded_at(WLX_Context *ctx, WLX_Rect r, int from) {
    for (size_t i = (size_t)(from < 0 ? 0 : from); i < ctx->arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(ctx)[i];
        if (c->type == WLX_CMD_RECT_ROUNDED && gs_rect_eq(c->data.rect_rounded.rect, r))
            return (int)i;
    }
    return -1;
}

// Index of the first CIRCLE inscribed in `r` (a square rect drawn with
// roundness 1.0 dispatches to draw_circle), at or after `from`.
static int gs_find_circle_at(WLX_Context *ctx, WLX_Rect r, int from) {
    float cx = r.x + r.w * 0.5f, cy = r.y + r.h * 0.5f, rad = r.w * 0.5f;
    for (size_t i = (size_t)(from < 0 ? 0 : from); i < ctx->arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(ctx)[i];
        if (c->type == WLX_CMD_CIRCLE &&
            fabsf(c->data.circle.cx - cx) < 0.01f &&
            fabsf(c->data.circle.cy - cy) < 0.01f &&
            fabsf(c->data.circle.radius - rad) < 0.01f)
            return (int)i;
    }
    return -1;
}

TEST(glow_shadow_progress_segmented_run_glow) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);

    WLX_Color fillc = {10, 20, 30, 255}; // unique: marks the filled cells

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    // value 0.5 of 4 segments -> 2 filled cells; run spans cell 0..1.
    wlx_progress(&ctx, 0.5f,
        .segments = 4, .segment_gap = 4,
        .fill_color = fillc, .track_color = (WLX_Color){40, 40, 40, 255},
        .roundness = 0.2f, .rounded_segments = 4,
        .glow_color = (WLX_Color){0, 200, 255, 180}, .glow_spread = 3.0f, .glow_rings = 2);

    ASSERT_EQ_INT(1, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    int fi = gs_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED); // first cell fill
    ASSERT_TRUE(gi >= 0 && fi >= 0);
    ASSERT_TRUE(gi < fi); // glow sits behind the cells

    // The glow rect is the union of the filled cells (first..last filled).
    WLX_Rect run = wlx_pool_commands(&ctx)[gi].data.glow.rect;
    float minx = 1e9f, maxr = -1e9f, cy = 0, ch = 0;
    int found = 0;
    for (size_t i = 0; i < ctx.arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(&ctx)[i];
        if (c->type == WLX_CMD_RECT_ROUNDED && wlx_color_eq(c->data.rect_rounded.color, fillc)) {
            WLX_Rect cr = c->data.rect_rounded.rect;
            if (cr.x < minx) minx = cr.x;
            if (cr.x + cr.w > maxr) maxr = cr.x + cr.w;
            cy = cr.y; ch = cr.h; found++;
        }
    }
    ASSERT_EQ_INT(2, found); // exactly the two filled cells
    ASSERT_TRUE(gs_rect_eq(run, ((WLX_Rect){ minx, cy, maxr - minx, ch })));

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_progress_segmented_gating) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);

    // filled == 0 (value 0): no effect even with a glow color set.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_progress(&ctx, 0.0f, .segments = 4, .segment_gap = 4,
        .glow_color = (WLX_Color){0, 200, 255, 180});
    ASSERT_EQ_INT(0, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // filled > 0 but no effect color: nothing emitted.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_progress(&ctx, 0.75f, .segments = 4, .segment_gap = 4);
    ASSERT_EQ_INT(0, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    ASSERT_EQ_INT(0, gs_count_cmd(&ctx, WLX_CMD_SHADOW));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_progress_continuous_track) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);

    WLX_Color trackc = {40, 40, 40, 255};

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    // Continuous mode (segments == 0): effects forwarded onto the track box.
    wlx_progress(&ctx, 0.6f,
        .track_color = trackc, .border_width = 0,
        .roundness = 0.2f, .rounded_segments = 4,
        .glow_color = (WLX_Color){0, 200, 255, 180}, .glow_spread = 3.0f, .glow_rings = 2);

    ASSERT_EQ_INT(1, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    int ti = gs_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED); // track fill is drawn first
    ASSERT_TRUE(gi >= 0 && ti >= 0);
    ASSERT_TRUE(gi < ti); // effect precedes the track fill

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_slider_thumb_glow) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);
    float value = 0.5f;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_slider(&ctx, "s", &value,
        .glow_color = (WLX_Color){0, 200, 255, 180}, .glow_spread = 3.0f, .glow_rings = 2);

    ASSERT_EQ_INT(1, gs_count_cmd(&ctx, WLX_CMD_GLOW));
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    ASSERT_TRUE(gi >= 0);
    // The thumb fill is a later RECT_ROUNDED over the identical glow rect.
    WLX_Rect thumb = wlx_pool_commands(&ctx)[gi].data.glow.rect;
    ASSERT_TRUE(gs_find_rect_rounded_at(&ctx, thumb, gi + 1) > gi);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_toggle_thumb_shadow) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);
    bool on = true;

    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_toggle(&ctx, "t", &on,
        .shadow_color = (WLX_Color){0, 0, 0, 200}, .shadow_offset_y = 3, .shadow_layers = 4);

    ASSERT_EQ_INT(1, gs_count_cmd(&ctx, WLX_CMD_SHADOW));
    int si = gs_find_cmd(&ctx, WLX_CMD_SHADOW);
    ASSERT_TRUE(si >= 0);
    // Shadow rect is the un-shifted thumb rect; the round thumb fill (a circle,
    // since the square thumb draws at roundness 1.0) follows over it.
    WLX_Rect thumb = wlx_pool_commands(&ctx)[si].data.shadow.rect;
    ASSERT_TRUE(gs_find_circle_at(&ctx, thumb, si + 1) > si);

    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(glow_shadow_widget_effect_opacity_scales) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200);
    float value = 0.5f;

    // Slider effect color scales with the resolved widget opacity, like a fill.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_slider(&ctx, "s", &value, .opacity = 0.5f,
        .glow_color = (WLX_Color){0, 200, 255, 180});
    int gi = gs_find_cmd(&ctx, WLX_CMD_GLOW);
    ASSERT_TRUE(gi >= 0);
    ASSERT_EQ_INT(90, wlx_pool_commands(&ctx)[gi].data.glow.color.a); // 180 * 0.5
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(glow_shadow) {
    RUN_TEST(glow_shadow_shadow_layer_rect_geometry);
    RUN_TEST(glow_shadow_shadow_layer_alpha_formula);
    RUN_TEST(glow_shadow_glow_ring_rect_geometry);
    RUN_TEST(glow_shadow_glow_ring_alpha_formula);
    RUN_TEST(glow_shadow_glow_alpha_parity_with_dashboard);
    RUN_TEST(glow_shadow_default_resolution);
    RUN_TEST(glow_shadow_theme_resolution);
    RUN_TEST(glow_shadow_explicit_override_wins);
    RUN_TEST(glow_shadow_sentinel_emits_nothing);
    RUN_TEST(glow_shadow_render_order_before_fill);
    RUN_TEST(glow_shadow_opacity_scales_effect_color);
    RUN_TEST(glow_shadow_fallback_replay_counts);
    RUN_TEST(glow_shadow_native_callback_used);
    RUN_TEST(glow_shadow_immediate_mode_renders);
    RUN_TEST(glow_shadow_progress_segmented_run_glow);
    RUN_TEST(glow_shadow_progress_segmented_gating);
    RUN_TEST(glow_shadow_progress_continuous_track);
    RUN_TEST(glow_shadow_slider_thumb_glow);
    RUN_TEST(glow_shadow_toggle_thumb_shadow);
    RUN_TEST(glow_shadow_widget_effect_opacity_scales);
}

// test_corner_radius.c - absolute pixel corner radius (corner_radius) tests.
// Included from test_main.c (single TU build).
//
// Covers the opt-in px corner radius: the central px->fraction conversion helper
// (incl. zero/negative and degenerate min == 0), size-invariance (equal px on
// two differently sized rects yields equal pixel radii - the regression the
// feature exists for), precedence of px over a set roundness, the 0 sentinel
// reproducing the roundness-only command stream byte-for-byte, and the clamp at
// min(w,h)/2 (fraction 1.0, no overshoot).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// Command-buffer probes (prefixed cr_)
// ============================================================================

static int cr_find_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) return (int)i;
    return -1;
}
static int cr_count_cmd(WLX_Context *ctx, WLX_Cmd_Type t) {
    int n = 0;
    for (size_t i = 0; i < ctx->arena.commands.count; i++)
        if (wlx_pool_commands(ctx)[i].type == t) n++;
    return n;
}

// ============================================================================
// Helper math: wlx_corner_radius_to_roundness
// ============================================================================

TEST(corner_radius_helper_basic_conversion) {
    // 8 px radius on a 200-tall, 100-wide rect: min = 100, f = 2*8/100 = 0.16.
    WLX_Rect r = {0, 0, 100, 200};
    ASSERT_EQ_F(0.16f, wlx_corner_radius_to_roundness(r, 8.0f), 0.0001f);
    // min uses the shorter side regardless of orientation.
    WLX_Rect r2 = {0, 0, 200, 100};
    ASSERT_EQ_F(0.16f, wlx_corner_radius_to_roundness(r2, 8.0f), 0.0001f);
}

TEST(corner_radius_helper_zero_and_negative_off) {
    WLX_Rect r = {0, 0, 100, 100};
    // px <= 0 is "no override" -> negative sentinel.
    ASSERT_TRUE(wlx_corner_radius_to_roundness(r, 0.0f) < 0.0f);
    ASSERT_TRUE(wlx_corner_radius_to_roundness(r, -4.0f) < 0.0f);
}

TEST(corner_radius_helper_degenerate_min_off) {
    // min(w,h) == 0 cannot resolve a fraction -> "no override" (no div by zero).
    ASSERT_TRUE(wlx_corner_radius_to_roundness((WLX_Rect){0, 0, 0, 50}, 8.0f) < 0.0f);
    ASSERT_TRUE(wlx_corner_radius_to_roundness((WLX_Rect){0, 0, 50, 0}, 8.0f) < 0.0f);
}

TEST(corner_radius_helper_clamps_to_one) {
    // px >= min/2 saturates the fraction at 1.0 (never overshoots min/2).
    WLX_Rect r = {0, 0, 100, 60}; // min = 60, min/2 = 30
    ASSERT_EQ_F(1.0f, wlx_corner_radius_to_roundness(r, 30.0f), 0.0001f);
    ASSERT_EQ_F(1.0f, wlx_corner_radius_to_roundness(r, 100.0f), 0.0001f);
}

// ============================================================================
// Size-invariance: equal px on different sizes -> equal pixel radii
// ============================================================================

TEST(corner_radius_size_invariant_pixel_radius) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);

    test_frame_begin(&ctx, 0, 0, false, false);
    // Two differently sized boxes with the same absolute corner radius.
    WLX_Rect small = {0, 0, 120, 80};   // min = 80
    WLX_Rect big   = {0, 0, 300, 200};  // min = 200
    wlx_draw_box(&ctx, small, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}, .corner_radius = 8.0f});
    wlx_draw_box(&ctx, big, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}, .corner_radius = 8.0f});

    ASSERT_EQ_INT(2, cr_count_cmd(&ctx, WLX_CMD_RECT_ROUNDED));
    // Find the two rounded-rect commands in order.
    int seen = 0;
    float px_small = -1.0f, px_big = -1.0f;
    for (size_t i = 0; i < ctx.arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(&ctx)[i];
        if (c->type != WLX_CMD_RECT_ROUNDED) continue;
        float m = (c->data.rect_rounded.rect.w < c->data.rect_rounded.rect.h)
                ? c->data.rect_rounded.rect.w : c->data.rect_rounded.rect.h;
        float px = c->data.rect_rounded.roundness * m * 0.5f;
        if (seen == 0) px_small = px; else px_big = px;
        seen++;
    }
    // Different fractions (0.2 vs 0.08) but identical resolved pixel radius (8).
    ASSERT_EQ_F(8.0f, px_small, 0.001f);
    ASSERT_EQ_F(8.0f, px_big, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Precedence: px overrides an explicitly set roundness
// ============================================================================

TEST(corner_radius_overrides_roundness) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);

    test_frame_begin(&ctx, 0, 0, false, false);
    // roundness would give fraction 0.5; corner_radius = 8 on min=100 wins (0.16).
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 100, 200}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}, .roundness = 0.5f, .corner_radius = 8.0f});
    int i = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ_F(0.16f, wlx_pool_commands(&ctx)[i].data.rect_rounded.roundness, 0.0001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Off by default: corner_radius = 0 reproduces the roundness-only output
// ============================================================================

TEST(corner_radius_zero_is_byte_identical_to_roundness_only) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);

    WLX_Rect rect = {0, 0, 100, 200};
    WLX_Color fill = {20, 20, 20, 255};
    WLX_Color border = {200, 200, 200, 255};

    // Baseline: roundness only, corner_radius unset (0).
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){
        .fill = fill, .border = border, .border_width = 2, .roundness = 0.3f});
    int base_n = (int)ctx.arena.commands.count;
    float base_fill_ro = 0, base_line_ro = 0;
    int bi = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED);
    int bli = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED_LINES);
    ASSERT_TRUE(bi >= 0 && bli >= 0);
    base_fill_ro = wlx_pool_commands(&ctx)[bi].data.rect_rounded.roundness;
    base_line_ro = wlx_pool_commands(&ctx)[bli].data.rect_rounded_lines.roundness;
    test_frame_end(&ctx);

    // Same call with corner_radius explicitly 0: identical command count and
    // identical recorded fractions (the feature is fully gated off).
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){
        .fill = fill, .border = border, .border_width = 2, .roundness = 0.3f,
        .corner_radius = 0.0f});
    ASSERT_EQ_INT(base_n, (int)ctx.arena.commands.count);
    int i = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED);
    int li = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED_LINES);
    ASSERT_TRUE(i >= 0 && li >= 0);
    ASSERT_EQ_F(base_fill_ro, wlx_pool_commands(&ctx)[i].data.rect_rounded.roundness, 0.0f);
    ASSERT_EQ_F(base_line_ro, wlx_pool_commands(&ctx)[li].data.rect_rounded_lines.roundness, 0.0f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Clamp: corner_radius >= min/2 yields fraction 1.0 (no overshoot)
// ============================================================================

TEST(corner_radius_clamps_at_min_half) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);

    test_frame_begin(&ctx, 0, 0, false, false);
    // min = 60, min/2 = 30; a 50 px radius must saturate at fraction 1.0.
    // Non-square rect so the rounded-rect path is kept (no circle dispatch).
    wlx_draw_box(&ctx, (WLX_Rect){0, 0, 100, 60}, (WLX_Box_Style){
        .fill = {20, 20, 20, 255}, .corner_radius = 50.0f});
    int i = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ_F(1.0f, wlx_pool_commands(&ctx)[i].data.rect_rounded.roundness, 0.0001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Container decor path: corner_radius declared at wlx_layout_begin resolves
// ============================================================================

TEST(corner_radius_container_decor_resolves) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 200); // root min = 200

    test_frame_begin(&ctx, 0, 0, false, false);
    // A root-sized container: 8 px on min=200 -> fraction 0.08.
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = (WLX_Color){20, 20, 20, 255}, .corner_radius = 8.0f);
    wlx_layout_end(&ctx);
    int i = cr_find_cmd(&ctx, WLX_CMD_RECT_ROUNDED);
    ASSERT_TRUE(i >= 0);
    ASSERT_EQ_F(0.08f, wlx_pool_commands(&ctx)[i].data.rect_rounded.roundness, 0.0001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Per-corner radius: a corner mask squares off the corners it omits by
// overdrawing their corner box, with no backend rounded-corner-mask op.
// ============================================================================

TEST(per_corner_top_squares_bottom_corners) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);

    WLX_Rect rect = {0, 0, 200, 40};      // r = ceil(0.4 * min(200,40) / 2) = 8
    WLX_Color fill = {20, 20, 20, 255};

    // Default mask (0 -> all corners): one rounded fill, no overdraw squares.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){ .fill = fill, .roundness = 0.4f });
    ASSERT_EQ_INT(1, cr_count_cmd(&ctx, WLX_CMD_RECT_ROUNDED));
    ASSERT_EQ_INT(0, cr_count_cmd(&ctx, WLX_CMD_RECT));
    test_frame_end(&ctx);

    // Top corners only: one rounded fill plus two square overdraws at the bottom.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){
        .fill = fill, .roundness = 0.4f, .rounded_corners = WLX_CORNERS_TOP });
    ASSERT_EQ_INT(1, cr_count_cmd(&ctx, WLX_CMD_RECT_ROUNDED));
    ASSERT_EQ_INT(2, cr_count_cmd(&ctx, WLX_CMD_RECT));
    int seen_bl = 0, seen_br = 0;
    for (size_t i = 0; i < ctx.arena.commands.count; i++) {
        WLX_Cmd *c = &wlx_pool_commands(&ctx)[i];
        if (c->type != WLX_CMD_RECT) continue;
        WLX_Rect q = c->data.rect.rect;
        ASSERT_EQ_F(32.0f, q.y, 0.001f);  // bottom band: y = h - r
        ASSERT_EQ_F(8.0f, q.w, 0.001f);
        ASSERT_EQ_F(8.0f, q.h, 0.001f);
        if (q.x == 0.0f)   seen_bl = 1;    // bottom-left
        if (q.x == 192.0f) seen_br = 1;    // bottom-right: x = w - r
    }
    ASSERT_TRUE(seen_bl && seen_br);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(per_corner_all_matches_default_no_overdraw) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);
    WLX_Rect rect = {0, 0, 200, 40};
    WLX_Color fill = {20, 20, 20, 255};

    // Explicit WLX_CORNERS_ALL is identical to the unset (0) mask: no overdraws.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){
        .fill = fill, .roundness = 0.4f, .rounded_corners = WLX_CORNERS_ALL });
    ASSERT_EQ_INT(1, cr_count_cmd(&ctx, WLX_CMD_RECT_ROUNDED));
    ASSERT_EQ_INT(0, cr_count_cmd(&ctx, WLX_CMD_RECT));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(per_corner_ignored_when_square) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme = &wlx_theme_dark;
    ctx.rect = wlx_rect(0, 0, 400, 400);
    WLX_Rect rect = {0, 0, 200, 40};
    WLX_Color fill = {20, 20, 20, 255};

    // roundness 0 -> sharp fill; the mask is moot, so a single plain rect and no
    // rounded fill or overdraw squares.
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_draw_box(&ctx, rect, (WLX_Box_Style){
        .fill = fill, .roundness = 0.0f, .rounded_corners = WLX_CORNERS_TOP });
    ASSERT_EQ_INT(0, cr_count_cmd(&ctx, WLX_CMD_RECT_ROUNDED));
    ASSERT_EQ_INT(1, cr_count_cmd(&ctx, WLX_CMD_RECT));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(corner_radius) {
    RUN_TEST(corner_radius_helper_basic_conversion);
    RUN_TEST(corner_radius_helper_zero_and_negative_off);
    RUN_TEST(corner_radius_helper_degenerate_min_off);
    RUN_TEST(corner_radius_helper_clamps_to_one);
    RUN_TEST(corner_radius_size_invariant_pixel_radius);
    RUN_TEST(corner_radius_overrides_roundness);
    RUN_TEST(corner_radius_zero_is_byte_identical_to_roundness_only);
    RUN_TEST(corner_radius_clamps_at_min_half);
    RUN_TEST(corner_radius_container_decor_resolves);
    RUN_TEST(per_corner_top_squares_bottom_corners);
    RUN_TEST(per_corner_all_matches_default_no_overdraw);
    RUN_TEST(per_corner_ignored_when_square);
}

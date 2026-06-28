// test_per_side_border.c - per-side border colors and widths.
// Covers the shared resolver (sentinel rules + all_equal), the per-side hover/
// disabled tint on the widget path, and deferred command-replay on both the
// widget path and the container path (including uniform no-regression).
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

// ============================================================================
// Recording backend (psb_ prefix) - captures rect/line type, color, geometry.
// ============================================================================

#define PSB_RECT          0
#define PSB_RECT_LINES    1
#define PSB_RECT_ROUNDED  2
#define PSB_RECT_ROUNDED_LINES 3
#define PSB_LOG_CAP 256

typedef struct { int type; WLX_Rect rect; WLX_Color color; } PSB_Entry;

static PSB_Entry _psb_log[PSB_LOG_CAP];
static int _psb_n = 0;

static void psb_push(int type, WLX_Rect r, WLX_Color c) {
    if (_psb_n < PSB_LOG_CAP) _psb_log[_psb_n++] = (PSB_Entry){ type, r, c };
}

static void psb_draw_rect(WLX_Rect r, WLX_Color c) { psb_push(PSB_RECT, r, c); }
static void psb_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)thick; psb_push(PSB_RECT_LINES, r, c);
}
static void psb_draw_rect_rounded(WLX_Rect r, float roundness, int seg, WLX_Color c) {
    (void)roundness; (void)seg; psb_push(PSB_RECT_ROUNDED, r, c);
}
static void psb_draw_rect_rounded_lines(WLX_Rect r, float roundness, int seg, float thick, WLX_Color c) {
    (void)roundness; (void)seg; (void)thick; psb_push(PSB_RECT_ROUNDED_LINES, r, c);
}

static void psb_reset(void) { _psb_n = 0; }

static WLX_Backend psb_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect               = psb_draw_rect;
    b.draw_rect_lines         = psb_draw_rect_lines;
    b.draw_rect_rounded       = psb_draw_rect_rounded;
    b.draw_rect_rounded_lines = psb_draw_rect_rounded_lines;
    return b;
}

static void psb_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = psb_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, w, h);
}

static int psb_count(int type, WLX_Color c) {
    int n = 0;
    for (int i = 0; i < _psb_n; i++)
        if (_psb_log[i].type == type && wlx_color_eq(_psb_log[i].color, c)) n++;
    return n;
}

static int psb_count_type(int type) {
    int n = 0;
    for (int i = 0; i < _psb_n; i++) if (_psb_log[i].type == type) n++;
    return n;
}

// Find a RECT of color c whose geometry matches (x,y,w,h) within eps.
static bool psb_has_rect(WLX_Color c, float x, float y, float w, float h, float eps) {
    for (int i = 0; i < _psb_n; i++) {
        if (_psb_log[i].type != PSB_RECT || !wlx_color_eq(_psb_log[i].color, c)) continue;
        WLX_Rect r = _psb_log[i].rect;
        if (fabsf(r.x - x) <= eps && fabsf(r.y - y) <= eps &&
            fabsf(r.w - w) <= eps && fabsf(r.h - h) <= eps) return true;
    }
    return false;
}

// Marker colors (distinct from theme-dark roles).
static const WLX_Color PSB_FILL   = {12, 14, 16, 255};
static const WLX_Color PSB_LIGHT  = {200, 210, 220, 255};
static const WLX_Color PSB_DARK   = {30, 34, 40, 255};
static const WLX_Color PSB_ACCENT = {255, 120, 0, 255};
static const WLX_Color PSB_BORD   = {44, 99, 188, 255};

// ============================================================================
// Resolution unit tests (pure - no backend)
// ============================================================================

TEST(psb_resolve_uniform_stays_all_equal) {
    WLX_Border_Sides s = wlx_resolve_border_sides(
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0},
        -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_TRUE(s.all_equal);
    ASSERT_TRUE(wlx_color_eq(s.top.color, PSB_BORD));
    ASSERT_TRUE(wlx_color_eq(s.left.color, PSB_BORD));
    ASSERT_EQ_F(s.top.width, 2.0f, 0.01f);
    ASSERT_EQ_F(s.right.width, 2.0f, 0.01f);
}

TEST(psb_resolve_side_color_flips_all_equal) {
    WLX_Border_Sides s = wlx_resolve_border_sides(
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, PSB_ACCENT,  // left explicit
        -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_TRUE(!s.all_equal);
    ASSERT_TRUE(wlx_color_eq(s.left.color, PSB_ACCENT));
    ASSERT_TRUE(wlx_color_eq(s.top.color, PSB_BORD));   // unset side inherits uniform
}

TEST(psb_resolve_width_negative_falls_back) {
    WLX_Border_Sides s = wlx_resolve_border_sides(
        PSB_BORD, 3.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0},
        -1.0f, -1.0f, -1.0f, -1.0f);
    ASSERT_EQ_F(s.left.width, 3.0f, 0.01f);   // -1 -> uniform width
    ASSERT_TRUE(s.all_equal);
}

TEST(psb_resolve_width_zero_switches_side_off) {
    WLX_Border_Sides s = wlx_resolve_border_sides(
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0},
        0.0f, -1.0f, -1.0f, -1.0f);   // top off
    ASSERT_EQ_F(s.top.width, 0.0f, 0.01f);
    ASSERT_EQ_F(s.right.width, 2.0f, 0.01f);
    ASSERT_TRUE(!s.all_equal);        // 0 != 2 -> not equal
}

TEST(psb_resolve_width_explicit_wins) {
    WLX_Border_Sides s = wlx_resolve_border_sides(
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0},
        -1.0f, -1.0f, -1.0f, 5.0f);   // left explicit
    ASSERT_EQ_F(s.left.width, 5.0f, 0.01f);
    ASSERT_TRUE(!s.all_equal);
}

// Per-side tint: hover lightens, disabled darkens, unset side inherits uniform.
TEST(psb_widget_tint_hover_and_disabled) {
    const WLX_Color base = {100, 100, 100, 255};
    // No hover, not disabled, full opacity -> explicit color unchanged.
    WLX_Border_Sides plain = wlx_border_sides_for_widget(
        &wlx_theme_dark, false, false, 1.0f,
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, base,
        -1.0f, -1.0f, -1.0f, 2.0f);
    ASSERT_TRUE(wlx_color_eq(plain.left.color, base));
    ASSERT_TRUE(wlx_color_eq(plain.top.color, PSB_BORD));  // unset inherits uniform

    // Hover -> lightened (dark theme hover_brightness = +0.08).
    WLX_Border_Sides hov = wlx_border_sides_for_widget(
        &wlx_theme_dark, true, false, 1.0f,
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, base,
        -1.0f, -1.0f, -1.0f, 2.0f);
    ASSERT_TRUE(hov.left.color.r > base.r);
    ASSERT_TRUE(hov.left.color.g > base.g);

    // Disabled -> darkened (dark theme disabled_brightness = -0.35).
    WLX_Border_Sides dis = wlx_border_sides_for_widget(
        &wlx_theme_dark, false, true, 1.0f,
        PSB_BORD, 2.0f,
        (WLX_Color){0}, (WLX_Color){0}, (WLX_Color){0}, base,
        -1.0f, -1.0f, -1.0f, 2.0f);
    ASSERT_TRUE(dis.left.color.r < base.r);
    ASSERT_TRUE(dis.left.color.b < base.b);
}

// ============================================================================
// Command-replay tests - widget path
// ============================================================================

// Button with a left-only accent bar records exactly one sharp left span and
// no uniform border line.
TEST(psb_widget_left_bar_records_one_span) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 1000, 1000, false, false);   // mouse away -> no hover
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_button(&ctx, "x",
            .back_color = PSB_FILL, .roundness = 0,
            .border_color_left = PSB_ACCENT, .border_width_left = 2,
            .border_width_top = 0, .border_width_right = 0, .border_width_bottom = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_ACCENT), 1);          // one left span
    ASSERT_EQ_INT(psb_count_type(PSB_RECT_LINES), 0);           // no uniform border
    ASSERT_EQ_INT(psb_count_type(PSB_RECT_ROUNDED_LINES), 0);
    wlx_context_destroy(&ctx);
}

// Uniform-border button records the legacy single sharp border line (no spans).
TEST(psb_widget_uniform_no_regression) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 1000, 1000, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
        wlx_button(&ctx, "x",
            .back_color = PSB_FILL, .roundness = 0,
            .border_color = PSB_BORD, .border_width = 2);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT_LINES, PSB_BORD), 1);      // single legacy border
    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_ACCENT), 0);         // no per-side spans
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Command-replay tests - container path
// ============================================================================

// Glass bevel layout records four sharp spans (2 light, 2 dark) with the
// expected geometry and no uniform border line.
TEST(psb_container_glass_bevel_records_four_spans) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = PSB_FILL,
        .border_color_top = PSB_LIGHT, .border_color_left = PSB_LIGHT,
        .border_color_bottom = PSB_DARK, .border_color_right = PSB_DARK,
        .border_width_top = 1, .border_width_left = 1,
        .border_width_bottom = 1, .border_width_right = 1);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_LIGHT), 2);
    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_DARK), 2);
    ASSERT_EQ_INT(psb_count_type(PSB_RECT_LINES), 0);
    // Geometry: top + left light, bottom + right dark.
    ASSERT_TRUE(psb_has_rect(PSB_LIGHT, 0, 0, 200, 1, 0.5f));    // top
    ASSERT_TRUE(psb_has_rect(PSB_LIGHT, 0, 0, 1, 200, 0.5f));    // left
    ASSERT_TRUE(psb_has_rect(PSB_DARK, 0, 199, 200, 1, 0.5f));   // bottom
    ASSERT_TRUE(psb_has_rect(PSB_DARK, 199, 0, 1, 200, 0.5f));   // right
    wlx_context_destroy(&ctx);
}

// Layout with only border_width_left = 2 records exactly one left span.
TEST(psb_container_left_only_records_one_span) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = PSB_FILL,
        .border_color_left = PSB_ACCENT, .border_width_left = 2,
        .border_width_top = 0, .border_width_right = 0, .border_width_bottom = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_ACCENT), 1);
    ASSERT_EQ_INT(psb_count_type(PSB_RECT_LINES), 0);
    ASSERT_TRUE(psb_has_rect(PSB_ACCENT, 0, 0, 2, 200, 0.5f));
    wlx_context_destroy(&ctx);
}

// Uniform-border layout records the legacy single sharp border line.
TEST(psb_container_uniform_no_regression) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = PSB_FILL, .border_color = PSB_BORD,
        .border_width = 3, .roundness = 0);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT_LINES, PSB_BORD), 1);
    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_ACCENT), 0);
    ASSERT_EQ_INT(psb_count(PSB_RECT, PSB_LIGHT), 0);
    wlx_context_destroy(&ctx);
}

// Uniform rounded border still routes through the legacy rounded-line path.
TEST(psb_container_uniform_rounded_no_regression) {
    psb_reset();
    WLX_Context ctx;
    psb_ctx_init(&ctx, 200, 200);
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = PSB_FILL, .border_color = PSB_BORD,
        .border_width = 3, .roundness = 0.2f, .rounded_segments = 8);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT(psb_count(PSB_RECT_ROUNDED_LINES, PSB_BORD), 1);
    ASSERT_EQ_INT(psb_count_type(PSB_RECT_LINES), 0);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(per_side_border) {
    RUN_TEST(psb_resolve_uniform_stays_all_equal);
    RUN_TEST(psb_resolve_side_color_flips_all_equal);
    RUN_TEST(psb_resolve_width_negative_falls_back);
    RUN_TEST(psb_resolve_width_zero_switches_side_off);
    RUN_TEST(psb_resolve_width_explicit_wins);
    RUN_TEST(psb_widget_tint_hover_and_disabled);
    RUN_TEST(psb_widget_left_bar_records_one_span);
    RUN_TEST(psb_widget_uniform_no_regression);
    RUN_TEST(psb_container_glass_bevel_records_four_spans);
    RUN_TEST(psb_container_left_only_records_one_span);
    RUN_TEST(psb_container_uniform_no_regression);
    RUN_TEST(psb_container_uniform_rounded_no_regression);
}

// test_interactive_container.c - interaction-aware containers and the per-call
// wlx_button hover override.
// Covers: container .interact compute + interact_out result, interact == 0
// no-op, hover-variant chrome (replace semantics, background + per-side border
// appearing-bar idiom), click capture, id stability across frames, the
// non-interactive-children precedence behaviour, and wlx_button hover_brightness
// / hover_back_color overrides.
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
// Recording backend (tic_ prefix) - captures rect/line fill colors + geometry.
// ============================================================================

#define TIC_RECT          0
#define TIC_RECT_LINES    1
#define TIC_RECT_ROUNDED  2
#define TIC_RECT_ROUNDED_LINES 3
#define TIC_LOG_CAP 256

typedef struct { int type; WLX_Rect rect; WLX_Color color; } TIC_Entry;

static TIC_Entry _tic_log[TIC_LOG_CAP];
static int _tic_n = 0;

static void tic_push(int type, WLX_Rect r, WLX_Color c) {
    if (_tic_n < TIC_LOG_CAP) _tic_log[_tic_n++] = (TIC_Entry){ type, r, c };
}

static void tic_draw_rect(WLX_Rect r, WLX_Color c) { tic_push(TIC_RECT, r, c); }
static void tic_draw_rect_lines(WLX_Rect r, float thick, WLX_Color c) {
    (void)thick; tic_push(TIC_RECT_LINES, r, c);
}
static void tic_draw_rect_rounded(WLX_Rect r, float roundness, int seg, WLX_Color c) {
    (void)roundness; (void)seg; tic_push(TIC_RECT_ROUNDED, r, c);
}
static void tic_draw_rect_rounded_lines(WLX_Rect r, float roundness, int seg, float thick, WLX_Color c) {
    (void)roundness; (void)seg; (void)thick; tic_push(TIC_RECT_ROUNDED_LINES, r, c);
}

static void tic_reset(void) { _tic_n = 0; }

static WLX_Backend tic_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect               = tic_draw_rect;
    b.draw_rect_lines         = tic_draw_rect_lines;
    b.draw_rect_rounded       = tic_draw_rect_rounded;
    b.draw_rect_rounded_lines = tic_draw_rect_rounded_lines;
    return b;
}

static void tic_ctx_init(WLX_Context *ctx, float w, float h) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->backend = tic_backend();
    ctx->theme   = &wlx_theme_dark;
    ctx->rect    = wlx_rect(0, 0, w, h);
}

static int tic_count(int type, WLX_Color c) {
    int n = 0;
    for (int i = 0; i < _tic_n; i++)
        if (_tic_log[i].type == type && wlx_color_eq(_tic_log[i].color, c)) n++;
    return n;
}

// Find a rect (solid or rounded fill) of color c whose geometry matches.
static bool tic_has_fill(WLX_Color c, float x, float y, float w, float h, float eps) {
    for (int i = 0; i < _tic_n; i++) {
        if ((_tic_log[i].type != TIC_RECT && _tic_log[i].type != TIC_RECT_ROUNDED) ||
            !wlx_color_eq(_tic_log[i].color, c)) continue;
        WLX_Rect r = _tic_log[i].rect;
        if (fabsf(r.x - x) <= eps && fabsf(r.y - y) <= eps &&
            fabsf(r.w - w) <= eps && fabsf(r.h - h) <= eps) return true;
    }
    return false;
}

// Marker colors (distinct from theme-dark roles and from each other).
static const WLX_Color TIC_BASE   = {20,  22,  26,  255};
static const WLX_Color TIC_HOVER  = {44,  99,  188, 255};
static const WLX_Color TIC_BAR    = {255, 120, 0,   255};

// ============================================================================
// Container interaction: compute + result
// ============================================================================

TEST(container_interact_reports_hover_and_writes_result) {
    tic_reset();
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);

    WLX_Interaction it = {0};
    test_frame_begin(&ctx, 20, 20, false, false);   // mouse inside container
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .interact = WLX_INTERACT_HOVER, .interact_out = &it,
        .back_color = TIC_BASE, .hover_back_color = TIC_HOVER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(it.hover);
    // Hover variant replaces the background fill for this frame.
    ASSERT_TRUE(tic_has_fill(TIC_HOVER, 0, 0, 200, 50, 0.5f));
    ASSERT_EQ_INT(tic_count(TIC_RECT, TIC_BASE), 0);
    wlx_context_destroy(&ctx);
}

TEST(container_interact_out_false_when_mouse_outside) {
    tic_reset();
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);

    WLX_Interaction it = {0};
    test_frame_begin(&ctx, 500, 500, false, false);  // mouse outside
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .interact = WLX_INTERACT_HOVER, .interact_out = &it,
        .back_color = TIC_BASE, .hover_back_color = TIC_HOVER);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_TRUE(!it.hover);
    // Base background, no hover variant.
    ASSERT_TRUE(tic_has_fill(TIC_BASE, 0, 0, 200, 50, 0.5f));
    ASSERT_EQ_INT(tic_count(TIC_RECT, TIC_HOVER), 0);
    wlx_context_destroy(&ctx);
}

TEST(container_interact_zero_is_noop) {
    tic_reset();
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);

    test_frame_begin(&ctx, 20, 20, false, false);    // mouse inside
    wlx_layout_begin(&ctx, 1, WLX_VERT,
        .back_color = TIC_BASE, .hover_back_color = TIC_HOVER);  // interact omitted -> 0
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // No interaction queried: hot_id untouched, base chrome, hover twin ignored.
    ASSERT_EQ_INT((int)ctx.interaction.hot_id, 0);
    ASSERT_TRUE(tic_has_fill(TIC_BASE, 0, 0, 200, 50, 0.5f));
    ASSERT_EQ_INT(tic_count(TIC_RECT, TIC_HOVER), 0);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Hover-variant per-side border: the appearing-bar idiom
// ============================================================================

// A left bar that is transparent at rest and accent on hover, expressed with a
// constant width and a transparent -> accent color toggle (no hover-width field).
static void tic_bar_row(WLX_Context *ctx, WLX_Interaction *out) {
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .interact = WLX_INTERACT_HOVER, .interact_out = out,
        .back_color = TIC_BASE,
        .border_width_left = 2, .border_color_left = (WLX_Color){0},
        .hover_border_color_left = TIC_BAR,
        .border_width_top = 0, .border_width_right = 0, .border_width_bottom = 0);
    wlx_layout_end(ctx);
}

TEST(container_hover_border_appears_only_on_hover) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);
    WLX_Interaction it = {0};

    // Rest: mouse outside -> no accent bar recorded.
    tic_reset();
    test_frame_begin(&ctx, 500, 500, false, false);
    tic_bar_row(&ctx, &it);
    test_frame_end(&ctx);
    ASSERT_TRUE(!it.hover);
    ASSERT_EQ_INT(tic_count(TIC_RECT, TIC_BAR), 0);

    // Hover: mouse inside -> the 2px left bar is recorded in accent.
    tic_reset();
    test_frame_begin(&ctx, 20, 20, false, false);
    tic_bar_row(&ctx, &it);
    test_frame_end(&ctx);
    ASSERT_TRUE(it.hover);
    ASSERT_TRUE(tic_has_fill(TIC_BAR, 0, 0, 2, 50, 0.5f));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Container click + id stability
// ============================================================================

static void tic_click_row(WLX_Context *ctx, WLX_Interaction *out) {
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = out,
        .back_color = TIC_BASE);
    wlx_layout_end(ctx);
}

TEST(container_click_fires_on_release) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);
    WLX_Interaction it = {0};

    // Press inside: active, pressed, not yet clicked.
    test_frame_begin(&ctx, 20, 20, true, true);
    tic_click_row(&ctx, &it);
    test_frame_end(&ctx);
    ASSERT_TRUE(it.pressed);
    ASSERT_TRUE(!it.clicked);
    size_t id_press = it.id;

    // Release while still inside: clicked fires.
    test_frame_begin(&ctx, 20, 20, false, false);
    tic_click_row(&ctx, &it);
    test_frame_end(&ctx);
    ASSERT_TRUE(it.clicked);

    // Id is stable across frames (same call site + scope).
    ASSERT_EQ_INT((int)id_press, (int)it.id);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Precedence: an interactive container holds non-interactive children only.
// A nested interactive child is shadowed by the container's first-writer press;
// the container wins the click and the child never fires. This is why
// interactive containers must hold non-interactive children only.
// ============================================================================

// One stable call site per frame so the container id is consistent across the
// press and release frames (mirroring a reusable dashboard row function).
static void tic_wrap_child(WLX_Context *ctx, WLX_Interaction *out, bool *child_clicked) {
    wlx_layout_begin(ctx, 1, WLX_VERT,
        .interact = WLX_INTERACT_CLICK | WLX_INTERACT_HOVER, .interact_out = out,
        .back_color = TIC_BASE);
        *child_clicked |= wlx_button(ctx, "child", .id = "child",
            .roundness = 0, .border_width = 0);
    wlx_layout_end(ctx);
}

TEST(interactive_container_shadows_interactive_child) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 200, 50);
    WLX_Interaction it = {0};
    bool child_clicked = false;

    // Press, then release, over a CLICK container that wraps a CLICK button.
    test_frame_begin(&ctx, 20, 20, true, true);
    tic_wrap_child(&ctx, &it, &child_clicked);
    test_frame_end(&ctx);
    ASSERT_TRUE(!child_clicked);

    test_frame_begin(&ctx, 20, 20, false, false);
    tic_wrap_child(&ctx, &it, &child_clicked);
    test_frame_end(&ctx);

    // Container captured the press; the child's click was swallowed.
    ASSERT_TRUE(it.clicked);
    ASSERT_TRUE(!child_clicked);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_button per-call hover override
// ============================================================================

// Bright fill so a negative hover_brightness produces a clearly darker result.
static const WLX_Color TIC_BTN_FILL = {200, 200, 200, 255};
static const WLX_Color TIC_BTN_MARK = {10, 90, 200, 255};

// Find the button fill rect (full ctx rect) of any color; return it.
static bool tic_button_fill(WLX_Color *out) {
    for (int i = 0; i < _tic_n; i++) {
        if (_tic_log[i].type != TIC_RECT && _tic_log[i].type != TIC_RECT_ROUNDED) continue;
        WLX_Rect r = _tic_log[i].rect;
        if (fabsf(r.x) <= 0.5f && fabsf(r.y) <= 0.5f &&
            fabsf(r.w - 120) <= 0.5f && fabsf(r.h - 40) <= 0.5f) {
            *out = _tic_log[i].color;
            return true;
        }
    }
    return false;
}

static WLX_Color tic_render_button(WLX_Context *ctx, int mx, int my, WLX_Button_Opt opt) {
    tic_reset();
    test_frame_begin(ctx, mx, my, false, false);
    wlx_layout_begin(ctx, 1, WLX_VERT);  // non-interactive host layout
        wlx_button_impl(ctx, "go", opt, __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    WLX_Color c = {0};
    tic_button_fill(&c);
    return c;
}

TEST(button_hover_brightness_negative_darkens) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 120, 40);

    WLX_Button_Opt opt = wlx_default_button_opt(
        .back_color = TIC_BTN_FILL, .border_width = 0, .roundness = 0,
        .hover_brightness = -0.5f);

    WLX_Color out = tic_render_button(&ctx, 500, 500, opt);  // not hovered
    ASSERT_TRUE(wlx_color_eq(out, TIC_BTN_FILL));

    WLX_Color over = tic_render_button(&ctx, 60, 20, opt);   // hovered -> darker
    ASSERT_TRUE(over.r < TIC_BTN_FILL.r);
    ASSERT_TRUE(over.g < TIC_BTN_FILL.g);
    ASSERT_TRUE(over.b < TIC_BTN_FILL.b);
    wlx_context_destroy(&ctx);
}

TEST(button_hover_back_color_replaces_fill) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 120, 40);

    WLX_Button_Opt opt = wlx_default_button_opt(
        .back_color = TIC_BTN_FILL, .border_width = 0, .roundness = 0,
        .hover_back_color = TIC_BTN_MARK);

    WLX_Color over = tic_render_button(&ctx, 60, 20, opt);   // hovered -> replaced
    ASSERT_TRUE(wlx_color_eq(over, TIC_BTN_MARK));

    WLX_Color out = tic_render_button(&ctx, 500, 500, opt);  // not hovered -> base
    ASSERT_TRUE(wlx_color_eq(out, TIC_BTN_FILL));
    wlx_context_destroy(&ctx);
}

TEST(button_hover_brightness_unset_matches_theme) {
    WLX_Context ctx;
    tic_ctx_init(&ctx, 120, 40);

    WLX_Button_Opt def = wlx_default_button_opt(
        .back_color = TIC_BTN_FILL, .border_width = 0, .roundness = 0);
    WLX_Button_Opt explicit_theme = wlx_default_button_opt(
        .back_color = TIC_BTN_FILL, .border_width = 0, .roundness = 0,
        .hover_brightness = wlx_theme_dark.hover_brightness);

    WLX_Color a = tic_render_button(&ctx, 60, 20, def);
    WLX_Color b = tic_render_button(&ctx, 60, 20, explicit_theme);
    ASSERT_TRUE(wlx_color_eq(a, b));   // unset resolves to theme->hover_brightness
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(interactive_container) {
    RUN_TEST(container_interact_reports_hover_and_writes_result);
    RUN_TEST(container_interact_out_false_when_mouse_outside);
    RUN_TEST(container_interact_zero_is_noop);
    RUN_TEST(container_hover_border_appears_only_on_hover);
    RUN_TEST(container_click_fires_on_release);
    RUN_TEST(interactive_container_shadows_interactive_child);
    RUN_TEST(button_hover_brightness_negative_darkens);
    RUN_TEST(button_hover_back_color_replaces_fill);
    RUN_TEST(button_hover_brightness_unset_matches_theme);
}

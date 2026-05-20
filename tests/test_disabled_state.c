// test_disabled_state.c - disabled-state model tests
//
// Covers the disabled gating layer (wlx_get_interaction_for) plus the
// visual contract of WLX_APPLY_DISABLED on the six interactive widgets.
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
// Local recording backend: captures the last fill rect colour so visual
// tests can assert against expected brightness/opacity transforms.
// ============================================================================

static WLX_Color _ds_last_fill_color;

static void ds_rec_draw_rect(WLX_Rect r, WLX_Color c) {
    (void)r;
    _ds_last_fill_color = c;
}

static WLX_Backend ds_backend(void) {
    WLX_Backend b = mock_backend();
    b.draw_rect = ds_rec_draw_rect;
    return b;
}

static void ds_reset(void) {
    _ds_last_fill_color = (WLX_Color){0};
}

// Stable call-site wrappers so widget IDs are constant across frames.
static bool _ds_button_disabled(WLX_Context *ctx) {
    return wlx_button(ctx, "Save", .back_color = WLX_RGBA(80, 80, 80, 255),
                                   .disabled = true);
}
static bool _ds_button_enabled(WLX_Context *ctx) {
    return wlx_button(ctx, "Save", .back_color = WLX_RGBA(80, 80, 80, 255));
}
static bool _ds_inputbox_disabled(WLX_Context *ctx, char *buf, size_t sz) {
    return wlx_inputbox(ctx, "Name", buf, sz, .disabled = true);
}
static bool _ds_slider_disabled(WLX_Context *ctx, float *val) {
    return wlx_slider(ctx, NULL, val, .height = 40, .show_label = false,
                                       .disabled = true);
}

// ============================================================================
// 1. Disabled button does not produce clicked even after a full press cycle.
// ============================================================================

TEST(disabled_interaction_button_no_click) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    // Frame 1: press inside disabled button
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool clicked_press = _ds_button_disabled(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(clicked_press);
    ASSERT_EQ_INT((int)ctx.interaction.active_id, 0);

    // Frame 2: release inside same widget
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool clicked_release = _ds_button_disabled(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(clicked_release);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 2. Disabled inputbox does not capture focus, so keystrokes don't reach
//    the buffer. Verified end-to-end via wlx_inputbox.
// ============================================================================

TEST(disabled_interaction_inputbox_no_focus) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    char buf[64] = "hi";

    // Click into disabled inputbox.
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    (void)_ds_inputbox_disabled(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // No widget became active.
    ASSERT_EQ_INT((int)ctx.interaction.active_id, 0);

    // Send a follow-up frame with text input. Because focus was never
    // claimed, the inputbox's keys path must not run and the buffer is
    // unchanged.
    bool keys_down[WLX_KEY_COUNT] = {0};
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    test_frame_begin_ex(&ctx, 200, 150, false, false, false, 0.0f,
                        keys_down, keys_pressed, "x");
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    (void)_ds_inputbox_disabled(&ctx, buf, sizeof(buf));
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_STR(buf, "hi");

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 3. Disabled slider does not drag: holding the mouse over the track must
//    not mutate the underlying value pointer.
// ============================================================================

TEST(disabled_interaction_slider_no_drag) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    float value = 0.25f;

    // Frame 1: press inside slider track
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    (void)_ds_slider_disabled(&ctx, &value);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_F(value, 0.25f, 0.0001f);

    // Frame 2: hold and drag - value still must not move.
    test_frame_begin(&ctx, 350, 150, true, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    (void)_ds_slider_disabled(&ctx, &value);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_EQ_F(value, 0.25f, 0.0001f);

    ASSERT_EQ_INT((int)ctx.interaction.active_id, 0);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 4. wlx_get_interaction_for(disabled=true) must not mutate hot_id or
//    active_id. Direct test of the gating layer.
// ============================================================================

static WLX_Interaction _ds_interact_disabled(WLX_Context *ctx, WLX_Rect r) {
    return wlx_get_interaction_for(ctx, r,
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK | WLX_INTERACT_KEYBOARD,
        true, __FILE__, __LINE__);
}

TEST(disabled_does_not_claim_hot_id) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    ctx.theme = &wlx_theme_dark;
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction s = _ds_interact_disabled(&ctx, r);
    test_frame_end(&ctx);

    ASSERT_TRUE(s.disabled);
    ASSERT_TRUE(s.hover);          // hover still reported (tooltip anchor)
    ASSERT_FALSE(s.clicked);
    ASSERT_FALSE(s.pressed);
    ASSERT_FALSE(s.focused);
    ASSERT_FALSE(s.active);
    ASSERT_FALSE(s.just_focused);
    ASSERT_FALSE(s.just_unfocused);
    ASSERT_EQ_INT((int)ctx.interaction.hot_id, 0);
    ASSERT_EQ_INT((int)ctx.interaction.active_id, 0);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 5. Disabled-state visual colours: a disabled button records a fill
//    whose RGB equals the theme back_color shifted by disabled_brightness
//    and whose alpha equals 255 * disabled_opacity.
// ============================================================================

TEST(disabled_visual_colors) {
    ds_reset();
    WLX_Context ctx = {0};
    ctx.backend = ds_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness            = 0;
    theme.rounded_segments     = 0;
    theme.hover_brightness     = 0.0f;          // isolate the disabled shift
    theme.disabled_brightness  = -0.5f;
    theme.disabled_opacity     = 0.6f;
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    const WLX_Color base = WLX_RGBA(120, 120, 120, 255);

    // Mouse far away from the widget - no hover involvement.
    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_button(&ctx, "x", .back_color = base, .disabled = true);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    WLX_Color shifted = wlx_color_brightness(base, theme.disabled_brightness);
    WLX_Color expected = wlx_color_apply_opacity(shifted, theme.disabled_opacity);

    ASSERT_EQ_COLOR(_ds_last_fill_color, expected);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 6. Hover tint must NOT stack on top of disabled treatment. With mouse
//    inside a disabled button, the recorded fill colour equals the
//    disabled-shifted colour, not the disabled-shifted-then-hover-tinted
//    colour.
// ============================================================================

TEST(disabled_skips_hover_tint) {
    ds_reset();
    WLX_Context ctx = {0};
    ctx.backend = ds_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness            = 0;
    theme.rounded_segments     = 0;
    theme.hover_brightness     = 0.3f;          // would brighten if applied
    theme.disabled_brightness  = -0.4f;
    theme.disabled_opacity     = 1.0f;          // isolate the brightness path
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    const WLX_Color base = WLX_RGBA(150, 150, 150, 255);

    // Mouse inside the single full-screen slot -> would normally hover.
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_button(&ctx, "x", .back_color = base, .disabled = true);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    WLX_Color expected = wlx_color_brightness(base, theme.disabled_brightness);
    expected = wlx_color_apply_opacity(expected, 1.0f);

    ASSERT_EQ_COLOR(_ds_last_fill_color, expected);

    // Sanity: a hover-then-disabled stack would produce a different colour.
    WLX_Color hover_then_disabled =
        wlx_color_brightness(
            wlx_color_brightness(base, theme.hover_brightness),
            theme.disabled_brightness);
    ASSERT_FALSE(_ds_last_fill_color.r == hover_then_disabled.r &&
                 _ds_last_fill_color.g == hover_then_disabled.g &&
                 _ds_last_fill_color.b == hover_then_disabled.b);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 7. Built-in preset defaults for disabled_brightness and disabled_opacity.
// ============================================================================

TEST(disabled_theme_defaults) {
    ASSERT_EQ_F(wlx_theme_dark.disabled_brightness,  -0.35f, 0.0001f);
    ASSERT_EQ_F(wlx_theme_dark.disabled_opacity,      0.55f, 0.0001f);

    ASSERT_EQ_F(wlx_theme_light.disabled_brightness,  0.30f, 0.0001f);
    ASSERT_EQ_F(wlx_theme_light.disabled_opacity,     0.55f, 0.0001f);

    ASSERT_EQ_F(wlx_theme_glass.disabled_brightness, -0.25f, 0.0001f);
    ASSERT_EQ_F(wlx_theme_glass.disabled_opacity,     0.55f, 0.0001f);
}

// ============================================================================
// 8. Sentinel inheritance: a custom theme with disabled_brightness =
//    WLX_FLOAT_UNSET and disabled_opacity < 0 results in no brightness
//    shift and no extra alpha multiplier when a widget is disabled.
// ============================================================================

TEST(disabled_sentinel_inheritance) {
    ds_reset();
    WLX_Context ctx = {0};
    ctx.backend = ds_backend();
    WLX_Theme theme = wlx_theme_dark;
    theme.roundness            = 0;
    theme.rounded_segments     = 0;
    theme.hover_brightness     = 0.0f;
    theme.disabled_brightness  = WLX_FLOAT_UNSET;
    theme.disabled_opacity     = -1.0f;
    theme.opacity              = -1.0f;
    ctx.theme = &theme;
    ctx.rect  = wlx_rect(0, 0, 400, 300);

    const WLX_Color base = WLX_RGBA(100, 110, 120, 255);

    test_frame_begin(&ctx, -1, -1, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    wlx_button(&ctx, "x", .back_color = base, .disabled = true);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);

    // No shift, no alpha multiplier. Resolved opacity defaults to 1.0,
    // so the captured colour equals the input verbatim.
    ASSERT_EQ_COLOR(_ds_last_fill_color, base);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// 9. Back compat: a call site that does not set `.disabled` defaults to
//    false and behaves exactly as before (hover brightening still fires,
//    click still completes).
// ============================================================================

TEST(disabled_back_compat) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    ctx.theme   = &wlx_theme_dark;
    ctx.rect    = wlx_rect(0, 0, 400, 300);

    // Frame 1: press inside enabled button.
    test_frame_begin(&ctx, 200, 150, true, true);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool clicked_press = _ds_button_enabled(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_FALSE(clicked_press);
    ASSERT_NEQ_INT((int)ctx.interaction.active_id, 0);

    // Frame 2: release while still hovering -> clicked fires.
    test_frame_begin(&ctx, 200, 150, false, false);
    wlx_layout_begin(&ctx, 1, WLX_VERT);
    bool clicked_release = _ds_button_enabled(&ctx);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    ASSERT_TRUE(clicked_release);

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(disabled_state) {
    RUN_TEST(disabled_interaction_button_no_click);
    RUN_TEST(disabled_interaction_inputbox_no_focus);
    RUN_TEST(disabled_interaction_slider_no_drag);
    RUN_TEST(disabled_does_not_claim_hot_id);
    RUN_TEST(disabled_visual_colors);
    RUN_TEST(disabled_skips_hover_tint);
    RUN_TEST(disabled_theme_defaults);
    RUN_TEST(disabled_sentinel_inheritance);
    RUN_TEST(disabled_back_compat);
}

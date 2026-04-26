// test_opacity_stack.c - tests for wlx_push_opacity / wlx_pop_opacity /
// wlx_get_opacity and the three-layer opacity resolution.
// Included from test_main.c (single TU build).

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif

// ============================================================================
// wlx_get_opacity - empty stack returns 1.0
// ============================================================================

TEST(opacity_stack_empty) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_push_opacity / wlx_pop_opacity
// ============================================================================

TEST(opacity_stack_push_single) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.5f);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.5f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(opacity_stack_nested) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.5f);
    wlx_push_opacity(&ctx, 0.5f);
    // 0.5 * 0.5 = 0.25
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.25f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.5f, 0.001f);
    wlx_pop_opacity(&ctx);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(opacity_stack_resets_each_frame) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    // Frame 1: push opacity
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.3f);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 0.3f, 0.001f);
    test_frame_end(&ctx);
    // Frame 2: stack should be reset
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_F(wlx_get_opacity(&ctx), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// wlx_resolve_opacity - three-layer multiplication
// ============================================================================

TEST(resolve_opacity_all_defaults) {
    // All sentinels -> 1.0
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, -1.0f, 1.0f), 1.0f, 0.001f);
}

TEST(resolve_opacity_widget_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(0.5f, -1.0f, 1.0f), 0.5f, 0.001f);
}

TEST(resolve_opacity_theme_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, 0.8f, 1.0f), 0.8f, 0.001f);
}

TEST(resolve_opacity_ctx_only) {
    ASSERT_EQ_F(wlx_resolve_opacity(-1.0f, -1.0f, 0.6f), 0.6f, 0.001f);
}

TEST(resolve_opacity_all_three) {
    // 0.5 * 0.8 * 0.6 = 0.24
    ASSERT_EQ_F(wlx_resolve_opacity(0.5f, 0.8f, 0.6f), 0.24f, 0.001f);
}

TEST(resolve_opacity_zero_widget) {
    // widget=0 means fully transparent, not sentinel
    ASSERT_EQ_F(wlx_resolve_opacity(0.0f, 1.0f, 1.0f), 0.0f, 0.001f);
}

// ============================================================================
// wlx_resolve_opacity_for - wrapper reads theme + ctx stack automatically
// ============================================================================

TEST(resolve_opacity_for_all_defaults) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = -1.0f; // sentinel -> no theme attenuation
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    // ctx opacity stack is empty -> 1.0; theme sentinel -> 1.0; opt sentinel -> 1.0
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, -1.0f), 1.0f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(resolve_opacity_for_theme_attenuates) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = 0.5f;
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    // opt sentinel -> 1.0; theme 0.5; ctx 1.0 -> 0.5
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, -1.0f), 0.5f, 0.001f);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

TEST(resolve_opacity_for_stack_attenuates) {
    WLX_Context ctx = {0};
    ctx.backend = mock_backend();
    WLX_Theme theme = {0};
    theme.opacity = -1.0f;
    ctx.theme = &theme;
    test_frame_begin(&ctx, 0, 0, false, false);
    wlx_push_opacity(&ctx, 0.4f);
    // opt 0.5; theme sentinel -> 1.0; ctx 0.4 -> 0.2
    ASSERT_EQ_F(wlx_resolve_opacity_for(&ctx, 0.5f), 0.2f, 0.001f);
    wlx_pop_opacity(&ctx);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(opacity_stack) {
    // Stack push/pop/get
    RUN_TEST(opacity_stack_empty);
    RUN_TEST(opacity_stack_push_single);
    RUN_TEST(opacity_stack_nested);
    RUN_TEST(opacity_stack_resets_each_frame);

    // Three-layer resolve
    RUN_TEST(resolve_opacity_all_defaults);
    RUN_TEST(resolve_opacity_widget_only);
    RUN_TEST(resolve_opacity_theme_only);
    RUN_TEST(resolve_opacity_ctx_only);
    RUN_TEST(resolve_opacity_all_three);
    RUN_TEST(resolve_opacity_zero_widget);

    // wlx_resolve_opacity_for wrapper
    RUN_TEST(resolve_opacity_for_all_defaults);
    RUN_TEST(resolve_opacity_for_theme_attenuates);
    RUN_TEST(resolve_opacity_for_stack_attenuates);
}

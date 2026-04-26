// test_container_scope.c - regression tests for container Scope ID isolation
//
// Covers:
//   Positive: two containers with distinct .id values produce different
//             descendant state IDs even when the descendant call sites share
//             the same source file and line.
//   Negative: two containers without .id produce identical descendant state
//             IDs from the same source line (expected collision).

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
// Helpers: call wlx_get_state from a stable source location so that
// two containers can be compared without accidental line-number variance.
// ============================================================================

// Each helper captures a fixed file/line. The call site is this function body,
// not the caller, so both containers see the same base hash.

static size_t state_id_from_site_A(WLX_Context *ctx) {
    WLX_State s = wlx_get_state(ctx, int);
    return s.id;
}

// A second helper at a different line to verify distinctness is not just luck.
static size_t state_id_from_site_B(WLX_Context *ctx) {
    WLX_State s = wlx_get_state(ctx, int);
    return s.id;
}

// ============================================================================
// Positive tests: distinct .id values isolate descendant state IDs
// ============================================================================

TEST(layout_scope_id_isolates_descendants) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t id_from_a = 0;
    size_t id_from_b = 0;

    wlx_layout_begin(&ctx, 2, WLX_VERT);

        wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "scope_a");
            id_from_a = state_id_from_site_A(&ctx);
        wlx_layout_end(&ctx);

        wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "scope_b");
            id_from_b = state_id_from_site_A(&ctx);
        wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);

    test_frame_end(&ctx);

    // Same call site, different container scope -> must produce different IDs
    ASSERT_TRUE(id_from_a != 0);
    ASSERT_TRUE(id_from_b != 0);
    ASSERT_TRUE(id_from_a != id_from_b);
}

TEST(grid_scope_id_isolates_descendants) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t id_from_a = 0;
    size_t id_from_b = 0;

    wlx_layout_begin(&ctx, 2, WLX_VERT);

        wlx_grid_begin(&ctx, 1, 1, .id = "grid_a");
            id_from_a = state_id_from_site_B(&ctx);
        wlx_grid_end(&ctx);

        wlx_grid_begin(&ctx, 1, 1, .id = "grid_b");
            id_from_b = state_id_from_site_B(&ctx);
        wlx_grid_end(&ctx);

    wlx_layout_end(&ctx);

    test_frame_end(&ctx);

    ASSERT_TRUE(id_from_a != 0);
    ASSERT_TRUE(id_from_b != 0);
    ASSERT_TRUE(id_from_a != id_from_b);
}

TEST(panel_scope_id_isolates_descendants) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t id_from_a = 0;
    size_t id_from_b = 0;

    wlx_layout_begin(&ctx, 2, WLX_VERT);

        wlx_panel_begin(&ctx, .id = "panel_a");
            id_from_a = state_id_from_site_A(&ctx);
        wlx_panel_end(&ctx);

        wlx_panel_begin(&ctx, .id = "panel_b");
            id_from_b = state_id_from_site_A(&ctx);
        wlx_panel_end(&ctx);

    wlx_layout_end(&ctx);

    test_frame_end(&ctx);

    ASSERT_TRUE(id_from_a != 0);
    ASSERT_TRUE(id_from_b != 0);
    ASSERT_TRUE(id_from_a != id_from_b);
}

TEST(scope_id_id_stack_balanced_after_layout) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t stack_depth_before = ctx.arena.id_stack.count;

    wlx_layout_begin(&ctx, 1, WLX_VERT, .id = "check_balance");
        (void)state_id_from_site_A(&ctx);
    wlx_layout_end(&ctx);

    size_t stack_depth_after = ctx.arena.id_stack.count;

    test_frame_end(&ctx);

    // The scope id must be popped at layout_end so the stack depth is restored.
    ASSERT_EQ_INT((int)stack_depth_before, (int)stack_depth_after);
}

// ============================================================================
// Negative test: omitting .id causes same-source-line IDs to collide
// ============================================================================

TEST(no_scope_id_collides_descendants) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);

    size_t id_from_a = 0;
    size_t id_from_b = 0;

    // Both layouts omit .id; the id stack is empty for both bodies.
    // Descendants at the same source line produce the same state ID.
    wlx_layout_begin(&ctx, 2, WLX_VERT);

        wlx_layout_begin(&ctx, 1, WLX_VERT);
            id_from_a = state_id_from_site_B(&ctx);
        wlx_layout_end(&ctx);

        wlx_layout_begin(&ctx, 1, WLX_VERT);
            id_from_b = state_id_from_site_B(&ctx);
        wlx_layout_end(&ctx);

    wlx_layout_end(&ctx);

    test_frame_end(&ctx);

    // Without a scope id the two calls share the same hash -> collision expected.
    ASSERT_TRUE(id_from_a != 0);
    ASSERT_TRUE(id_from_b != 0);
    ASSERT_EQ_INT((int)id_from_a, (int)id_from_b);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(container_scope) {
    RUN_TEST(layout_scope_id_isolates_descendants);
    RUN_TEST(grid_scope_id_isolates_descendants);
    RUN_TEST(panel_scope_id_isolates_descendants);
    RUN_TEST(scope_id_id_stack_balanced_after_layout);
    RUN_TEST(no_scope_id_collides_descendants);
}

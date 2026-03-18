// test_interaction.c — interaction system tests (hover, click, focus, drag, keyboard, ID stack)

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

// Helper: call wlx_get_interaction inside a frame. Because IDs are derived from
// file/line + ID stack + frame-local sequence, we use wrapper functions so that
// each "widget" has its own stable source location.

static WLX_Interaction interact_widget_A(WLX_Context *ctx, WLX_Rect r, uint32_t flags) {
    return wlx_get_interaction(ctx, r, flags, __FILE__, __LINE__);
}

static WLX_Interaction interact_widget_B(WLX_Context *ctx, WLX_Rect r, uint32_t flags) {
    return wlx_get_interaction(ctx, r, flags, __FILE__, __LINE__);
}

// ============================================================================
// HOVER tests
// ============================================================================

TEST(hover_sets_hot) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Mouse inside the widget rect
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_TRUE(s.hover);
    test_frame_end(&ctx);
}

TEST(hover_clears_on_leave) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: mouse inside → hover true
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_TRUE(s1.hover);
    test_frame_end(&ctx);

    // Frame 2: mouse outside → hover false
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_FALSE(s2.hover);
    test_frame_end(&ctx);
}

TEST(hover_edge_cases) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Exactly on top-left corner → inside (>=)
    test_frame_begin(&ctx, 100, 100, false, false);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_TRUE(s1.hover);
    test_frame_end(&ctx);

    // Exactly on bottom-right edge (x+w, y+h) → outside (<)
    test_frame_begin(&ctx, 300, 150, false, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_FALSE(s2.hover);
    test_frame_end(&ctx);
}

// ============================================================================
// CLICK tests
// ============================================================================

TEST(click_basic) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: mouse click (press) inside widget
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(s1.hover);
    ASSERT_TRUE(s1.pressed);
    ASSERT_TRUE(s1.active);
    ASSERT_FALSE(s1.clicked);  // clicked fires on release
    test_frame_end(&ctx);

    // Frame 2: release while still hovering → clicked
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(s2.clicked);
    ASSERT_FALSE(s2.pressed);
    test_frame_end(&ctx);
}

TEST(click_release_outside) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: press inside
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(s1.active);
    test_frame_end(&ctx);

    // Frame 2: release outside → no clicked
    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_FALSE(s2.clicked);
    ASSERT_FALSE(s2.active);
    test_frame_end(&ctx);
}

TEST(click_pressed_state) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: click
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(s1.pressed);
    ASSERT_FALSE(s1.clicked);
    test_frame_end(&ctx);

    // Frame 2: still held (mouse_down + mouse_held, no new click)
    test_frame_begin(&ctx, 150, 120, true, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(s2.pressed);
    ASSERT_TRUE(s2.active);
    ASSERT_FALSE(s2.clicked);
    test_frame_end(&ctx);
}

TEST(click_no_double_active) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect rA = wlx_rect(100, 100, 100, 50);
    WLX_Rect rB = wlx_rect(300, 100, 100, 50);

    // Frame 1: click on widget A → A becomes active
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction sA1 = interact_widget_A(&ctx, rA, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    WLX_Interaction sB1 = interact_widget_B(&ctx, rB, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_TRUE(sA1.active);
    ASSERT_FALSE(sB1.active);
    test_frame_end(&ctx);
}

TEST(click_outside_no_activate) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Click outside the widget rect
    test_frame_begin(&ctx, 0, 0, true, true);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_CLICK);
    ASSERT_FALSE(s.active);
    ASSERT_FALSE(s.hover);
    ASSERT_FALSE(s.pressed);
    ASSERT_FALSE(s.clicked);
    test_frame_end(&ctx);
}

// ============================================================================
// FOCUS tests
// ============================================================================

TEST(focus_click_to_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Click on widget → focused + just_focused
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_TRUE(s.focused);
    ASSERT_TRUE(s.just_focused);
    ASSERT_TRUE(s.active);
    test_frame_end(&ctx);
}

TEST(focus_persists) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: click to focus
    test_frame_begin(&ctx, 150, 120, true, true);
    interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    test_frame_end(&ctx);

    // Frame 2: no input → focus persists, just_focused is false
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_TRUE(s.focused);
    ASSERT_FALSE(s.just_focused);
    ASSERT_TRUE(s.active);
    test_frame_end(&ctx);
}

TEST(focus_click_away) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: click to focus
    test_frame_begin(&ctx, 150, 120, true, true);
    interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    test_frame_end(&ctx);

    // Frame 2: click outside → unfocused
    test_frame_begin(&ctx, 0, 0, true, true);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_FALSE(s.focused);
    ASSERT_TRUE(s.just_unfocused);
    ASSERT_FALSE(s.active);
    test_frame_end(&ctx);
}

TEST(focus_enter_unfocus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: click to focus
    test_frame_begin(&ctx, 150, 120, true, true);
    interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    test_frame_end(&ctx);

    // Frame 2: press Enter → unfocused
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_ENTER] = true;
    test_frame_begin_ex(&ctx, 150, 120, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_FALSE(s.focused);
    ASSERT_TRUE(s.just_unfocused);
    test_frame_end(&ctx);
}

TEST(focus_transfer) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect rA = wlx_rect(100, 100, 100, 50);
    WLX_Rect rB = wlx_rect(300, 100, 100, 50);

    // Frame 1: click on A → A focused
    test_frame_begin(&ctx, 150, 120, true, true);
    WLX_Interaction sA1 = interact_widget_A(&ctx, rA, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    interact_widget_B(&ctx, rB, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_TRUE(sA1.focused);
    test_frame_end(&ctx);

    // Frame 2: click on B → A loses focus, B gains focus
    test_frame_begin(&ctx, 350, 120, true, true);
    WLX_Interaction sA2 = interact_widget_A(&ctx, rA, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    WLX_Interaction sB2 = interact_widget_B(&ctx, rB, WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS);
    ASSERT_FALSE(sA2.focused);
    ASSERT_TRUE(sA2.just_unfocused);
    ASSERT_TRUE(sB2.focused);
    ASSERT_TRUE(sB2.just_focused);
    test_frame_end(&ctx);
}

// ============================================================================
// DRAG tests
// ============================================================================

TEST(drag_while_held) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: mouse down inside (not a "click" — drag uses mouse_down)
    test_frame_begin(&ctx, 150, 120, true, false);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG);
    ASSERT_TRUE(s1.active);
    ASSERT_TRUE(s1.pressed);
    test_frame_end(&ctx);

    // Frame 2: still held, mouse moves outside the rect → stays active
    test_frame_begin(&ctx, 0, 0, true, false);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG);
    ASSERT_TRUE(s2.active);
    ASSERT_TRUE(s2.pressed);
    ASSERT_FALSE(s2.hover);   // mouse is outside
    test_frame_end(&ctx);
}

TEST(drag_release) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: mouse down inside
    test_frame_begin(&ctx, 150, 120, true, false);
    interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG);
    test_frame_end(&ctx);

    // Frame 2: release → no longer active
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG);
    ASSERT_FALSE(s.active);
    ASSERT_FALSE(s.pressed);
    test_frame_end(&ctx);
}

TEST(drag_no_activate_outside) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Mouse down outside the rect → should not activate
    test_frame_begin(&ctx, 0, 0, true, false);
    WLX_Interaction s = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER | WLX_INTERACT_DRAG);
    ASSERT_FALSE(s.active);
    ASSERT_FALSE(s.pressed);
    test_frame_end(&ctx);
}

// ============================================================================
// KEYBOARD tests
// ============================================================================

TEST(keyboard_space) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Mouse hovers (so widget becomes hot), then press space
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_SPACE] = true;
    test_frame_begin_ex(&ctx, 150, 120, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    WLX_Interaction s = interact_widget_A(&ctx, r,
                                          WLX_INTERACT_HOVER | WLX_INTERACT_KEYBOARD);
    ASSERT_TRUE(s.hover);
    ASSERT_TRUE(s.clicked);
    test_frame_end(&ctx);
}

TEST(keyboard_enter) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_ENTER] = true;
    test_frame_begin_ex(&ctx, 150, 120, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    WLX_Interaction s = interact_widget_A(&ctx, r,
                                          WLX_INTERACT_HOVER | WLX_INTERACT_KEYBOARD);
    ASSERT_TRUE(s.clicked);
    test_frame_end(&ctx);
}

TEST(keyboard_not_hot) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Mouse is NOT over the widget → not hot → keyboard should have no effect
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[WLX_KEY_SPACE] = true;
    test_frame_begin_ex(&ctx, 0, 0, false, false, false, 0.0f,
                         NULL, keys_pressed, NULL);
    WLX_Interaction s = interact_widget_A(&ctx, r,
                                          WLX_INTERACT_HOVER | WLX_INTERACT_KEYBOARD);
    ASSERT_FALSE(s.hover);
    ASSERT_FALSE(s.clicked);
    test_frame_end(&ctx);
}

// ============================================================================
// ID stack tests
// ============================================================================

TEST(id_push_pop_unique) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Two interactions from the SAME wrapper function, but with different push_id
    // → they should get different IDs
    test_frame_begin(&ctx, 150, 120, false, false);

    wlx_push_id(&ctx, 0);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    wlx_pop_id(&ctx);

    wlx_push_id(&ctx, 1);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    wlx_pop_id(&ctx);

    ASSERT_NEQ_INT((int)s1.id, (int)s2.id);
    test_frame_end(&ctx);
}

TEST(id_same_stack_same_id_across_frames) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Frame 1: get ID
    test_frame_begin(&ctx, 150, 120, false, false);
    wlx_push_id(&ctx, 42);
    WLX_Interaction s1 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    wlx_pop_id(&ctx);
    test_frame_end(&ctx);

    // Frame 2: same push_id + same wrapper → same ID
    test_frame_begin(&ctx, 150, 120, false, false);
    wlx_push_id(&ctx, 42);
    WLX_Interaction s2 = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    wlx_pop_id(&ctx);
    test_frame_end(&ctx);

    ASSERT_EQ_INT((int)s1.id, (int)s2.id);
}

TEST(id_different_without_stack) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    WLX_Rect r = wlx_rect(100, 100, 200, 50);

    // Two different wrapper functions (different __LINE__) → different IDs
    test_frame_begin(&ctx, 150, 120, false, false);
    WLX_Interaction sA = interact_widget_A(&ctx, r, WLX_INTERACT_HOVER);
    WLX_Interaction sB = interact_widget_B(&ctx, r, WLX_INTERACT_HOVER);
    ASSERT_NEQ_INT((int)sA.id, (int)sB.id);
    test_frame_end(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(interaction) {
    // Hover
    RUN_TEST(hover_sets_hot);
    RUN_TEST(hover_clears_on_leave);
    RUN_TEST(hover_edge_cases);

    // Click
    RUN_TEST(click_basic);
    RUN_TEST(click_release_outside);
    RUN_TEST(click_pressed_state);
    RUN_TEST(click_no_double_active);
    RUN_TEST(click_outside_no_activate);

    // Focus
    RUN_TEST(focus_click_to_focus);
    RUN_TEST(focus_persists);
    RUN_TEST(focus_click_away);
    RUN_TEST(focus_enter_unfocus);
    RUN_TEST(focus_transfer);

    // Drag
    RUN_TEST(drag_while_held);
    RUN_TEST(drag_release);
    RUN_TEST(drag_no_activate_outside);

    // Keyboard
    RUN_TEST(keyboard_space);
    RUN_TEST(keyboard_enter);
    RUN_TEST(keyboard_not_hot);

    // ID stack
    RUN_TEST(id_push_pop_unique);
    RUN_TEST(id_same_stack_same_id_across_frames);
    RUN_TEST(id_different_without_stack);
}

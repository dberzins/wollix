// test_input_contract.c - unit tests for the core keyboard/pointer/clipboard
// input contract: modifier bits, key auto-repeat actuation, command-modifier
// resolution, right/middle mouse buttons, the float wheel axes, whole-struct
// input staging, and the clipboard get/set wrappers (including UTF-8-safe
// copy-out).

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
// Key auto-repeat actuation
// ============================================================================

TEST(actuated_on_pressed_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.input.keys_pressed[WLX_KEY_BACKSPACE] = true;
    ASSERT_TRUE(wlx_is_key_actuated(&ctx, WLX_KEY_BACKSPACE));
    ASSERT_FALSE(wlx_is_key_actuated(&ctx, WLX_KEY_DELETE));
    wlx_context_destroy(&ctx);
}

TEST(actuated_on_repeat_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.input.keys_repeated[WLX_KEY_DELETE] = true;
    ASSERT_TRUE(wlx_is_key_actuated(&ctx, WLX_KEY_DELETE));
    ASSERT_FALSE(wlx_is_key_pressed(&ctx, WLX_KEY_DELETE));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Modifier bitfield
// ============================================================================

TEST(mod_down_single_and_combo) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.input.modifiers = WLX_MOD_SHIFT | WLX_MOD_CTRL;
    ASSERT_TRUE(wlx_mod_down(&ctx, WLX_MOD_SHIFT));
    ASSERT_TRUE(wlx_mod_down(&ctx, WLX_MOD_CTRL));
    ASSERT_TRUE(wlx_mod_down(&ctx, WLX_MOD_SHIFT | WLX_MOD_CTRL));
    ASSERT_FALSE(wlx_mod_down(&ctx, WLX_MOD_ALT));
    ASSERT_FALSE(wlx_mod_down(&ctx, WLX_MOD_SHIFT | WLX_MOD_ALT));
    wlx_context_destroy(&ctx);
}

TEST(mod_command_resolves_per_platform) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
#if defined(__APPLE__)
    ctx.input.modifiers = WLX_MOD_SUPER;
    ASSERT_TRUE(wlx_mod_command_down(&ctx));
    ctx.input.modifiers = WLX_MOD_CTRL;
    ASSERT_FALSE(wlx_mod_command_down(&ctx));
#else
    ctx.input.modifiers = WLX_MOD_CTRL;
    ASSERT_TRUE(wlx_mod_command_down(&ctx));
    ctx.input.modifiers = WLX_MOD_SUPER;
    ASSERT_FALSE(wlx_mod_command_down(&ctx));
#endif
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Key enum growth (F-keys, Insert)
// ============================================================================

TEST(key_enum_growth_appended) {
    // Appended codes and the grown count are part of the WASM input ABI;
    // pin the numeric values so a reorder cannot land silently.
    ASSERT_EQ_INT(51, WLX_KEY_F1);
    ASSERT_EQ_INT(62, WLX_KEY_F12);
    ASSERT_EQ_INT(63, WLX_KEY_INSERT);
    ASSERT_EQ_INT(64, WLX_KEY_COUNT);
}

TEST(fkeys_and_insert_roundtrip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.input.keys_pressed[WLX_KEY_F5] = true;
    ASSERT_TRUE(wlx_is_key_pressed(&ctx, WLX_KEY_F5));
    ASSERT_FALSE(wlx_is_key_pressed(&ctx, WLX_KEY_F6));
    ctx.input.keys_repeated[WLX_KEY_INSERT] = true;
    ASSERT_TRUE(wlx_is_key_actuated(&ctx, WLX_KEY_INSERT));
    ASSERT_FALSE(wlx_is_key_pressed(&ctx, WLX_KEY_INSERT));
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Mouse buttons (right/middle)
// ============================================================================

TEST(mouse_button_helpers_read_contract_fields) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ASSERT_FALSE(wlx_is_mouse_right_down(&ctx));
    ASSERT_FALSE(wlx_is_mouse_right_clicked(&ctx));
    ASSERT_FALSE(wlx_is_mouse_middle_down(&ctx));
    ASSERT_FALSE(wlx_is_mouse_middle_clicked(&ctx));

    ctx.input.mouse_right_down = true;
    ctx.input.mouse_right_clicked = true;
    ASSERT_TRUE(wlx_is_mouse_right_down(&ctx));
    ASSERT_TRUE(wlx_is_mouse_right_clicked(&ctx));
    ASSERT_FALSE(wlx_is_mouse_middle_down(&ctx));

    ctx.input.mouse_right_clicked = false;   // held past the press edge
    ctx.input.mouse_middle_down = true;
    ctx.input.mouse_middle_clicked = true;
    ASSERT_TRUE(wlx_is_mouse_right_down(&ctx));
    ASSERT_FALSE(wlx_is_mouse_right_clicked(&ctx));
    ASSERT_TRUE(wlx_is_mouse_middle_down(&ctx));
    ASSERT_TRUE(wlx_is_mouse_middle_clicked(&ctx));
    wlx_context_destroy(&ctx);
}

TEST(whole_struct_staging_reaches_every_field) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);

    WLX_Input_State in = {0};
    in.mouse_x = 42;
    in.mouse_y = 24;
    in.wheel_delta = 0.25f;
    in.wheel_delta_x = -1.5f;
    in.mouse_right_down = true;
    in.mouse_right_clicked = true;
    in.mouse_middle_down = true;
    in.keys_pressed[WLX_KEY_F12] = true;
    in.modifiers = WLX_MOD_ALT;
    test_frame_begin_input(&ctx, &in);

    ASSERT_EQ_INT(42, ctx.input.mouse_x);
    ASSERT_EQ_INT(24, ctx.input.mouse_y);
    ASSERT_EQ_F(ctx.input.wheel_delta, 0.25f, 0.0001f);
    ASSERT_EQ_F(ctx.input.wheel_delta_x, -1.5f, 0.0001f);
    ASSERT_TRUE(wlx_is_mouse_right_down(&ctx));
    ASSERT_TRUE(wlx_is_mouse_right_clicked(&ctx));
    ASSERT_TRUE(wlx_is_mouse_middle_down(&ctx));
    ASSERT_FALSE(wlx_is_mouse_middle_clicked(&ctx));
    ASSERT_TRUE(wlx_is_key_pressed(&ctx, WLX_KEY_F12));
    ASSERT_TRUE(wlx_mod_down(&ctx, WLX_MOD_ALT));
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Right-press interaction (topmost-wins press-frame edge)
// ============================================================================

TEST(right_click_bootstrap_uses_containment) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    // First frame of a context: no candidate list yet, containment decides.
    WLX_Input_State in = {0};
    in.mouse_x = 50;
    in.mouse_y = 20;
    in.mouse_right_down = true;
    in.mouse_right_clicked = true;
    test_frame_begin_input(&ctx, &in);
    WLX_Interaction hit = wlx_get_interaction(&ctx, ((WLX_Rect){0, 0, 100, 40}),
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, "rc_hit", 1);
    WLX_Interaction miss = wlx_get_interaction(&ctx, ((WLX_Rect){0, 60, 100, 40}),
        WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, "rc_miss", 1);
    test_frame_end(&ctx);

    ASSERT_TRUE(hit.right_clicked);
    ASSERT_FALSE(miss.right_clicked);
    ASSERT_FALSE(hit.clicked);   // right press is not a left click
    wlx_context_destroy(&ctx);
}

TEST(right_click_topmost_wins_under_overlap) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    bool base_rc = false, over_rc = false, base_left = false;
    for (int frame = 0; frame < 2; frame++) {
        WLX_Input_State in = {0};
        in.mouse_x = 60;
        in.mouse_y = 20;
        if (frame == 1) {
            in.mouse_right_down = true;
            in.mouse_right_clicked = true;
        }
        test_frame_begin_input(&ctx, &in);
        wlx_layout_begin(&ctx, 1, WLX_VERT);
        WLX_Interaction base = wlx_get_interaction(&ctx, ((WLX_Rect){0, 0, 200, 100}),
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, "rc_base", 1);
        wlx_overlay_begin(&ctx, 1, ((WLX_Rect){20, 0, 100, 40}));
        WLX_Interaction over = wlx_get_interaction(&ctx, ((WLX_Rect){20, 0, 100, 40}),
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, "rc_over", 1);
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
        base_rc |= base.right_clicked;
        over_rc |= over.right_clicked;
        base_left |= base.clicked;
    }

    ASSERT_TRUE(over_rc);     // popup content wins the right press
    ASSERT_FALSE(base_rc);    // covered base widget never sees it
    ASSERT_FALSE(base_left);
    wlx_context_destroy(&ctx);
}

TEST(right_press_does_not_blur_focus) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    WLX_Rect field_rect = {0, 0, 100, 40};
    WLX_Rect below_rect = {0, 60, 100, 40};

    WLX_Interaction field = {0}, below = {0};
    for (int frame = 0; frame < 4; frame++) {
        WLX_Input_State in = {0};
        if (frame == 1) {
            // Left click inside the field: acquires focus.
            in.mouse_x = 50; in.mouse_y = 20;
            in.mouse_down = true; in.mouse_clicked = true; in.mouse_held = true;
        } else if (frame == 3) {
            // Right press over the other widget: must not release focus.
            in.mouse_x = 50; in.mouse_y = 80;
            in.mouse_right_down = true; in.mouse_right_clicked = true;
        } else {
            in.mouse_x = 50; in.mouse_y = (frame < 2) ? 20 : 80;
        }
        test_frame_begin_input(&ctx, &in);
        field = wlx_get_interaction(&ctx, field_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_FOCUS, "rc_field", 1);
        below = wlx_get_interaction(&ctx, below_rect,
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, "rc_below", 1);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(field.focused);        // unlike a left press, no frame-begin release
    ASSERT_FALSE(field.just_unfocused);
    ASSERT_TRUE(below.right_clicked);  // the widget under the pointer got the edge
    wlx_context_destroy(&ctx);
}

TEST(disabled_widget_never_right_clicks) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    WLX_Interaction d = {0};
    for (int frame = 0; frame < 2; frame++) {
        WLX_Input_State in = {0};
        in.mouse_x = 50;
        in.mouse_y = 20;
        if (frame == 1) {
            in.mouse_right_down = true;
            in.mouse_right_clicked = true;
        }
        test_frame_begin_input(&ctx, &in);
        d = wlx_get_interaction_for(&ctx, ((WLX_Rect){0, 0, 100, 40}),
            WLX_INTERACT_HOVER | WLX_INTERACT_CLICK, true, "rc_disabled", 1);
        test_frame_end(&ctx);
    }

    ASSERT_TRUE(d.disabled);
    ASSERT_FALSE(d.right_clicked);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Float wheel detents
// ============================================================================

// Single scroll panel state in the context (fixture mirrors ev_state).
static WLX_Scroll_Panel_State *ic_panel_state(WLX_Context *ctx) {
    for (size_t i = 0; i < ctx->states.capacity; i++) {
        WLX_State_Map_Slot *slot = &ctx->states.slots[i];
        if (slot->id != 0 && slot->data_size == sizeof(WLX_Scroll_Panel_State))
            return (WLX_Scroll_Panel_State *)slot->data;
    }
    return NULL;
}

TEST(fractional_wheel_reaches_scroll_panel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    for (int f = 0; f < 2; f++) {
        float wheel = (f == 1) ? -0.4f : 0.0f;   // fractional trackpad detent
        test_frame_begin_ex(&ctx, 200, 150, false, false, false, wheel,
                            NULL, NULL, NULL);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
        wlx_scroll_panel_begin_impl(&ctx, 600.0f,
            wlx_default_scroll_panel_opt(), "ic_panel", 1);
        wlx_scroll_panel_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    WLX_Scroll_Panel_State *panel = ic_panel_state(&ctx);
    ASSERT_TRUE(panel != NULL);
    // 0.4 detents at the default speed 20 px/detent: no quantization loss.
    ASSERT_EQ_F(panel->scroll_offset, 8.0f, 0.001f);
    ASSERT_EQ_F(ctx.input.wheel_delta, 0.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

TEST(wheel_x_left_alone_by_scroll_panel) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);

    for (int f = 0; f < 2; f++) {
        WLX_Input_State in = {0};
        in.mouse_x = 200;
        in.mouse_y = 150;
        in.wheel_delta_x = (f == 1) ? -2.0f : 0.0f;
        test_frame_begin_input(&ctx, &in);
        wlx_layout_begin(&ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
        wlx_scroll_panel_begin_impl(&ctx, 600.0f,
            wlx_default_scroll_panel_opt(), "ic_panel_x", 1);
        wlx_scroll_panel_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }

    // Scroll panels are vertical-only: the horizontal delta stays for a
    // horizontal-capable consumer and the panel does not move.
    WLX_Scroll_Panel_State *panel = ic_panel_state(&ctx);
    ASSERT_TRUE(panel != NULL);
    ASSERT_EQ_F(panel->scroll_offset, 0.0f, 0.001f);
    ASSERT_EQ_F(ctx.input.wheel_delta_x, -2.0f, 0.001f);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Clipboard transport wrappers (backed by the mock stub)
// ============================================================================

TEST(clipboard_set_get_roundtrip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    const char *msg = "hello wollix";
    wlx_clipboard_set_text(&ctx, msg, strlen(msg));
    ASSERT_EQ_STR(msg, test_get_clipboard());

    char out[64];
    size_t n = wlx_clipboard_get_copy(&ctx, out, sizeof(out));
    ASSERT_EQ_INT((int)strlen(msg), (int)n);
    ASSERT_EQ_STR(msg, out);
    wlx_context_destroy(&ctx);
}

TEST(clipboard_get_truncates_on_utf8_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    // Three 2-byte codepoints (U+00E9 'e-acute' = 0xC3 0xA9), 6 bytes total.
    test_set_clipboard("\xC3\xA9\xC3\xA9\xC3\xA9");

    // out_size 5 -> max 4 payload bytes; must back off to 2 whole codepoints.
    char out[5];
    size_t n = wlx_clipboard_get_copy(&ctx, out, sizeof(out));
    ASSERT_EQ_INT(4, (int)n);
    ASSERT_EQ_INT('\0', out[4]);

    // out_size 4 -> max 3 payload bytes; must back off to 1 whole codepoint (2 bytes).
    char out2[4];
    size_t n2 = wlx_clipboard_get_copy(&ctx, out2, sizeof(out2));
    ASSERT_EQ_INT(2, (int)n2);
    ASSERT_EQ_INT('\0', out2[2]);
    wlx_context_destroy(&ctx);
}

TEST(clipboard_noop_without_hooks) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.backend.clipboard_get = NULL;
    ctx.backend.clipboard_set = NULL;
    // Must be safe no-ops, not crash.
    wlx_clipboard_set_text(&ctx, "x", 1);
    char out[8];
    size_t n = wlx_clipboard_get_copy(&ctx, out, sizeof(out));
    ASSERT_EQ_INT(0, (int)n);
    ASSERT_EQ_INT('\0', out[0]);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_contract) {
    RUN_TEST(actuated_on_pressed_only);
    RUN_TEST(actuated_on_repeat_only);
    RUN_TEST(mod_down_single_and_combo);
    RUN_TEST(mod_command_resolves_per_platform);
    RUN_TEST(key_enum_growth_appended);
    RUN_TEST(fkeys_and_insert_roundtrip);
    RUN_TEST(mouse_button_helpers_read_contract_fields);
    RUN_TEST(whole_struct_staging_reaches_every_field);
    RUN_TEST(right_click_bootstrap_uses_containment);
    RUN_TEST(right_click_topmost_wins_under_overlap);
    RUN_TEST(right_press_does_not_blur_focus);
    RUN_TEST(disabled_widget_never_right_clicks);
    RUN_TEST(fractional_wheel_reaches_scroll_panel);
    RUN_TEST(wheel_x_left_alone_by_scroll_panel);
    RUN_TEST(clipboard_set_get_roundtrip);
    RUN_TEST(clipboard_get_truncates_on_utf8_boundary);
    RUN_TEST(clipboard_noop_without_hooks);
}

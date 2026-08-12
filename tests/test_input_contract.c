// test_input_contract.c - unit tests for the core keyboard/clipboard input
// contract: modifier bits, key auto-repeat actuation, command-modifier
// resolution, and the clipboard get/set wrappers (including UTF-8-safe copy-out).

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
}

TEST(actuated_on_repeat_only) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 100, 100);
    ctx.input.keys_repeated[WLX_KEY_DELETE] = true;
    ASSERT_TRUE(wlx_is_key_actuated(&ctx, WLX_KEY_DELETE));
    ASSERT_FALSE(wlx_is_key_pressed(&ctx, WLX_KEY_DELETE));
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
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_contract) {
    RUN_TEST(actuated_on_pressed_only);
    RUN_TEST(actuated_on_repeat_only);
    RUN_TEST(mod_down_single_and_combo);
    RUN_TEST(mod_command_resolves_per_platform);
    RUN_TEST(clipboard_set_get_roundtrip);
    RUN_TEST(clipboard_get_truncates_on_utf8_boundary);
    RUN_TEST(clipboard_noop_without_hooks);
}

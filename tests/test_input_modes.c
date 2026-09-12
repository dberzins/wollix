// test_input_modes.c - inputbox password and read-only modes: masked render
// with an intact plaintext buffer, copy/cut suppression while masked, edit
// rejection in read-only (with focus/selection/copy still working), and the
// combination of both modes.

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
// Fixture
// ============================================================================

static uint32_t modes_command_mod(void) {
#if defined(__APPLE__)
    return WLX_MOD_SUPER;
#else
    return WLX_MOD_CTRL;
#endif
}

static int _modes_text_draw_count = 0;
static char _modes_text_drawn[64];

static void modes_capture_draw_text(const char *text, float x, float y, WLX_Text_Style style) {
    (void)x; (void)y; (void)style;
    _modes_text_draw_count++;
    const char *source = text ? text : "";
    size_t len = strlen(source);
    if (len >= sizeof(_modes_text_drawn)) len = sizeof(_modes_text_drawn) - 1;
    memcpy(_modes_text_drawn, source, len);
    _modes_text_drawn[len] = '\0';
}

static void modes_reset_text_capture(void) {
    _modes_text_draw_count = 0;
    _modes_text_drawn[0] = '\0';
}

static bool modes_inputbox(WLX_Context *ctx, char *buf, size_t buf_size,
                           bool password, bool read_only) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0,
            .password = password, .read_only = read_only,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static bool modes_frame_mouse(WLX_Context *ctx, char *buf, size_t buf_size,
                              bool password, bool read_only,
                              int mx, bool down, bool clicked) {
    test_frame_begin(ctx, mx, 150, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    bool focused = modes_inputbox(ctx, buf, buf_size, password, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    return focused;
}

static void modes_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                            bool password, bool read_only,
                            WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    modes_inputbox(ctx, buf, buf_size, password, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void modes_frame_type(WLX_Context *ctx, char *buf, size_t buf_size,
                             bool password, bool read_only, const char *text) {
    test_frame_begin_ex(ctx, 200, 150, false, false, false, 0.0f,
                        NULL, NULL, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    modes_inputbox(ctx, buf, buf_size, password, read_only);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// ============================================================================
// Password mode
// ============================================================================

TEST(modes_password_masks_display_keeps_buffer) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = modes_capture_draw_text;
    char buf[64] = "abc";

    modes_reset_text_capture();
    modes_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);

    // The drawn text is one mask char per codepoint; the buffer is untouched.
    ASSERT_EQ_INT(1, _modes_text_draw_count);
    ASSERT_EQ_STR(_modes_text_drawn, "***");
    ASSERT_EQ_STR(buf, "abc");

    // Typing still edits the plaintext and the mask follows.
    modes_reset_text_capture();
    modes_frame_type(&ctx, buf, sizeof(buf), true, false, "d");
    ASSERT_EQ_STR(buf, "abcd");
    ASSERT_EQ_STR(_modes_text_drawn, "****");
    wlx_context_destroy(&ctx);
}

TEST(modes_password_masks_one_char_per_codepoint) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = modes_capture_draw_text;
    // o-umlaut (2 bytes) + 'B': 3 bytes, 2 codepoints.
    char buf[64] = "\xC3\xB6\x42";

    modes_reset_text_capture();
    modes_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);

    ASSERT_EQ_STR(_modes_text_drawn, "**");
    ASSERT_EQ_STR(buf, "\xC3\xB6\x42");
    wlx_context_destroy(&ctx);
}

TEST(modes_password_copy_cut_suppressed) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("seed");
    char buf[64] = "secret";

    modes_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    modes_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_A, modes_command_mod());

    // Neither copy nor cut may export the plaintext; cut must not delete it
    // either.
    modes_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_C, modes_command_mod());
    ASSERT_EQ_STR("seed", test_get_clipboard());
    modes_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_X, modes_command_mod());
    ASSERT_EQ_STR("seed", test_get_clipboard());
    ASSERT_EQ_STR(buf, "secret");
    wlx_context_destroy(&ctx);
}

TEST(modes_password_paste_and_edit_work) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("XY");
    char buf[64] = "ab";

    modes_frame_mouse(&ctx, buf, sizeof(buf), true, false, 380, true, true);
    modes_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_V, modes_command_mod());
    ASSERT_EQ_STR(buf, "abXY");

    modes_frame_key(&ctx, buf, sizeof(buf), true, false, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "abX");
    wlx_context_destroy(&ctx);
}

TEST(modes_password_click_maps_to_plaintext_offset) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // Two codepoints, 3 bytes: the mask is "**" and display boundary 1 sits
    // at x = 9 + 5 (mock measure: 5px per char).
    char buf[64] = "\xC3\xB6\x42";

    // Click between the two mask chars: the caret must land after the 2-byte
    // codepoint (plaintext byte 2), so typing lands between the characters.
    modes_frame_mouse(&ctx, buf, sizeof(buf), true, false, 15, true, true);
    modes_frame_type(&ctx, buf, sizeof(buf), true, false, "X");
    ASSERT_EQ_STR(buf, "\xC3\xB6X\x42");
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Read-only mode
// ============================================================================

TEST(modes_readonly_rejects_all_edits) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("ZZ");
    char buf[64] = "AB";

    bool focused = modes_frame_mouse(&ctx, buf, sizeof(buf), false, true, 380, true, true);
    ASSERT_TRUE(focused);

    modes_frame_type(&ctx, buf, sizeof(buf), false, true, "X");
    ASSERT_EQ_STR(buf, "AB");
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_BACKSPACE, 0);
    ASSERT_EQ_STR(buf, "AB");
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_DELETE, 0);
    ASSERT_EQ_STR(buf, "AB");
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_V, modes_command_mod());
    ASSERT_EQ_STR(buf, "AB");

    // Cut must neither modify the buffer nor reach the clipboard.
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_A, modes_command_mod());
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_X, modes_command_mod());
    ASSERT_EQ_STR(buf, "AB");
    ASSERT_EQ_STR("ZZ", test_get_clipboard());
    wlx_context_destroy(&ctx);
}

TEST(modes_readonly_allows_selection_and_copy) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("");
    char buf[64] = "hello";

    modes_frame_mouse(&ctx, buf, sizeof(buf), false, true, 380, true, true);
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_A, modes_command_mod());
    modes_frame_key(&ctx, buf, sizeof(buf), false, true, WLX_KEY_C, modes_command_mod());

    ASSERT_EQ_STR("hello", test_get_clipboard());
    ASSERT_EQ_STR(buf, "hello");
    wlx_context_destroy(&ctx);
}

TEST(modes_password_readonly_combo) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.draw_text = modes_capture_draw_text;
    test_set_clipboard("seed");
    char buf[64] = "pw";

    modes_reset_text_capture();
    modes_frame_mouse(&ctx, buf, sizeof(buf), true, true, 380, true, true);
    ASSERT_EQ_STR(_modes_text_drawn, "**");

    // No edits, no clipboard export.
    modes_frame_type(&ctx, buf, sizeof(buf), true, true, "X");
    ASSERT_EQ_STR(buf, "pw");
    modes_frame_key(&ctx, buf, sizeof(buf), true, true, WLX_KEY_A, modes_command_mod());
    modes_frame_key(&ctx, buf, sizeof(buf), true, true, WLX_KEY_C, modes_command_mod());
    ASSERT_EQ_STR("seed", test_get_clipboard());
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_modes) {
    RUN_TEST(modes_password_masks_display_keeps_buffer);
    RUN_TEST(modes_password_masks_one_char_per_codepoint);
    RUN_TEST(modes_password_copy_cut_suppressed);
    RUN_TEST(modes_password_paste_and_edit_work);
    RUN_TEST(modes_password_click_maps_to_plaintext_offset);
    RUN_TEST(modes_readonly_rejects_all_edits);
    RUN_TEST(modes_readonly_allows_selection_and_copy);
    RUN_TEST(modes_password_readonly_combo);
}

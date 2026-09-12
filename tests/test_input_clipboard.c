// test_input_clipboard.c - inputbox clipboard shortcuts through the mock
// backend clipboard stub: copy/cut/paste/select-all on the command modifier,
// UTF-8-safe paste truncation, empty-selection no-ops, and the masked-field
// hard gate (copy and cut fully suppressed).

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
// Fixture (same stable-call-site pattern as the selection suite)
// ============================================================================

static uint32_t clip_command_mod(void) {
#if defined(__APPLE__)
    return WLX_MOD_SUPER;
#else
    return WLX_MOD_CTRL;
#endif
}

static bool clip_inputbox(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0, .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static void clip_frame_focus(WLX_Context *ctx, char *buf, size_t buf_size) {
    test_frame_begin(ctx, 200, 150, true, true);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    clip_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void clip_frame_key(WLX_Context *ctx, char *buf, size_t buf_size,
                           WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    clip_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void clip_frame_type(WLX_Context *ctx, char *buf, size_t buf_size, const char *text) {
    test_frame_begin_ex(ctx, 200, 150, false, false, false, 0.0f,
                        NULL, NULL, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    clip_inputbox(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// Masked-field twin of the fixture: same geometry, .password set.
static bool clip_inputbox_pw(WLX_Context *ctx, char *buf, size_t buf_size) {
    bool focused = false;
    (void)wlx_inputbox_impl(ctx, NULL, buf, buf_size,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0, .password = true,
            .out_focused = &focused),
        __FILE__, __LINE__);
    return focused;
}

static void clip_frame_focus_pw(WLX_Context *ctx, char *buf, size_t buf_size) {
    test_frame_begin(ctx, 200, 150, true, true);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    clip_inputbox_pw(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void clip_frame_key_pw(WLX_Context *ctx, char *buf, size_t buf_size,
                              WLX_Key_Code key, uint32_t mods) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, false, false, false, 0.0f,
                          NULL, keys_pressed, NULL, mods, NULL);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    clip_inputbox_pw(ctx, buf, buf_size);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// ============================================================================
// Tests
// ============================================================================

TEST(clip_copy_then_paste_roundtrip) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("");
    char buf[64] = "hello";

    clip_frame_focus(&ctx, buf, sizeof(buf));

    // Select all, copy: the clipboard receives the whole buffer.
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_A, clip_command_mod());
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_C, clip_command_mod());
    ASSERT_EQ_STR("hello", test_get_clipboard());
    ASSERT_EQ_STR(buf, "hello");

    // Replace everything with one typed char, then paste after it.
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_A, clip_command_mod());
    clip_frame_type(&ctx, buf, sizeof(buf), "X");
    ASSERT_EQ_STR(buf, "X");

    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());
    ASSERT_EQ_STR(buf, "Xhello");
    wlx_context_destroy(&ctx);
}

TEST(clip_cut_removes_and_copies) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("");
    char buf[64] = "ABCD";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);

    // Cut selection [2,4).
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_X, clip_command_mod());
    ASSERT_EQ_STR(buf, "AB");
    ASSERT_EQ_STR("CD", test_get_clipboard());

    // Paste it back twice at the caret (collapsed to the cut point).
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());
    ASSERT_EQ_STR(buf, "ABCDCD");
    wlx_context_destroy(&ctx);
}

TEST(clip_select_all_then_delete) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "hello world";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_A, clip_command_mod());
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_BACKSPACE, 0);

    ASSERT_EQ_STR(buf, "");
    wlx_context_destroy(&ctx);
}

TEST(clip_paste_truncates_on_utf8_boundary) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    // Three 2-byte codepoints (6 bytes); the 6-byte buffer holds 5 payload
    // bytes, so the paste must back off to 4 bytes (2 whole codepoints).
    test_set_clipboard("\xC3\xA9\xC3\xA9\xC3\xA9");
    char buf[6] = "";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());

    ASSERT_EQ_STR(buf, "\xC3\xA9\xC3\xA9");
    ASSERT_EQ_INT(4, (int)strlen(buf));
    wlx_context_destroy(&ctx);
}

TEST(clip_paste_replaces_selection) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("XY");
    char buf[64] = "ABCD";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_LEFT, WLX_MOD_SHIFT);
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());

    ASSERT_EQ_STR(buf, "ABXY");
    wlx_context_destroy(&ctx);
}

TEST(clip_empty_selection_copy_is_noop) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("seed");
    char buf[64] = "AB";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_C, clip_command_mod());
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_X, clip_command_mod());

    // Neither copy nor cut touched the clipboard or the buffer.
    ASSERT_EQ_STR("seed", test_get_clipboard());
    ASSERT_EQ_STR(buf, "AB");
    wlx_context_destroy(&ctx);
}

TEST(clip_paste_empty_clipboard_is_noop) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("");
    char buf[64] = "AB";

    clip_frame_focus(&ctx, buf, sizeof(buf));
    clip_frame_key(&ctx, buf, sizeof(buf), WLX_KEY_V, clip_command_mod());

    ASSERT_EQ_STR(buf, "AB");
    wlx_context_destroy(&ctx);
}

TEST(clip_masked_field_never_exports_plaintext) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_set_clipboard("sentinel");
    char buf[64] = "secret";

    clip_frame_focus_pw(&ctx, buf, sizeof(buf));
    clip_frame_key_pw(&ctx, buf, sizeof(buf), WLX_KEY_A, clip_command_mod());

    // Copy on a live selection: the clipboard keeps its previous content.
    clip_frame_key_pw(&ctx, buf, sizeof(buf), WLX_KEY_C, clip_command_mod());
    ASSERT_EQ_STR("sentinel", test_get_clipboard());

    // Cut is fully rejected: the clipboard keeps its content AND the text
    // survives (a silent delete would suggest the bytes were exported).
    clip_frame_key_pw(&ctx, buf, sizeof(buf), WLX_KEY_X, clip_command_mod());
    ASSERT_EQ_STR("sentinel", test_get_clipboard());
    ASSERT_EQ_STR(buf, "secret");
    wlx_context_destroy(&ctx);
}

// The transport must carry spans far beyond the historic 1 KB adapter
// buffers: a 5,000-byte round-trip through the core helpers must come back
// byte-identical, not truncated.
TEST(clip_large_round_trip_grows_transport) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    enum { BIG = 5000 };
    static char big[BIG + 1];
    for (size_t i = 0; i < BIG; i++) big[i] = (char)('a' + (i % 23));
    big[BIG] = '\0';

    wlx_clipboard_set_text(&ctx, big, BIG);

    static char out[BIG + 16];
    size_t got = wlx_clipboard_get_copy(&ctx, out, sizeof(out));
    ASSERT_TRUE(got == (size_t)BIG);
    ASSERT_TRUE(memcmp(out, big, BIG) == 0);
    ASSERT_TRUE(out[BIG] == '\0');

    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(input_clipboard) {
    RUN_TEST(clip_large_round_trip_grows_transport);
    RUN_TEST(clip_copy_then_paste_roundtrip);
    RUN_TEST(clip_cut_removes_and_copies);
    RUN_TEST(clip_select_all_then_delete);
    RUN_TEST(clip_paste_truncates_on_utf8_boundary);
    RUN_TEST(clip_paste_replaces_selection);
    RUN_TEST(clip_empty_selection_copy_is_noop);
    RUN_TEST(clip_paste_empty_clipboard_is_noop);
    RUN_TEST(clip_masked_field_never_exports_plaintext);
}

// test_undo_disabled.c - WLX_TEXT_UNDO_ENTRIES 0 compiles the undo journal
// out: the text widgets build and edit without history, the lookup yields
// no journal, the context keeps no cache, and the undo chords are inert,
// in the inputbox and in the editor (wollix_editor.h is compiled here too).
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with the override predefined before including wollix.h.

#define WLX_TEXT_UNDO_ENTRIES 0

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "wollix_editor.h"
#include "test_mock_backend.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if WLX_TEXT_UNDO_ENTRIES != 0
#error "WLX_TEXT_UNDO_ENTRIES override was clobbered by wollix.h"
#endif

static bool last_changed = false;

static void frame(WLX_Context *ctx, char *buf, size_t cap, bool click,
                  WLX_Key_Code key, uint32_t mods, const char *text) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, click, click, click, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    last_changed = wlx_inputbox_impl(ctx, NULL, buf, cap,
        wlx_default_inputbox_opt(.content_padding = 4, .font_size = 10,
            .wrap = false, .border_width = 0),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static void editor_frame(WLX_Context *ctx, char *buf, size_t cap, size_t *len, bool click,
                         WLX_Key_Code key, uint32_t mods, const char *text) {
    bool keys_pressed[WLX_KEY_COUNT] = {0};
    if (key != WLX_KEY_NONE) keys_pressed[key] = true;
    test_frame_begin_full(ctx, 200, 150, click, click, click, 0.0f, NULL,
        key != WLX_KEY_NONE ? keys_pressed : NULL, NULL, mods, text);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    last_changed = wlx_editor(ctx, "doc", buf, cap, len, .font_size = 10);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

int main(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    char buf[64] = "";

    frame(&ctx, buf, sizeof(buf), true, WLX_KEY_NONE, 0, NULL);   // focus
    frame(&ctx, buf, sizeof(buf), false, WLX_KEY_NONE, 0, "ab");  // type
    assert(strcmp(buf, "ab") == 0);
    assert(last_changed);
    assert(ctx.text_undo.count == 0 && ctx.text_undo.items == NULL);
    assert(wlx_text_undo_get(&ctx, 1, 2, 0, false) == NULL);
    assert(wlx_text_undo_find(&ctx, 1) == NULL);

    // The chords are inert: nothing changes, nothing is reported.
    frame(&ctx, buf, sizeof(buf), false, WLX_KEY_Z, test_command_mod(), NULL);
    assert(strcmp(buf, "ab") == 0);
    assert(!last_changed);
    frame(&ctx, buf, sizeof(buf), false, WLX_KEY_Z, test_command_mod() | WLX_MOD_SHIFT, NULL);
    frame(&ctx, buf, sizeof(buf), false, WLX_KEY_Y, test_command_mod(), NULL);
    assert(strcmp(buf, "ab") == 0);
    assert(!last_changed);

    wlx_context_destroy(&ctx);

    // The editor: an edit lands, nothing is journaled, the chords are inert.
    test_ctx_init(&ctx, 400, 300);
    char doc[64] = "line";
    size_t len = strlen(doc);
    editor_frame(&ctx, doc, sizeof(doc), &len, true, WLX_KEY_NONE, 0, NULL);       // focus
    editor_frame(&ctx, doc, sizeof(doc), &len, false, WLX_KEY_END, 0, NULL);
    editor_frame(&ctx, doc, sizeof(doc), &len, false, WLX_KEY_NONE, 0, "s");       // type
    assert(len == 5 && memcmp(doc, "lines", 5) == 0);
    assert(last_changed);
    assert(ctx.text_undo.count == 0 && ctx.text_undo.items == NULL);
    editor_frame(&ctx, doc, sizeof(doc), &len, false, WLX_KEY_Z, test_command_mod(), NULL);
    assert(len == 5 && memcmp(doc, "lines", 5) == 0);
    assert(!last_changed);
    editor_frame(&ctx, doc, sizeof(doc), &len, false, WLX_KEY_Y, test_command_mod(), NULL);
    assert(len == 5 && memcmp(doc, "lines", 5) == 0);
    assert(!last_changed);
    wlx_context_destroy(&ctx);

    printf("test_undo_disabled: OK (journal compiled out, undo chords inert in the inputbox and the editor)\n");
    return 0;
}

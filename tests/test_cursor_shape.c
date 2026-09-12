// test_cursor_shape.c - the cursor-shape contract: the previous frame's
// topmost candidate under the pointer decides the shape pushed through
// backend.set_cursor; text-editing surfaces (inputbox, editor text region)
// ask for the I-beam, everything else is the arrow; the active widget's
// shape survives a press-drag that leaves its rect; the push happens only
// on change and is a safe no-op without the callback.
//
// Every test stages at least two frames: the shape is resolved from the
// candidate list recorded on the frame before.

#ifndef WOLLIX_H_
#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#endif
#ifndef WOLLIX_EDITOR_H_
#include "wollix_editor.h"
#endif
#ifndef TESTS_H_
#include "tests.h"
#endif
#ifndef TEST_MOCK_BACKEND_H_
#include "test_mock_backend.h"
#endif

// ============================================================================
// Fixtures
// ============================================================================

// Button at y 0..40, inputbox at y 60..100, both full width of a 400x300
// context (40px pixel slots separated by a 20px gap; widgets fill their
// slots). One frame at (mx, my) with the given left-button state.
static void cs_frame_button_and_input(WLX_Context *ctx, int mx, int my,
                                      bool down, bool clicked) {
    static char buf[32] = "hello";
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(40), WLX_SLOT_FLEX(1)),
        .padding = 0, .gap = 20);
    wlx_button(ctx, "btn");
    wlx_inputbox(ctx, NULL, buf, sizeof(buf));
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

// Editor with a line-number gutter in a 400x100 context (fixture geometry
// from test_editor_view.c: text band starts past x 24 when 40 lines are
// numbered).
static void cs_frame_editor(WLX_Context *ctx, char *buf, size_t cap, size_t *len,
                            int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin(ctx, 1, WLX_VERT, .padding = 0, .gap = 0);
    (void)wlx_editor_impl(ctx, NULL, buf, cap, len,
        wlx_default_editor_opt(.content_padding = 4, .font_size = 10,
            .border_width = 0, .line_numbers = true),
        __FILE__, __LINE__);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
}

static size_t cs_fill_lines(char *buf, size_t cap, size_t n) {
    size_t off = 0;
    for (size_t i = 0; i < n && off + 4 < cap; i++) {
        buf[off++] = 'l';
        buf[off++] = (char)('0' + (i / 10) % 10);
        buf[off++] = (char)('0' + i % 10);
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    return off;
}

// ============================================================================
// Shape selection
// ============================================================================

TEST(cursor_ibeam_over_inputbox_arrow_over_button) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_reset_mock_cursor();

    // Over the inputbox: frame 1 records candidates, frame 2 resolves.
    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());   // nothing to resolve yet
    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());

    // Move over the button: arrow again.
    cs_frame_button_and_input(&ctx, 200, 20, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());

    // Off every widget: arrow.
    cs_frame_button_and_input(&ctx, 200, 250, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());
    wlx_context_destroy(&ctx);
}

TEST(cursor_pushed_only_on_change) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_reset_mock_cursor();

    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    int after_enter = mock_cursor_calls();
    ASSERT_EQ_INT(1, after_enter);                       // ARROW -> IBEAM once
    cs_frame_button_and_input(&ctx, 210, 85, false, false);
    cs_frame_button_and_input(&ctx, 220, 90, false, false);
    ASSERT_EQ_INT(after_enter, mock_cursor_calls());     // still IBEAM, no push
    cs_frame_button_and_input(&ctx, 200, 20, false, false);
    ASSERT_EQ_INT(after_enter + 1, mock_cursor_calls()); // IBEAM -> ARROW once
    wlx_context_destroy(&ctx);
}

TEST(cursor_editor_text_ibeam_gutter_arrow) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 100);
    test_reset_mock_cursor();

    char buf[512];
    size_t len = cs_fill_lines(buf, sizeof(buf), 40);

    // Two plain frames teach the state its gutter width and record the
    // gutter-trimmed candidate; then hovering the gutter strip must stay
    // arrow while the text band shows the I-beam.
    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);
    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 0, 0, false, false);
    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 15, 30, false, false);
    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 15, 30, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());

    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 100, 50, false, false);
    cs_frame_editor(&ctx, buf, sizeof(buf), &len, 100, 50, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());
    wlx_context_destroy(&ctx);
}

TEST(cursor_overlay_covering_input_is_arrow) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_reset_mock_cursor();

    static char buf[32] = "hello";
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 200, 20, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_FLEX(1)), .padding = 0, .gap = 0);
        wlx_inputbox(&ctx, NULL, buf, sizeof(buf));                 // base, y 0..40
        wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 150, 0, 100, 40 }));
        wlx_button(&ctx, "over");                             // covers x 150..250
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    // The topmost candidate (the button on layer 1) wins: arrow, not the
    // I-beam of the inputbox underneath.
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());

    // Beside the overlay the inputbox is topmost: I-beam.
    for (int frame = 0; frame < 2; frame++) {
        test_frame_begin(&ctx, 50, 20, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT,
            WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_FLEX(1)), .padding = 0, .gap = 0);
        wlx_inputbox(&ctx, NULL, buf, sizeof(buf));
        wlx_overlay_begin(&ctx, 1, ((WLX_Rect){ 150, 0, 100, 40 }));
        wlx_button(&ctx, "over");
        wlx_overlay_end(&ctx);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Active-widget override during a press-drag
// ============================================================================

TEST(cursor_drag_out_of_inputbox_keeps_ibeam) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    test_reset_mock_cursor();

    // Hover, press inside the inputbox (focus + active), then drag the
    // pointer over the button while still holding: the active widget's
    // I-beam must survive; releasing over the button returns the arrow.
    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    cs_frame_button_and_input(&ctx, 200, 80, true, true);
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());
    cs_frame_button_and_input(&ctx, 200, 20, true, false);
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());
    cs_frame_button_and_input(&ctx, 200, 20, true, false);
    ASSERT_EQ_INT(WLX_CURSOR_IBEAM, mock_last_cursor());
    cs_frame_button_and_input(&ctx, 200, 20, false, false);
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, mock_last_cursor());
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Missing callback
// ============================================================================

TEST(cursor_null_callback_is_safe) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    ctx.backend.set_cursor = NULL;
    test_reset_mock_cursor();

    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    cs_frame_button_and_input(&ctx, 200, 80, false, false);
    cs_frame_button_and_input(&ctx, 200, 20, false, false);
    ASSERT_EQ_INT(0, mock_cursor_calls());
    ASSERT_EQ_INT(WLX_CURSOR_ARROW, ctx.cursor_applied);  // tracked, never pushed
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(cursor_shape) {
    RUN_TEST(cursor_ibeam_over_inputbox_arrow_over_button);
    RUN_TEST(cursor_pushed_only_on_change);
    RUN_TEST(cursor_editor_text_ibeam_gutter_arrow);
    RUN_TEST(cursor_overlay_covering_input_is_arrow);
    RUN_TEST(cursor_drag_out_of_inputbox_keeps_ibeam);
    RUN_TEST(cursor_null_callback_is_safe);
}

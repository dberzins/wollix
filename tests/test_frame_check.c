// test_frame_check.c - WLX_FRAME_CHECK: the core reports a frame that drew
// something new although no input arrived and no frame request was due.
//
// A loop that waits on wlx_frame_delay would never have produced such a
// frame, so the change would not have been shown. The check runs with
// frames produced continuously and judges each one as the waiting loop
// would have seen it. What it has to catch is a change after a rest: a
// widget that changes on every frame keeps itself alive, because a frame
// that differs from the one before always asks for the next.
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with the switch and a capturing print sink predefined
// before including wollix.h.

#define WLX_FRAME_CHECK

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Capture the reports instead of printing them.
#define FC_LINES_MAX 8
static char fc_lines[FC_LINES_MAX][400];
static int  fc_line_count = 0;
static void fc_sink(const char *msg) {
    if (fc_line_count < FC_LINES_MAX) {
        snprintf(fc_lines[fc_line_count], sizeof fc_lines[0], "%s", msg);
    }
    fc_line_count++;
}
#define WLX_ERROR_PRINT(msg) fc_sink(msg)

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "test_mock_backend.h"

#ifndef WLX_FRAME_CHECK
#error "WLX_FRAME_CHECK was undefined by wollix.h"
#endif

// ============================================================================
// Fixture
// ============================================================================

typedef enum {
    FC_STATIC,          // nothing changes
    FC_BLINK,           // the button changes colour every FC_PERIOD frames, asking for nothing
    FC_BLINK_ASKS,      // the same, asking each frame for the time left to the change
    FC_BLINK_ASKS_LATE, // the same, asking for a frame a second away
    FC_APPEARS          // a second label appears after a rest
} Fc_Mode;

#define FC_PERIOD 4

static Fc_Mode fc_mode = FC_STATIC;
static int     fc_frame_no = 0;
static float   fc_dt = 1.0f / 60.0f;
static float fc_get_frame_time(void *user) {
    (void)user; return fc_dt; }

// A label and a button, 40 px each, at the top of the root.
static void fc_frame(WLX_Context *ctx, int mx, int my, bool down, bool clicked) {
    test_frame_begin(ctx, mx, my, down, clicked);
    wlx_layout_begin_s(ctx, WLX_VERT,
        WLX_SIZES(WLX_SLOT_PX(40), WLX_SLOT_PX(40), WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(ctx, "above", .height = 40);

    bool lit = (fc_mode == FC_BLINK || fc_mode == FC_BLINK_ASKS || fc_mode == FC_BLINK_ASKS_LATE)
        && ((fc_frame_no / FC_PERIOD) % 2) == 1;
    WLX_Color back = lit ? (WLX_Color){ 200, 60, 60, 255 } : (WLX_Color){ 40, 60, 90, 255 };
    (void)wlx_button(ctx, "blink", .height = 40, .back_color = back);

    if (fc_mode == FC_BLINK_ASKS) {
        int frames_left = FC_PERIOD - (fc_frame_no % FC_PERIOD);
        wlx_request_frame_after(ctx, (float)frames_left * fc_dt);
    } else if (fc_mode == FC_BLINK_ASKS_LATE) {
        wlx_request_frame_after(ctx, 1.0f);
    }
    if (fc_mode == FC_APPEARS && fc_frame_no >= 6) wlx_label(ctx, "later", .height = 40);

    wlx_layout_end(ctx);
    test_frame_end(ctx);
    fc_frame_no++;
}

static void fc_reset(WLX_Context *ctx, Fc_Mode mode) {
    test_ctx_init(ctx, 400, 300);
    ctx->backend.get_frame_time = fc_get_frame_time;
    fc_mode = mode;
    fc_frame_no = 0;
    fc_dt = 1.0f / 60.0f;
    fc_line_count = 0;
}

static int fc_reports(const WLX_Context *ctx) {
    return ctx->frame_check != NULL ? ctx->frame_check->reports : 0;
}

static void fc_run(WLX_Context *ctx, int frames) {
    for (int i = 0; i < frames; i++) fc_frame(ctx, 300, 250, false, false);
}

// ============================================================================
// Tests
// ============================================================================

static void test_static_ui_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_run(&ctx, 12);
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    assert(wlx_frame_delay(&ctx) < 0.0f);      // the signal itself is unchanged by the check
    wlx_context_destroy(&ctx);
    assert(ctx.frame_check == NULL);
}

// A change after a rest, with no request: every such frame is counted, the
// place is printed once, and the report names what changed and where.
static void test_unrequested_change_is_reported_once_and_named(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_BLINK);
    fc_run(&ctx, 4);
    assert(fc_reports(&ctx) == 0);             // frames 0-3: nothing changed yet
    fc_run(&ctx, 1);
    assert(fc_reports(&ctx) == 1);             // frame 4: the colour flips
    assert(fc_line_count == 1);
    fc_run(&ctx, 7);                           // frames 5-11: one more flip at 8
    assert(fc_reports(&ctx) == 2);
    assert(fc_line_count == 1);                // the same place prints once

    assert(strstr(fc_lines[0], "wollix frame check") != NULL);
    assert(strstr(fc_lines[0], "differs") != NULL);
    assert(strstr(fc_lines[0], "400x40") != NULL);          // the button's row
    assert(strstr(fc_lines[0], "wlx_request_frame") != NULL);
    wlx_context_destroy(&ctx);
}

static void test_requested_change_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_BLINK_ASKS);
    fc_run(&ctx, 13);
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

// Asking is not enough: the request has to be due by the time of the change.
static void test_late_request_is_reported(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_BLINK_ASKS_LATE);
    fc_run(&ctx, 9);
    assert(fc_reports(&ctx) == 2);
    assert(fc_line_count == 1);
    wlx_context_destroy(&ctx);
}

static void test_change_after_input_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_run(&ctx, 4);
    fc_frame(&ctx, 50, 60, false, false);      // onto the button: hover
    fc_frame(&ctx, 50, 60, false, false);
    fc_frame(&ctx, 50, 60, true, true);        // press
    fc_frame(&ctx, 50, 60, true, false);
    fc_frame(&ctx, 50, 60, false, false);      // release
    fc_run(&ctx, 4);                           // away again
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

// A resized window redraws by right: the window event wakes a waiting loop.
static void test_resize_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_run(&ctx, 4);
    ctx.rect = wlx_rect(0, 0, 520, 300);
    fc_run(&ctx, 4);
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

static void test_new_draw_call_is_named_as_new(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_APPEARS);
    fc_run(&ctx, 8);
    assert(fc_reports(&ctx) == 1);
    assert(fc_line_count == 1);
    assert(strstr(fc_lines[0], "is new") != NULL);
    wlx_context_destroy(&ctx);
}

static WLX_Text_Style fc_recolour(WLX_Text_Style style, void *user) {
    (void)user;
    style.color.r = (uint8_t)(255 - style.color.r);
    return style;
}

static WLX_Text_Style fc_bigger(WLX_Text_Style style, void *user) {
    (void)user;
    style.font_size += 2;
    return style;
}

// A style transform set between frames, with no input and no request. One
// that only recolours leaves every draw call as it was and changes how the
// text is drawn; the report says so. One that resizes moves the fitted text,
// and the report names the first text draw that moved.
static void test_style_transform_change_is_reported(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_run(&ctx, 4);
    wlx_set_style_transform(&ctx, fc_recolour, NULL);
    fc_run(&ctx, 2);
    assert(fc_reports(&ctx) == 1);
    assert(fc_line_count == 1);
    assert(strstr(fc_lines[0], "text style transform") != NULL);

    wlx_set_style_transform(&ctx, fc_bigger, NULL);
    fc_run(&ctx, 2);
    assert(fc_reports(&ctx) == 2);
    assert(fc_line_count == 2);
    assert(strstr(fc_lines[1], "(text)") != NULL);
    wlx_set_style_transform(&ctx, NULL, NULL);
    wlx_context_destroy(&ctx);
}

// The core's own timers ask for their frames: a blinking caret and a
// tooltip never trip the check.
static void test_caret_blink_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_dt = 0.1f;
    char buf[32] = "abc";
    for (int i = 0; i < 40; i++) {
        bool press = i == 1;
        test_frame_begin(&ctx, 100, 20, press, press);
        wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
        (void)wlx_inputbox(&ctx, NULL, buf, sizeof buf, .height = 40);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

static void test_tooltip_is_quiet(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    fc_dt = 0.2f;
    bool shown = false;
    for (int i = 0; i < 12; i++) {
        int mx = i < 3 ? 300 : 50, my = i < 3 ? 200 : 20;
        test_frame_begin(&ctx, mx, my, false, false);
        wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
        wlx_label(&ctx, "anchor", .height = 40);
        shown = wlx_tooltip_for(&ctx, ((WLX_Rect){ 0, 0, 400, 40 }), "hint", .delay = 0.5f);
        wlx_layout_end(&ctx);
        test_frame_end(&ctx);
    }
    assert(shown);
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

// Nothing is compared under wlx_begin_immediate, so nothing is judged.
static void test_immediate_mode_is_not_judged(void) {
    WLX_Context ctx;
    fc_reset(&ctx, FC_STATIC);
    for (int i = 0; i < 8; i++) {
        memset(&_test_staged_input, 0, sizeof _test_staged_input);
        wlx_begin_immediate(&ctx, ctx.rect, _test_input_handler);
        wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
        wlx_label(&ctx, (i / 2) % 2 ? "one" : "two", .height = 40);
        wlx_layout_end(&ctx);
        wlx_end(&ctx);
    }
    assert(fc_reports(&ctx) == 0 && fc_line_count == 0);
    wlx_context_destroy(&ctx);
}

int main(void) {
    test_static_ui_is_quiet();
    test_unrequested_change_is_reported_once_and_named();
    test_requested_change_is_quiet();
    test_late_request_is_reported();
    test_change_after_input_is_quiet();
    test_resize_is_quiet();
    test_new_draw_call_is_named_as_new();
    test_style_transform_change_is_reported();
    test_caret_blink_is_quiet();
    test_tooltip_is_quiet();
    test_immediate_mode_is_not_judged();
    printf("test_frame_check: OK (WLX_FRAME_CHECK reports a change nobody asked for, 11 cases)\n");
    return 0;
}

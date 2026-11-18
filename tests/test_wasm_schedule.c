// test_wasm_schedule.c - wlx_wasm_schedule_frame against a host stand-in.
//
// On the web the host owns the frame loop, so the adapter's frame helper
// does not wait: it hands wlx_frame_delay to the host through the
// schedule_frame import, once per frame, and the host decides when to run
// the next one. Off wasm the import declaration in wollix_wasm.h is a plain
// prototype, so this TU defines it and records what the helper passed.
//
// Never reference wlx_backend_wasm() here: its initializer takes the address
// of every import wrapper and would pull all the undefined imports into the
// link.

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
#include "wollix_wasm.h"

// ============================================================================
// Host stand-in
// ============================================================================

static float g_sched_host_seconds = 123.0f;
static int   g_sched_host_calls   = 0;

void wlx_wasm_import_schedule_frame(float seconds) {
    g_sched_host_seconds = seconds;
    g_sched_host_calls++;
}

// One frame of a static label, ended the way a web application ends it.
static void ws_frame(WLX_Context *ctx) {
    test_frame_begin(ctx, 300, 250, false, false);
    wlx_layout_begin_s(ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(ctx, "idle", .height = 40);
    wlx_layout_end(ctx);
    test_frame_end(ctx);
    wlx_wasm_schedule_frame(ctx);
}

// ============================================================================
// Tests
// ============================================================================

// The host is told what wlx_frame_delay says, once per frame: now for a
// frame that changed, "only on input" once the UI rests, and the delay of a
// timed request.
TEST(wasm_schedule_frame_passes_the_frame_delay) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 400, 300);
    g_sched_host_calls = 0;

    ws_frame(&ctx);                                  // a context's first frame
    ASSERT_EQ_INT(1, g_sched_host_calls);
    ASSERT_EQ_F(0.0f, g_sched_host_seconds, 0.0f);

    ws_frame(&ctx);
    ws_frame(&ctx);
    ASSERT_EQ_INT(3, g_sched_host_calls);
    ASSERT_TRUE(g_sched_host_seconds < 0.0f);        // at rest: nothing scheduled
    ASSERT_EQ_F(wlx_frame_delay(&ctx), g_sched_host_seconds, 0.0f);

    // A timed request made before the helper is what the host hears.
    test_frame_begin(&ctx, 300, 250, false, false);
    wlx_layout_begin_s(&ctx, WLX_VERT, WLX_SIZES(WLX_SLOT_PX(40)), .padding = 0);
    wlx_label(&ctx, "idle", .height = 40);
    wlx_layout_end(&ctx);
    test_frame_end(&ctx);
    wlx_request_frame_after(&ctx, 0.75f);
    wlx_wasm_schedule_frame(&ctx);
    ASSERT_EQ_INT(4, g_sched_host_calls);
    ASSERT_EQ_F(0.75f, g_sched_host_seconds, 0.0f);

    wlx_context_destroy(&ctx);
}

SUITE(wasm_schedule) {
    RUN_TEST(wasm_schedule_frame_passes_the_frame_delay);
}

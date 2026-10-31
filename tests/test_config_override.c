// test_config_override.c - the documented limit macros are overridable.
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with the documented limit macros predefined to non-default values
// before including wollix.h. The header must honor a prior definition
// instead of clobbering it back to the default, and the library must stay
// functional under the overridden limits.

#define WLX_MAX_SLOT_COUNT 512
#define WLX_CONTENT_SLOTS_MAX 8
#define WLX_OFFSET_STACK_LIMIT 16
#define WLX_DA_INIT_CAP 32
#define WLX_OVERLAY_MAX_LAYERS 4
#define WLX_TEXT_UNDO_ENTRIES 64
#define WLX_TEXT_UNDO_BYTES 4096
#define WLX_CONTENT_SCALE_MAX 3.0f
#define WLX_STATE_MIN_AGE 1

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"
#include "test_mock_backend.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

// Compile-time proof: a clobbering redefinition inside the header would put
// the defaults back (and warn); these #errors fire in that case.
#if WLX_MAX_SLOT_COUNT != 512
#error "WLX_MAX_SLOT_COUNT override was clobbered by wollix.h"
#endif
#if WLX_CONTENT_SLOTS_MAX != 8
#error "WLX_CONTENT_SLOTS_MAX override was clobbered by wollix.h"
#endif
#if WLX_OFFSET_STACK_LIMIT != 16
#error "WLX_OFFSET_STACK_LIMIT override was clobbered by wollix.h"
#endif
#if WLX_DA_INIT_CAP != 32
#error "WLX_DA_INIT_CAP override was clobbered by wollix.h"
#endif
#if WLX_OVERLAY_MAX_LAYERS != 4
#error "WLX_OVERLAY_MAX_LAYERS override was clobbered by wollix.h"
#endif
#if WLX_TEXT_UNDO_ENTRIES != 64
#error "WLX_TEXT_UNDO_ENTRIES override was clobbered by wollix.h"
#endif
#if WLX_TEXT_UNDO_BYTES != 4096
#error "WLX_TEXT_UNDO_BYTES override was clobbered by wollix.h"
#endif
#if WLX_STATE_MIN_AGE != 1
#error "WLX_STATE_MIN_AGE override was clobbered by wollix.h"
#endif

static int approx(float a, float b) { return fabsf(a - b) < 0.01f; }
static int   co_errors = 0;
static float co_scale  = 1.0f;
static void  co_capture(const WLX_Error *e, void *user) { (void)e; (void)user; co_errors++; }
static float co_get_scale(void *user) { (void)user; return co_scale; }

int main(void) {
    // Smoke: the offset solver works under the overridden limits, on the
    // above-stack-limit path (count > WLX_OFFSET_STACK_LIMIT with a min
    // constraint forces the redistribution scratch fallback).
    enum { N = 24 };
    WLX_Slot_Size sizes[N];
    float off[N + 1];
    for (int i = 0; i < N; i++) sizes[i] = WLX_SLOT_FLEX(1);
    sizes[0] = WLX_SLOT_FLEX_MIN(1, 50);

    wlx_compute_offsets(off, N, 240.0f, 240.0f, sizes, 0.0f);
    assert(approx(off[0], 0.0f));
    assert(off[1] >= 50.0f);              // min clamp held
    assert(approx(off[N], 240.0f));       // redistribution keeps the total

    // The float range override is honoured at run time (the preprocessor
    // cannot compare floats, so this is its check): 4.0 is outside the
    // overridden maximum and reports, 2.0 is inside and does not.
    assert(WLX_CONTENT_SCALE_MAX == 3.0f);
    {
        WLX_Context ctx;
        test_ctx_init(&ctx, 100, 50);
        ctx.backend.get_content_scale = co_get_scale;
        wlx_set_error_handler(&ctx, co_capture, NULL);
        co_scale = 4.0f;
        test_frame_begin(&ctx, 0, 0, false, false);
        test_frame_end(&ctx);
        assert(co_errors == 1 && approx(wlx_content_scale(&ctx), 1.0f));
        co_scale = 2.0f;
        test_frame_begin(&ctx, 0, 0, false, false);
        test_frame_end(&ctx);
        assert(co_errors == 1 && approx(wlx_content_scale(&ctx), 2.0f));
        wlx_context_destroy(&ctx);
    }

    // WLX_STATE_MIN_AGE 1, the smallest legal age: under pressure on every
    // frame, an entry requested every frame keeps its value through every
    // sweep, and one skipped for a single frame is reclaimed.
    {
        WLX_Context ctx;
        test_ctx_init(&ctx, 100, 50);
        int lost = 0, skipped_reclaimed = 0;
        for (int f = 0; f < 20; f++) {
            ctx.states.trigger = 1;   // a sweep at every wlx_begin
            test_frame_begin(&ctx, 0, 0, false, false);
            for (WLX_Id k = 0; k < 2; k++) {
                wlx_push_id(&ctx, k);
                int *v = (int *)wlx_get_state_impl(&ctx, sizeof(int), "co/age.c", 1).data;
                wlx_pop_id(&ctx);
                if (f > 0 && *v != 1) lost++;
                *v = 1;
            }
            if (f % 2 == 0) {
                int *w = (int *)wlx_get_state_impl(&ctx, sizeof(int), "co/age.c", 2).data;
                if (f > 0 && *w == 0) skipped_reclaimed++;
                *w = 1;
            }
            test_frame_end(&ctx);
        }
        assert(lost == 0);
        assert(skipped_reclaimed == 9);
        wlx_context_destroy(&ctx);
    }
    printf("test_config_override: OK (pre-include limit overrides honored, MIN_AGE 1 keeps every-frame state)\n");
    return 0;
}

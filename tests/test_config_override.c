// test_config_override.c - the documented limit macros are overridable.
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with the documented limit macros predefined to non-default values
// before including wollix.h. The header must honor a prior definition
// instead of clobbering it back to the default, and the library must stay
// functional under the overridden limits. It also defines WLX_SHORT_NAMES
// and expands every short alias once (short_names_frame).

#define WLX_MAX_SLOT_COUNT 512
#define WLX_CONTENT_SLOTS_MAX 8
#define WLX_OFFSET_STACK_LIMIT 16
#define WLX_DA_INIT_CAP 32
#define WLX_OVERLAY_MAX_LAYERS 4
#define WLX_TEXT_UNDO_ENTRIES 64
#define WLX_TEXT_UNDO_BYTES 4096
#define WLX_CONTENT_SCALE_MAX 3.0f
#define WLX_STATE_MIN_AGE 1
#define WLX_SHORT_NAMES

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

// Every WLX_SHORT_NAMES alias, each expanded once in one frame, most in the
// README's form without options. An alias is a macro, so a stale one fails
// only where it is expanded. `make docs-check` holds the calls between the
// markers and the header's alias list to one set.
static int short_names_frame(void) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 1400);
    co_errors = 0;
    wlx_set_error_handler(&ctx, co_capture, NULL);
    test_frame_begin(&ctx, 0, 0, false, false);
    static char field[32] = "in", area[64] = "area";
    static const char *opts[] = { "a", "b" };
    bool on = true, menu_open = true, sub_open = true, button_open = false;
    float value = 0.5f;
    int active = 0, selected = 0;
    WLX_Texture tex = { 1, 8, 8 };
    // short-names:begin
    layout_begin(&ctx, 26, WLX_VERT);
    label(&ctx, "label");
    (void)button(&ctx, "button");
    (void)checkbox(&ctx, "check", &on);
    (void)inputbox(&ctx, "in", field, sizeof(field));
    (void)textarea(&ctx, "area", area, sizeof(area));
    (void)slider(&ctx, "slider", &value);
    separator(&ctx);
    progress(&ctx, value);
    (void)toggle(&ctx, "toggle", &on);
    (void)radio(&ctx, "radio", &active, 0);
    (void)dropdown(&ctx, "drop", &selected, opts, 2);
    widget(&ctx);
    image(&ctx, tex);
    (void)tooltip_for(&ctx, last_rect(&ctx), "tip");
    slot_style(&ctx, .back_color = WLX_RGBA(1, 2, 3, 255));
    layout_begin_s(&ctx, WLX_HORZ, WLX_SIZES(WLX_SLOT_FLEX(1), WLX_SLOT_FLEX(1)), .padding = 0);
        label(&ctx, "s1");
        label(&ctx, "s2");
    layout_end(&ctx);
    layout_begin_auto(&ctx, WLX_VERT, 0);
        layout_auto_slot(&ctx, WLX_SLOT_PX(10));
        label(&ctx, "a1");
        layout_auto_slot_px(&ctx, 10);
        label(&ctx, "a2");
    layout_end(&ctx);
    grid_begin(&ctx, 2, 2);
        grid_cell(&ctx, 1, 1);
        label(&ctx, "g11");
        grid_cell_style(&ctx, .back_color = WLX_RGBA(4, 5, 6, 255));
        label(&ctx, "g00");
    grid_end(&ctx);
    grid_begin_auto(&ctx, 2, 10);
        label(&ctx, "ga1");
        label(&ctx, "ga2");
    grid_end(&ctx);
    grid_begin_auto_tile(&ctx, 100, 10);
        label(&ctx, "gt1");
    grid_end(&ctx);
    scroll_panel_begin(&ctx, 100);
        label(&ctx, "scrolled");
    scroll_panel_end(&ctx);
    split_begin(&ctx);
        label(&ctx, "left");
    split_next(&ctx);
        label(&ctx, "right");
    split_end(&ctx);
    panel_begin(&ctx, .title = "panel");
        label(&ctx, "in panel");
    panel_end(&ctx);
    push_id(&ctx, 7);
    push_opacity(&ctx, 0.5f);
        label(&ctx, "faded");
    pop_opacity(&ctx);
    pop_id(&ctx);
    if (menu_button_begin(&ctx, "file", &button_open)) menu_end(&ctx);
    overlay_begin(&ctx, 1, wlx_rect(600, 1200, 100, 40));
        label(&ctx, "over");
    overlay_end(&ctx);
    if (menu_begin(&ctx, &menu_open, 600, 1000)) {
        (void)menu_item(&ctx, "more", .keep_open = true);
        if (submenu_begin(&ctx, &sub_open)) {
            (void)menu_item(&ctx, "leaf");
            menu_end(&ctx);
        }
        menu_end(&ctx);
    }
    layout_end(&ctx);
    // short-names:end
    test_frame_end(&ctx);
    int errors = co_errors;
    wlx_context_destroy(&ctx);
    return errors;
}

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
    assert(short_names_frame() == 0);

    printf("test_config_override: OK (pre-include limit overrides honored, MIN_AGE 1 keeps every-frame state, "
           "every short name expands)\n");
    return 0;
}

// test_single_pass.c - opt-out validation for WLX_SLOT_SINGLE_PASS_CLAMP.
//
// Compiled as its own translation unit with -DWLX_SLOT_SINGLE_PASS_CLAMP (it is
// NOT part of the single-TU test_runner). Asserts that defining the opt-out
// restores the legacy single-pass clamp: a clamped slot does not hand its
// surplus/deficit back to siblings, so the boundary may overflow or leave a gap.

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef WLX_SLOT_SINGLE_PASS_CLAMP
#error "compile this with -DWLX_SLOT_SINGLE_PASS_CLAMP"
#endif

static int approx(float a, float b) { return fabsf(a - b) < 0.01f; }

int main(void) {
    // flex_with_min: slot 0 clamps up to 150; without redistribution slots 1&2
    // keep their single-pass 100 each, so the boundary overshoots total.
    {
        WLX_Slot_Size sizes[] = {
            WLX_SLOT_FLEX_MIN(1, 150),
            WLX_SLOT_FLEX(1),
            WLX_SLOT_FLEX(1),
        };
        float off[4];
        wlx_compute_offsets(off, 3, 300.0f, 300.0f, sizes, 0.0f);
        assert(approx(off[0],   0.0f));
        assert(approx(off[1], 150.0f));
        assert(approx(off[2], 250.0f));
        assert(approx(off[3], 350.0f));  // overflow: no redistribution
    }

    // flex_with_max: slot 0 clamps down to 100; slot 1 keeps its single-pass
    // 200, leaving a 100px gap versus the 400 total.
    {
        WLX_Slot_Size sizes[] = {
            WLX_SLOT_FLEX_MAX(1, 100),
            WLX_SLOT_FLEX(1),
        };
        float off[3];
        wlx_compute_offsets(off, 2, 400.0f, 400.0f, sizes, 0.0f);
        assert(approx(off[0],   0.0f));
        assert(approx(off[1], 100.0f));
        assert(approx(off[2], 300.0f));  // gap: no redistribution
    }

    printf("test_single_pass: OK (single-pass clamp, no redistribution)\n");
    return 0;
}

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

#define WOLLIX_IMPLEMENTATION
#include "wollix.h"

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

static int approx(float a, float b) { return fabsf(a - b) < 0.01f; }

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

    printf("test_config_override: OK (pre-include limit overrides honored)\n");
    return 0;
}

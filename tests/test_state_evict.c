// test_state_evict.c - persistent state reclamation pins
//
// Covers:
//   Nothing is reclaimed while the map is at or under its trigger, however
//   old; above it, entries unrequested for the minimum age go and younger
//   ones keep their values, with the trigger re-armed to twice the survivors
//   (never below the threshold); wlx_state_prune mid-frame keeps every entry
//   requested this frame and reports the count; reclaimed blocks are reused
//   zeroed for the same size; backward-shift deletion agrees with a
//   reference model over random sequences on a 64-slot table with crafted
//   collisions across the wrap; a trigger no count reaches disables the
//   automatic sweep while the explicit prune still works.

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

#include <string.h>

// ============================================================================
// Helpers
// ============================================================================

// One int of state at a synthetic site under push_id(k), so a test can hold
// any number of entries from one source line.
static int *sev_state(WLX_Context *ctx, const char *file, int line, WLX_Id k) {
    wlx_push_id(ctx, k);
    WLX_State s = wlx_get_state_impl(ctx, sizeof(int), file, line);
    wlx_pop_id(ctx);
    return (int *)s.data;
}

static void sev_empty_frames(WLX_Context *ctx, int n) {
    for (int i = 0; i < n; i++) {
        test_frame_begin(ctx, 0, 0, false, false);
        test_frame_end(ctx);
    }
}

// ============================================================================
// Tests
// ============================================================================

TEST(evict_never_at_or_below_trigger) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);
    for (WLX_Id k = 0; k < 100; k++) *sev_state(&ctx, "sev/a.c", 1, k) = 7;
    test_frame_end(&ctx);

    sev_empty_frames(&ctx, WLX_STATE_MIN_AGE + 10);   // far past the minimum age

    test_frame_begin(&ctx, 0, 0, false, false);
    int kept = 0;
    for (WLX_Id k = 0; k < 100; k++) kept += *sev_state(&ctx, "sev/a.c", 1, k) == 7;
    test_frame_end(&ctx);

    ASSERT_EQ_INT(kept, 100);
    ASSERT_EQ_INT((int)ctx.states.count, 100);
    wlx_context_destroy(&ctx);
}

TEST(evict_sweeps_stale_above_trigger) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    enum { LIVE = 5000, STALE = 3000 };
    const size_t threshold = (size_t)WLX_STATE_EVICT_THRESHOLD;
    ASSERT_TRUE(LIVE > (int)threshold);

    // Two frames with every entry requested: the second wlx_begin sees the
    // count above the threshold, sweeps (nothing is old) and re-arms the
    // trigger to twice the survivors.
    for (int f = 0; f < 2; f++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        for (WLX_Id k = 0; k < LIVE; k++) *sev_state(&ctx, "sev/b.c", 1, k) = 3;
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT((int)ctx.states.count, LIVE);
    ASSERT_TRUE(ctx.states.trigger == (size_t)LIVE * 2);

    // Above the threshold but under the trigger: nothing sweeps, so the
    // STALE entries survive well past the minimum age.
    for (int f = 0; f < WLX_STATE_MIN_AGE + 5; f++) {
        test_frame_begin(&ctx, 0, 0, false, false);
        for (WLX_Id k = STALE; k < LIVE; k++) (void)sev_state(&ctx, "sev/b.c", 1, k);
        test_frame_end(&ctx);
    }
    ASSERT_EQ_INT((int)ctx.states.count, LIVE);

    // Pressure: the next wlx_begin sweeps. Stale entries go, live ones keep
    // their values, the trigger becomes max(threshold, 2 * survivors).
    ctx.states.trigger = threshold;
    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_INT((int)ctx.states.count, LIVE - STALE);
    int live_kept = 0, stale_zero = 0;
    for (WLX_Id k = STALE; k < LIVE; k++) live_kept += *sev_state(&ctx, "sev/b.c", 1, k) == 3;
    for (WLX_Id k = 0; k < 10; k++) stale_zero += *sev_state(&ctx, "sev/b.c", 1, k) == 0;
    test_frame_end(&ctx);
    ASSERT_EQ_INT(live_kept, LIVE - STALE);
    ASSERT_EQ_INT(stale_zero, 10);
    size_t twice = (size_t)(LIVE - STALE) * 2;
    ASSERT_TRUE(ctx.states.trigger == (twice > threshold ? twice : threshold));
    wlx_context_destroy(&ctx);
}

TEST(prune_mid_frame_keeps_current) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);
    *sev_state(&ctx, "sev/c.c", 1, 0) = 11;
    *sev_state(&ctx, "sev/c.c", 2, 0) = 22;
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    int *a = sev_state(&ctx, "sev/c.c", 1, 0);   // requested this frame
    size_t gone = wlx_state_prune(&ctx, 0);      // 0 clamps to 1
    ASSERT_EQ_INT((int)gone, 1);
    ASSERT_EQ_INT(*a, 11);                       // pointer and value intact
    ASSERT_TRUE(sev_state(&ctx, "sev/c.c", 1, 0) == a);
    ASSERT_EQ_INT(*sev_state(&ctx, "sev/c.c", 2, 0), 0);  // back zeroed
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

#if WLX_STATE_RECYCLE
TEST(evict_recycles_blocks) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);

    test_frame_begin(&ctx, 0, 0, false, false);
    WLX_State x = wlx_get_state_impl(&ctx, 16, "sev/d.c", 1);
    memset(x.data, 0xAB, 16);
    test_frame_end(&ctx);

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_INT((int)wlx_state_prune(&ctx, 1), 1);
    WLX_State y = wlx_get_state_impl(&ctx, 16, "sev/d.c", 2);   // same size
    ASSERT_TRUE(y.data == x.data);
    unsigned char zero[16] = {0};
    ASSERT_TRUE(memcmp(y.data, zero, 16) == 0);
    WLX_State z = wlx_get_state_impl(&ctx, 32, "sev/d.c", 3);   // another size
    ASSERT_TRUE(z.data != x.data);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}
#endif

// Reference model for the table: a flat list of live ids with the data
// pointer and stamp each carries. Ids share eight home slots straddling the
// table's wrap so clusters cross it and backward shift is exercised there.
typedef struct { WLX_Id id; void *data; uint32_t seen; } Sev_Model_Entry;

static unsigned int sev_rng;
static unsigned int sev_rand(void) {
    sev_rng = sev_rng * 1103515245u + 12345u;
    return (sev_rng >> 16) & 0x7fff;
}

TEST(evict_backward_shift_model) {
    enum { CAP = 64, POOL = 40, STEPS = 4000 };
    static const unsigned homes[8] = { 60, 61, 62, 63, 0, 1, 2, 3 };
    WLX_Id pool[POOL];
    for (int p = 0; p < POOL; p++) pool[p] = (WLX_Id)homes[p % 8] | ((WLX_Id)(p + 1) << 6);

    WLX_State_Map map = {0};
    Sev_Model_Entry model[POOL];
    int live = 0;
    uint32_t now = 0xFFFFFF00u;   // ages cross the 32-bit wrap during the run
    sev_rng = 0x5EED;

    for (int step = 0; step < STEPS; step++) {
        unsigned op = sev_rand() % 8;
        if (op < 5) {
            // Request: insert or refresh, at most 40 live in 64 slots (under the load factor).
            WLX_Id id = pool[sev_rand() % POOL];
            WLX_State_Map_Slot *slot = wlx_state_map_get(&map, id, 8);
            slot->last_seen = now;
            int m = -1;
            for (int k = 0; k < live; k++) if (model[k].id == id) { m = k; break; }
            if (m < 0) { model[live].id = id; model[live].data = slot->data; live++; m = live - 1; }
            ASSERT_TRUE(model[m].data == slot->data);   // data blocks never move
            model[m].seen = now;
        } else if (op < 7) {
            now += 1 + sev_rand() % 40;
        } else {
            uint32_t min_age = 1 + sev_rand() % 80;
            size_t evicted = wlx_state_map_sweep(&map, now, min_age);
            int expect = 0, w = 0;
            for (int k = 0; k < live; k++) {
                if ((uint32_t)(now - model[k].seen) >= min_age) expect++;
                else model[w++] = model[k];
            }
            live = w;
            ASSERT_EQ_INT((int)evicted, expect);
        }
        ASSERT_EQ_INT((int)map.count, live);
        ASSERT_TRUE(map.capacity == CAP);
        // Every live id is found with its block; every other pool id is absent.
        for (int p = 0; p < POOL; p++) {
            WLX_State_Map_Slot *s = wlx_state_map_find(&map, pool[p]);
            int m = -1;
            for (int k = 0; k < live; k++) if (model[k].id == pool[p]) { m = k; break; }
            if (m >= 0) { ASSERT_TRUE(s->id == pool[p]); ASSERT_TRUE(s->data == model[m].data); }
            else ASSERT_TRUE(s->id == 0);
        }
    }
    for (size_t i = 0; i < map.capacity; i++) if (map.slots[i].id != 0) wlx_free(map.slots[i].data);
    for (size_t i = 0; i < map.free_class_count; i++) {
        void *b = map.free_classes[i].head;
        while (b) { void *n; memcpy(&n, b, sizeof n); wlx_free(b); b = n; }
    }
    wlx_free(map.free_classes);
    wlx_free(map.slots);
}

TEST(evict_unreachable_trigger_disables_sweep) {
    WLX_Context ctx;
    test_ctx_init(&ctx, 800, 600);
    ctx.states.trigger = SIZE_MAX;   // models WLX_STATE_EVICT_THRESHOLD 0

    test_frame_begin(&ctx, 0, 0, false, false);
    for (WLX_Id k = 0; k < 100; k++) *sev_state(&ctx, "sev/e.c", 1, k) = 5;
    test_frame_end(&ctx);
    sev_empty_frames(&ctx, WLX_STATE_MIN_AGE + 10);
    ASSERT_EQ_INT((int)ctx.states.count, 100);

    test_frame_begin(&ctx, 0, 0, false, false);
    ASSERT_EQ_INT((int)wlx_state_prune(&ctx, 1), 100);   // the explicit path still works
    ASSERT_EQ_INT((int)ctx.states.count, 0);
    test_frame_end(&ctx);
    wlx_context_destroy(&ctx);
}

// ============================================================================
// Suite
// ============================================================================

SUITE(state_evict) {
    RUN_TEST(evict_never_at_or_below_trigger);
    RUN_TEST(evict_sweeps_stale_above_trigger);
    RUN_TEST(prune_mid_frame_keeps_current);
#if WLX_STATE_RECYCLE
    RUN_TEST(evict_recycles_blocks);
#endif
    RUN_TEST(evict_backward_shift_model);
    RUN_TEST(evict_unreachable_trigger_disables_sweep);
}
